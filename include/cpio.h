#ifndef CPIO_H
#define CPIO_H

#include "string.h"
#include "uart.h"
#include "utils.h"

struct cpio_newc_header {
    char c_magic[6];
    char c_ino[8];
    char c_mode[8];
    char c_uid[8];
    char c_gid[8];
    char c_nlink[8];
    char c_mtime[8];
    char c_filesize[8];
    char c_devmajor[8];
    char c_devminor[8];
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8];
    char c_check[8];
};

void cpio_list();
void cpio_cat(char *target_file);
char* cpio_get_exec(char *target_file, char *exec_addr);
unsigned int cpio_get_file_size(char *target_file);

#endif /* CPIO_H */