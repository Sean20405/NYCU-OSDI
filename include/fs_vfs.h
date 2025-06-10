#ifndef FS_VFS_H
#define FS_VFS_H

#include <stddef.h>
#include "string.h"
#include "fs_tmpfs.h"
#include "fs_initramfs.h"
#include "dev_uart.h"
#include "dev_framebuffer.h"

// Placeholder for O_CREAT flag, typically from <fcntl.h>
#define O_CREAT 00000100  // Example value, ensure it matches your system\'s O_CREAT

// VFS Error Codes
// These should ideally map to standard errno values or be consistently used.
#define ENOENT_VFS -2 // No such file or directory
#define ENOMEM_VFS -3 // Not enough memory
#define EINVAL_VFS -4 // Invalid argument
#define ENOSYS_VFS -5 // Function not implemented or vnode doesn't support operation
#define EACCES_VFS -6 // Permission denied
#define EEXIST_VFS -7 // File exists
#define EINTERR_VFS -8 // Internal error in file system
#define ENODEV_VFS -9  // No such device (used for filesystem type not found)
#define ENOTDIR_VFS -10 // Not a directory
#define EBUSY_VFS -11   // Device or resource busy (used for mount point busy)

#define SEEK_SET 0 // Seek from the beginning of the file
#define SEEK_CUR 1 // Seek from the current position
#define SEEK_END 2 // Seek from the end of the file

#define MAX_FILE_NAME 64
#define MAX_CHILDREN 16
#define DEFAULT_FILE_SIZE 4096 

// Specific return code from vfs_lookup to indicate path not found,
// used by vfs_open to trigger create logic.
#define VFS_LOOKUP_NOT_FOUND -2 // Note: Same as ENOENT_VFS. This implies vfs_lookup returns ENOENT_VFS for "not found".

struct vnode {
    struct mount* mount;  // Indicate whether this vnode is a mount point. If yes, it will point to the mount structure.
    struct vnode_operations* v_ops;
    struct file_operations* f_ops;
    void* internal;
    struct vnode* parent;
    int parent_is_mount; // Indicates if the parent vnode is a mount point
};

// file handle
struct file {
    struct vnode* vnode;
    size_t f_pos;  // RW position of this file handle
    struct file_operations* f_ops;
    int flags;
};

struct mount {
    struct vnode* root;
    struct filesystem* fs;
};

struct filesystem {
    const char* name;
    int (*setup_mount)(struct filesystem* fs, struct mount* mount);
};

struct file_operations {
    int (*write)(struct file* file, const void* buf, size_t len);
    int (*read)(struct file* file, void* buf, size_t len);
    int (*open)(struct vnode* file_node, struct file** target);
    int (*close)(struct file* file);
    long (*lseek64)(struct file* file, long offset, int whence); // Corrected syntax
};

struct vnode_operations {
    int (*lookup)(struct vnode* dir_node, struct vnode** target,
                  const char* component_name);
    int (*create)(struct vnode* dir_node, struct vnode** target,
                  const char* component_name);
    int (*mkdir)(struct vnode* dir_node, struct vnode** target,
                 const char* component_name);
};

extern struct mount* rootfs;

int register_filesystem(struct filesystem* fs);

int vfs_open(const char* pathname, int flags, struct file** target);
int vfs_close(struct file* file);
int vfs_write(struct file* file, const void* buf, size_t len);
int vfs_read(struct file* file, void* buf, size_t len);

int vfs_mkdir(const char* pathname);
int vfs_mknod(const char* pathname, struct file_operations* f_ops);
int vfs_mount(const char* target, const char* filesystem);
int vfs_lookup(const char* pathname, struct vnode** target);

void vfs_init();

#endif // FS_VFS_H