#include "fs_initramfs.h"

extern uint32_t cpio_addr;

struct file_operations initramfs_f_ops = {
    .open = initramfs_open,
    .close = initramfs_close,
    .write = initramfs_write,
    .read = initramfs_read,
    .lseek64 = initramfs_lseek64,
};

struct vnode_operations initramfs_v_ops = {
    .lookup = initramfs_lookup,
    .create = initramfs_create,
    .mkdir = initramfs_mkdir,
};

int register_initramfs() {
    struct filesystem* initramfs_fs = (struct filesystem*)alloc(sizeof(struct filesystem));
    initramfs_fs->name = "initramfs";
    initramfs_fs->setup_mount = initramfs_setup_mount;
    return register_filesystem(initramfs_fs);
}

int initramfs_setup_mount(struct filesystem* fs, struct mount* mount) {
    if (fs == NULL || mount == NULL) {
        return EINVAL_VFS;
    }

    // TODO
    struct initramfs_node* initramfs_root = (struct initramfs_node*)alloc(sizeof(struct initramfs_node));
    if (!initramfs_root) {
        uart_puts("initramfs_setup_mount: Failed to create root internal node\r\n");
        return ENOMEM_VFS;
    }

    mount->fs = fs;
    mount->root = (struct vnode*)alloc(sizeof(struct vnode));
    if (mount->root == NULL) {
        uart_puts("initramfs_setup_mount: Failed to allocate memory for root vnode\r\n");
        if (initramfs_root->data) free(initramfs_root->data); // Clean up allocated internal node data
        free(initramfs_root); // Clean up allocated internal node
        return ENOMEM_VFS;
    }

    mount->root->mount = 0;
    mount->root->v_ops = &initramfs_v_ops;
    mount->root->f_ops = &initramfs_f_ops;
    mount->root->internal = initramfs_root;
    mount->root->parent_is_mount = 1;

    return 0; // Success
}

void initramfs_init(struct vnode* rootvnode) {
    uart_puts("[initramfs_init] rootvnode: ");
    uart_hex((unsigned long)rootvnode);
    uart_puts("\r\n");

    ((struct initramfs_node*)(rootvnode->internal))->type = INITRAMFS_NODE_DIRECTORY;

    const unsigned long HEADER_SIZE = sizeof(struct cpio_newc_header);
    struct cpio_newc_header *header = (struct cpio_newc_header *)cpio_addr;
    char magic[6] = "070701";
    char header_magic[6];
    
    while (1) {
        memcpy(header_magic, header->c_magic, 6);
        if (strcmp(magic, header_magic) == 0) {
            char *filename_addr = (char *)header + HEADER_SIZE;
            char filename[256];
            unsigned int filenamesize = hex_to_uint(header->c_namesize, 8);
            unsigned int filesize = hex_to_uint(header->c_filesize, 8);
            memcpy(filename, filename_addr, filenamesize);

            if (strcmp(filename, "TRAILER!!!") == 0) {  // End of the archive
                break;
            }

            uart_puts("[initramfs_init] Found file: ");
            uart_puts(filename);
            uart_puts(" (size: ");
            uart_hex(filesize);
            uart_puts(")\r\n");

            struct initramfs_node* new_node = (struct initramfs_node*)alloc(sizeof(struct initramfs_node));
            memcpy(new_node->name, filename, filenamesize);
            new_node->name[filenamesize] = '\0';
            new_node->type = INITRAMFS_NODE_FILE;
            new_node->parent = NULL; // Will be set later
            new_node->size = filesize;
            new_node->data = alloc(filesize);
            memcpy(new_node->data, (char *)header + align(HEADER_SIZE + filenamesize, 4), filesize);
            new_node->capacity = filesize;
            new_node->num_children = 0;
            for (int i = 0; i < MAX_CHILDREN; ++i) {
                new_node->children[i] = NULL;
            }

            struct vnode* new_vnode = (struct vnode*)alloc(sizeof(struct vnode));
            new_vnode->mount = 0;
            new_vnode->v_ops = &initramfs_v_ops;
            new_vnode->f_ops = &initramfs_f_ops;
            new_vnode->internal = new_node;
            new_vnode->parent = rootvnode;
            new_vnode->parent_is_mount = 0;

            ((struct initramfs_node*)rootvnode->internal)->children[((struct initramfs_node*)rootvnode->internal)->num_children++] = new_vnode;

            // Jump to the next header
            filenamesize = align(HEADER_SIZE + filenamesize, 4) - HEADER_SIZE;
            filesize = align(filesize, 4);
            header = (struct cpio_newc_header *)((char *)header + HEADER_SIZE + filenamesize + filesize);
        }
        else {
            uart_puts("Invalid cpio header\r\n");
            break;
        }
    }
}

