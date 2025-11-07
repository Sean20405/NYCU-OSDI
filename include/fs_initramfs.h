#ifndef FS_INITRAMFS_H
#define FS_INITRAMFS_H

#include "fs_vfs.h"
#include "cpio.h"

#define MAX_FILE_NAME 64
#define MAX_CHILDREN 16
#define DEFAULT_FILE_SIZE 4096 

typedef enum {
    INITRAMFS_NODE_FILE,
    INITRAMFS_NODE_DIRECTORY
} initramfs_node_type_t;

struct initramfs_node {
    char name[MAX_FILE_NAME];
    initramfs_node_type_t type;
    struct initramfs_node* parent; // Pointer to parent directory node

    // For files
    char* data;      // File content
    size_t size;     // Current size of the file content
    size_t capacity; // Allocated buffer capacity for data

    // For directories
    struct vnode* children[MAX_CHILDREN];
    int num_children;
};

// Filesystem operations for initramfs
extern struct file_operations initramfs_file_ops;
extern struct vnode_operations initramfs_vnode_ops;

// Function to set up a initramfs mount
int register_initramfs(void);
int initramfs_setup_mount(struct filesystem* fs, struct mount* mount);
void initramfs_init(struct vnode* rootvnode);

int initramfs_lookup(struct vnode* dir_node, struct vnode** target, const char* component_name);
int initramfs_create(struct vnode* dir_node, struct vnode** target, const char* component_name);
int initramfs_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name);

int initramfs_open(struct vnode* file_node, struct file** target);
int initramfs_close(struct file* file);
int initramfs_write(struct file* file, const void* buf, size_t len);
int initramfs_read(struct file* file, void* buf, size_t len);
long initramfs_lseek64(struct file* file, long offset, int whence);



#endif // FS_INITRAMFS_H