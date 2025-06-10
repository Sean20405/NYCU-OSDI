#include "fs_vfs.h"

#define MAX_FILESYSTEMS 10

struct mount* rootfs = NULL;
static struct filesystem* filesystems[MAX_FILESYSTEMS];
static int num_filesystems = 0;


int register_filesystem(struct filesystem* fs) {
    if (fs == NULL || fs->name == NULL) {
      return EINVAL_VFS;
    }
    if (num_filesystems < MAX_FILESYSTEMS) {
        for (int i = 0; i < num_filesystems; i++) {
            if (filesystems[i] != NULL && filesystems[i]->name != NULL && strcmp(filesystems[i]->name, fs->name) == 0) {
              return EEXIST_VFS;
            }
        }
        filesystems[num_filesystems++] = fs;
        return 0; // Success
    }
    return ENOMEM_VFS;
}

// TODO: refactor
int vfs_open(const char* pathname, int flags, struct file** target) {
    if (pathname == NULL || target == NULL) {
        return EINVAL_VFS;
    }

    *target = NULL; // Initialize target
    struct vnode* vnode = NULL;
    int lookup_result = vfs_lookup(pathname, &vnode);
    int ret;

    if (lookup_result != 0) {  // Not found
        if (flags & O_CREAT) {  // Create a file
            uart_puts("[vfs_open] File not found, attempting to create: ");
            uart_puts(pathname);
            uart_puts("\r\n");

            struct vnode* parent_vnode = NULL;
            char* path_copy_for_create = strdup(pathname);
            if (!path_copy_for_create) {
                return ENOMEM_VFS;
            }
            int path_len = strlen(path_copy_for_create);

            // Parse parent directory path to find the vnode
            int last_slash_index = -1;
            for (int i = path_len - 1; i >= 0; i--) {
                if (path_copy_for_create[i] == '/') {
                    last_slash_index = i;
                    break;
                }
            }
            if (last_slash_index == -1) {  // Path is like 'newfile.txt'
                struct vnode* cwd = get_current()->cwd;
                if (cwd) {
                    parent_vnode = cwd;
                }
                else {
                    uart_puts("[vfs_open] Warning: CWD not set, creating relative to root.\n");
                    parent_vnode = rootfs->root;
                }
            }
            else if (last_slash_index == 0) {  // Path starts with '/', e.g., "/file.txt"
                int ret_lookup_root = vfs_lookup("/", &parent_vnode);
                if (ret_lookup_root != 0) {
                    free(path_copy_for_create);
                    return ret_lookup_root;
                }
            }
            else {  // Path is like "/path/to/file" or "path/to/file"
                path_copy_for_create[last_slash_index] = '\0';
                ret = vfs_lookup(path_copy_for_create, &parent_vnode);
                if (ret != 0) {
                    free(path_copy_for_create);
                    uart_puts("Parent directory lookup failed\n");
                    return ret;
                }
            }
        
            uart_puts("[vfs_open] Parent directory found: ");
            uart_puts(path_copy_for_create);
            uart_puts("(");
            uart_hex((unsigned long)parent_vnode);
            uart_puts(")\r\n");

            // Create file on the parent vnode
            char* basename = path_copy_for_create + last_slash_index + 1; // Get the file name
            if (parent_vnode && parent_vnode->v_ops && parent_vnode->v_ops->create) {
                ret = parent_vnode->v_ops->create(parent_vnode, &vnode, basename);
                free(path_copy_for_create);
            }
            else {
                free(path_copy_for_create);
                uart_puts("Parent directory does not support create operation\n");
                return EACCES_VFS; // Or ENOSYS if create op not supported
            }
        }
        else {
            return lookup_result; // Return the error code from vfs_lookup
        }
    }

    // Open file
    *target = (struct file*)alloc(sizeof(struct file));
    ret = vnode->f_ops->open(vnode, target);
    if (ret != 0) {
        uart_puts("File open operation failed\n");
        free(*target); // Clean up allocated file handle
        return ret; // Return the error code from open operation
    }
    (*target)->flags = flags;

    return 0; // Success
}

int vfs_close(struct file* file) {
    if (file == NULL) return EINVAL_VFS;

    uart_puts("[vfs_close] file: ");
    uart_hex((unsigned long)file);
    uart_puts("\r\n");

    int ret = 0;
    if (file->f_ops && file->f_ops->close) {
        ret = file->f_ops->close(file);
    }
    else {
        return ENOSYS_VFS; // Close operation not implemented
    }

    return ret;
}

int vfs_write(struct file* file, const void* buf, size_t len) {
    if (file == NULL || buf == NULL) return EINVAL_VFS;
    if (len == 0) return 0;

    if (file->f_ops && file->f_ops->write) {
      return file->f_ops->write(file, buf, len);
    }
    return ENOSYS_VFS;
}

