#include "cpio.h"

#define CPIO_ARCHIVE_START 0x20000000  // In QEMU. If on the board, it should be 0x20000000

const unsigned long HEADER_SIZE = sizeof(struct cpio_newc_header);

void cpio_list() {
    struct cpio_newc_header *header = (struct cpio_newc_header *)CPIO_ARCHIVE_START;

    while (1) {
        char magic[6] = "070701";
        char header_magic[6];
        memcpy(header_magic, header->c_magic, 6);
        if (strcmp(magic, header_magic) == 0) {
            char *filename_addr = (char *)header + HEADER_SIZE;
            char filename[256];
            unsigned int filenamesize = hexstr_to_uint(header->c_namesize, 8);
            memcpy(filename, filename_addr, filenamesize);

            if (strcmp(filename, "TRAILER!!!") == 0) {  // End of the archive
                break;
            }

            uart_puts(filename);
            uart_puts("\r\n");

            // Jump to the next header
            unsigned int filesize = hexstr_to_uint(header->c_filesize, 8);
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


void cpio_cat() {
    uart_puts("Filename: ");
    char target_file[256];
    uart_gets(target_file);

    struct cpio_newc_header *header = (struct cpio_newc_header *)CPIO_ARCHIVE_START;

    while (1) {
        char magic[6] = "070701";
        char header_magic[6];
        memcpy(header_magic, header->c_magic, 6);
        if (strcmp(magic, header_magic) == 0) {
            char *filename_addr = (char *)header + HEADER_SIZE;
            char filename[256];
            unsigned int filenamesize = hexstr_to_uint(header->c_namesize, 8);
            memcpy(filename, filename_addr, filenamesize);

            if (strcmp(filename, "TRAILER!!!") == 0) {  // End of the archive
                uart_puts(target_file);
                uart_puts(":  No such file or directory\r\n");
                break;
            }

            if (strcmp(filename, target_file) == 0) {
                unsigned int filesize = hexstr_to_uint(header->c_filesize, 8);
                char *file_addr = (char *)header + align(HEADER_SIZE + filenamesize, 4);
                for (unsigned int i = 0; i < filesize; i++) {
                    uart_putc(*file_addr);
                    file_addr++;
                }
                uart_puts("\r\n");
                break;
            }

            // Jump to the next header
            unsigned int filesize = hexstr_to_uint(header->c_filesize, 8);
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

