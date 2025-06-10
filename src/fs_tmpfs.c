# include "fs_tmpfs.h"

struct vnode_operations tmpfs_v_ops = {
    .lookup = tmpfs_lookup,
    .create = tmpfs_create,
    .mkdir = tmpfs_mkdir,
};

struct file_operations tmpfs_f_ops = {
    .open = tmpfs_open,
    .close = tmpfs_close,
    .write = tmpfs_write,
    .read = tmpfs_read,
    .lseek64 = tmpfs_lseek64,
};

struct tmpfs_node* tmpfs_create_internal_node(const char* name, tmpfs_node_type_t type, struct tmpfs_node* parent) {
    struct tmpfs_node* new_node = (struct tmpfs_node*)alloc(sizeof(struct tmpfs_node));
    if (!new_node) {
        uart_puts("tmpfs_create_internal_node: Failed to allocate memory for node\\\r\n");
        return NULL;
    }

    int name_len = strlen(name);
    memcpy(new_node->name, name, name_len);
    new_node->name[name_len] = '\0';
    new_node->type = type;
    new_node->parent = parent;
    new_node->data = NULL;
    new_node->size = 0;
    new_node->capacity = 0;
    new_node->num_children = 0;
    for (int i = 0; i < MAX_CHILDREN; ++i) {
        new_node->children[i] = NULL;
    }

    if (type == TMPFS_NODE_FILE) {
        new_node->data = (char*)alloc(DEFAULT_FILE_SIZE);
        if (!new_node->data) {
            uart_puts("tmpfs_create_internal_node: Failed to allocate memory for file data\r\n");
            free(new_node);
            return NULL;
        }
        new_node->capacity = DEFAULT_FILE_SIZE;
    }
    return new_node;
}

// Add tmpfs to filesystem list
int register_tmpfs() {
    struct filesystem* tmpfs_fs = (struct filesystem*)alloc(sizeof(struct filesystem));
    tmpfs_fs->name = "tmpfs";
    tmpfs_fs->setup_mount = tmpfs_setup_mount;
    return register_filesystem(tmpfs_fs);
}

// Setup mount for tmpfs, handle the operation after entering the mount point
int tmpfs_setup_mount(struct filesystem* fs, struct mount* mount) {
    if (fs == NULL || mount == NULL) {
        return EINVAL_VFS;
    }

    struct tmpfs_node* tmpfs_root = tmpfs_create_internal_node("/", TMPFS_NODE_DIRECTORY, NULL);
    if (!tmpfs_root) {
        uart_puts("tmpfs_setup_mount: Failed to create root internal node\r\n");
        return ENOMEM_VFS;
    }

    mount->fs = fs;
    mount->root = (struct vnode*)alloc(sizeof(struct vnode));
    if (mount->root == NULL) {
        uart_puts("tmpfs_setup_mount: Failed to allocate memory for root vnode\r\n");
        if (tmpfs_root->data) free(tmpfs_root->data); // Clean up allocated internal node data
        free(tmpfs_root); // Clean up allocated internal node
        return ENOMEM_VFS;
    }

    mount->root->mount = 0;
    mount->root->v_ops = &tmpfs_v_ops;
    mount->root->f_ops = &tmpfs_f_ops;
    mount->root->internal = tmpfs_root;
    mount->root->parent_is_mount = 1;
    
    return 0; // Success
}

