# ❓ Lab 1 – Câu hỏi vấn đáp (kèm đáp án)

> Bộ câu hỏi ôn tập cho buổi **demo / Q&A** với TA. TA thường hỏi mỗi thành viên 2–3 câu về code đã nộp, nên **cả hai người đều phải hiểu toàn bộ code**.
> Ký hiệu: 🟢 Cơ bản · 🟡 Trung bình · 🔴 Nâng cao

---

## 📑 Mục lục

- [A. Xv6 & System call](#a-xv6--system-call)
- [B. sleep](#b-sleep)
- [C. sixfive](#c-sixfive)
- [D. memdump](#d-memdump)
- [E. find](#e-find)
- [F. find -exec (fork / exec / wait)](#f-find--exec-fork--exec--wait)
- [G. Build, Git & nộp bài](#g-build-git--nộp-bài)
- [H. Câu hỏi tình huống / sửa code](#h-câu-hỏi-tình-huống--sửa-code)

---

## A. Xv6 & System call

### Q1. 🟢 Xv6 là gì? Chạy trên nền tảng nào?

Xv6 là hệ điều hành giáo dục kiểu Unix v6 của MIT, viết bằng C cho kiến trúc **RISC-V**. Trong lab, nó chạy trên máy ảo **QEMU**.

---

### Q2. 🟢 System call là gì? Vì sao chương trình user cần nó?

System call là giao diện để chương trình ở **user mode** yêu cầu dịch vụ từ **kernel** (đọc/ghi file, tạo process, ngủ…). Chương trình user bị giới hạn quyền, không được truy cập phần cứng trực tiếp, nên bắt buộc phải nhờ kernel.

---

### Q3. 🟡 Mô tả đường đi của lời gọi `pause(10)` từ user xuống kernel.

1. `user/user.h` khai báo prototype `pause()` để code C gọi được.
2. `user/usys.S` (sinh từ `usys.pl`) chứa stub: `li a7, SYS_pause` rồi `ecall`.
3. `ecall` chuyển CPU sang **kernel mode** và nhảy vào trap handler.
4. `syscall()` đọc số hiệu ở thanh ghi `a7`, tra bảng `syscalls[]` và gọi `sys_pause()` (`kernel/sysproc.c`).
5. `sys_pause` lấy tham số bằng `argint(0, &n)`, ngủ đủ `n` tick, rồi trả kết quả.
6. Giá trị trả về nằm ở `a0` và quay lại chương trình user.

---

### Q4. 🟢 File descriptor là gì? Ba fd mặc định là gì?

Fd là một **số nguyên nhỏ** đại diện cho file/pipe/thiết bị đang mở của process, dùng làm chỉ số vào bảng fd của process.

| fd | Tên |
| :-: | :-- |
| 0 | stdin |
| 1 | stdout |
| 2 | stderr |

---

### Q5. 🟡 Giá trị trả về của `read()` có những trường hợp nào?

- `> 0`: số byte đọc được.
- `0`: gặp **EOF** (hết file).
- `< 0`: lỗi.

Vòng lặp đọc thường là `while ((n = read(fd, buf, size)) > 0)`, và sau vòng lặp kiểm tra `n < 0` để báo lỗi.

---

### Q6. 🟢 Tick là gì? `sleep 10` ngủ bao lâu?

Tick là khoảng thời gian giữa hai lần ngắt của timer – đơn vị thời gian của kernel xv6. `sleep 10` ngủ **10 tick** (khoảng 1 giây trong QEMU, mỗi tick xấp xỉ 0.1 giây), **không phải 10 giây**.

---

### Q7. 🟡 Mỗi process xv6 mở được tối đa bao nhiêu file? Điều này ảnh hưởng gì tới `find`?

`NOFILE = 16` (trong `kernel/param.h`). `find` đệ quy mà mỗi tầng giữ một fd của thư mục đang duyệt, nên nếu **không `close`** hoặc cây quá sâu, fd sẽ cạn và `open` thất bại.

---

### Q8. 🟢 Vì sao chương trình xv6 thường kết thúc bằng `exit()`? `return` từ `main` có được không?

Process kết thúc bằng system call `exit(status)`. Trong bản xv6 này, `user/ulib.c` có hàm `start()` gọi `exit(main(argc, argv))`, nên `return` từ `main` vẫn hợp lệ (nó tự gọi `exit`). Tuy nhiên theo quy ước của xv6 và đề lab, ta gọi `exit(0)` / `exit(1)` tường minh để mã trạng thái rõ ràng.

---

### Q9. 🟢 Làm sao thoát QEMU? `Ctrl-p` làm gì?

- Thoát: nhấn `Ctrl-a` rồi `x`.
- `Ctrl-p`: kernel in thông tin các process đang có (xv6 không có lệnh `ps`).

---

## B. sleep

### Q10. 🟢 Chương trình `sleep` của bạn hoạt động thế nào?

1. Kiểm tra `argc == 2`, sai thì in usage ra stderr và `exit(1)`.
2. `atoi(argv[1])` đổi chuỗi thành số tick.
3. Gọi `pause(ticks)`; nếu `ticks < 0` hoặc `pause` trả `< 0` thì báo lỗi, `exit(1)`.
4. `exit(0)`.

---

### Q11. 🟢 Vì sao phải kiểm tra `argc`?

Nếu không có đối số, `argv[1]` là `NULL`; `atoi(NULL)` sẽ truy cập con trỏ rỗng và làm chương trình crash. Kiểm tra `argc` cho phép in thông báo thân thiện và thoát đúng mã lỗi.

---

### Q12. 🟡 `argv[0]` là gì? Đối số thật bắt đầu từ đâu?

`argv[0]` là **tên chương trình** (ví dụ `"sleep"`), đối số thật bắt đầu từ `argv[1]`. `argc` tính cả `argv[0]`, nên `sleep 10` có `argc = 2`.

---

### Q13. 🟡 `pause()` trong kernel kết thúc sớm trong trường hợp nào?

Khi process bị `kill` trong lúc đang ngủ: vòng lặp `while (ticks - ticks0 < n)` kiểm tra `killed(myproc())`, nếu đúng thì nhả lock và trả `-1`. Vì vậy user code nên kiểm tra giá trị trả về của `pause`.

---

### Q14. 🟡 Nếu truyền `sleep abc` thì chuyện gì xảy ra?

`atoi("abc")` trả `0` (không có chữ số nào), nên `pause(0)` trả về ngay và chương trình thoát bình thường mà không báo lỗi. Để chặt chẽ hơn có thể tự kiểm tra từng ký tự là chữ số.

---

## C. sixfive

### Q15. 🟢 `sixfive` làm gì? Số được định nghĩa thế nào?

In ra mọi số là **bội của 5 hoặc 6**, theo thứ tự xuất hiện. "Số" là chuỗi chữ số liên tiếp, các token được ngăn cách bởi ký tự trong `" -\r\t\n./,"`.

---

### Q16. 🟡 Giải thích ba biến `has_token`, `valid`, `value`.

| Biến | Vai trò |
| :-- | :-- |
| `has_token` | Có đang ở trong một token hay không (tránh in token rỗng khi gặp nhiều dấu phân cách liền nhau) |
| `valid` | Token chỉ gồm chữ số hay không (nếu có ký tự khác như chữ cái → bỏ token) |
| `value` | Giá trị số tích lũy: `value = value * 10 + digit` |

---

### Q17. 🟡 Vì sao `xv6` không được coi là số `6`?

Vì `x` và `v` không phải ký tự phân cách, chúng thuộc cùng token với chữ số `6`. Gặp ký tự không phải chữ số → `valid = 0`, nên khi hết token sẽ **không in** dù `6` là bội của 6.

---

### Q18. 🔴 Vì sao phải xử lý lại sau khi hết vòng lặp `read`?

Nếu file **không kết thúc bằng ký tự phân cách** (ví dụ `...18` rồi EOF) thì token cuối chưa được chốt trong vòng lặp. Đoạn xử lý sau vòng lặp (`if (has_token && valid && ...)`) đảm bảo số cuối cùng vẫn được in.

---

### Q19. 🟡 Chương trình xử lý nhiều file và stdin ra sao?

- `argc < 2` → gọi `sixfive(0)` đọc từ **stdin** (fd 0).
- Ngược lại, lặp qua từng `argv[i]`: `open` → `sixfive(fd)` → `close(fd)`.
- Nếu `open` lỗi: in `sixfive: cannot open xxx` ra stderr, ghi nhận `failed = 1`, **tiếp tục** file tiếp theo; cuối cùng `exit(failed)`.

---

### Q20. 🔴 Đoạn `if (value > (2147483647 - digit) / 10)` để làm gì?

Chặn **tràn số nguyên** `int`. Trước khi thực hiện `value * 10 + digit`, kiểm tra kết quả có vượt `INT_MAX = 2147483647` không; nếu vượt thì đánh dấu token `valid = 0` thay vì để `value` bị tràn thành số âm/sai.

---

### Q21. 🟢 Số `0` có được in không?

Có. `0 % 5 == 0` nên `0` thỏa điều kiện bội của 5 (và của 6).

---

### Q22. 🟢 Vì sao phải `close(fd)` sau khi xử lý mỗi file?

Để giải phóng fd. Mỗi process chỉ có 16 fd; nếu không đóng, xử lý nhiều file sẽ cạn fd và rò rỉ tài nguyên.

---

## D. memdump

### Q23. 🟢 `memdump` làm gì?

Diễn giải `len` byte tại `data` theo chuỗi định dạng `fmt`, mỗi ký tự của `fmt` tiêu thụ một số byte và in một giá trị trên một dòng.

| fmt | Byte | In |
| :-: | :-: | :-- |
| `i` | 4 | int, thập phân |
| `h` | 2 | short, thập phân |
| `p` | 8 | 64-bit, hex |
| `c` | 1 | ký tự |
| `s` | 8 | con trỏ → in chuỗi C mà nó trỏ tới |
| `S` | còn lại | chuỗi C nằm ngay trong dữ liệu |

---

### Q24. 🟡 Vì sao `echo deadc0de | memdump hhcccc` in ra `25956` cho giá trị đầu?

RISC-V là **little-endian**: byte thấp nằm ở địa chỉ thấp. Hai byte đầu là `'d'` = `0x64` và `'e'` = `0x65`. Ghép lại theo little-endian: byte đầu là phần thấp → `0x6564 = 25956`. Giá trị `h` thứ hai là `'a'`,`'d'` = `0x61`,`0x64` → `0x6461 = 25697`. Bốn `c` còn lại in `c`, `0`, `d`, `e`.

---

### Q25. 🟡 Vì sao `echo a | memdump i` báo `not enough data for 'i'`?

`echo a` xuất ra 2 byte (`'a'` và `'\n'`), nhưng `i` cần **4 byte**. Kiểm tra `*data + 4 > end` đúng nên in `memdump: not enough data for 'i'` rồi dừng.

---

### Q26. 🔴 Giải thích kết quả Example 4 (`pihcS`) và Example 5 (`sccccc`) dựa trên layout bộ nhớ.

Layout `struct sss` (24 byte sau padding):

| Offset | Trường | Giá trị |
| :-: | :-- | :-- |
| 0–7 | `char *ptr` | địa chỉ của `"hello"` |
| 8–11 | `int num1` | `1819438967` = `0x6C6C6F77` → bytes `w o r l` |
| 12–13 | `short num2` | `100` (`'d'`) |
| 14 | `char byte` | `'z'` |
| 15–22 | `char bytes[8]` | `"xyzzy"` |

- **Example 4 `pihcS`:** `p` in địa chỉ (hex), `i` in `1819438967`, `h` in `100`, `c` in `z`, `S` in `xyzzy`.
- **Example 5 `sccccc`:** `s` đọc con trỏ và in `hello`; năm `c` kế tiếp đọc các byte offset 8–12: `w`, `o`, `r`, `l`, `d` (4 byte của `num1` cộng byte thấp của `num2`).

---

### Q27. 🔴 Tại sao dùng `memmove` thay vì ép kiểu `*(int*)data`?

Ép kiểu con trỏ buộc dữ liệu phải **căn lề (alignment)** theo kiểu đó; `data` có thể nằm ở địa chỉ lẻ sau khi cộng offset (ví dụ sau `c` hoặc `h`), gây truy cập sai lề (lỗi/hiệu năng trên RISC-V). `memmove` copy từng byte vào biến đúng kiểu nên an toàn với mọi địa chỉ.

---

### Q28. 🟡 Vì sao phải kiểm tra giới hạn `len`? Cách bạn làm?

Để **không đọc quá vùng dữ liệu hợp lệ** (lỗi tràn bộ đệm/đọc rác). Trong `getBytes`, so sánh `*data + n > end` với `end = data + len`; nếu thiếu thì in `memdump: not enough data for 'x'` ra stderr và dừng.

---

### Q29. 🟡 Cấu trúc `struct spec` và `union value` mang lại lợi ích gì?

- `union value` cho phép **một biến** chứa kết quả của mọi kiểu (int/short/uint64/char/char*), nên chỉ cần một hàm `getBytes` chung.
- `struct spec` gắn mỗi ký tự định dạng với **kích thước** và **hàm in** (con trỏ hàm). Thêm định dạng mới chỉ cần thêm một dòng vào bảng `specs[]` thay vì mở rộng `switch`.

---

### Q30. 🟡 Định dạng `S` khác `s` thế nào?

- `s`: lấy **8 byte con trỏ** từ dữ liệu, rồi in chuỗi ở **địa chỉ đó**.
- `S`: chuỗi nằm **trực tiếp trong dữ liệu**, in từ vị trí hiện tại đến byte `'\0'` (hoặc hết dữ liệu) và tiêu thụ phần còn lại.

---

### Q31. 🟢 Vì sao `p` in bằng `%lx`?

`p` là số 64-bit (`uint64`). `%lx` in số `long` dạng hex, đủ 64 bit; `%x` chỉ 32 bit sẽ cắt mất phần cao.

---

## E. find

### Q32. 🟢 `find` hoạt động thế nào?

Duyệt đệ quy cây thư mục từ đường dẫn gốc; với mỗi mục (file **hoặc** thư mục) có tên (basename) trùng `target` thì in đường dẫn đầy đủ.

---

### Q33. 🟡 Vì sao `find` phải bỏ qua `.` và `..`?

Mỗi thư mục chứa hai mục đặc biệt: `.` (chính nó) và `..` (thư mục cha). Nếu đệ quy vào chúng sẽ **lặp vô hạn** (`.` → `./.` → `./././...`, hoặc đi ngược lên cha rồi xuống lại).

---

### Q34. 🟡 Thư mục được biểu diễn thế nào và đọc ra sao?

Thư mục là một **file đặc biệt chứa mảng `struct dirent`** `{ ushort inum; char name[DIRSIZ]; }`. Đọc thư mục bằng `open` + `read` từng `sizeof(struct dirent)` byte. `inum == 0` là ô trống → bỏ qua. Dùng `fstat` để biết mục đó là `T_DIR` hay `T_FILE`.

---

### Q35. 🔴 Vì sao phải copy `de.name` sang buffer rồi thêm `'\0'`?

`DIRSIZ = 14` và tên đúng 14 ký tự thì `de.name` **không có `'\0'` kết thúc**. Dùng thẳng với `strcmp`/`strcpy` sẽ đọc lan sang byte sau. Nên copy `DIRSIZ` byte vào mảng `char name[DIRSIZ + 1]` và gán `name[DIRSIZ] = '\0'`.

---

### Q36. 🟡 Vì sao không dùng `==` để so sánh tên?

`==` trên `char *` so sánh **địa chỉ con trỏ**, không so sánh nội dung chuỗi. Phải dùng `strcmp(a, b) == 0`.

---

### Q37. 🟡 Hàm `basename` làm gì? Tại sao cần?

Lấy thành phần cuối của đường dẫn (`./a/aa/b` → `b`) bằng cách duyệt ngược từ cuối tới ký tự `/` gần nhất. Cần vì ta so khớp **tên** với `target`, không phải cả đường dẫn.

---

### Q38. 🔴 `find` của bạn tìm thấy cả thư mục, không chỉ file – ở đâu trong code?

Điều kiện `st.type == T_FILE || st.type == T_DIR` trước khi `strcmp(basename(path), target)`. Mục tiêu bài là "all the files **and directories**". Với thư mục khớp tên, vừa in nó, vừa tiếp tục duyệt vào bên trong.

---

### Q39. 🔴 Điều gì xảy ra nếu đường dẫn quá dài?

Buffer `child` có kích thước cố định. Trước khi nối, code kiểm tra `length + 1 + DIRSIZ + 1 > sizeof(child)`; nếu vượt thì in `find: path too long`, đóng fd rồi return, tránh **tràn bộ đệm**.

---

### Q40. 🔴 Hạn chế của cách cài đặt đệ quy này là gì?

- Mỗi tầng đệ quy giữ **1 fd** (tối đa 16) và **một buffer đường dẫn trên stack** cùng các biến cục bộ; stack user của xv6 nhỏ (~1 trang 4 KB). Cây thư mục rất sâu có thể cạn fd hoặc tràn stack.
- Có thể cải thiện bằng cách đóng fd thư mục sớm sau khi đọc xong các entry, hoặc giảm kích thước buffer.

---

### Q41. 🟢 Làm sao có file system sạch giữa các lần chạy QEMU?

File system lưu trong `fs.img` nên **tồn tại giữa các lần chạy**. Chạy `make clean` rồi `make qemu` để tạo lại từ đầu.

---

## F. find -exec (fork / exec / wait)

### Q42. 🟢 `find . wc -exec echo hi` in ra gì? Vì sao?

In `hi ./wc`. `find` tìm thấy `./wc`, rồi thay vì in tên, tạo `argv = ["echo", "hi", "./wc", 0]` và chạy `exec("echo", argv)`.

---

### Q43. 🟡 Bạn phân tách đối số `-exec` như thế nào?

`argv[1]` = thư mục, `argv[2]` = tên cần tìm, `argv[3]` phải là `"-exec"`. Phần còn lại `argv + 4` là lệnh và đối số: `exec_argv = argv + 4`, `exec_argc = argc - 4`. Khi chạy, copy `exec_argv` vào `argv` mới, thêm đường dẫn tìm được, cuối cùng là `0`.

---

### Q44. 🟡 `fork()` trả về gì ở cha và con?

- Cha nhận **pid của con** (> 0).
- Con nhận **0**.
- Lỗi → **-1** (không tạo được process).

---

### Q45. 🔴 **(Câu TA hay hỏi)** Chuyện gì xảy ra nếu `exec` thất bại trong tiến trình con?

`exec` chỉ return khi **thất bại** (khi thành công, chương trình mới thay thế hoàn toàn process nên không bao giờ quay lại). Vì vậy dòng ngay sau `exec` chính là đường xử lý lỗi: in thông báo và `exit(1)`. Nếu quên `exit`, tiến trình con **tiếp tục chạy code của `find`** (tiếp tục duyệt cây, nhân đôi kết quả, nhân bản process).

---

### Q46. 🟡 Vì sao cha phải gọi `wait()` sau `fork()`?

- Để lệnh con **chạy xong rồi mới** tìm tiếp, output không bị xen kẽ.
- Để **thu dọn** process con (tránh **zombie** – con đã kết thúc nhưng chưa được cha "wait").

---

### Q47. 🟡 Vì sao mảng `argv` truyền cho `exec` phải kết thúc bằng `0`?

`exec` không biết có bao nhiêu đối số, nó duyệt `argv` đến khi gặp con trỏ `NULL`. Thiếu `0` thì `exec` đọc ra ngoài mảng, kết quả không xác định.

---

### Q48. 🟡 `MAXARG` là gì? Bạn xử lý ra sao?

`MAXARG = 32` (trong `kernel/param.h`) là số đối số tối đa của `exec`. Code kiểm tra `exec_argc + 1 >= MAXARG` (còn chỗ cho đường dẫn và `0`) và báo `too many arguments to exec` rồi thoát, tránh tràn mảng `argv[MAXARG]`.

---

### Q49. 🔴 Vì sao tách `found()` ra riêng?

`found(path)` quyết định làm gì khi khớp: nếu có `-exec` thì `run_exec(path)`, ngược lại `printf`. Nhờ vậy hàm duyệt `find()` không cần biết chế độ nào; `find` thường **không bị ảnh hưởng** khi thêm `-exec`.

---

## G. Build, Git & nộp bài

### Q50. 🟢 Làm sao để chương trình mới chạy được trong xv6?

Thêm `$U/_tên\` vào biến `UPROGS` trong `Makefile`, rồi `make qemu`. Make sẽ biên dịch và đưa chương trình vào file system của xv6.

---

### Q51. 🟢 Làm sao chạy test cho một bài riêng?

```bash
./grade-lab-util sleep
# hoặc
make GRADEFLAGS=sleep grade
```

---

### Q52. 🟢 `time.txt` chứa gì? Có bắt buộc không?

Một số nguyên duy nhất = số giờ nhóm đã làm lab. Autograder kiểm tra file này; thiếu thì mất 0.25đ mục Git & time.txt.

---

### Q53. 🟡 `make zipball` tạo ra gì? Vì sao phải commit trước?

Tạo `lab.zip` bằng `git archive ... HEAD` → chỉ chứa **file đã commit**. Thay đổi chưa commit sẽ **không** có trong zip.

---

### Q54. 🟢 Cấu trúc file nộp lên Moodle?

Thư mục `<MSSV1>_<MSSV2>/` chứa `lab.zip` và `Report.pdf`, nén thành `<MSSV1>_<MSSV2>.zip` (cá nhân: `<MSSV>.zip`). Không nộp `*.o`, `kernel/kernel`, `fs.img`.

---

### Q55. 🟢 Sửa script chấm (`grade-lab-util`, `gradelib.py`) có được không?

**Không.** Đó là gian lận → 0 điểm cả lab.

---

### Q56. 🟢 Báo cáo dùng AI thì cần làm gì?

Được dùng AI để học và debug nhưng phải **khai báo rõ trong report**, và phải giải thích được từng dòng code nộp.

---

## H. Câu hỏi tình huống / sửa code

### Q57. 🔴 Nếu yêu cầu `sixfive` thêm dấu `_` làm ký tự phân cách, bạn sửa ở đâu?

Thêm `_` vào chuỗi phân cách trong `strchr(" -\r\t\n./,", c)` thành `" -\r\t\n./,_"`. Phần còn lại không đổi.

---

### Q58. 🔴 Muốn `memdump` hỗ trợ định dạng `l` (số 64-bit thập phân) thì làm gì?

Thêm `long l;` (hoặc dùng `uint64`) vào `union value`, viết `print_l` dùng `printf("%ld\n", ...)`, rồi thêm một dòng `{'l', sizeof(long), print_l}` vào bảng `specs[]`. Không cần sửa `memdump()` hay `getBytes()`.

---

### Q59. 🔴 Muốn `find` hỗ trợ khớp tên không phân biệt hoa thường?

Thay `strcmp(basename(path), target)` bằng một hàm so sánh tự viết chuyển từng ký tự về chữ thường (`c | 0x20` cho chữ cái) trước khi so sánh. Chỉ cần sửa tại điểm so khớp.

---

### Q60. 🔴 Nếu `find` chạy `-exec` mà không `wait`, quan sát được gì?

Output của các lệnh con bị **xen kẽ/lộn xộn** và có thể in sau khi `find` đã xong (prompt `$` hiện giữa chừng). Các con kết thúc mà không ai wait sẽ thành **zombie** đến khi `init` thu dọn.

---

### Q61. 🔴 Có thể dùng `argv` mảng `argv[MAXARG]` cục bộ trong `run_exec` mà không nguy hiểm không?

Được, vì `exec` **sao chép** các đối số vào không gian địa chỉ mới của chương trình, và `run_exec` đã kiểm tra `exec_argc + 1 < MAXARG` ở `main` trước đó để chắc chắn mảng đủ chỗ cho cmd + args + path + `0`.

---

### Q62. 🟡 Mô tả toàn bộ chuỗi lệnh khi gõ `find . b` trong shell xv6.

1. Shell `sh` `fork` một tiến trình con.
2. Con `exec("find", ["find", ".", "b", 0])`.
3. `main` của `find` kiểm tra `argc`, gọi `find(".", "b")`.
4. `find` mở `.`, `fstat` thấy là thư mục, duyệt các `dirent`, bỏ `.`/`..`, nối đường dẫn và đệ quy.
5. Mục nào có basename `b` thì `printf` đường dẫn (`./b`, `./a/b`, `./a/aa/b`).
6. `exit(0)`; shell `wait` thấy con xong và in lại prompt `$`.

---

## ✅ Mẹo trả lời vấn đáp

- Nói theo thứ tự: **mục đích → ý tưởng/thuật toán → syscall dùng → trường hợp biên**.
- Sẵn sàng giải thích: *bỏ `.` `..`*, *exec thất bại*, *little-endian trong `memdump`*, *token cuối của `sixfive`*.
- Khi được yêu cầu sửa code: chỉ rõ **chính xác dòng/hàm** cần thay đổi và tại sao các phần khác không ảnh hưởng.
- Nếu chưa chắc: nêu điều đã biết chắc rồi suy luận từ đó, đừng đoán bừa.