int vfs_read(struct file* file, void* buf, size_t len) {
    uart_puts("[vfs_read] called with file: ");
    uart_hex((unsigned long)file);
    uart_puts(" and buf: ");
    uart_hex((unsigned long)buf);
    uart_puts(" and len: ");
    uart_puts(itoa(len));
    uart_puts("\r\n");

    if (file == NULL || buf == NULL) return EINVAL_VFS;
    if (len == 0) return 0;

    if (file->f_ops && file->f_ops->read) {
        return file->f_ops->read(file, buf, len);
    }
    return ENOSYS_VFS;
}

int vfs_mkdir(const char* pathname) {
    uart_puts("[vfs_mkdir] called with pathname: ");
    uart_puts(pathname);
    uart_puts("\r\n");

    if (pathname == NULL) {
        return EINVAL_VFS;
    }

    char* path_copy = strdup(pathname);
    if (!path_copy) {
        return ENOMEM_VFS;
    }

    int path_len = strlen(path_copy);
    int last_slash_index = -1;
    for (int i = path_len - 1; i >= 0; i--) {
        if (path_copy[i] == '/') {
            last_slash_index = i;
            break;
        }
    }

    uart_puts("[vfs_mkdir] last_slash_index = ");
    uart_puts(itoa(last_slash_index)); // Assuming you have a function itoa to convert int to string
    uart_puts("\r\n");

    struct vnode* cwd = get_current()->cwd;

    struct vnode* parent_vnode = NULL;
    char* dir_name_to_create;

    if (last_slash_index == -1) { // No slash in path, e.g., "dirname"
        // Relative to CWD
        if (!cwd) {
            uart_puts("[vfs_mkdir] Error: CWD not set for relative path.\r\n");
            free(path_copy);
            return EINTERR_VFS; // Internal error, CWD should be set
        }
        parent_vnode = cwd; // Parent is CWD
        dir_name_to_create = path_copy;
    }
    else if (last_slash_index == 0) { // Path is like "/dirname"
        int ret_lookup_root = vfs_lookup("/", &parent_vnode);
        if (ret_lookup_root != 0) {
            free(path_copy);
            return ret_lookup_root;
        }
        dir_name_to_create = path_copy + 1;
    }
    else { // Path is like "/path/to/dirname" or "path/to/dirname"
        path_copy[last_slash_index] = '\0'; // Null-terminate parent path
        dir_name_to_create = path_copy + last_slash_index + 1;

        int ret_lookup = vfs_lookup(path_copy, &parent_vnode);
        if (ret_lookup != 0) {
            free(path_copy); 
            return ret_lookup; 
        }
    }

    if (strlen(dir_name_to_create) == 0) {
        free(path_copy);
        return EINVAL_VFS; // Empty directory name
    }

    if (parent_vnode && parent_vnode->v_ops && parent_vnode->v_ops->mkdir) {
        struct vnode* new_dir_vnode = NULL;
        int ret_mkdir = parent_vnode->v_ops->mkdir(parent_vnode, &new_dir_vnode, dir_name_to_create);
        if (ret_mkdir == 0) {
            free(path_copy);
            uart_puts("[vfs_mkdir] Directory created successfully: ");
            uart_puts(pathname);
            uart_puts("\r\n");
            return 0; // Success
        }
        else {
            free(path_copy);
            uart_puts("[vfs_mkdir] Failed to create directory: ");
            uart_puts(pathname);
            uart_puts("\r\n");
            return ret_mkdir; // Return the error code from mkdir operation
        }
        // free(path_copy);
        // return ret_mkdir;
    }
    else {
        free(path_copy);
        uart_puts("Parent directory does not support mkdir operation\n");
        return EACCES_VFS;
    }
}

int vfs_mknod(const char* pathname, struct file_operations* f_ops) {
    if (pathname == NULL) {
        return EINVAL_VFS;
    }

    struct file *dev = NULL;
    int ret = vfs_open(pathname, O_CREAT, &dev);
    if (ret != 0) {
        uart_puts("[vfs_mknod] vfs_open failed: ");
        uart_puts(pathname);
        uart_puts("\r\n");
        return ret; // Return the error code from vfs_open
    }

    dev->vnode->f_ops = f_ops;
    
    ret = vfs_close(dev);
    if (ret != 0) {
        uart_puts("[vfs_mknod] vfs_close failed: ");
        uart_puts(pathname);
        uart_puts("\r\n");
        return ret; // Return the error code from vfs_close
    }
    return 0;
}

