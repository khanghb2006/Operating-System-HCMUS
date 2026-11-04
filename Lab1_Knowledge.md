# 📘 Lab 1 – Xv6 & Unix Utilities: Tóm tắt kiến thức

> **Môn:** Operating Systems (CS333) · **Nguồn gốc:** MIT 6.1810 – *Xv6 and Unix utilities*
> **Nhiệm vụ:** viết 4 chương trình user-level chạy trên xv6 (RISC-V + QEMU): `sleep`, `sixfive`, `memdump`, `find` (+ `-exec`).

---

## 📑 Mục lục

1. [Tổng quan lab](#1-tổng-quan-lab)
2. [Kiến thức nền về xv6](#2-kiến-thức-nền-về-xv6)
3. [System call dùng trong lab](#3-system-call-dùng-trong-lab)
4. [Bài 1 – sleep](#4-bài-1--sleep-10đ)
5. [Bài 2 – sixfive](#5-bài-2--sixfive-15đ)
6. [Bài 3 – memdump](#6-bài-3--memdump-25đ)
7. [Bài 4 – find](#7-bài-4--find-20đ)
8. [Bài 5 – find -exec](#8-bài-5--find--exec-15đ)
9. [Kiến thức C cần nhớ](#9-kiến-thức-c-cần-nhớ)
10. [Quy trình build, test, nộp bài](#10-quy-trình-build-test-nộp-bài)
11. [Chấm điểm & lỗi hay mất điểm](#11-chấm-điểm--lỗi-hay-mất-điểm)
12. [Cheat sheet](#12-cheat-sheet)

---

## 1. Tổng quan lab

| Mục | Nội dung |
| :-- | :-- |
| Mục tiêu | Làm quen xv6 và các system call qua việc viết tiện ích kiểu Unix |
| Nhóm | Tối đa 2 sinh viên (≥ 3 người: không chấm) |
| Tổng điểm | **10đ** (8.5đ chức năng + 1.5đ report/code/process) |
| Công cụ | `git`, `make`, QEMU, toolchain RISC-V (`riscv64-unknown-elf-gcc`) |
| Tài liệu nên đọc | Chương 1 sách xv6 *"Operating system interfaces"*, K&R (C) |

### Bảng điểm

| # | Bài | File | Điểm | Test |
| :-: | :-- | :-- | :-: | :-- |
| 1 | sleep | `user/sleep.c` | 1.0 | `sleep` |
| 2 | sixfive | `user/sixfive.c` | 1.5 | `sixfive` |
| 3 | memdump | `user/memdump.c` | 2.5 | `memdump` |
| 4 | find | `user/find.c` | 2.0 | `find` |
| 5 | find -exec | `user/find.c` | 1.5 | `exec` |
| 6 | Code quality, Git, `time.txt` | – | 0.5 | `time` |
| 7 | Report | `Report.pdf` | 1.0 | – |

---

## 2. Kiến thức nền về xv6

### 2.1 xv6 là gì?

- Hệ điều hành **giáo dục** kiểu Unix (v6), viết lại bằng C cho kiến trúc **RISC-V**.
- Chạy trên **QEMU** (máy ảo). Boot xong sẽ chạy `init` → mở shell `sh` với prompt `$`.
- Cấu trúc thư mục chính:

| Thư mục | Vai trò |
| :-- | :-- |
| `kernel/` | Mã nguồn kernel (process, memory, file system, syscall…) |
| `user/` | Chương trình user (`ls`, `cat`, `echo`, `sh`… và code của bạn) |
| `Makefile` | Build + danh sách `UPROGS` (chương trình đưa vào file system) |
| `grade-lab-util` | Script chấm điểm tự động |

### 2.2 User mode vs Kernel mode

```text
 ┌───────────────────────┐
 │  Chương trình user    │   chạy ở user mode (quyền hạn chế)
 │  (sleep, find, ...)   │
 └──────────┬────────────┘
            │  system call (ecall)
 ┌──────────▼────────────┐
 │        Kernel         │   chạy ở kernel mode (toàn quyền phần cứng)
 └───────────────────────┘
```

Chương trình user **không** truy cập trực tiếp phần cứng; muốn đọc file, tạo process, ngủ… phải xin kernel qua **system call**.

### 2.3 Đường đi của một system call (ví dụ `pause`)

```text
pause(n)               user/user.h      khai báo prototype C
   │
   ▼
usys.S (sinh từ usys.pl)                li a7, SYS_pause ; ecall
   │                                    → nạp số hiệu syscall vào a7, "gõ cửa" kernel
   ▼
kernel/trap.c  → syscall()              đọc a7, tra bảng syscalls[]
   │
   ▼
kernel/sysproc.c: sys_pause()           lấy tham số bằng argint(0,&n), thực thi
   │
   ▼
trả giá trị về a0 → user                0 = thành công, -1 = lỗi
```

### 2.4 Process & File descriptor

- Mỗi process có bảng **file descriptor (fd)**, tối đa `NOFILE = 16` fd.
- 3 fd mặc định:

| fd | Tên | Ý nghĩa |
| :-: | :-- | :-- |
| 0 | stdin | đầu vào chuẩn |
| 1 | stdout | đầu ra chuẩn |
| 2 | stderr | lỗi (`fprintf(2, ...)`) |

- `open()` trả về **fd nhỏ nhất đang trống**.
- `read()` trả về số byte đọc được; **0 = hết file (EOF)**; **< 0 = lỗi**.
- Luôn `close(fd)` sau khi dùng để không rò rỉ fd.

### 2.5 Tick

- **Tick** = khoảng thời gian giữa 2 lần ngắt (interrupt) của timer. Là đơn vị thời gian của kernel xv6 (trong QEMU xấp xỉ 0.1 giây/tick).
- `sleep 10` nghĩa là ngủ **10 tick**, không phải 10 giây.

### 2.6 Phím tắt hữu ích trong QEMU

| Phím | Tác dụng |
| :-- | :-- |
| `Ctrl-a` rồi `x` | Thoát QEMU |
| `Ctrl-p` | Kernel in thông tin các process (xv6 không có `ps`) |

---

## 3. System call dùng trong lab

| Syscall | Prototype | Dùng ở bài | Ghi chú |
| :-- | :-- | :-: | :-- |
| `pause` | `int pause(int ticks)` | sleep | Ngủ `ticks` tick; `-1` nếu process bị kill |
| `open` | `int open(char*, int flags)` | sixfive, find | Cờ `O_RDONLY`…, trả fd hoặc `-1` |
| `read` | `int read(int fd, void *buf, int n)` | sixfive, memdump, find | `0` = EOF, `-1` = lỗi |
| `close` | `int close(int fd)` | tất cả | Giải phóng fd |
| `fstat` | `int fstat(int fd, struct stat*)` | find | Lấy `type`, `ino`, `size`… |
| `fork` | `int fork(void)` | find -exec | Con trả `0`, cha trả `pid` của con, `-1` nếu lỗi |
| `exec` | `int exec(char*, char**)` | find -exec | Thay "ảnh" process; **chỉ return khi thất bại** |
| `wait` | `int wait(int *status)` | find -exec | Cha chờ con kết thúc |
| `exit` | `void exit(int status)` | tất cả | Kết thúc process, `0` = OK |

### Hàm thư viện user (`user/ulib.c`, `user/printf.c`)

| Hàm | Công dụng |
| :-- | :-- |
| `atoi(s)` | Chuỗi → số nguyên |
| `strchr(s, c)` | Tìm ký tự `c` trong `s`, trả `NULL` (0) nếu không có |
| `strcmp(a, b)` | So sánh chuỗi (`0` nếu bằng nhau) |
| `strlen`, `strcpy`, `memmove`, `memset` | Thao tác chuỗi / bộ nhớ |
| `printf(fmt, ...)` | In ra stdout: hỗ trợ `%d %x %lx %p %s %c` |
| `fprintf(fd, fmt, ...)` | In ra fd chỉ định (dùng `2` cho lỗi) |

---

## 4. Bài 1 – `sleep` (1.0đ)

**Yêu cầu:** `sleep N` → tạm dừng N tick. Thiếu đối số → in lỗi.

### Ý tưởng

1. Kiểm tra `argc == 2`, sai → in usage (stderr) và `exit(1)`.
2. `atoi(argv[1])` đổi chuỗi sang số.
3. Gọi `pause(ticks)`.
4. `exit(0)`.

### Code chính

```c
if (argc != 2) {
    fprintf(2, "Usage: sleep <ticks>\n");
    exit(1);
}
int ticks = atoi(argv[1]);
if (ticks < 0 || pause(ticks) < 0) {
    fprintf(2, "sleep: pause failed\n");
    exit(1);
}
exit(0);
```

### Điểm cần nhớ

- Theo đề, `main` nên kết thúc bằng **`exit(0)`** (hoặc `exit(1)` khi lỗi). `user/ulib.c` có `start()` gọi `exit(main(...))` nên `return` cũng chạy được, nhưng gọi `exit` tường minh là quy ước của xv6.
- `argv[0]` là tên chương trình, đối số thật bắt đầu từ `argv[1]`.
- Kernel: `sys_pause` lặp `while (ticks - ticks0 < n)` rồi `sleep()`; nếu `killed(myproc())` thì return `-1`.
- Phải thêm `$U/_sleep\` vào `UPROGS` trong `Makefile`.

---

## 5. Bài 2 – `sixfive` (1.5đ)

**Yêu cầu:** in mọi **số** trong file là bội của **5 hoặc 6**. Số là chuỗi chữ số ngăn cách bởi các ký tự trong `" -\r\t\n./,"`.

### Ví dụ

```text
$ sixfive sixfive.txt
5
100
18
6
```

### Ý tưởng – máy trạng thái đọc từng ký tự

| Biến | Ý nghĩa |
| :-- | :-- |
| `has_token` | Đang trong 1 token (chưa gặp ký tự phân cách) |
| `valid` | Token chỉ gồm chữ số (không dính chữ cái như `xv6`) |
| `value` | Giá trị số đang tích lũy |

```text
Đọc 1 ký tự c
├─ c là ký tự phân cách → KẾT THÚC token:
│     nếu has_token && valid && (value%5==0 || value%6==0) → in value
│     reset has_token=0, valid=1, value=0
└─ ngược lại → has_token=1
      ├─ c là chữ số → value = value*10 + digit  (có chặn tràn int)
      └─ c khác      → valid = 0   (vd "xv6" bị loại)
Hết file (EOF) → xử lý nốt token cuối (file không có dấu phân cách ở cuối)
```

### Điểm cần nhớ

- Không có đối số → đọc **stdin** (`fd = 0`); có đối số → mở từng file.
- `open` lỗi → `sixfive: cannot open xxx` (stderr), **tiếp tục** file tiếp theo, cuối cùng `exit(1)`.
- Token cuối cùng **không có dấu phân cách** phải được xử lý sau vòng lặp.
- Chặn tràn: `if (value > (INT_MAX - digit) / 10) valid = 0;`
- `0 % 5 == 0` nên số `0` cũng được in.
- Luôn `close(fd)` mỗi file đã mở.

---

## 6. Bài 3 – `memdump` (2.5đ)

**Yêu cầu:** `memdump(char *fmt, char *data, int len)` diễn giải `len` byte của `data` theo chuỗi định dạng `fmt`, mỗi giá trị một dòng.

### Bảng định dạng

| fmt | Số byte | In ra |
| :-: | :-: | :-- |
| `i` | 4 | số nguyên 32-bit, thập phân |
| `h` | 2 | số nguyên 16-bit, thập phân |
| `p` | 8 | số 64-bit, **hex** |
| `c` | 1 | 1 ký tự ASCII |
| `s` | 8 | con trỏ 64-bit tới chuỗi C → in chuỗi đó |
| `S` | phần còn lại | chuỗi C kết thúc bằng `\0` nằm ngay trong dữ liệu |

### Hai chế độ của `main` (đã cho sẵn)

- **Không đối số:** chạy 5 ví dụ có sẵn.
- **1 đối số (format):** đọc stdin đến EOF (tối đa 512 byte) rồi gọi `memdump(format, data, n)`.

### Kiến thức nền then chốt

**① Endianness – RISC-V là little-endian** (byte thấp nằm ở địa chỉ thấp):

```text
echo deadc0de | memdump hhcccc

bytes:  'd' 'e' | 'a' 'd' | 'c' | '0' | 'd' | 'e'
hex:    64  65  | 61  64  | 63  | 30  | 64  | 65
h #1 = 0x6564 = 25956     (byte sau là phần cao!)
h #2 = 0x6461 = 25697
c c c c → c 0 d e
```

**② Cấu trúc `struct sss` và padding (Example 4, 5):**

```text
offset  0 ─ 7   : char *ptr   (8 byte)   → "hello"
offset  8 ─ 11  : int  num1   (4 byte)   = 1819438967 = 0x6C6C6F77 → bytes 'w','o','r','l'
offset 12 ─ 13  : short num2  (2 byte)   = 100 = 'd'
offset 14       : char byte   (1 byte)   = 'z'
offset 15 ─ 22  : char bytes[8]          = "xyzzy"
(dữ liệu thật dùng 23 byte → sizeof = 24 do compiler padding để căn lề 8)
```

- Example 4 `pihcS` → `p` (địa chỉ), `i` = 1819438967, `h` = 100, `c` = `z`, `S` = `xyzzy`.
- Example 5 `sccccc` → `s` in `hello`, rồi 5 `c` đọc 5 byte kế tiếp: `w o r l d` (4 byte của `num1` + byte thấp của `num2`).

### Thiết kế đã dùng (bảng tra + con trỏ hàm)

```c
union value { int i; short h; uint64 p; char c; char *s; };

struct spec {
    char fmt;                      // ký tự định dạng
    int  size;                     // số byte tiêu thụ
    void (*print)(union value v);  // hàm in tương ứng
};

static struct spec specs[] = {
    {'i', sizeof(int),    print_i},
    {'h', sizeof(short),  print_h},
    {'p', sizeof(uint64), print_p},
    {'c', sizeof(char),   print_c},
    {'s', sizeof(char *), print_s},
};
```

**Luồng chạy của `memdump`:**

```text
end = data + len
for mỗi ký tự trong fmt:
    'S'  → in từ data đến byte '\0' hoặc end; kết thúc
    else → tra bảng specs
           ├─ không có   → báo "unknown format", return
           ├─ data + size > end → "memdump: not enough data for 'x'", return
           └─ memmove(&v, data, size); data += size; gọi print(v)
```

### Điểm cần nhớ

- **Không bao giờ đọc quá `len` byte** → kiểm tra `*data + n > end` trước khi đọc.
- Dùng `memmove` vào biến đúng kiểu để tránh vấn đề **căn lề (alignment)** khi ép kiểu con trỏ.
- `p` in bằng `%lx` (64-bit hex).
- Lệnh `echo a | memdump i` → dữ liệu chỉ 2 byte (`a` + `\n`) < 4 → `memdump: not enough data for 'i'`.

---

## 7. Bài 4 – `find` (2.0đ)

**Yêu cầu:** `find <thư mục> <tên>` → in đường dẫn của **mọi file và thư mục** tên `<tên>` trong cây thư mục.

```text
$ find . b
./b
./a/b
./a/aa/b
```

### Cấu trúc file system liên quan

```c
// kernel/fs.h
#define DIRSIZ 14
struct dirent {
    ushort inum;        // 0 = ô trống (bỏ qua)
    char name[DIRSIZ];  // KHÔNG đảm bảo kết thúc bằng '\0'
};

// kernel/stat.h
#define T_DIR    1   // thư mục
#define T_FILE   2   // file thường
#define T_DEVICE 3   // thiết bị (console)
```

> Thư mục trong xv6 thực chất là **một file chứa mảng các `struct dirent`**. Đọc thư mục = `read()` từng `sizeof(struct dirent)` byte.

### Thuật toán (đệ quy)

```text
find(path, target):
    fd = open(path); fstat(fd, &st)
    nếu basename(path) == target        → found(path)   // in hoặc exec
    nếu st.type == T_DIR:
        child = path + "/"
        lặp read(fd, &de, sizeof de):
            bỏ qua de.inum == 0
            name = de.name + '\0'
            bỏ qua "." và ".."
            child = path + "/" + name
            find(child, target)
    close(fd)
```

### Điểm cần nhớ

- **Bỏ qua `.` và `..`** → nếu không sẽ lặp vô hạn (`.` là chính nó, `..` đi ngược lên).
- So sánh chuỗi bằng `strcmp()`, **không dùng `==`** (so sánh con trỏ).
- `de.name` có thể đủ 14 ký tự và không có `\0` → phải copy ra buffer `DIRSIZ + 1` và tự thêm `\0`.
- Kiểm tra độ dài đường dẫn trước khi nối (`length + 1 + DIRSIZ + 1 > sizeof(child)`).
- So khớp trên **basename** (thành phần cuối của đường dẫn): `./a/aa/b` → `b`.
- `close(fd)` ở **mọi** đường thoát (kể cả khi lỗi) – mỗi tầng đệ quy giữ 1 fd, mà mỗi process chỉ có 16 fd.
- File system **lưu lại giữa các lần chạy QEMU** → muốn sạch: `make clean` rồi `make qemu`.

---

## 8. Bài 5 – `find -exec` (1.5đ)

**Yêu cầu:** `find <dir> <tên> -exec cmd [args...]` → với mỗi file khớp, chạy `cmd [args...] <đường-dẫn>` thay vì in tên.

```text
$ find . wc -exec echo hi
hi ./wc
```

### Phân tích đối số

```text
find   .    wc   -exec  echo  hi
[0]   [1]  [2]    [3]   [4]  [5]
                        └─ exec_argv = argv + 4 ; exec_argc = argc - 4
```

### Xây `argv` mới & chạy

```text
argv_mới = [ "echo", "hi", "./wc", 0 ]
              └─ cmd + args ─┘ └ path  └ kết thúc bằng NULL
```

```c
pid = fork();
if (pid < 0)  { /* lỗi fork */ return; }
if (pid == 0) {                     // tiến trình con
    exec(argv[0], argv);
    // tới được dòng này nghĩa là exec THẤT BẠI
    fprintf(2, "find : exec %s failed\n", argv[0]);
    exit(1);
}
wait(0);                            // cha chờ con xong rồi mới tìm tiếp
```

### Bộ ba `fork` / `exec` / `wait`

| Syscall | Vai trò | Lưu ý |
| :-- | :-- | :-- |
| `fork` | Nhân bản process hiện tại | Con nhận `0`, cha nhận `pid` con |
| `exec` | Thay chương trình của process bằng chương trình khác | Thành công thì **không return**; `argv` phải kết thúc bằng `0` |
| `wait` | Cha đợi con kết thúc | Thiếu `wait` → output xen kẽ + process zombie |

### Điểm cần nhớ

- Con **phải `exit(1)` khi `exec` fail**, nếu không con sẽ tiếp tục chạy code của `find` (nhân đôi tiến trình!).
- Giới hạn: `MAXARG = 32` (trong `kernel/param.h`) → kiểm tra `exec_argc + 1 < MAXARG`.
- `find` thường (không `-exec`) vẫn phải chạy đúng → dùng `found()` rẽ nhánh in / exec.

---

## 9. Kiến thức C cần nhớ

| Chủ đề | Nội dung |
| :-- | :-- |
| `argc` / `argv` | `argc` = số đối số (tính cả tên lệnh); `argv[argc]` luôn là `NULL` |
| Chuỗi C | Mảng `char` kết thúc bằng `'\0'`; `==` chỉ so sánh con trỏ → dùng `strcmp` |
| Ép kiểu con trỏ | `*(int*)data` đọc 4 byte tại `data`; dễ lỗi alignment → ưu tiên `memmove` |
| `union` | Các thành viên dùng chung vùng nhớ → 1 biến chứa được nhiều kiểu |
| Con trỏ hàm | `void (*print)(union value)` cho phép tra bảng thay vì `switch` dài |
| `struct` padding | Compiler chèn byte đệm để căn lề → `sizeof` ≥ tổng kích thước các trường |
| `static` | Hàm/biến `static` chỉ thấy trong file hiện tại |
| Endianness | RISC-V little-endian: byte thấp ở địa chỉ thấp |
| Stack | Mảng cục bộ lớn + đệ quy sâu → dễ tốn stack (stack user xv6 chỉ ~1 trang 4 KB) |

---

## 10. Quy trình build, test, nộp bài

### Build & chạy

```bash
make qemu                    # biên dịch và boot xv6
make clean                   # xóa build + file system cũ
```

### Test

```bash
make grade                          # chạy toàn bộ test
./grade-lab-util sleep              # chỉ test 1 bài
make GRADEFLAGS=find grade          # cách khác để test 1 bài
```

Tên test: `sleep`, `sixfive`, `memdump`, `find`, `exec`, `time`.

### Thêm chương trình mới

Thêm vào `UPROGS` trong `Makefile`:

```make
UPROGS=\
	$U/_sleep\
	$U/_find\
	$U/_sixfive\
	$U/_memdump
```

### Git

```bash
git add user/sleep.c Makefile
git commit -am 'my solution for util lab exercise 1'
git diff                      # thay đổi từ lần commit gần nhất
git diff origin/util          # thay đổi so với code gốc của lab
```

### Nộp bài

1. `make grade` đạt hết.
2. Có `time.txt` (một số nguyên = số giờ làm) và **commit hết** (`git status` sạch).
3. `make zipball` → sinh `lab.zip` (dùng `git archive` nên chỉ gồm file đã commit).
4. Tạo thư mục `<MSSV1>_<MSSV2>/` chứa `lab.zip` + `Report.pdf`, nén thành `<MSSV1>_<MSSV2>.zip`.
5. Một thành viên upload lên Moodle.

> ⚠️ Không nộp file build (`*.o`, `kernel/kernel`, `fs.img`). Code không biên dịch được → tối đa 30% điểm mỗi bài.

### Nội dung Report.pdf (4–10 trang)

- Trang bìa (môn, lab, nhóm, MSSV, họ tên)
- Bảng đóng góp (% mỗi thành viên, tổng = 100%)
- Mỗi bài: thuật toán, syscall chính, đoạn code quan trọng, screenshot chạy trên xv6
- Screenshot `make grade` đầy đủ
- Thảo luận khó khăn / lỗi / tự đánh giá
- Tài liệu tham khảo (khai báo rõ nếu dùng AI)

---

## 11. Chấm điểm & lỗi hay mất điểm

### Vấn đáp cá nhân (nhóm 2 người)

| Mức | Mô tả | Hệ số |
| :-- | :-- | :-: |
| Hiểu đầy đủ | Trả lời đúng hết, sửa được code khi được yêu cầu | × 1.0 |
| Tốt | Hiểu phần lớn, còn sót nhỏ | × 0.9 |
| Yếu | Chỉ giải thích được phần của mình | × 0.7 |
| Không hiểu | Không giải thích được code | × 0.3 |

### Khấu trừ

| Vấn đề | Hậu quả |
| :-- | :-- |
| Sai tên file / cấu trúc nộp | −0.5 |
| Thiếu `time.txt` | Mất 0.25đ mục Git & time |
| `make qemu` không biên dịch | Tối đa 30% mỗi bài |
| Đạo văn / sửa script chấm | **0 điểm cả lab** |

### Lỗi thường gặp

| Bài | Lỗi |
| :-- | :-- |
| sleep | Quên check `argc`; không `exit` đúng mã (0/1); nhầm tick với giây |
| sixfive | Quên token cuối khi hết file; nhận `xv6` thành số `6`; không reset trạng thái |
| memdump | Đọc quá `len`; quên `\0` khi in chuỗi; nhầm endianness |
| find | Không bỏ qua `.`/`..`; dùng `==` so chuỗi; không `close(fd)`; quên thêm `\0` cho `de.name` |
| find -exec | Quên `wait`; không `exit` khi `exec` fail; `argv` không kết thúc bằng 0 |

---

## 12. Cheat sheet

```text
SLEEP    argc check → atoi → pause(ticks) → exit(0)
SIXFIVE  đọc 1 ký tự: phân cách ⇒ chốt token ; chữ số ⇒ cộng dồn ; khác ⇒ invalid ; EOF ⇒ chốt nốt
MEMDUMP  end = data+len ; mỗi fmt: tra bảng → check bound → memmove → data += size → print
FIND     open+fstat → khớp basename? → là DIR? → read dirent → bỏ . .. → nối path → đệ quy → close
EXEC     argv = cmd args path 0 → fork → con: exec / exit(1) → cha: wait
```

| Nhớ | Giá trị |
| :-- | :-- |
| `NOFILE` | 16 fd / process |
| `MAXARG` | 32 đối số cho `exec` |
| `DIRSIZ` | 14 ký tự tên file |
| `T_DIR / T_FILE / T_DEVICE` | 1 / 2 / 3 |
| fd chuẩn | 0 stdin · 1 stdout · 2 stderr |
| Thoát QEMU | `Ctrl-a` `x` |
