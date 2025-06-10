#include "dev_uart.h"

struct file_operations uart_f_ops = {
    .open = dev_uart_open,
    .close = dev_uart_close,
    .write = dev_uart_write,
    .read = dev_uart_read,
    .lseek64 = dev_uart_lseek64,
};

int dev_uart_open(struct vnode* file_node, struct file** target) {
    if (file_node == NULL || target == NULL) {
        return EINVAL_VFS;
    }
    
    *target = (struct file*)alloc(sizeof(struct file));
    if (*target == NULL) {
        return ENOMEM_VFS;
    }
    
    (*target)->vnode = file_node;
    (*target)->f_pos = 0;  // Initial position
    (*target)->f_ops = &uart_f_ops;

    return 0;  // Success
}

int dev_uart_close(struct file* file) {
    if (file == NULL) {
        return EINVAL_VFS;
    }

    free(file);
    return 0;  // Success
}

int dev_uart_write(struct file* file, const void* buf, size_t len) {
    if (file == NULL || buf == NULL || len == 0) {
        return EINVAL_VFS;
    }

    // Simulate UART write operation
    for (size_t i = 0; i < len; i++) {
        uart_putc(((const char*)buf)[i]);
    }
    
    file->f_pos += len;  // Update file position
    return len;  // Return number of bytes written
}

int dev_uart_read(struct file* file, void* buf, size_t len) {
    if (file == NULL || buf == NULL || len == 0) {
        return EINVAL_VFS;
    }

    // Simulate UART read operation
    size_t bytes_read = 0;
    for (size_t i = 0; i < len; i++) {
        char ch = uart_getc();  // Read a character from UART
        if (ch == '\0') break;  // Stop reading on null character
        ((char*)buf)[i] = ch;
        bytes_read++;
    }

    file->f_pos += bytes_read;  // Update file position
    return bytes_read;  // Return number of bytes read
}

long dev_uart_lseek64(struct file* file, long offset, int whence) {
    if (file == NULL) {
        return EINVAL_VFS;
    }

    long new_pos = file->f_pos;

    switch (whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos += offset;
            break;
        case SEEK_END:
            // For UART, we can consider the end as the current position
            new_pos = file->f_pos;  // No real end for UART
            break;
        default:
            return EINVAL_VFS;  // Invalid whence
    }

    if (new_pos < 0) {
        return EINVAL_VFS;  // Negative position not allowed
    }

    file->f_pos = new_pos;  // Update file position
    return new_pos;  // Return new position
}