// Mount a filesystem at a target path, handle the operation before entering the mount point
int vfs_mount(const char* target_pathname, const char* filesystem_name) {
    if (target_pathname == NULL || filesystem_name == NULL) {
        return EINVAL_VFS;
    }

    // Find the filesystem
    struct filesystem* fs_to_mount = NULL;
    for (int i = 0; i < num_filesystems; i++) {
        if (strcmp(filesystems[i]->name, filesystem_name) == 0) {
            fs_to_mount = filesystems[i];
            break;
        }
    }
    if (fs_to_mount == NULL) {
        uart_puts("Filesystem type not found: ");
        uart_puts(filesystem_name);
        uart_puts("\r\n");
        return ENODEV_VFS; // Filesystem type not found or not registered
    }

    // Check if the target path exists and is a valid mount point
    struct vnode* target_vnode = NULL;
    int lookup_result = vfs_lookup(target_pathname, &target_vnode);
    if (lookup_result != 0) {
        return lookup_result;
    }
    if (target_vnode->v_ops == NULL || target_vnode->v_ops->mkdir == NULL) {
        uart_puts("Target path does not support mounting: ");
        uart_puts(target_pathname);
        uart_puts("\r\n");
        return ENOSYS_VFS; // Target path does not support mounting
    }
    if (target_vnode->mount != NULL || target_vnode->parent_is_mount) {
        uart_puts("[vfs_mount] Target path is already mounted: ");
        uart_puts(target_pathname);
        uart_puts("\r\n");
        return EBUSY_VFS; // Target path is already mounted
    }

    // Create a new mount point structure
    struct mount* new_mount = (struct mount*)alloc(sizeof(struct mount));
    if (new_mount == NULL) {
        return ENOMEM_VFS;
    }
    new_mount->fs = fs_to_mount;
    new_mount->root = NULL;  // Will be set by fs_to_mount->setup_mount

    target_vnode->mount = new_mount;

    // Call the filesystem's setup_mount function
    if (fs_to_mount->setup_mount == NULL) {
        free(new_mount);
        uart_puts("Filesystem does not support mounting: ");
        uart_puts(filesystem_name);
        uart_puts("\r\n");
        return ENOSYS_VFS; // Filesystem does not support mounting
    }
    int setup_result = fs_to_mount->setup_mount(fs_to_mount, new_mount);
    if (setup_result != 0) {
        target_vnode->mount = NULL; 
        free(new_mount);
        uart_puts("[vfs_mount] Filesystem setup_mount failed for: ");
        uart_puts(filesystem_name);
        uart_puts("\r\n");
        return setup_result;
    }

    new_mount->root->parent = target_vnode;

    uart_puts("[vfs_mount] Mounted filesystem '");
    uart_puts(filesystem_name);
    uart_puts("' at target path '");
    uart_puts(target_pathname);
    uart_puts("'. New mount root vnode: ");
    uart_puts(((struct tmpfs_node*)(new_mount->root->internal))->name);
    uart_puts(" (");
    uart_hex((unsigned long)new_mount->root);
    uart_puts(")\r\n");

    return 0;  // Success
}

// Get the next component from a path
// @note This function will modify `path_ptr`
static char* get_next_component(char** path_ptr) {
    char* component = *path_ptr;
    if (component == NULL || *component == '\0') {
        return NULL;
    }

    // Skip leading slashes
    while (*component == '/') {
        component++;
    }

    if (*component == '\0') {
        *path_ptr = component; // Path ended with slashes
        return NULL;
    }

    char* end = component;
    while (*end != '\0' && *end != '/') {
        end++;
    }

    if (*end == '/') {
        *end = '\0'; // Null-terminate the component
        *path_ptr = end + 1;
    } else {
        *path_ptr = end; // End of path
    }
    return component;
}