int tmpfs_lookup(struct vnode* dir_node, struct vnode** target, const char* component_name) {
    if (!dir_node || !dir_node->internal || !target || !component_name) {
        return EINVAL_VFS;
    }

    *target = NULL;
    struct tmpfs_node* parent_internal = (struct tmpfs_node*)dir_node->internal;

    if (parent_internal->type != TMPFS_NODE_DIRECTORY) {
        return EACCES_VFS; // Cannot lookup in a non-directory
    }

    for (int i = 0; i < parent_internal->num_children; ++i) {
        char *child_name = ((struct tmpfs_node*)(parent_internal->children[i]->internal))->name;
        uart_puts("[tmpfs_lookup] Checking child: ");
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

// Common function for create and mkdir
int tmpfs_create_or_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name, tmpfs_node_type_t type) {
    if (!dir_node || !dir_node->internal || !target || !component_name || strlen(component_name) == 0) {
        return EINVAL_VFS;
    }
    *target = NULL;
    struct tmpfs_node* parent_internal = (struct tmpfs_node*)dir_node->internal;

    if (parent_internal->type != TMPFS_NODE_DIRECTORY) {
        return EACCES_VFS; // Cannot create in a non-directory
    }

    if (strlen(component_name) >= MAX_FILE_NAME) {
        return EINVAL_VFS; // Name too long
    }

    // Check if name already exists
    for (int i = 0; i < parent_internal->num_children; ++i) {
        char *child_name = ((struct tmpfs_node*)(parent_internal->children[i]->internal))->name;
        if (strcmp(child_name, component_name) == 0) {
            return EEXIST_VFS;
        }
    }

    if (parent_internal->num_children >= MAX_CHILDREN) {
        return ENOMEM_VFS; // Directory full
    }

    struct tmpfs_node* new_internal = tmpfs_create_internal_node(component_name, type, parent_internal);
    if (!new_internal) {
        return ENOMEM_VFS;
    }

    struct vnode* new_vnode = (struct vnode*)alloc(sizeof(struct vnode));
    if (!new_vnode) {
        if (new_internal->data) free(new_internal->data);
        free(new_internal);
        return ENOMEM_VFS;
    }

    new_vnode->mount = dir_node->mount;
    new_vnode->v_ops = &tmpfs_v_ops;
    new_vnode->f_ops = &tmpfs_f_ops;
    new_vnode->internal = new_internal;
    new_vnode->parent = dir_node;
    new_vnode->parent_is_mount = 0;

    parent_internal->children[parent_internal->num_children++] = new_vnode;
    *target = new_vnode;

    uart_puts("[tmpfs_create_or_mkdir] Created ");
    uart_puts(type == TMPFS_NODE_FILE ? "file" : "directory");
    uart_puts(" '");
    uart_puts(component_name);
    uart_puts("' (");
    uart_hex((unsigned long)new_vnode);
    uart_puts(") in directory '");
    uart_puts(parent_internal->name);
    uart_puts("' (");
    uart_hex((unsigned long)dir_node);
    uart_puts(")\r\n");
    return 0; // Success
}

int tmpfs_create(struct vnode* dir_node, struct vnode** target, const char* component_name) {
    return tmpfs_create_or_mkdir(dir_node, target, component_name, TMPFS_NODE_FILE);
}

int tmpfs_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name) {
    return tmpfs_create_or_mkdir(dir_node, target, component_name, TMPFS_NODE_DIRECTORY);
}

int tmpfs_open(struct vnode* file_node, struct file** target) {
    if (!file_node || !target || !(*target)) { // Check if *target itself is NULL (should be pre-allocated by vfs_open)
        return EINVAL_VFS;
    }

    (*target)->f_pos = 0;
    (*target)->vnode = file_node;
    (*target)->f_ops = file_node->f_ops;

    return 0; 
}

int tmpfs_close(struct file* file) {
    if (!file) {
        return EINVAL_VFS;
    }
    file->vnode = NULL;
    file->f_pos = 0;
    file->f_ops = NULL;
    file->flags = 0;
    return 0;
}

int tmpfs_write(struct file* file, const void* buf, size_t len) {
    if (!file || !file->vnode || !file->vnode->internal || !buf) {
        return EINVAL_VFS;
    }
    if (len == 0) return 0;

    struct tmpfs_node* internal_node = (struct tmpfs_node*)file->vnode->internal;
    if (internal_node->type != TMPFS_NODE_FILE) {
        return EACCES_VFS; // Cannot write to a directory
    }

    // Check if we need to reallocate buffer
    if (file->f_pos + len > internal_node->capacity) {
        size_t required_capacity = file->f_pos + len;
        size_t new_capacity = internal_node->capacity > 0 ? internal_node->capacity : DEFAULT_FILE_SIZE;
        while (new_capacity < required_capacity) {
            new_capacity *= 2; // Double the capacity
        }
        if (new_capacity > internal_node->capacity) { // only realloc if new_capacity is actually larger
            char* new_data = (char*)alloc(new_capacity);
            if (!new_data) return ENOMEM_VFS;
            if (internal_node->data) {
                memcpy(new_data, internal_node->data, internal_node->size);
                free(internal_node->data);
            }
            internal_node->data = new_data;
            internal_node->capacity = new_capacity;
        }
    }

    memcpy(internal_node->data + file->f_pos, buf, len);
    file->f_pos += len;
    if (file->f_pos > internal_node->size) {
        internal_node->size = file->f_pos;
    }
    return len;
}

int tmpfs_read(struct file* file, void* buf, size_t len) {
    if (!file || !file->vnode || !file->vnode->internal || !buf) {
        return EINVAL_VFS;
    }
    if (len == 0) return 0;

    struct tmpfs_node* internal_node = (struct tmpfs_node*)file->vnode->internal;
    if (internal_node->type != TMPFS_NODE_FILE) {
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

long tmpfs_lseek64(struct file* file, long offset, int whence) {
    if (!file || !file->vnode || !file->vnode->internal) {
        return EINVAL_VFS;
    }
    struct tmpfs_node* internal_node = (struct tmpfs_node*)file->vnode->internal;
    if (internal_node->type != TMPFS_NODE_FILE) {
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