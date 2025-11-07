#ifndef FS_TMPFS_H
#define FS_TMPFS_H

#include "fs_vfs.h"
#include "alloc.h"
#include "string.h"
#include "uart.h"
#include <stddef.h>

#define MAX_FILE_NAME 64
#define MAX_CHILDREN 16
#define DEFAULT_FILE_SIZE 4096 

// Enum to distinguish between file and directory
typedef enum {
    TMPFS_NODE_FILE,
    TMPFS_NODE_DIRECTORY
} tmpfs_node_type_t;

struct tmpfs_node {
    char name[MAX_FILE_NAME];
    tmpfs_node_type_t type;
    struct tmpfs_node* parent; // Pointer to parent directory node

    // For files
    char* data;      // File content
    size_t size;     // Current size of the file content
    size_t capacity; // Allocated buffer capacity for data

    // For directories
    struct vnode* children[MAX_CHILDREN];
    int num_children;
};

// Filesystem operations for tmpfs
extern struct file_operations tmpfs_file_ops;
extern struct vnode_operations tmpfs_vnode_ops;

// Function to set up a tmpfs mount
int tmpfs_setup_mount(struct filesystem* fs, struct mount* mount);
int register_tmpfs(void);
void initramfs_init(struct vnode* rootvnode);

int tmpfs_lookup(struct vnode* dir_node, struct vnode** target, const char* component_name);
int tmpfs_create(struct vnode* dir_node, struct vnode** target, const char* component_name);
int tmpfs_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name);

int tmpfs_open(struct vnode* file_node, struct file** target);
int tmpfs_close(struct file* file);
int tmpfs_write(struct file* file, const void* buf, size_t len);
int tmpfs_read(struct file* file, void* buf, size_t len);
long tmpfs_lseek64(struct file* file, long offset, int whence);

#endif // FS_TMPFS_H