int initramfs_lookup(struct vnode* dir_node, struct vnode** target, const char* component_name) {
    uart_puts("[initramfs_lookup] Looking up component: ");
    uart_puts(component_name);
    uart_puts(", dir_node: ");
    uart_hex((unsigned long)dir_node);
    uart_puts("\r\n");
    
    if (!dir_node || !dir_node->internal || !target || !component_name) {
        return EINVAL_VFS;
    }

    *target = NULL;
    struct initramfs_node* parent_internal = (struct initramfs_node*)dir_node->internal;

    if (parent_internal->type != INITRAMFS_NODE_DIRECTORY) {
        uart_puts("[initramfs_lookup] Error: dir_node is not a directory\r\n");
        return EACCES_VFS; // Cannot lookup in a non-directory
    }

    for (int i = 0; i < parent_internal->num_children; ++i) {
        char *child_name = ((struct initramfs_node*)(parent_internal->children[i]->internal))->name;
        uart_puts("[initramfs_lookup] Checking child: ");
        uart_puts(child_name);
        uart_puts(" (");
        uart_hex((unsigned long)parent_internal->children[i]);
        uart_puts(")\r\n");
        if (strcmp(child_name, component_name) == 0) {
            *target = parent_internal->children[i];
            return 0;
        }
    }
    return ENOENT_VFS;
}

int initramfs_create(struct vnode* dir_node, struct vnode** target, const char* component_name) {
    return EACCES_VFS;  // read-only filesystem
}

int initramfs_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name) {
    return EACCES_VFS;  // read-only filesystem
}

int initramfs_open(struct vnode* file_node, struct file** target) {
    if (!file_node || !target || !(*target)) { // Check if *target itself is NULL (should be pre-allocated by vfs_open)
        return EINVAL_VFS;
    }
    
    (*target)->f_pos = 0;
    (*target)->vnode = file_node;
    (*target)->f_ops = file_node->f_ops;

    return 0; 
}

int initramfs_close(struct file* file) {
    if (!file) {
        return EINVAL_VFS;
    }
    file->vnode = NULL;
    file->f_pos = 0;
    file->f_ops = NULL;
    file->flags = 0;
    return 0;
}

int initramfs_write(struct file* file, const void* buf, size_t len) {
    return EACCES_VFS;  // read-only filesystem
}

int initramfs_read(struct file* file, void* buf, size_t len) {
    if (!file || !file->vnode || !file->vnode->internal || !buf) {
        return EINVAL_VFS;
    }
    if (len == 0) return 0;

    struct initramfs_node* internal_node = (struct initramfs_node*)file->vnode->internal;
    if (internal_node->type != INITRAMFS_NODE_FILE) {
        return EACCES_VFS; // Cannot read a directory this way
    }

    if (file->f_pos >= internal_node->size) {
        return 0; // EOF
    }

    size_t readable_len = len;
    if (file->f_pos + len > internal_node->size) {
        readable_len = internal_node->size - file->f_pos;
    }

    memcpy(buf, internal_node->data + file->f_pos, readable_len);
    file->f_pos += readable_len;
    return readable_len;
}

long initramfs_lseek64(struct file* file, long offset, int whence) {
    if (!file || !file->vnode || !file->vnode->internal) {
        return EINVAL_VFS;
    }
    struct initramfs_node* internal_node = (struct initramfs_node*)file->vnode->internal;
    if (internal_node->type != INITRAMFS_NODE_FILE) {
        return EINVAL_VFS; // lseek on non-file
    }

    long new_pos;
    switch (whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = file->f_pos + offset;
            break;
        case SEEK_END:
            new_pos = internal_node->size + offset;
            break;
        default:
            return EINVAL_VFS;
    }

    if (new_pos < 0) {
        return EINVAL_VFS;
    }

    file->f_pos = new_pos;
    return new_pos;
}