#ifndef DEV_FRAMEBUFFER_H
#define DEV_FRAMEBUFFER_H

#include "fs_vfs.h"
#include "mailbox.h"
#include "uart.h"
#include <stddef.h>

struct framebuffer_info {
    unsigned int width;
    unsigned int height;
    unsigned int pitch;
    unsigned int isrgb;
};

extern struct file_operations framebuffer_f_ops;

int dev_framebuffer_open(struct vnode* file_node, struct file** target);
int dev_framebuffer_close(struct file* file);
int dev_framebuffer_write(struct file* file, const void* buf, size_t len);
int dev_framebuffer_read(struct file* file, void* buf, size_t len);
long dev_framebuffer_lseek64(struct file* file, long offset, int whence);
int dev_framebuffer_ioctl(struct framebuffer_info* info);

#endif // DEV_FRAMEBUFFER_H