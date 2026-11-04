# ❓ CS333 – Lab 2: Câu hỏi vấn đáp

### System Calls · xv6-riscv · kèm đáp án

> Tài liệu đi kèm: [`Lab2_Knowledge.md`](Lab2_Knowledge.md) (kiến thức nền, sơ đồ, bảng tra nhanh).
>
> Đáp án bám theo **code thật** trong `xv6-labs-2026` và `answers-syscall.txt`. Địa chỉ như `0x80001d6e` chỉ đúng với bản build đã chụp ảnh; build lại có thể khác.

**Quy ước độ khó:** 🟢 cơ bản · 🟡 trung bình · 🔴 nâng cao

---

## Mục lục

| Phần | Chủ đề | Số câu |
|:---:|---|:---:|
| [A](#a-system-call-và-trap) | System call và trap | 11 |
| [B](#b-gdb-và-kernel-panic-bài-31) | GDB và kernel panic (bài 3.1) | 11 |
| [C](#c-interpose-theo-mask-bài-32) | `interpose` theo mask (bài 3.2) | 13 |
| [D](#d-pathname-và-chống-thoát-sandbox-bài-33) | Pathname và chống thoát sandbox (bài 3.3) | 14 |
| [E](#e-tấn-công-xv6-bài-34) | Tấn công xv6 (bài 3.4) | 13 |
| [F](#f-yêu-cầu-sửa-code-trực-tiếp) | Yêu cầu sửa code trực tiếp | 11 |
| [G](#g-quy-trình-git-và-report) | Quy trình, git, report | 6 |

## Mẹo khi trả lời vấn đáp

1. **Đi theo đường chạy của dữ liệu**: user → `ecall` → `usertrap` → `syscall()` → `rejected()` → handler → `a0`. Câu nào về syscall cũng bắt đầu từ đây.
2. **Nói rõ "vì sao"**, không chỉ "làm gì": mỗi dòng code trong bài đều có lý do (fail closed, chỉ tích luỹ, không hard-code offset…).
3. **Gắn câu trả lời với test**: nói được test nào kiểm tra điều đó (`sandbox_escape`, `sandbox_minus`, `sandbox_block`…) là điểm cộng lớn.
4. Khi được yêu cầu **sửa trực tiếp**, nói hướng làm và các file bị ảnh hưởng trước khi gõ, rồi chạy lại `./grade-lab-syscall`.
5. Biết **điểm yếu của bài mình** (ví dụ `attack.c` có thể in dòng trống) và nói thẳng, đừng để người hỏi tự phát hiện.

---

## A. System call và trap

### A1 🟢 System call là gì? Vì sao cần có?

**Trả lời:** System call là cổng vào kernel có kiểm soát. Chương trình user chạy ở User mode nên **không** được truy cập phần cứng hay bộ nhớ kernel; muốn làm việc đó (mở file, tạo process, xin bộ nhớ…) thì phải nhờ kernel qua syscall. Nhờ vậy kernel kiểm tra quyền và đối số trước khi làm, và các process bị **cô lập** với nhau.

### A2 🟢 Mô tả đường đi của lời gọi `interpose(mask, path)` từ user vào kernel và quay về.

**Trả lời:**

1. Chương trình gọi `interpose()`, prototype ở `user/user.h`.
2. Stub trong `usys.S` (sinh từ `usys.pl`) nạp `a7 = SYS_interpose` (23) rồi chạy `ecall`.
3. CPU chuyển sang Supervisor mode, nhảy tới `uservec` (`trampoline.S`): lưu thanh ghi vào trapframe, đổi sang page table kernel.
4. `usertrap()` (`trap.c`) thấy `scause == 8` (ecall), cộng `epc += 4`, bật ngắt, gọi `syscall()`.
5. `syscall()` đọc `num = a7`, hỏi `rejected(p, num)`. Nếu không bị từ chối thì gọi `syscalls[num]()` = `sys_interpose()`.
6. Kết quả ghi vào `trapframe->a0`.
7. `usertrapret()` → `userret` khôi phục thanh ghi, `sret` về user ngay sau `ecall`. Giá trị trả về là `a0`.

### A3 🟢 Số syscall, đối số và kết quả đi qua thanh ghi nào?

**Trả lời:** Số syscall ở **`a7`**; đối số ở **`a0`–`a5`**; kết quả trả về ở **`a0`**. Kernel đọc/ghi các giá trị này trong `p->trapframe`.

### A4 🟡 Vì sao `usertrap()` cộng `epc += 4` trước khi gọi `syscall()`?

**Trả lời:** `sepc` lưu địa chỉ của chính lệnh `ecall`. Nếu không cộng 4, sau `sret` CPU chạy lại `ecall` và lặp vô hạn. Cộng 4 để quay về **lệnh kế tiếp**.

### A5 🟢 `usys.S` đến từ đâu? Một stub trông thế nào?

**Trả lời:** `usys.S` **được sinh ra lúc `make`** từ `user/usys.pl` (Perl). Mỗi `entry("name")` sinh một stub:

```
name:
    li   a7, SYS_name
    ecall
    ret
```

Vì vậy muốn thêm syscall chỉ cần thêm `entry("interpose");` vào `usys.pl`, không sửa `usys.S` bằng tay.

### A6 🟡 Vì sao kernel không dùng thẳng con trỏ do user đưa vào?

**Trả lời:** Hai lý do: (1) user và kernel dùng **page table khác nhau**, địa chỉ user không có nghĩa trong không gian kernel; (2) con trỏ có thể **sai hoặc độc hại** (trỏ ra ngoài vùng của process, vào bộ nhớ kernel…). Vì thế kernel dùng `copyin`, `copyinstr`, `copyout`, các hàm này dịch địa chỉ bằng page table của process và **kiểm tra giới hạn**.

### A7 🟡 `argint`, `argaddr`, `argstr` khác nhau thế nào? Hàm nào có thể lỗi?

**Trả lời:**

| Hàm | Lấy gì | Có thể lỗi? |
|---|---|:---:|
| `argint(n, &x)` | Đối số `n` dạng số nguyên | Không |
| `argaddr(n, &a)` | Đối số `n` dạng địa chỉ, **không** kiểm tra hợp lệ | Không |
| `argstr(n, buf, max)` | Chuỗi mà đối số `n` trỏ tới, copy vào `buf` | **Có** (trả `-1`) |

Chỉ `argstr` thất bại được (con trỏ sai, chuỗi dài hơn `max` mà không có `'\0'`), nên code **phải kiểm tra `argstr < 0`**.

### A8 🟡 Trapframe là gì? Nó lưu ở đâu?

**Trả lời:** `struct trapframe` là vùng nhớ (một trang, `p->trapframe`) lưu **toàn bộ thanh ghi user** tại thời điểm trap, cộng vài giá trị kernel cần như `epc`, `kernel_satp`, `kernel_sp`. `uservec` lưu thanh ghi vào đây khi vào kernel, `userret` nạp lại khi ra. Kernel đọc đối số syscall (`a0`–`a5`, `a7`) và ghi kết quả (`a0`) qua trapframe.

### A9 🔴 Trang trampoline dùng để làm gì?

**Trả lời:** `uservec` phải đổi `satp` từ page table user sang page table kernel. Ngay sau lệnh đổi đó CPU vẫn lấy lệnh kế tiếp theo **cùng địa chỉ ảo**. Vì trampoline được map ở **cùng một địa chỉ ảo** trong cả hai page table nên chương trình vẫn chạy đúng qua lúc chuyển đổi.

### A10 🟡 Nếu số syscall không hợp lệ thì `syscall()` làm gì?

**Trả lời:** Điều kiện `num > 0 && num < NELEM(syscalls) && syscalls[num]` sai ⇒ nhánh `else`: in `"%d %s: unknown sys call %d"` bằng **`printk`** (hàm in của kernel) và trả `-1`. Điều kiện `syscalls[num]` bắt cả ô trống trong bảng (bảng khởi tạo bằng designated initializer nên có thể thủng).

### A11 🔴 Khi chạy `ecall`, phần cứng làm gì và phần mềm phải làm gì?

**Trả lời:**

| Phần cứng (tự động) | Phần mềm xv6 (`uservec`, `usertrap`) |
|---|---|
| Ghi `sepc` = địa chỉ lệnh `ecall` | Lưu 32 thanh ghi user vào trapframe |
| Ghi `scause` = 8, `sstatus.SPP` = 0 | Đổi `satp` sang page table kernel |
| Tắt ngắt (`SIE` → `SPIE`, `SIE` = 0) | Chuyển sang kernel stack |
| Chuyển sang Supervisor mode | Đọc `scause`, `epc += 4`, bật ngắt, gọi `syscall()` |
| Nhảy tới địa chỉ trong `stvec` (`uservec`) | |

Phần cứng **không** đổi page table và **không** lưu thanh ghi nào ngoài `sepc`; phần đó là việc của phần mềm.

---

## B. GDB và kernel panic (bài 3.1)

### B1 🟢 Cách dùng GDB với xv6?

**Trả lời:** Mở hai terminal tại thư mục xv6. Terminal 1 chạy `make qemu-gdb` (QEMU dừng chờ GDB), terminal 2 chạy `gdb-multiarch` (hoặc `riscv64-unknown-elf-gdb`). `make qemu-gdb` sinh `.gdbinit` từ `.gdbinit.tmpl-riscv` nên GDB tự kết nối. Rồi `b syscall`, `c`, `layout src`, `backtrace`.

### B2 🟡 Q1: Hàm nào gọi `syscall()`?

**Trả lời:** **`usertrap()`**, ở `kernel/trap.c:68` trong bản build này. `backtrace` cho thấy frame 0 là `syscall()`, frame 1 là `usertrap()`; frame 2 hiện `?? ()` vì GDB không unwind được qua ranh giới trap. Luồng đúng là: user `ecall` → trap-entry → `usertrap()` → `syscall()`.

### B3 🟡 Q2: `p->trapframe->a7` bằng bao nhiêu và có ý nghĩa gì?

**Trả lời:** Giá trị quan sát được là **15**, và 15 = `SYS_open`. `a7` chứa **số syscall** mà kernel dùng để chọn handler. Process lúc đó là `init` (pid 1).

**Vì sao khác hướng dẫn?** Đề gợi ý `user/initcode.S` (gọi `exec` = 7), nhưng bản xv6 này **không có** file đó: `forkret()` gọi thẳng `kexec("/init", …)`. Syscall đầu tiên của `init` là `open("console", O_RDWR)` nên `a7 = 15`.

### B4 🟡 Q3: CPU trước đó ở mode nào? Xác định thế nào?

**Trả lời:** **User mode.** `p /x $sstatus` cho `0x200000022`; bit 8 là **`SPP`** (Previous Privilege) bằng **0** nghĩa là trước khi trap CPU ở User mode (1 = Supervisor). Đúng với thực tế: process user chạy `ecall` để vào kernel.

### B5 🔴 Giải mã `sstatus = 0x200000022`.

**Trả lời:** Tách thành `0x2_0000_0000` + `0x22`:

- `0x2_0000_0000` = bit 33 ⇒ trường `UXL` = 2, tức User mode chạy 64-bit.
- `0x22` = bit 1 (**`SIE`**) + bit 5 (**`SPIE`**). `SIE` đang bật vì `usertrap()` đã gọi `intr_on()`; `SPIE` bật để `sret` bật lại ngắt khi về user.
- Bit 8 (**`SPP`**) = 0 ⇒ User mode.

### B6 🟡 Q4: Lệnh assembly gây panic là gì? Thanh ghi nào ứng với `num`?

**Trả lời:** `sepc = 0x80001d6e`; tra `kernel/kernel.asm` thấy lệnh **`lw a3,0(zero)`**, tức `num = *(int *)0;` chuyển thành "load 32-bit từ địa chỉ 0 vào `a3`". Vậy **`a3`** là thanh ghi ứng với `num`. `zero` (`x0`) luôn bằng 0 nên `0(zero)` = địa chỉ 0.

> `a3` là thanh ghi **trình biên dịch chọn** cho `num` ở bản build này, không phải quy ước cố định.

### B7 🟡 Q5: Vì sao kernel crash?

**Trả lời:** `num = *(int *)0` đọc từ **địa chỉ ảo 0**, mà địa chỉ 0 **không được map** trong kernel page table (xem Figure 3-3 trong xv6 book). CPU báo **load page fault**:

- `scause = 0xd` (13) = load page fault,
- `stval = 0x0` = địa chỉ gây lỗi,
- `sepc = 0x80001d6e` = lệnh gây lỗi.

Lỗi xảy ra **trong kernel** và không có cách phục hồi, nên `kerneltrap()` gọi `panic("kerneltrap")`.

### B8 🟡 Q6: Process nào đang chạy lúc panic? Pid bao nhiêu?

**Trả lời:** **`init`, pid 1.** Đặt `b *0x80001d6e` để dừng đúng tại lệnh lỗi (trước khi nó chạy), rồi `p p->name` và `p p->pid`. Đó là process đầu tiên gọi syscall sau khi boot.

### B9 🟡 `sepc`, `scause`, `stval` là gì?

**Trả lời:**

| Thanh ghi | Ý nghĩa |
|---|---|
| `sepc` | Địa chỉ lệnh đang chạy khi trap xảy ra |
| `scause` | Nguyên nhân trap (8 = ecall, 13 = load page fault, 15 = store page fault, 12 = instruction page fault) |
| `stval` | Thông tin phụ; với page fault là **địa chỉ ảo gây lỗi** |

### B10 🟡 Từ `sepc` tìm ra lệnh/dòng code gây lỗi thế nào?

**Trả lời:** Tìm giá trị `sepc` trong `kernel/kernel.asm` (assembly của kernel đã biên dịch); xen kẽ có cả dòng C tương ứng. Hoặc trong GDB: `b *<sepc>` rồi `layout asm` / `x/i $pc`. Nhớ rằng địa chỉ đổi mỗi khi build lại, nên phải tra trên **bản build hiện tại**.

### B11 🟡 Vì sao lỗi trong kernel gây `panic`, còn lỗi ở user chỉ giết process?

**Trả lời:** Trap từ User mode vào `usertrap()`: nếu lỗi không xử lý được, kernel in `unexpected scause …` rồi `setkilled(p)`, chỉ process đó chết, kernel vẫn ổn. Trap xảy ra **khi đang ở Supervisor mode** đi vào `kerneltrap()`: kernel tự có lỗi nên trạng thái không đáng tin, không có process nào để "giết" thay, nên gọi `panic()` dừng hệ thống.

---

## C. `interpose` theo mask (bài 3.2)

### C1 🟢 `interpose(int mask, const char *path)` làm gì?

**Trả lời:** Hạn chế process gọi nó. Bit `n` của `mask` bật ⇒ syscall số `n` bị **từ chối** (trả `-1`, handler **không chạy**). Hạn chế được **kế thừa** qua `fork`. Tham số `path` cho phép ngoại lệ với `open`/`exec` (bài 3.3); `"-"` nghĩa là không có ngoại lệ.

### C2 🟢 Tính mask để chặn `open`; `open` + `exec`; mọi syscall trừ `exit`.

**Trả lời:** Mask là tổng các `1 << SYS_x`:

| Muốn chặn | Mask |
|---|---:|
| `open` (số 15) | `1 << 15` = **32768** |
| `exec` (số 7) | `1 << 7` = 128 |
| `open` + `exec` | 32768 + 128 = **32896** |
| Mọi thứ trừ `exit` (số 2) | bit 0–30 trừ bit 2 = **2147483643** (`0x7FFFFFFB`) |

### C3 🟡 Vì sao kiểm tra mask trong `syscall()` mà không trong từng handler?

**Trả lời:** **Mọi** syscall đều đi qua `syscall()`, nên kiểm tra một lần ở đó là đủ và **không thể bỏ sót** syscall nào (kể cả syscall thêm sau này). Rải check trong từng handler dễ quên, lặp code, và dễ sai (ví dụ chỉ xử lý `open`). Đây là tiêu chí 2b của rubric.

### C4 🟡 Syscall bị từ chối có chạy handler không? Chứng minh thế nào?

**Trả lời:** **Không.** `syscall()` ghi `a0 = -1` và bỏ qua `syscalls[num]()`. Test `sandbox_block` chứng minh: chặn `fork` rồi chạy `sbtest fork` thì **không** có dòng `CHILD RAN` và `fork` trả `-1`; chặn `getpid` thì `getpid` trả `-1`. Process vẫn sống (dòng `sbtest: alive` xuất hiện đủ hai lần).

### C5 🟡 Sandbox được kế thừa qua `fork` ở đâu?

**Trả lời:** Trong `kfork()`, ngay cạnh chỗ copy tên process: `np->sandbox = p->sandbox;`. Vì `struct sandbox` chứa cả `mask` và `path`, **một phép gán** copy cả hai. Kế thừa chạy theo chuỗi nên cháu, chắt cũng bị hạn chế (test `sandbox_fork`).

### C6 🟡 Vì sao phải xoá sandbox ở cả `allocproc()` và `freeproc()`?

**Trả lời:** Slot trong bảng `proc` được **tái sử dụng**. Nếu không xoá, process mới có thể nhận sandbox của process cũ không liên quan (rò state, bị trừ ở tiêu chí 2c). Xoá khi cấp và khi giải phóng bằng `memset(&p->sandbox, 0, sizeof(p->sandbox))`. Sau đó `mask = 0` nên process không bị hạn chế gì.

### C7 🟡 `exec` có làm mất sandbox không?

**Trả lời:** **Không.** `exec` chỉ thay ảnh bộ nhớ của process; `struct proc` (kể cả `sandbox`) giữ nguyên. Đây chính là điều làm chương trình `sandbox` hoạt động: con gọi `interpose` rồi `exec` lệnh cần chạy, lệnh đó vẫn bị hạn chế.

### C8 🟡 Vì sao gom `mask` và `path` vào `struct sandbox`?

**Trả lời:** Để `kfork` sao chép bằng **một phép gán** và `allocproc`/`freeproc` xoá bằng **một `memset`**. Không thể quên một nửa state, và thiết kế gọn, dễ đọc.

### C9 🟡 Có cần khoá (`p->lock`) khi đọc/ghi sandbox không?

**Trả lời:** **Không.** Field này chỉ do **chính process đó** đọc/ghi (giống `p->sz`, `p->name`). Trong `kfork` ta đọc từ `p` và ghi vào `np` khi `np` chưa chạy được nên cũng không cần thêm khoá.

### C10 🔴 Vì sao `sandbox_most` chừa bit 2 (`exit`)?

**Trả lời:** Nếu `exit` cũng bị chặn thì lời gọi `exit()` trả `-1` và process **không thể tự kết thúc**. Mask `2147483643` chặn mọi syscall trừ `exit` để process còn thoát được. Syscall `exec grep` vẫn qua nhờ path `grep`.

### C11 🟡 Trong `sandbox <mask> <path> <lệnh>`, process nào bị hạn chế?

**Trả lời:** Chỉ **process con**. Cha gọi `fork()` rồi `wait()`; con gọi `interpose()` rồi `exec(lệnh)`. Cha không gọi `interpose` nên không bị hạn chế.

### C12 🔴 `mask` kiểu `uint`, còn `1 << num` thì sao? Giới hạn là gì?

**Trả lời:** `mask` là `uint` để thao tác bit 31 an toàn khi so mask lớn như `2147483643`. Với `num` < 24 (số syscall tối đa hiện là 23) thì `1 << num` luôn nằm trong 32 bit. Nếu sau này có syscall số ≥ 32 thì mask 32-bit **không đủ**: phải đổi sang `uint64`, viết `1ULL << num`, và lấy đối số bằng `argaddr` (vì `argint` chỉ cho 32-bit).

### C13 🟢 Kết quả mong đợi của `sandbox 32768 - cat README`? Giải thích.

**Trả lời:** In `cat: cannot open README`. Con `interpose(32768, "-")`, rồi `exec("cat")` (bit 7 không bật nên qua). `cat` gọi `open("README")`: bit 15 bật, `num` là `open`, path là `"-"` ⇒ `rejected()` = 1 ⇒ `a0 = -1` ⇒ `cat` báo không mở được.

---

## D. Pathname và chống thoát sandbox (bài 3.3)

### D1 🟢 Bài 3.3 thêm gì so với 3.2?

**Trả lời:** Một syscall **`open` hoặc `exec` bị mask** vẫn được chạy nếu **pathname của lời gọi bằng `path`** đã đăng ký. Mọi syscall bị mask khác, và `open`/`exec` có pathname khác, vẫn bị từ chối. `path = "-"` nghĩa là không pathname nào được phép.

### D2 🟡 Tóm tắt thứ tự quyết định trong `rejected()`.

**Trả lời:**

1. Bit `num` không bật ⇒ **cho chạy** (không đọc path).
2. Bit bật nhưng `num` không phải `open`/`exec` ⇒ **từ chối**.
3. Path được phép là `"-"` ⇒ **từ chối**.
4. `argstr(0, …)` lỗi ⇒ **từ chối** (fail closed).
5. Pathname khác path được phép ⇒ **từ chối**; bằng nhau ⇒ cho chạy.

### D3 🟡 Vì sao gọi `argstr(0, …)` ngay trong `syscall()`, trước handler, vẫn lấy đúng pathname?

**Trả lời:** Pathname của cả `open` lẫn `exec` đều là **đối số 0**, nằm ở `a0` trong trapframe. Khi `rejected()` chạy, handler chưa chạy nên `a0` **chưa bị ghi đè** bằng giá trị trả về. `argstr` đọc `a0` rồi copy chuỗi từ user memory.

### D4 🟡 Vì sao `"-"` phải so khớp **trọn chuỗi** chứ không so ký tự đầu?

**Trả lời:** Nếu chỉ so ký tự đầu thì file tên `---` bị coi nhầm là `-`. Đề nói rõ `---` là pathname **bình thường**; test `sandbox_minus` đặt path `---` và mong file `---` mở được. `strncmp(sb->path, "-", MAXPATH) == 0` chỉ đúng khi cả chuỗi đúng là `"-"`.

### D5 🟡 `./README` có bằng `README` không?

**Trả lời:** **Không.** Bài so sánh **chính xác từng ký tự**, không chuẩn hoá đường dẫn. Đề yêu cầu "pathname bằng `path`", nên `./README` ≠ `README`. Chuẩn hoá (bỏ `./`, xử lý `..`, symlink) phức tạp và dễ mở lỗ hổng.

### D6 🟡 Vì sao `argstr` lỗi thì **từ chối**?

**Trả lời:** Đó là nguyên tắc **fail closed**: khi không xác định được pathname thì không thể kết luận nó là path được phép, nên an toàn nhất là từ chối. Cho qua khi lỗi sẽ tạo lỗ hổng. Rubric 3a cũng yêu cầu kiểm tra lỗi `argstr`.

### D7 🟡 Có lỗi TOCTOU không (kiểm tra xong, handler đọc lại thì chuỗi đã đổi)?

**Trả lời:** **Không** trong xv6. Kernel đọc pathname ở `rejected()` rồi handler đọc lại; trong khoảng đó process **đang ở trong kernel** và xv6 **không có thread** hay bộ nhớ chia sẻ nên không ai sửa được chuỗi trong user memory.

### D8 🔴 "Thoát sandbox" (`sandbox_escape`) là gì? Chặn bằng cách nào?

**Trả lời:** Nếu `interpose` chỉ **ghi đè** mask và path, process bị chặn `open` có thể gọi `interpose(0, "README")` hoặc `interpose(0, "-")` để tự gỡ hạn chế. Có **hai luật** phòng thủ trong `sys_interpose`:

| Luật | Cách làm |
|---|---|
| **Mask chỉ tích luỹ** | `sb->mask \|= mask`: không bao giờ xoá bit ⇒ `interpose(0, …)` không đổi mask |
| **Path không được nới rộng** | Nếu mask hiện tại đã hạn chế `open`/`exec`, path mới chỉ được **bằng path hiện tại** hoặc là `"-"`; còn lại trả `-1` |

### D9 🔴 Vì sao kiểm tra "đã bị hạn chế chưa" phải làm **trước** `sb->mask |= mask`?

**Trả lời:** Câu hỏi cần trả lời là "process **đang** bị hạn chế open/exec chưa". Nếu cộng mask mới trước, một process chưa bị hạn chế gọi `interpose(open, "X")` lần đầu sẽ bị coi như "đã hạn chế" và bị từ chối nhầm. Phải xét trạng thái **cũ** rồi mới cập nhật.

### D10 🔴 `interpose(0, 0)` (con trỏ NULL) xử lý ra sao?

**Trả lời:** Chương trình user của xv6 được link ở **địa chỉ ảo 0** (`user.ld`: `. = 0x0`), nên địa chỉ 0 là **text của chương trình**, không phải vùng không map. Vì vậy `argstr` **không chắc** thất bại: có thể đọc "chuỗi" rác từ đầu text (nếu thấy `'\0'` trong 128 byte) hoặc thất bại (nếu không thấy). Với process **đã bị hạn chế**, kết quả vẫn luôn là `-1`:

- `argstr` lỗi ⇒ trả `-1` ngay;
- `argstr` thành công ⇒ chuỗi rác khác path hiện tại và khác `"-"` ⇒ luật path trả `-1`.

### D11 🟡 Vì sao `sys_interpose` đọc path vào buffer cục bộ trước khi ghi vào `struct proc`?

**Trả lời:** Để **kiểm tra hết rồi mới thay đổi state**. Nếu đọc thẳng vào `sb->path` mà `argstr` lỗi giữa chừng, `path` bị ghi dở (trạng thái nửa vời, còn có thể làm hỏng sandbox hiện có). Đọc vào `path[MAXPATH]` cục bộ, kiểm tra xong, rồi mới `safestrcpy` vào `sb->path`.

### D12 🟡 Vì sao `sandbox_most` cần path `grep`?

**Trả lời:** `sandbox.c` gọi `interpose()` rồi **`exec(lệnh)`**, và chính `exec` này cũng bị lọc (mask chặn mọi syscall trừ `exit`, trong đó có `exec`). Đặt path `grep` thì `exec("grep")` có pathname trùng path nên được cho qua.

### D13 🟡 Giải thích test `sandbox 128 sh …` trong `sandbox_path`.

**Trả lời:** Mask `128` = chặn `exec`; path `sh`. `exec("sh")` có pathname trùng ⇒ cho chạy. Nhưng `sh` vẫn bị sandbox (sandbox còn nguyên sau `exec`), `sh` fork ra con kế thừa sandbox, con `exec("echo")` có pathname khác `sh` ⇒ bị từ chối. Vậy `exec sh` qua còn các `exec` con thì không.

### D14 🔴 Khi mask **chưa** hạn chế open/exec, process có được đặt path tuỳ ý không? Có hại không?

**Trả lời:** Được, vì lúc đó path **chưa có tác dụng** (chỉ dùng khi bit `open`/`exec` bật). Không gây hại: lần đầu process tự `interpose(1<<SYS_open, "X")` là **tự hạn chế chặt thêm**, không phải nới. Sau khi bit `open`/`exec` đã bật thì mới áp dụng luật "path chỉ giữ nguyên hoặc thành `-`".

---

## E. Tấn công xv6 (bài 3.4)

### E1 🟢 Lỗi nào đã được cố ý cài vào xv6 ở bài này?

**Trả lời:** Khi biên dịch với `LAB_SYSCALL`, ba câu lệnh xoá/tô rác bộ nhớ bị bỏ (`#ifndef LAB_SYSCALL`): `memset(mem, 0, sz)` trong `uvmalloc()` (`vm.c`), `memset(pa, 1, PGSIZE)` trong `kfree()` và `memset(r, 5, PGSIZE)` trong `kalloc()` (`kalloc.c`). Hệ quả: trang mới cấp (qua `sbrk`) **giữ nguyên nội dung lần dùng trước**, tức dữ liệu rò giữa các process.

### E2 🟡 Giải thích vì sao tấn công hoạt động, theo từng bước.

**Trả lời:**

1. `secret <S>` gọi `sbrk(9 trang)`, ghi `"Here it is: <S>"` vào trang đầu rồi `exit`.
2. Shell gọi `wait()` ⇒ `freeproc` ⇒ `proc_freepagetable` ⇒ `uvmfree` ⇒ `kfree` từng trang. Vì **không còn `memset`**, nội dung (kể cả secret) **không bị xoá**.
3. Free list của `kalloc` là **LIFO**, nên các trang vừa trả được cấp lại ngay.
4. `attack` gọi `sbrk(64 trang)` ⇒ `uvmalloc` ⇒ `kalloc` lấy lại những trang đó, nguyên nội dung cũ, **không bị xoá**.
5. `attack` quét tìm chuỗi `"Here it is: "` và in secret đứng sau.

### E3 🟡 Free list LIFO nghĩa là gì và vì sao quan trọng?

**Trả lời:** `kfree(X)` đưa `X` vào **đầu** danh sách (`X->next = freelist; freelist = X`), `kalloc()` lấy trang ở **đầu** danh sách. Nên trang **vừa giải phóng** là trang **được cấp lại đầu tiên**. Nhờ vậy các trang của `secret` (vừa thoát) rất dễ rơi vào `attack` (chạy ngay sau).

### E4 🟡 `secret.c` đặt secret ở vị trí nào trong bộ nhớ?

**Trả lời:** Trong **trang đầu** của vùng 9 trang: `sbrk(9 * 4096)`, **căn lên đầu trang** kế tiếp nếu chưa căn, rồi cộng thêm `32 + ASCII(ký tự đầu của secret)`. Tại đó ghi `"Here it is: "` (12 byte), secret nằm ngay sau. Vậy marker ở offset `32 + ASCII(ký tự đầu)` và secret ở offset `+12`.

Ví dụ: `abc123XY` → marker offset **129**; `Zq9mLpT4` → **122**; `xyzzy` → **152**.

### E5 🟡 Vì sao `secret.c` tránh đặt secret ở 8 byte đầu trang?

**Trả lời:** `kfree()` ghi con trỏ free-list (`struct run`, trường `next`) vào **8 byte đầu** của trang đã giải phóng, làm hỏng dữ liệu ở đó. Code `secret.c` có comment *"avoid putting the secret near the very start of a page"*. Với secret chỉ gồm chữ và số, marker nằm trong khoảng offset **80–154**, secret trong **92–166**, đều xa 8 byte đầu.

### E6 🔴 Vì sao không thể hard-code offset hay chỉ số trang?

**Trả lời:** (1) Offset **phụ thuộc ký tự đầu** của secret nên đổi theo mỗi secret. (2) Chỉ số trang trong vùng `sbrk` của `attack` không suy ra được: `fork`, `exec` của `attack` cũng lấy và trả một số trang (page table, trapframe, bản sao…), thứ tự cấp phát thay đổi. Một lần đo cho thấy secret ở trang 19/64 (lần `attack` đầu) và 39/64 (lần hai). Vì vậy phải **dò chuỗi marker** trên cả vùng.

### E7 🟡 Vì sao `attack` xin tới 64 trang?

**Trả lời:** `secret` chỉ dùng 9 trang và `fork`/`exec` của `attack` lấy bớt trang từ free list, nên chỉ số trang chứa secret không biết trước. Xin vùng **lớn hơn nhiều** (64) để chắc chắn phủ được trang chứa marker.

### E8 🟡 Vì sao dùng `sbrk` mà không dùng `sbrklazy`?

**Trả lời:** `sbrk` (eager) cấp trang ngay qua `uvmalloc`, đã bỏ `memset`, nên trang giữ nguyên dữ liệu cũ. `sbrklazy` cấp trang lúc page fault qua **`vmfault()`**, mà hàm này vẫn có `memset(mem, 0, PGSIZE)` **không điều kiện** (không bọc `#ifndef LAB_SYSCALL`). Trang cấp theo kiểu lazy luôn toàn số 0 nên **không lộ** secret.

### E9 🟡 Vì sao chỉ in secret, không in kèm `"Here it is: "`?

**Trả lời:** Test kiểm tra bằng regex `^<secret>.*`, dòng output phải **bắt đầu bằng secret**. Đề cũng yêu cầu in "một dòng chỉ chứa secret". In kèm marker làm dòng không khớp regex.

### E10 🟡 Vì sao grader chạy `attack` hai lần?

**Trả lời:** Tuỳ cách cài, lần chạy đầu có thể chưa gặp trang chứa secret (do thứ tự cấp phát thay đổi) và lần chạy sau mới thấy. Grader chạy `attack` hai lần và **đạt nếu một trong hai lần in ra secret**.

### E11 🔴 Giải thích từng phần của `attack.c`.

**Trả lời:**

| Đoạn code | Ý nghĩa |
|---|---|
| `len = NPAGES * PGSIZE`, `buffer = sbrk(len)` | Xin 64 trang cấp thật; nếu trả `(char*)-1` thì `exit(1)` |
| `for (i = 0; i + msglen < len; i++)` | Quét từng byte, dừng đủ sớm để `memcmp` không đọc quá vùng |
| `memcmp(buffer + i, MSG, msglen) != 0 → continue` | Chưa phải marker thì bỏ qua |
| `secret = buffer + i + msglen` | Secret nằm ngay sau marker 12 byte |
| `while (… chữ hoặc số …) n++` | Đếm ký tự chữ/số liên tiếp, dừng ở ký tự đầu tiên không phải chữ/số (`'\0'` do `strcpy` đặt sẵn) |
| `secret[n] = 0; printf("%s\n", secret)` | Cắt chuỗi rồi in đúng secret, không kèm marker |

Điều kiện `i + msglen + n < len - 1` trong vòng `while` chặn đọc ra ngoài vùng nhớ.

### E12 🟡 Cách vá lỗi này?

**Trả lời:** Kernel **không bao giờ được cấp cho user trang chưa xoá**: bật lại `memset(mem, 0, sz)` trong `uvmalloc()` (bỏ `#ifndef LAB_SYSCALL`). Khi đó trang `attack` nhận về toàn số 0. Việc tô rác ở `kalloc`/`kfree` chỉ để debug (bắt dangling reference) nên không thay thế được: user cần trang **số 0** theo quy ước của `sbrk`, không phải trang đầy giá trị rác.

### E13 🔴 `attack.c` của bạn có thể in dòng trống không? Tại sao? Sửa thế nào?

**Trả lời:** **Có thể.** Vòng lặp in cho **mọi** chỗ khớp marker, kể cả khi sau marker không có ký tự chữ/số nào (`n == 0`). Bản sao cũ của marker còn sót trong các trang đã giải phóng (ảnh của chương trình `secret`/`attack` đã chạy trước) có thể tạo ra dòng trống. Test vẫn qua vì regex chỉ cần **một** dòng khớp, nhưng output chưa sạch.

**Hướng sửa:** bỏ qua chỗ khớp có `n == 0` (`continue`), và/hoặc thoát ngay sau khi in secret đầu tiên. Cuối cùng nên `exit(0)` khi thành công.

---

## F. Yêu cầu sửa code trực tiếp

> Phần này chỉ gợi **hướng làm và các file bị ảnh hưởng**. Khi luyện, hãy tự gõ lại và chạy `./grade-lab-syscall`, đừng chép sẵn.

### F1 🟡 Thêm syscall `getmask()` trả về mask hiện tại.

**Hướng làm:** Làm đủ 5 chỗ như `interpose`: số mới ở `kernel/syscall.h` (24), `entry("getmask")` ở `user/usys.pl`, prototype ở `user/user.h`, khai báo `extern` + mục trong bảng `syscalls[]` ở `kernel/syscall.c`, handler `sys_getmask()` ở `kernel/sysproc.c` trả `myproc()->sandbox.mask`. Nhớ build lại (`make fs.img`) vì `usys.S` được sinh lúc build.

### F2 🟡 In ra số syscall bị từ chối (để debug).

**Hướng làm:** Trong `syscall()`, ở nhánh `rejected(p, num)` thêm một lệnh `printk` in `p->pid`, `p->name` và `num`. Đây là nơi **duy nhất** biết chính xác syscall bị chặn, nhờ thiết kế kiểm tra tập trung. Gỡ dòng debug sau khi dùng.

### F3 🟡 Chỉ lọc `open`, bỏ `exec`.

**Hướng làm:** Sửa **hai chỗ**: điều kiện `num != SYS_open && num != SYS_exec` trong `rejected()` và mặt nạ `(1 << SYS_open) | (1 << SYS_exec)` trong luật chống thoát của `sys_interpose()`. Sửa một chỗ mà quên chỗ kia sẽ làm lệch giữa hai nơi và có thể mở lại đường thoát. Chạy lại `sandbox_mask` (test `exec` sẽ đổi kết quả) và `sandbox_escape`.

### F4 🔴 Cho phép cả thư mục (tiền tố) thay vì tên file đúng.

**Hướng làm:** Trong `rejected()` đổi phép so sánh cuối thành so **tiền tố** (`strncmp` với độ dài `strlen(sb->path)`). Cần chú ý: (1) tiền tố `"/a"` khớp nhầm `"/ab"`, nên phải kiểm tra ký tự ranh giới `'/'` hoặc hết chuỗi; (2) luật chống thoát phải đổi theo: path mới chỉ được là **con** của path hiện tại (hẹp hơn) hoặc `"-"`; (3) `"-"` vẫn so trọn chuỗi.

### F5 🔴 Cho phép **nhiều** path.

**Hướng làm:** Đổi `char path[MAXPATH]` thành mảng path (kèm số lượng) trong `struct sandbox`; `rejected()` duyệt mảng; `kfork` vẫn copy bằng một phép gán nhưng `memset` ở `freeproc` vẫn đủ. Quan trọng: để không mở đường thoát, mỗi lần `interpose` chỉ được **thu hẹp** tập path (giao với tập cũ), không bao giờ thêm path mới.

### F6 🟡 Mở rộng mask lên 64 bit.

**Hướng làm:** Đổi `mask` sang `uint64`; ở `rejected()` dùng `1ULL << num`; ở `sys_interpose` lấy mask bằng **`argaddr`** (vì `argint` chỉ cho 32 bit) và ở `user.h` đổi kiểu tham số. Chú ý `sandbox_most` dùng mask 32-bit nên phải vẫn đúng.

### F7 🔴 "Cho phép process gỡ hạn chế của chính nó" thì được không?

**Trả lời:** Không nên, vì **phá vỡ** mục đích sandbox: process bị chặn chỉ cần gọi hàm gỡ là tự do, `sandbox_escape` sẽ thất bại. Hạn chế chỉ được **chặt thêm** (mask tích luỹ, path chỉ giữ hoặc thành `"-"`). Nếu thật sự cần "tháo" thì phải do một tiến trình **cha có đặc quyền** làm từ bên ngoài, không phải chính process bị nhốt.

### F8 🟡 Sửa `attack.c`: bỏ dòng trống và dừng sau khi in secret.

**Hướng làm:** Trong vòng lặp, sau khi đếm `n`, nếu `n == 0` thì `continue`; sau `printf` thêm `exit(0)` (và để `exit(0)` cuối chương trình nếu không tìm thấy). Chạy `./grade-lab-syscall attack` để chắc chắn vẫn qua.

### F9 🟡 In ra chỉ số trang và offset khi tìm thấy marker.

**Hướng làm:** Trong nhánh tìm thấy marker, in `i / PGSIZE` (trang thứ mấy trong 64) và `i % PGSIZE` (offset trong trang). Vì vùng `sbrk` của `attack` bắt đầu ở ranh giới trang nên `i % PGSIZE` chính là offset marker `32 + ASCII(ký tự đầu)`; secret là offset `+12`. Dùng làm bằng chứng cho report, rồi **xoá dòng debug**.

### F10 🟡 Vá kernel và chứng minh `attack` thất bại.

**Hướng làm:** Trong `uvmalloc()` (`kernel/vm.c`) bỏ `#ifndef LAB_SYSCALL` bao quanh `memset(mem, 0, sz)`. Build lại, chạy `secret abc` rồi `attack`: không còn in gì vì mọi trang cấp ra đã là số 0. Nhớ **hoàn nguyên** để không ảnh hưởng bài nộp.

### F11 🟢 Chặn mọi syscall **trừ** `exit` và `write`. Mask bằng bao nhiêu?

**Trả lời:** Lấy mọi bit 1–30 rồi bỏ bit 2 (`exit`) và bit 16 (`write`): `2147483643 − 65536 = 2147418107` (`0x7FFEFFFB`). Cách nghĩ: toàn bộ `0x7FFFFFFF` trừ `(1<<2) + (1<<16)`.

---

## G. Quy trình, git và report

### G1 🟢 Lệnh `make grade` chấm bao nhiêu điểm, gồm những gì?

**Trả lời:** Thang **60 điểm**: `answers-syscall.txt` (5), 8 test sandbox (`mask`, `fork`, `path`, `most`, `minus`, `block`, `escape`, `forkpath`; 5 điểm mỗi test, tổng 40), `attack` (14), `time` (1). Chạy riêng một test bằng `./grade-lab-syscall <tên>`.

### G2 🟢 Vì sao phải `git commit` trước `make zipball`?

**Trả lời:** `make zipball` đóng gói từ git nên **file chưa được track sẽ không vào `lab.zip`**. Đồng thời rubric chấm lịch sử git: nên có nhiều commit có ý nghĩa thay vì một commit duy nhất.

### G3 🟡 Chính sách dùng AI của lab này là gì?

**Trả lời:** Đề CS333 cho phép dùng AI **nếu khai báo trong report** và nêu rõ dùng vào việc gì. `AGENTS.md` trong repo trỏ tới chính sách MIT 6.1810 vốn khắt khe hơn (không cho AI viết hay debug code lab). Vì hai quy định khác nhau nên cần xác nhận với giảng viên, và dù thế nào cũng phải **khai báo trung thực**.

### G4 🟡 Bạn gặp khó khăn gì và xử lý thế nào? (gợi ý để trả lời)

**Trả lời gợi ý:**

- `sandbox_escape` không có trong đề: phát hiện khi đọc `grade-lab-syscall`, dẫn tới luật "mask chỉ tích luỹ, path chỉ giữ hoặc thành `-`".
- Phân biệt `"-"` với `---`: dùng so khớp trọn chuỗi.
- `sepc` đổi sau mỗi lần build: phải tra lại `kernel.asm` thay vì dùng địa chỉ cũ.
- Attack không đoán được trang/offset: chuyển sang quét marker; phát hiện offset phụ thuộc ký tự đầu của secret.

### G5 🟡 Nếu có thêm thời gian, bạn sẽ cải thiện gì?

**Trả lời gợi ý:** Làm `attack.c` in output sạch (bỏ dòng trống, thoát sau lần khớp đầu, `exit(0)` khi thành công); dọn lỗi chính tả trong comment và khoảng trắng thừa; cân nhắc mask 64-bit; viết thêm test cho trường hợp mask rỗng và `interpose` với path dài hơn `MAXPATH`.

### G6 🟡 Hệ số vấn đáp cá nhân tính thế nào?

**Trả lời:** Điểm cá nhân = điểm nhóm × hệ số: **×1.0** nếu giải thích được mọi phần và làm được thay đổi trực tiếp; ×0.9 hiểu ý chính, thiếu sót nhỏ; ×0.7 chỉ hiểu phần mình làm; ×0.3 không giải thích được bài nộp. Vì vậy mỗi người phải hiểu **toàn bộ** bài, không chỉ phần mình làm.

---

*Ôn tập theo thứ tự A → G: hiểu đường đi của syscall trước, rồi mới tới các quyết định thiết kế của `rejected()` và `sys_interpose()`.*
