# 📘 CS333 – Lab 2: System Calls

### Tổng hợp kiến thức · xv6-riscv

| | |
|---|---|
| **Môn học** | CS333 – Operating Systems (HCMUS) |
| **Nguồn đề** | MIT 6.1810 "Lab: System calls" |
| **Nền tảng** | xv6-riscv · QEMU · GDB |
| **Branch** | `syscall` (cấu hình `LAB=syscall` lấy từ `conf/lab.mk`) |
| **Thang điểm** | 10 điểm (làm cá nhân hoặc nhóm ≤ 2 người) |
| **Tài liệu đi kèm** | [`Lab2_Question.md`](Lab2_Question.md) – câu hỏi vấn đáp kèm đáp án |

---

## Mục lục

1. [Tổng quan](#1-tổng-quan)
2. [Đường đi của một system call](#2-đường-đi-của-một-system-call)
3. [Dùng GDB (bài 3.1)](#3-dùng-gdb-bài-31)
4. [Sandbox theo mask (bài 3.2)](#4-sandbox-theo-mask-bài-32)
5. [Sandbox theo pathname (bài 3.3)](#5-sandbox-theo-pathname-bài-33)
6. [Tấn công xv6 (bài 3.4)](#6-tấn-công-xv6-bài-34)
7. [Nộp bài và chấm điểm](#7-nộp-bài-và-chấm-điểm)
8. [Bảng tra nhanh](#8-bảng-tra-nhanh)
9. [Những chỗ dễ sai](#9-những-chỗ-dễ-sai)

---

## 1. Tổng quan

### 1.1 Mục tiêu

- Dùng **GDB + QEMU** để debug kernel: backtrace, xem `struct proc`, trapframe, thanh ghi đặc quyền, truy ra nguồn gốc một kernel panic.
- Nắm **đường đi của một syscall**: stub `usys.S` → trap → `syscall()` → handler `sys_*` → giá trị trả về.
- **Thêm một syscall mới** từ đầu đến cuối (prototype, stub, số syscall, handler, state theo từng process).
- Làm cơ chế **sandbox** (chặn syscall) và hiểu việc kế thừa qua `fork`.
- **Khai thác lỗi lộ bộ nhớ** do bộ nhớ cấp phát không được xoá.

### 1.2 Các bài làm

| Bài | Nội dung | Sản phẩm | Điểm |
|:---:|---|---|:---:|
| 3.1 | Dùng GDB, trả lời 6 câu hỏi | `answers-syscall.txt` + ảnh GDB trong report | 1.5 |
| 3.2 | `interpose(mask, "-")`: chặn syscall theo mask, con kế thừa | code kernel + user | 2.5 |
| 3.3 | Mở rộng: cho `open`/`exec` chạy nếu pathname bằng `path` | code kernel | 2.5 |
| 3.4 | Tấn công xv6: đọc secret còn sót trong bộ nhớ | `user/attack.c` | 2.0 |
| – | Chất lượng code, lịch sử git, `time.txt` | review tay | 0.5 |
| – | Report | review tay | 1.0 |

### 1.3 Thang điểm của `make grade`

`make grade` dùng thang **60 điểm** riêng (không phải thang 10 của đề):

| Test | Điểm |
|---|:---:|
| `answers-syscall.txt` | 5 |
| `sandbox_mask`, `sandbox_fork`, `sandbox_path`, `sandbox_most`, `sandbox_minus`, `sandbox_block`, `sandbox_escape`, `sandbox_forkpath` (8 test × 5) | 40 |
| `attack` | 14 |
| `time` | 1 |
| **Tổng** | **60** |

> 💡 Đề chỉ liệt kê 5 test sandbox, nhưng grader có **8 test**. Hai test hay bị bỏ sót là `sandbox_block` và `sandbox_escape`.

Chạy một test riêng: `./grade-lab-syscall <tên_test>`.

---

## 2. Đường đi của một system call

### 2.1 Toàn cảnh

```
 USER MODE
 ───────────────────────────────────────────────────────────────────
   interpose(mask, path)            prototype ở user/user.h
          │
          ▼
   stub trong usys.S                sinh ra từ user/usys.pl
      li   a7, SYS_interpose        a7 ← số syscall (23)
      ecall                         ← trap vào kernel
 ══════════════════════════════ ecall ═══════════════════════════════
 SUPERVISOR MODE (kernel)
 ───────────────────────────────────────────────────────────────────
   uservec        (trampoline.S)    lưu 32 thanh ghi user vào trapframe,
          │                         đổi sang page table của kernel
          ▼
   usertrap()     (trap.c)          scause == 8 → epc += 4 → intr_on()
          │
          ▼
   syscall()      (syscall.c)       num = trapframe->a7
          │                         rejected(p, num)?  ── có ──► a0 = -1
          │                                │
          │                               không
          ▼                                ▼
   syscalls[num]()                  gọi handler, ví dụ sys_interpose()
          │                         (handler lấy đối số bằng argint/argstr)
          ▼
   trapframe->a0 = kết quả
          │
          ▼
   usertrapret() → userret          khôi phục thanh ghi, chuyển lại user page table
          │
          ▼
   sret                             về user, ngay sau lệnh ecall
 ═══════════════════════════════ sret ════════════════════════════════
   ret                              giá trị trả về nằm ở a0
```

### 2.2 Quy ước gọi syscall (RISC-V)

| Thanh ghi | Vai trò |
|:---:|---|
| `a7` | **Số syscall** (xem `kernel/syscall.h`) |
| `a0` – `a5` | Đối số thứ 0 – 5 |
| `a0` | **Giá trị trả về** (kernel ghi vào `trapframe->a0`) |

### 2.3 Các file liên quan

| File | Vai trò |
|---|---|
| `user/usys.pl` → `user/usys.S` | Sinh các stub (`li a7, SYS_x; ecall; ret`). `usys.S` được tạo lúc `make` |
| `user/user.h` | Prototype hàm syscall cho chương trình user |
| `kernel/syscall.h` | Số syscall (`SYS_fork` = 1 … `SYS_interpose` = 23) |
| `kernel/syscall.c` | Bảng `syscalls[]`, hàm `syscall()`, các hàm lấy đối số |
| `kernel/sysproc.c`, `kernel/sysfile.c` | Cài đặt các handler `sys_*` |
| `kernel/proc.h`, `kernel/proc.c` | `struct proc`, `allocproc`, `freeproc`, `kfork` |
| `kernel/trap.c`, `kernel/trampoline.S` | Đường vào/ra kernel |

### 2.4 Lấy đối số từ user space

Kernel **không tin** con trỏ do user đưa vào: user và kernel dùng page table khác nhau, và con trỏ có thể sai hoặc độc hại. Vì vậy phải copy qua các hàm có kiểm tra.

| Hàm | Dùng để | Ghi chú |
|---|---|---|
| `argint(n, &x)` | Lấy đối số thứ `n` dạng số nguyên | đọc thẳng từ trapframe |
| `argaddr(n, &a)` | Lấy đối số thứ `n` dạng địa chỉ | **không** kiểm tra hợp lệ |
| `argstr(n, buf, max)` | Lấy đối số thứ `n` dạng chuỗi, copy vào `buf` | trả độ dài chuỗi, hoặc `-1` nếu lỗi |
| `fetchstr`, `fetchaddr` | Đọc chuỗi / số 64-bit tại một địa chỉ user | `argstr` gọi `fetchstr` |
| `copyin`, `copyinstr`, `copyout` | Copy giữa user và kernel, có kiểm tra | tầng thấp nhất, trong `vm.c` |

> ⚠️ `argint` và `argaddr` không có giá trị lỗi; chỉ `argstr` có thể thất bại, nên **phải kiểm tra** `argstr < 0`.

### 2.5 Ba chi tiết hay bị hỏi

1. **Vì sao `epc += 4` trước khi gọi `syscall()`?** `sepc` đang trỏ vào chính lệnh `ecall`. Cộng 4 để khi `sret` chương trình chạy tiếp lệnh sau `ecall`, thay vì lặp `ecall` mãi.
2. **Vì sao trả `-1` bằng cách ghi `a0`?** Khi quay về user, `userret` nạp lại thanh ghi từ trapframe, nên giá trị kernel ghi vào `trapframe->a0` chính là giá trị trả về của hàm syscall.
3. **Nhánh "unknown sys call"** trong `syscall()` (khi `num` ngoài bảng hoặc ô trống) in bằng `printk` rồi trả `-1`. Trong kernel dùng `printk`, `printf` là của thư viện user.

---

## 3. Dùng GDB (bài 3.1)

### 3.1 Cách chạy

```bash
# Terminal 1: chạy QEMU và chờ gdb kết nối
make qemu-gdb

# Terminal 2: cùng thư mục xv6
gdb-multiarch          # hoặc riscv64-unknown-elf-gdb
```

`make qemu-gdb` sinh `.gdbinit` từ `.gdbinit.tmpl-riscv` để gdb tự kết nối.

### 3.2 Lệnh GDB cần nhớ

| Lệnh | Tác dụng |
|---|---|
| `b syscall` | Đặt breakpoint tại hàm `syscall` |
| `b *0x80001d6e` | Breakpoint tại **địa chỉ** (dùng với `sepc`) |
| `c` | Chạy tiếp |
| `n` / `s` | Bước qua / bước vào dòng kế tiếp |
| `layout src` / `layout asm` | Chia màn hình, hiện mã nguồn / mã assembly |
| `backtrace` (`bt`) | In ngăn xếp lời gọi |
| `p /x *p` | In cả `struct proc` dạng hex (sau khi đã chạy qua `p = myproc()`) |
| `p p->name`, `p p->pid` | Tên và pid của process hiện tại |
| `p p->trapframe->a7` | Số syscall của lời gọi hiện tại |
| `p /x $sstatus` | In thanh ghi đặc quyền `sstatus` |
| `x/i $pc` | Xem lệnh assembly tại `pc` |

### 3.3 Thanh ghi đặc quyền liên quan

| Thanh ghi | Ý nghĩa |
|---|---|
| `sepc` | Địa chỉ lệnh đang chạy khi trap xảy ra |
| `scause` | Nguyên nhân trap |
| `stval` | Thông tin phụ (với page fault: **địa chỉ ảo gây lỗi**) |
| `sstatus` | Trạng thái: bit 1 `SIE`, bit 5 `SPIE`, **bit 8 `SPP`** (mode trước khi trap: `0` = User, `1` = Supervisor) |

Một số giá trị `scause` hay gặp:

| `scause` | Ý nghĩa |
|:---:|---|
| 8 | `ecall` từ User mode (system call) |
| 12 | Instruction page fault |
| 13 (`0xd`) | **Load page fault** |
| 15 (`0xf`) | Store page fault |

### 3.4 Quy trình truy một kernel panic

```
  1. Gây lỗi:   đổi  num = p->trapframe->a7;   thành   num = *(int *)0;
  2. make qemu → kernel in:   scause=0xd sepc=0x… stval=0x0
                              panic: kerneltrap
  3. Tra sepc trong kernel/kernel.asm → thấy lệnh bị lỗi và thanh ghi dùng
  4. gdb: b *<sepc> → c → dừng TRƯỚC lệnh đó → p p->name, p p->pid
  5. Nhớ KHÔI PHỤC lại   num = p->trapframe->a7;
```

> ⚠️ Địa chỉ `sepc` phụ thuộc bản build. Sửa code ở chỗ khác là địa chỉ đổi, nên luôn tra lại `kernel.asm` trên bản build hiện tại.

### 3.5 Sáu câu trả lời (từ `answers-syscall.txt`)

| # | Câu hỏi | Đáp án | Vì sao |
|:-:|---|---|---|
| Q1 | Hàm nào gọi `syscall()`? | **`usertrap()`** (`kernel/trap.c:68`) | `backtrace` frame 1; trap từ `ecall` vào `usertrap()`, rồi gọi `syscall()` |
| Q2 | `p->trapframe->a7` bằng bao nhiêu? | **15** = `SYS_open` | Process là `init`; syscall đầu tiên `init` gọi là `open("console", O_RDWR)` |
| Q3 | CPU trước đó ở mode nào? | **User mode** | `sstatus = 0x200000022`, bit 8 (`SPP`) = 0 |
| Q4 | Lệnh assembly gây panic? Thanh ghi ứng với `num`? | **`lw a3,0(zero)`**, thanh ghi **`a3`** | `sepc = 0x80001d6e`; lệnh load từ địa chỉ 0 vào `a3` |
| Q5 | Vì sao kernel crash? | **Load page fault** tại địa chỉ 0 | `scause = 0xd`, `stval = 0x0`; địa chỉ 0 không được map trong kernel page table |
| Q6 | Process nào đang chạy lúc panic? | **`init`, pid 1** | `p p->name`, `p p->pid` |

**Chi tiết đáng nhớ:**

- **Q2 khác hướng dẫn gốc.** Đề nhắc `user/initcode.S`, nhưng phiên bản xv6 này **không có** file đó: `forkret()` trong `kernel/proc.c` gọi thẳng `kexec("/init", …)`. Vì vậy syscall đầu tiên là `open` (15), không phải `exec` (7) như bản cũ.
- **Q3:** `0x200000022` = `0x2_0000_0000` (bit `UXL` = 2, chế độ 64-bit) + `0x22` (`SIE` bit 1 và `SPIE` bit 5). Bit 8 bằng 0 nên trước đó là User mode.
- **Q4:** `a3` là thanh ghi trình biên dịch **chọn** cho biến `num` trong bản build này, không phải quy ước cố định. `zero` (`x0`) luôn bằng 0, nên `0(zero)` chính là địa chỉ 0.
- **Q5:** lỗi xảy ra **trong kernel** và không có cách phục hồi, nên `kerneltrap()` gọi `panic()`.

---

## 4. Sandbox theo mask (bài 3.2)

### 4.1 Yêu cầu

Thêm syscall `int interpose(int mask, const char *path)`:

- Bit `n` của `mask` bật ⇒ syscall số `n` **phải thất bại** với process gọi.
- Syscall bị từ chối **không được chạy**, trả `-1` cho người gọi.
- Con tạo bằng `fork` **kế thừa** hạn chế của cha.
- Ở bài này tham số `path` luôn là `"-"` (xử lý thật sự ở bài 3.3).

### 4.2 Cách tính mask

Mask = tổng các `1 << SYS_x` của những syscall muốn chặn.

| Muốn chặn | Phép tính | Mask |
|---|---|---:|
| `open` (15) | `1 << 15` | **32768** |
| `exec` (7) | `1 << 7` | **128** |
| `open` + `exec` | `32768 + 128` | **32896** |
| `getpid` (11) | `1 << 11` | 2048 |
| `fork` (1) | `1 << 1` | 2 |
| Chặn mọi syscall **trừ `exit`** | bit 0–30, bỏ bit 2 | **2147483643** (`0x7FFFFFFB`) |
| Chính `interpose` (23) | `1 << 23` | 8388608 |

> 💡 `sandbox_most` dùng mask `2147483643` nhưng **chừa bit 2 (`exit`)**. Nếu chặn cả `exit` thì process bị nhốt không thoát được.

### 4.3 Chương trình `sandbox`

`user/sandbox.c` (có sẵn) chạy: `sandbox <mask> <path> <lệnh…>`.

```
  sandbox (cha)                       con
  ─────────────                       ───
  fork() ───────────────────────────► interpose(mask, path)   ← chỉ con bị hạn chế
  wait(0)                             exec(lệnh…)             ← exec vẫn giữ sandbox
```

Cha không bao giờ gọi `interpose`, nên cha không bị hạn chế.

### 4.4 Các file đã sửa

| File | Thay đổi |
|---|---|
| `kernel/syscall.h` | `#define SYS_interpose 23` |
| `user/usys.pl` | `entry("interpose");` |
| `user/user.h` | `int interpose(int, const char*);` |
| `Makefile` | thêm `$U/_sandbox` và `$U/_sbtest` vào `UPROGS` |
| `kernel/proc.h` | thêm `struct sandbox` và một field `sandbox` trong `struct proc` |
| `kernel/proc.c` | `allocproc`/`freeproc`: xoá state; `kfork`: sao chép state |
| `kernel/sysproc.c` | handler `sys_interpose()` |
| `kernel/syscall.c` | `extern`, mục trong `syscalls[]`, hàm `rejected()` và lời gọi trong `syscall()` |

### 4.5 State của sandbox

Hai giá trị (mask và path) được gom trong **một struct**:

```c
// kernel/proc.h
struct sandbox {
  uint mask;            // bit n bật ⇒ syscall n bị hạn chế
  char path[MAXPATH];   // pathname được phép cho open/exec ("-" = không có)
};

struct proc {
  ...
  char name[16];
  struct sandbox sandbox;   // đặt cùng nhóm state "private to the process"
};
```

**Lý do gom thành struct:** `kfork` sao chép bằng **một phép gán**, `freeproc` xoá bằng **một `memset`**, không thể quên một nửa.

`mask` là `uint` (không dấu) để thao tác bit 31 an toàn; `MAXPATH` = 128.

### 4.6 Vòng đời state qua process

| Thời điểm | Hàm | Việc làm | Vì sao |
|---|---|---|---|
| Cấp process mới | `allocproc()` | `memset(&p->sandbox, 0, …)` | Slot `proc` được **tái sử dụng**; tránh rò state cũ |
| Giải phóng | `freeproc()` | `memset(&p->sandbox, 0, …)` | như trên |
| `fork` | `kfork()` | `np->sandbox = p->sandbox;` | Con kế thừa **cả mask và path** |
| `exec` | `kexec()` | **không đụng** | `exec` chỉ thay ảnh bộ nhớ, `struct proc` giữ nguyên ⇒ sandbox còn nguyên sau `exec` |

**Về khoá:** field này chỉ do chính process đó đọc/ghi (giống `p->sz`), không cần `p->lock`. Trong `kfork`, con `np` chưa chạy được khi ta ghi vào nó.

### 4.7 Chỗ chặn: một điểm duy nhất trong `syscall()`

```c
void
syscall(void)
{
  int num;
  struct proc *p = myproc();

  num = p->trapframe->a7;
  if (num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    if (rejected(p, num))
      p->trapframe->a0 = -1;                 // từ chối: KHÔNG gọi handler
    else
      p->trapframe->a0 = syscalls[num]();
  } else {
    printk("%d %s: unknown sys call %d\n", p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
```

> ✅ Mọi syscall đều đi qua `syscall()`, nên kiểm tra **một lần ở đây** (không rải rác trong từng handler). Đây là tiêu chí chấm 2b.

### 4.8 Ví dụ

```
$ sandbox 32768 - cat README
cat: cannot open README
```

Diễn biến: con gọi `interpose(32768, "-")` → `exec("cat")` (bit 7 không bật nên qua) → `cat` gọi `open("README")` → bit 15 bật, path là `"-"` ⇒ `rejected()` trả 1 ⇒ `a0 = -1` ⇒ `cat` in "cannot open".

---

## 5. Sandbox theo pathname (bài 3.3)

### 5.1 Yêu cầu

Một syscall **`open` hoặc `exec` bị mask** vẫn được chạy nếu **pathname của lời gọi bằng `path`** đã đăng ký. Mọi syscall bị mask khác, và `open`/`exec` có pathname khác, bị từ chối như bài 3.2. `path = "-"` nghĩa là **không pathname nào** được phép.

### 5.2 Cây quyết định của `rejected(p, num)`

```
  bit num của mask có bật không?
   ├── không ─────────────────────────────────────────► CHO CHẠY        (không đọc path)
   └── có
        │
        num là open hoặc exec?
         ├── không ────────────────────────────────────► TỪ CHỐI
         └── có
              │
              path được phép là "-" ?
               ├── có ─────────────────────────────────► TỪ CHỐI
               └── không
                    │
                    argstr(0, …) đọc được pathname?
                     ├── lỗi ──────────────────────────► TỪ CHỐI         (fail closed)
                     └── được
                          │
                          pathname == path được phép ?
                           ├── bằng ───────────────────► CHO CHẠY
                           └── khác ───────────────────► TỪ CHỐI
```

Code tương ứng (trình bày lại cho gọn, logic giống code thật):

```c
// kernel/syscall.c
static int
rejected(struct proc *p, int num)
{
  struct sandbox *sb = &p->sandbox;
  char path[MAXPATH];

  if (!(sb->mask & (1 << num)))
    return 0;
  if (num != SYS_open && num != SYS_exec)
    return 1;
  if (strncmp(sb->path, "-", MAXPATH) == 0)
    return 1;
  if (argstr(0, path, MAXPATH) < 0)
    return 1;
  return strncmp(sb->path, path, MAXPATH) != 0;
}
```

### 5.3 Các điểm kỹ thuật

| Điểm | Lý do |
|---|---|
| Pathname của `open` **và** `exec` đều là **đối số 0** | Dùng chung `argstr(0, …)` |
| Gọi `argstr(0, …)` ngay trong `syscall()` được | `a0` trong trapframe chưa bị ghi đè trước khi handler chạy xong |
| `"-"` so khớp **trọn chuỗi** | File tên `---` là pathname bình thường (test `sandbox_minus`) |
| So sánh **chính xác từng ký tự**, không chuẩn hoá | `./README` ≠ `README` |
| Dùng `strncmp(…, MAXPATH)` | Kernel không có `strcmp`; `strncmp` bị chặn bởi kích thước buffer |
| `argstr` lỗi ⇒ từ chối | **Fail closed**: không xác định được pathname thì không cho qua |
| Chỉ `open`/`exec` có ngoại lệ pathname | Mọi syscall khác bị mask là bị từ chối luôn |
| Không có TOCTOU | xv6 không có thread, không ai đổi được chuỗi user giữa lúc kiểm tra và lúc handler đọc lại |
| `exec` ngay sau `interpose()` trong `sandbox.c` cũng bị lọc | Vì thế `sandbox_most` cần path `grep` để `exec grep` lọt qua |

### 5.4 `sys_interpose`: chống thoát sandbox (phần dễ sai nhất)

**Cuộc tấn công.** Nếu `interpose` chỉ **ghi đè** mask và path, một process đã bị chặn `open` có thể gọi `interpose(0, "README")` hoặc `interpose(0, "-")` để tự gỡ hạn chế. Test `sandbox_escape` kiểm tra đúng điều này.

**Hai luật phòng thủ:**

| Luật | Cách thực hiện |
|---|---|
| **Mask chỉ tích luỹ** | `sb->mask \|= mask` (hợp bit cũ với bit mới, không bao giờ xoá) ⇒ `interpose(0, …)` không làm gì với mask |
| **Path không được nới rộng** | Nếu mask **hiện tại** đã hạn chế `open` hoặc `exec`, path mới chỉ được **bằng path hiện tại** hoặc là `"-"` (chặt hơn); ngược lại trả `-1`, không đổi gì |

```c
// kernel/sysproc.c
uint64
sys_interpose(void)
{
  struct sandbox *sb = &myproc()->sandbox;
  int mask;
  char path[MAXPATH];

  argint(0, &mask);
  if (argstr(1, path, MAXPATH) < 0)      // đọc vào buffer cục bộ trước
    return -1;

  // open/exec đã bị hạn chế ⇒ path chỉ được giữ nguyên hoặc thành "-"
  if ((sb->mask & ((1 << SYS_open) | (1 << SYS_exec))) &&
      strncmp(path, sb->path, MAXPATH) != 0 &&
      strncmp(path, "-", MAXPATH) != 0)
    return -1;

  sb->mask |= mask;
  safestrcpy(sb->path, path, sizeof(sb->path));
  return 0;
}
```

**Điểm cần nhớ:**

- Kiểm tra dùng `sb->mask` **trước khi** cộng mask mới, vì ta đang hỏi "tiến trình **hiện đang** bị hạn chế chưa".
- Đọc path vào **buffer cục bộ**, kiểm tra xong mới ghi vào `struct proc`, để lỗi giữa chừng không để lại trạng thái nửa vời.
- Nếu mask có bit của chính `interpose` (bit 23), process không gọi lại được nữa.
- `interpose(0, 0)`: con trỏ NULL **không chắc làm `argstr` lỗi**, vì text của chương trình user nằm ở địa chỉ 0 nên `argstr` có thể đọc được "chuỗi" rác từ đầu text. Dù `argstr` lỗi hay thành công, process đã bị hạn chế vẫn nhận `-1`: nếu lỗi thì trả ngay, nếu thành công thì chuỗi rác khác path hiện tại và khác `"-"`.

### 5.5 Quá trình kiểm tra hai lớp

Ví dụ `sandbox 32768 README grep xv6 x`:

| Lời gọi trong con | Kết quả | Lý do |
|---|---|---|
| `exec("grep")` | cho chạy | bit 7 (`exec`) không bật trong mask 32768 |
| `open("x")` | **từ chối** | bit 15 bật, `"x"` ≠ `"README"` |
| `open("README")` | cho chạy | bit 15 bật, `"README"` = path được phép |

### 5.6 Tám test của grader

| Test | Chạy gì | Kiểm tra |
|---|---|---|
| `sandbox_mask` | mask `32768`, `128`, `32896`, path `-` | `cat`/`grep` không mở được README; `exec grep`, `exec sh` bị từ chối |
| `sandbox_fork` | `sandbox 32768 - sh < exec.sh` | Hạn chế **kế thừa qua cháu**: `sh` chạy được `echo`, `ls` không mở được `.` |
| `sandbox_path` | path `README`, rồi `x`, rồi mask `128` path `sh` | Mở đúng file được phép; `exec sh` qua nhưng các `exec` con của `sh` thì bị chặn |
| `sandbox_most` | mask `2147483643`, path `grep` | Mọi syscall bị chặn trừ `exit`; `exec grep` vẫn qua nhờ path |
| `sandbox_minus` | file tên `---`, path `---` | `---` là pathname **bình thường**, không phải `-` đặc biệt |
| `sandbox_block` | `sbtest getpid`, `sbtest fork` với bit tương ứng | Syscall bị chặn trả `-1`, **không chạy** (không có "CHILD RAN"), process vẫn sống |
| `sandbox_escape` | process bị chặn `open` rồi gọi `interpose(0,0)`, `interpose(0,"README")` | **Không thoát được** sandbox bằng cách gọi lại `interpose` |
| `sandbox_forkpath` | path `README`, con `fork` rồi `open` | Con kế thừa cả **path**: README mở được, `exec.sh` bị từ chối |

---

## 6. Tấn công xv6 (bài 3.4)

### 6.1 Lỗi bị cố ý cài vào

Khi biên dịch với `LAB_SYSCALL`, ba chỗ xoá/tô rác bộ nhớ bị **bỏ đi** (`#ifndef LAB_SYSCALL`):

| File | Câu lệnh bị bỏ | Tác dụng bình thường |
|---|---|---|
| `kernel/vm.c`, `uvmalloc()` | `memset(mem, 0, sz)` | **Xoá về 0** trang mới cấp cho user |
| `kernel/kalloc.c`, `kfree()` | `memset(pa, 1, PGSIZE)` | Tô rác khi giải phóng (bắt dangling reference) |
| `kernel/kalloc.c`, `kalloc()` | `memset(r, 5, PGSIZE)` | Tô rác khi cấp phát |

**Hệ quả:** trang mới cấp (ví dụ qua `sbrk`) **giữ nguyên nội dung của lần dùng trước**, tức rò dữ liệu giữa các process và phá vỡ cô lập.

### 6.2 Bộ cấp phát trang: free list kiểu LIFO

```
 kmem.freelist ──► [trang A] ──► [trang B] ──► [trang C] ──► …

 kfree(X):   X->next = freelist;  freelist = X     (X thành đầu danh sách)
 kalloc():   trả trang ở đầu danh sách              (lấy đúng trang vừa trả)
```

- Trang vừa `kfree` nằm **đầu** danh sách, nên là trang `kalloc` trả ra **đầu tiên**.
- `kfree` ghi con trỏ `next` vào **8 byte đầu** của trang (`struct run`). Các byte còn lại **không bị đụng** khi không có `memset`.

### 6.3 `secret.c` đặt secret ở đâu

```c
char *data = sbrk(9 * 4096);        // xin 9 trang
data += (4096 - data % 4096);       // căn lên ĐẦU trang kế tiếp (nếu chưa căn)
data += 32 + (argv[1][0] & 0xff);   // cộng offset phụ thuộc ký tự đầu của secret
strcpy(data, "Here it is: ");       // marker
strcpy(data + 12, argv[1]);         // secret ngay sau marker
```

Bố cục trong **trang đầu** của vùng 9 trang:

```
 byte 0         8                   32+c                    32+c+12
  │ next (kfree)│  (không dùng)      │ "Here it is: "        │ secret '\0'  …
  └─ 8 byte đầu ┘                    ▲ marker                ▲ secret
                                       c = ASCII(ký tự đầu của secret)
```

| Đại lượng | Công thức |
|---|---|
| Vị trí marker `"Here it is: "` | `32 + ASCII(ký tự đầu)` |
| Vị trí secret | marker `+ 12` (`strlen("Here it is: ")` = 12) |

Ví dụ: `abc123XY` → `'a'` = 97 → marker tại **129**; `Zq9mLpT4` → `'Z'` = 90 → **122**; `xyzzy` → `'x'` = 120 → **152**.

Vì secret chỉ gồm chữ và số (`'0'` = 48 … `'z'` = 122), marker luôn nằm trong khoảng **80 – 154**, secret trong khoảng **92 – 166** byte tính từ đầu trang. Cả hai đều **xa 8 byte đầu** nên không bị con trỏ `next` ghi đè (đó là lý do code `secret.c` có comment *"avoid putting the secret near the very start of a page"*).

> 💡 Offset **phụ thuộc ký tự đầu** của secret ⇒ **không thể hard-code**. Cách đúng là **dò chuỗi marker**.

### 6.4 Vì sao tấn công được (diễn biến)

```
  1. secret <S>        sbrk(9 trang), ghi "Here it is: <S>" vào trang đầu, exit
  2. shell wait()      freeproc → proc_freepagetable → uvmfree → kfree từng trang
                       ⇒ các trang trả về free list, KHÔNG bị xoá (không memset)
  3. attack            sbrk(64 trang) → uvmalloc → kalloc lấy lại các trang vừa trả
                       ⇒ nội dung cũ (có marker + secret) vẫn còn nguyên
  4. attack quét bộ nhớ tìm marker, in secret
```

- Vì free list là LIFO, các trang của `secret` được cấp lại ngay cho tiến trình kế tiếp.
- `fork` + `exec` của chính `attack` cũng lấy và trả một số trang (page table, trapframe, bản sao…) nên **không đoán được** trang nào chứa secret ⇒ phải **quét cả vùng**. Một lần đo cho thấy secret nằm ở trang 19/64 ở lần `attack` đầu và trang 39/64 ở lần hai.

### 6.5 Thuật toán `attack.c`

```c
#define NPAGES 64
#define MSG "Here it is: "

len    = NPAGES * PGSIZE;
buffer = sbrk(len);                        // eager sbrk: cấp thật qua uvmalloc
for (i = 0; i + msglen < len; i++) {       // quét từng byte
    if (memcmp(buffer + i, MSG, msglen) != 0)
        continue;                          // chưa phải marker
    secret = buffer + i + msglen;          // secret nằm ngay sau marker
    n = 0;                                 // đếm ký tự chữ/số liên tiếp
    while (…secret[n] là chữ hoặc số…) n++;
    secret[n] = 0;                         // cắt chuỗi
    printf("%s\n", secret);                // in đúng một dòng, KHÔNG kèm marker
}
```

| Quyết định | Lý do |
|---|---|
| Dùng `sbrk` (eager), **không** dùng `sbrklazy` | `sbrk` cấp trang qua `uvmalloc` (đã bỏ `memset`). `sbrklazy` cấp trang lúc page fault qua `vmfault()`, mà hàm này **vẫn `memset(mem, 0, PGSIZE)` không điều kiện** ⇒ trang luôn toàn số 0, không lộ secret |
| `NPAGES = 64` (≫ 9) | `fork`/`exec` lấy bớt trang nên cần vùng đủ rộng |
| Dừng ở ký tự không phải chữ/số | Đề đảm bảo secret chỉ gồm chữ và số; `strcpy` đã đặt `'\0'` ngay sau secret |
| In **chỉ secret** | Test dùng regex `^<secret>.*`; in kèm `"Here it is: "` thì không khớp |
| Grader chạy `attack` **hai lần** | Có khi lần đầu chưa thấy; chỉ cần một lần in đúng |

### 6.6 Cách vá (nguyên nhân gốc)

Kernel **không bao giờ được cấp cho user trang chưa xoá**. Bật lại `memset(mem, 0, sz)` trong `uvmalloc()` (hoặc xoá trang trong `kalloc`/`kfree`). Khi đó trang `attack` nhận về luôn toàn số 0, không còn secret của process trước.

### 6.7 Ràng buộc

- **Chỉ sửa `user/attack.c`.** Không sửa `secret.c`, không sửa kernel cho bài attack; gỡ sạch `printk` debug nếu có thêm.
- `_attack` và `_secret` đã có sẵn trong `UPROGS`.
- Mỗi lần thử nên khởi động lại QEMU cho sạch, vì các lần `secret` cũ để lại bản sao khác trong trang rảnh.
- Test: `secret <8 ký tự>` rồi `attack` hai lần (`./grade-lab-syscall attack`).

### 6.8 Lưu ý về `attack.c` hiện tại

> ⚠️ Đây là các điểm có thể bị hỏi hoặc bị yêu cầu sửa trực tiếp khi vấn đáp.

- **Có thể in dòng trống.** Vòng lặp in cho **mọi** chỗ khớp marker, kể cả khi sau marker không có chữ/số nào (`n == 0`). Bản sao cũ của marker còn sót trong ảnh của `secret`/`attack` đã chạy trước có thể khiến ra dòng trống. Test vẫn qua vì regex chỉ cần **một** dòng khớp, nhưng output chưa "sạch". Hướng sửa: bỏ qua chỗ khớp có `n == 0`.
- **Không dừng sau lần khớp đầu.** Chương trình quét hết vùng và in mọi chỗ khớp. Nếu chỉ cần một dòng thì thoát sau khi in.
- **Mã thoát `exit(1)`** ở cuối kể cả khi thành công; grader không kiểm tra mã thoát, nhưng theo quy ước thì thành công nên là `exit(0)`.

---

## 7. Nộp bài và chấm điểm

### 7.1 Danh sách việc cần làm

- [ ] `time.txt` (một số nguyên: số giờ làm) và `answers-syscall.txt` ở thư mục gốc xv6
- [ ] `git add` + `git commit` tất cả (file chưa track **không** vào zipball); commit nhiều lần có ý nghĩa
- [ ] `make grade` pass đủ **60/60**, không warning khi build
- [ ] `make zipball` ⇒ `lab.zip`
- [ ] `Report.pdf` (≤ 10 trang)
- [ ] Gói `lab.zip` + `Report.pdf` thành `<MSSV1>_<MSSV2>.zip` (hoặc `<MSSV>.zip`), mỗi nhóm một bài

### 7.2 Nội dung `Report.pdf`

1. Thông tin nhóm và bảng phân công (thành viên, việc làm, **% đóng góp**).
2. Bảng tự đánh giá từng bài, kèm **ảnh chụp toàn bộ `make grade`**.
3. Bài 3.1: 6 câu trả lời kèm ảnh GDB làm bằng chứng.
4. Bài 3.2–3.3: danh sách file đã sửa, field mới trong `struct proc`, **sơ đồ đường đi** của một lời gọi `interpose` và của một syscall bị từ chối.
5. Bài 3.4: giải thích vì sao tấn công được, trang nào được tái dùng, offset tìm thế nào và vì sao.
6. Khó khăn gặp phải, tài liệu tham khảo.
7. **Khai báo việc dùng AI**, nêu rõ dùng vào việc gì.

### 7.3 Rubric

| # | Tiêu chí | Điểm | Điều kiện đạt tối đa |
|:-:|---|:-:|---|
| 1 | Dùng GDB | 1.5 | 6 câu đúng, mỗi câu có bằng chứng |
| 2a | Plumbing của `interpose` | 0.5 | Đủ prototype, stub, số syscall, bảng, handler; build **không warning** |
| 2b | Từ chối theo mask | 1.25 | `sandbox_mask` qua; kiểm tra **một lần** trong `syscall()`; syscall bị chặn không chạy |
| 2c | Kế thừa qua fork | 0.75 | `sandbox_fork` qua; copy trong `kfork`; state mới được khởi tạo đúng |
| 3a | Lưu path cho phép | 0.75 | `argstr` vào buffer `MAXPATH` trong `proc`, kế thừa, xử lý `"-"` |
| 3b | Kiểm tra path của open/exec | 1.75 | `sandbox_path`, `sandbox_most`, `sandbox_minus` qua; test cũ **không hồi quy** |
| 4 | Attack | 2.0 | Test qua ổn định; chỉ sửa `attack.c`; **giải thích được** trang tái dùng và offset |
| 5 | Chất lượng code, git, `time.txt` | 0.5 | Code sạch, comment hữu ích, không còn debug, nhiều commit |
| 6 | Report | 1.0 | Đủ mục, giải thích bằng lời của nhóm, đúng ảnh chụp |

### 7.4 Hệ số vấn đáp cá nhân

Điểm cá nhân = điểm nhóm × hệ số. Mỗi người có thể bị yêu cầu **sửa code trực tiếp** ở bất kỳ phần nào.

| Hệ số | Mức độ hiểu bài |
|:---:|---|
| × 1.0 | Giải thích được mọi phần và làm được thay đổi trực tiếp |
| × 0.9 | Hiểu ý chính, còn thiếu sót nhỏ |
| × 0.7 | Chỉ hiểu phần mình làm |
| × 0.3 | Không giải thích được bài nộp |

### 7.5 Chính sách AI

- Đề CS333 cho dùng AI **nếu khai báo trong report**.
- `AGENTS.md` trong branch trỏ tới chính sách của MIT 6.1810, vốn **không cho** AI viết hay debug code lab.
- Hai quy định khác nhau, nên cần hỏi giảng viên CS333 mức độ được phép. Dù thế nào cũng phải **khai báo đúng** đã dùng AI vào đâu.

---

## 8. Bảng tra nhanh

### 8.1 Số syscall và giá trị mask

| Syscall | Số | Mask (`1 << n`) | | Syscall | Số | Mask (`1 << n`) |
|---|:-:|---:|---|---|:-:|---:|
| `fork` | 1 | 2 | | `open` | 15 | 32768 |
| `exit` | 2 | 4 | | `write` | 16 | 65536 |
| `wait` | 3 | 8 | | `mknod` | 17 | 131072 |
| `pipe` | 4 | 16 | | `unlink` | 18 | 262144 |
| `read` | 5 | 32 | | `link` | 19 | 524288 |
| `kill` | 6 | 64 | | `mkdir` | 20 | 1048576 |
| `exec` | 7 | 128 | | `close` | 21 | 2097152 |
| `fstat` | 8 | 256 | | `sync` | 22 | 4194304 |
| `chdir` | 9 | 512 | | **`interpose`** | **23** | **8388608** |
| `dup` | 10 | 1024 | | | | |
| `getpid` | 11 | 2048 | | | | |
| `sbrk` | 12 | 4096 | | | | |
| `pause` | 13 | 8192 | | | | |
| `uptime` | 14 | 16384 | | | | |

### 8.2 Lệnh thường dùng

| Mục đích | Lệnh |
|---|---|
| Build rồi chạy xv6 | `make qemu` |
| Chạy xv6 chờ GDB | `make qemu-gdb` |
| Build lại cả image file system | `make fs.img` |
| Chạy một test | `./grade-lab-syscall sandbox_mask` |
| Chạy toàn bộ test | `make grade` |
| Đóng gói nộp | `make zipball` |
| Thoát QEMU | `Ctrl-A` rồi `X` |

> 💡 `make` thường chỉ build kernel; muốn các chương trình user mới (như `sandbox`, `sbtest`) vào `fs.img` thì dùng `make fs.img` hoặc `make qemu`.

### 8.3 Ví dụ để tự thử trong xv6

```
$ sandbox 32768 - cat README                  # cat: cannot open README
$ sandbox 32768 README grep xv6 README        # in các dòng có "xv6"
$ sandbox 32768 README grep xv6 x             # grep: cannot open x
$ sandbox 32768 - sh < exec.sh                # hạn chế kế thừa qua cháu
$ secret xyzzy
$ attack                                      # in: xyzzy
```

---

## 9. Những chỗ dễ sai

| Tiêu chí | Lỗi thường gặp |
|:-:|---|
| 2a | Build có **warning**, code thừa |
| 2b | Rải check trong từng handler; chỉ xử lý `open`; chặn rồi mà vẫn chạy handler |
| 2c | Quên reset state ⇒ rò sang process mới; không kế thừa cho cháu |
| 3a | Không kiểm tra lỗi `argstr`; coi `---` là `-`; không kế thừa path |
| 3b | Làm hỏng test cũ khi sửa cho 3.3; **bỏ sót `sandbox_escape`** |
| 4 | Hard-code offset không giải thích; in kèm `"Here it is: "` hoặc dòng thừa; sửa kernel hay `secret.c` |
| 5 | Chỉ một commit; còn code debug; thiếu `time.txt` |
| Q&A | Một người không giải thích được phần của người kia ⇒ hệ số giảm mạnh |
| Chung | Quên **khôi phục** `num = p->trapframe->a7;` sau bài GDB; quên đổi địa chỉ `sepc` sau khi build lại |

---

*Tài liệu tham khảo: đề CS333 Lab 2; MIT 6.1810 "Lab: System calls"; xv6 book (Chương 2, mục 4.3 và 4.4 của Chương 4); RISC-V privileged architecture manual.*