int vfs_lookup(const char* pathname, struct vnode** target) {
    uart_puts("[vfs_lookup] called with pathname: ");
    uart_puts(pathname);
    uart_puts("\r\n");
    
    if (pathname == NULL || target == NULL) {
        return EINVAL_VFS; // Invalid arguments (changed from -1 for consistency)
    }

    *target = NULL;

    // Make a mutable copy of the pathname for get_next_component
    char *path_copy = strdup(pathname);
    if (!path_copy) {
        return ENOMEM_VFS;
    }

    char* current_path_ptr = path_copy;
    struct vnode* current_vnode;
    char* component_name;

    // Determine starting vnode based on path type (absolute or relative)
    if (path_copy[0] == '/') {
        current_vnode = rootfs->root;
    }
    else {
        struct ThreadTask *curr = get_current();
        struct vnode* cwd = curr->cwd;
        if (cwd == NULL) {
            uart_puts("[vfs_lookup] Error: CWD not set for relative path lookup.\n");
            free(path_copy);
            return EINTERR_VFS; // Internal error: CWD should always be initialized
        }
        current_vnode = cwd;
    }
    
    // Special case: "/"
    if (strcmp(pathname, "/") == 0) {
        *target = rootfs->root;
        free(path_copy);
        return 0;
    }
    // Special case: "" or "." (relative to CWD)
    if (strlen(pathname) == 0 || strcmp(pathname, ".") == 0) {
        *target = current_vnode;
        free(path_copy);
        return 0;
    }

    while ((component_name = get_next_component(&current_path_ptr)) != NULL) {
        if (strcmp(component_name, ".") == 0) {
            continue;
        }
        if (strcmp(component_name, "..") == 0) {
            if (current_vnode == rootfs->root) {
                continue; // ".." at root, stay at root
            }
            else if (current_vnode->parent == NULL) {
                uart_puts("[vfs_lookup] Error: '..' on a non-root vnode with NULL vfs_parent.\r\n");
                free(path_copy);
                return EINTERR_VFS;
            }
            else {
                struct vnode* vfs_p = current_vnode->parent;
                // Check if current_vnode is the root of a filesystem mounted on vfs_p
                if (vfs_p->mount != NULL && vfs_p->mount->root == current_vnode) {
                    if (vfs_p == rootfs->root) { // Mounted on global root
                        current_vnode = rootfs->root;
                    }
                    else if (vfs_p->parent != NULL) {
                        current_vnode = vfs_p->parent;
                    }
                    else {
                        uart_puts("[vfs_lookup] Error: Mount directory (not root) has no vfs_parent for '..'.\r\n");
                        free(path_copy);
                        return EINTERR_VFS;
                    }
                }
                else {  // Regular ".." - go to VFS parent.
                    current_vnode = vfs_p;
                }
            }
            *target = current_vnode;
            continue;
        }

        // If current_vnode is a mount point, look into its file system root
        if (current_vnode->mount != NULL) {
            uart_puts("[vfs_lookup] Current vnode '");
            uart_puts(((struct tmpfs_node*)(current_vnode->internal))->name);
            uart_puts("' is a mount point, switching to its root (");
            uart_hex((unsigned long)current_vnode->mount->root);
            uart_puts(")\r\n");

            current_vnode = current_vnode->mount->root;
        }

        uart_puts("[vfs_lookup] Looking up component '");
        uart_puts(component_name);
        uart_puts("' in path '");
        uart_puts(pathname);
        uart_puts("'. Under current vnode: ");
        uart_puts(((struct tmpfs_node*)(current_vnode->internal))->name);
        uart_puts(" (");
        uart_hex((unsigned long)current_vnode);
        uart_puts(")\r\n");

        if (current_vnode->v_ops == NULL || current_vnode->v_ops->lookup == NULL) {
            free(path_copy);
            return ENOSYS_VFS; // Filesystem or vnode does not support lookup
        }
        
        struct vnode* next_vnode = NULL;
        int result = current_vnode->v_ops->lookup(current_vnode, &next_vnode, component_name);

        if (result != 0) {
            free(path_copy);
            uart_puts("[vfs_lookup] Lookup failed for component '");
            uart_puts(component_name);
            uart_puts("' in path '");
            uart_puts(pathname);
            uart_puts("'. Error code: ");
            uart_puts(itoa(result));
            uart_puts("\r\n");
            return result; // Lookup failed
        }

        // If next_vnode is a mount point, look into its file system root
        if (next_vnode->mount != NULL) {
            uart_puts("[vfs_lookup] Next vnode '");
            uart_puts(((struct tmpfs_node*)(next_vnode->internal))->name);
            uart_puts("' is a mount point, switching to its root (");
            uart_hex((unsigned long)next_vnode->mount->root);
            uart_puts(")\r\n");

            next_vnode = next_vnode->mount->root;
        }

        current_vnode = next_vnode;
    }

    *target = current_vnode;
    free(path_copy);
    return 0; // Success
}

void vfs_init() {
    uart_puts("Initializing VFS...\n");
    // root FS
    register_tmpfs();
    rootfs = (struct mount*)alloc(sizeof(struct mount));
    if (rootfs == NULL) {
        uart_puts("Failed to allocate memory for root filesystem mount\n");
        return;
    }
    filesystems[0]->setup_mount(filesystems[0], rootfs);
    rootfs->root->parent = NULL;

    vfs_mkdir("/initramfs");
    register_initramfs();
    vfs_mount("/initramfs", "initramfs");
    struct vnode *initramfs_node;
    vfs_lookup("/initramfs", &initramfs_node);
    initramfs_init(initramfs_node);

    vfs_mkdir("/dev");
    vfs_mknod("/dev/uart", &uart_f_ops);
}
