#include "dev_framebuffer.h"

unsigned int width, height, pitch, isrgb; /* dimensions and channel order */
unsigned char *lfb;                       /* raw frame buffer address */

struct file_operations framebuffer_f_ops = {
    .open = dev_framebuffer_open,
    .close = dev_framebuffer_close,
    .write = dev_framebuffer_write,
    .read = dev_framebuffer_read,
    .lseek64 = dev_framebuffer_lseek64,
};

int dev_framebuffer_open(struct vnode* file_node, struct file** target) {
    if (file_node == NULL || target == NULL) {
        return EINVAL_VFS;
    }
    
    *target = (struct file*)alloc(sizeof(struct file));
    if (*target == NULL) {
        return ENOMEM_VFS;
    }
    
    (*target)->vnode = file_node;
    (*target)->f_pos = 0;  // Initial position
    (*target)->f_ops = &framebuffer_f_ops;

    return 0;  // Success
}

int dev_framebuffer_close(struct file* file) {
    if (file == NULL) {
        return EINVAL_VFS;
    }

    free(file);
    return 0;  // Success
}

int dev_framebuffer_write(struct file* file, const void* buf, size_t len) {
    if (file == NULL || buf == NULL || len == 0) {
        return EINVAL_VFS;
    }
    if (len == 0) return 0;

    memcpy(lfb + file->f_pos, (void *)buf, len);
    file->f_pos += len;
    return len;
}

int dev_framebuffer_read(struct file* file, void* buf, size_t len) {
    return -1;
}

long dev_framebuffer_lseek64(struct file* file, long offset, int whence) {
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
            // For framebuffer, we can consider the end as the current position
            new_pos = file->f_pos;  // No real end for framebuffer
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

int dev_framebuffer_ioctl(struct framebuffer_info* info) {
    if (info == NULL) {
        return EINVAL_VFS;
    }

    uart_puts("Initializing framebuffer...\n");
    unsigned int __attribute__((aligned(16))) mbox[36];

    mbox[0] = 35 * 4;
    mbox[1] = REQUEST_CODE;

    mbox[2] = 0x48003; // set phy wh
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = 1024; // FrameBufferInfo.width
    mbox[6] = 768;  // FrameBufferInfo.height

    mbox[7] = 0x48004; // set virt wh
    mbox[8] = 8;
    mbox[9] = 8;
    mbox[10] = 1024; // FrameBufferInfo.virtual_width
    mbox[11] = 768;  // FrameBufferInfo.virtual_height

    mbox[12] = 0x48009; // set virt offset
    mbox[13] = 8;
    mbox[14] = 8;
    mbox[15] = 0; // FrameBufferInfo.x_offset
    mbox[16] = 0; // FrameBufferInfo.y.offset

    mbox[17] = 0x48005; // set depth
    mbox[18] = 4;
    mbox[19] = 4;
    mbox[20] = 32; // FrameBufferInfo.depth

    mbox[21] = 0x48006; // set pixel order
    mbox[22] = 4;
    mbox[23] = 4;
    mbox[24] = 1; // RGB, not BGR preferably

    mbox[25] = 0x40001; // get framebuffer, gets alignment on request
    mbox[26] = 8;
    mbox[27] = 8;
    mbox[28] = 4096; // FrameBufferInfo.pointer
    mbox[29] = 0;    // FrameBufferInfo.size

    mbox[30] = 0x40008; // get pitch
    mbox[31] = 4;
    mbox[32] = 4;
    mbox[33] = 0; // FrameBufferInfo.pitch

    mbox[34] = END_TAG;

    // this might not return exactly what we asked for, could be
    // the closest supported resolution instead
    if (mailbox_call(mbox, MBOX_CH_PROP) && mbox[20] == 32 && mbox[28] != 0) {
        mbox[28] &= 0x3FFFFFFF; // convert GPU address to ARM address
        width = mbox[5];        // get actual physical width
        height = mbox[6];       // get actual physical height
        pitch = mbox[33];       // get number of bytes per line
        isrgb = mbox[24];       // get the actual channel order
        lfb = (void *)((unsigned long)mbox[28]);

        uart_puts("Framebuffer initialized successfully:\n");
        uart_puts("  Width: ");
        uart_puts(itoa(width));
        uart_puts("\n  Height: ");
        uart_puts(itoa(height));
        uart_puts("\n  Pitch: ");
        uart_puts(itoa(pitch));
        uart_puts("\n  Is RGB: ");
        uart_puts(isrgb ? "Yes\n" : "No\n");
        uart_puts("  Framebuffer Address: ");
        uart_hex((unsigned long)lfb);
        uart_puts("\n");
    }
    else {
        uart_puts("Unable to set screen resolution to 1024x768x32\n");
    }

    info->width = width;
    info->height = height;
    info->pitch = pitch;
    info->isrgb = isrgb;

    return 0;  // Success
}
