# So Sánh: Thao Tác File trong C vs System Calls

## 📚 KHÁI NIỆM

### 1. **C Standard Library Functions** (fopen, fread, fwrite, fclose)

- Là **high-level wrapper** (lớp bọc cao cấp)
- Nằm trong thư viện C chuẩn (`stdio.h`)
- Có **buffering tự động** (đệm dữ liệu)
- **Portable** - chạy được trên nhiều hệ điều hành

### 2. **System Calls** (open, read, write, close)

- Là **low-level** (mức thấp)
- Gọi **trực tiếp** vào Linux kernel
- **Không có buffering** (hoặc tối thiểu)
- Chỉ hoạt động trên hệ thống POSIX (Linux, Unix, macOS)

---

## 🔍 SO SÁNH CHI TIẾT

### Ví dụ: Ghi dữ liệu vào file

#### **Cách 1: Dùng C Standard Library**

```c
#include <stdio.h>

FILE *fp = fopen("data.txt", "w");  // Mở file (có buffering)
if (fp == NULL) {
    perror("Error opening file");
    return 1;
}

fwrite(buffer, sizeof(char), size, fp);  // Ghi (được đệm)
fflush(fp);  // Ép ghi xuống disk
fclose(fp);  // Đóng file
```

**Đặc điểm:**

- ✅ Dễ sử dụng
- ✅ Tự động buffering (nhanh hơn với nhiều lần ghi nhỏ)
- ✅ Portable (chạy trên Windows, Linux, macOS)
- ❌ Ít kiểm soát chi tiết

---

#### **Cách 2: Dùng System Calls**

```c
#include <fcntl.h>
#include <unistd.h>

int fd = open("data.txt", O_CREAT | O_WRONLY, 0644);  // Mở file (không buffering)
if (fd < 0) {
    perror("Error opening file");
    return 1;
}

write(fd, buffer, size);  // Ghi trực tiếp
fsync(fd);  // Ép ghi xuống disk
close(fd);  // Đóng file
```

**Đặc điểm:**

- ✅ Kiểm soát chi tiết (flags, mode, fd)
- ✅ Không có buffering overhead
- ✅ Atomic operations (rename, fsync)
- ❌ Phức tạp hơn
- ❌ Chỉ chạy trên POSIX (Linux, Unix, macOS)

---

## 🎯 TẠI SAO CODE NÀY DÙNG SYSTEM CALLS?

### 1. **Yêu cầu ban đầu:**

> "sử dụng system call trong linux"

### 2. **Atomic File Operations:**

```c
// Tạo file tạm → Ghi dữ liệu → Rename
// Đảm bảo không mất dữ liệu nếu crash
int tmpFd = open("file.tmp", ...);
write(tmpFd, data, size);
fsync(tmpFd);  // Ép ghi xuống disk
close(tmpFd);
rename("file.tmp", "file.txt");  // Atomic operation
```

### 3. **Kiểm soát fsync:**

- `fsync()` đảm bảo dữ liệu được ghi xuống disk ngay lập tức
- Với `fwrite()` + `fflush()` không đảm bảo 100%

### 4. **Performance:**

- Với dữ liệu lớn, system calls có thể nhanh hơn

---

## 📊 BẢNG SO SÁNH

| Tính năng       | C Standard Library         | System Calls           |
| --------------- | -------------------------- | ---------------------- |
| **Buffering**   | ✅ Có (tự động)            | ❌ Không               |
| **Portable**    | ✅ Windows/Linux/macOS     | ❌ Chỉ POSIX           |
| **Dễ sử dụng**  | ✅ Dễ                      | ❌ Phức tạp            |
| **Kiểm soát**   | ❌ Ít                      | ✅ Nhiều               |
| **Performance** | ⚡ Tốt (với nhiều ghi nhỏ) | ⚡⚡ Tốt (với ghi lớn) |
| **Atomic ops**  | ❌ Khó                     | ✅ Dễ (rename)         |

---

## 💡 KẾT LUẬN

**System calls và C functions là KHÁC NHAU:**

1. **C functions** (fopen, fread, fwrite):

   - Là wrapper cao cấp, có buffering
   - Dùng khi: code cần portable, dễ dùng

2. **System calls** (open, read, write):
   - Gọi trực tiếp vào OS kernel
   - Dùng khi: cần kiểm soát chi tiết, atomic operations

**Trong code này dùng system calls vì:**

- ✅ Yêu cầu ban đầu
- ✅ Cần atomic operations
- ✅ Cần fsync để đảm bảo dữ liệu

---

## 🔄 CÓ THỂ CHUYỂN SANG C FUNCTIONS KHÔNG?

**CÓ**, nhưng sẽ mất một số tính năng:

- ❌ Khó làm atomic operations (rename)
- ❌ Không có fsync trực tiếp
- ❌ Ít kiểm soát hơn

**Code hiện tại đã tối ưu cho yêu cầu, nên giữ nguyên system calls.**
