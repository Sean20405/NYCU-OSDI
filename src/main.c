#include "uart.h"
#include "shell.h"
#include "devicetree.h"
#include "timer.h"
#include "mm.h"
#include "sched.h"
#include "utils.h"
#include "syscall.h"
#include "exec.h"
#include "fs_vfs.h"

extern char *__stack_top;
extern uint32_t cpio_addr;
extern uint32_t cpio_end;
extern uint32_t fdt_total_size;

void foo(){
    for(int i = 0; i < 10; ++i) {
        unsigned int thread_id = ((struct ThreadTask *)get_current())->id;
        uart_puts("Thread id: ");
        uart_puts(itoa(thread_id));
        uart_puts(" ");
        uart_puts(itoa(i));
        uart_puts("\n");
        
        delay(1000000);
        schedule();
    }
    _exit();
}

void fork_test(){
    uart_puts("\r\nFork Test, pid ");
    uart_puts(itoa(get_pid()));
    uart_puts("\r\n");

    int cnt = 1;
    int ret = 0;
    if ((ret = fork()) == 0) { // child
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));

        uart_puts("first child pid: ");
        uart_puts(itoa(get_pid()));
        uart_puts(", cnt: ");
        uart_puts(itoa(cnt));
        uart_puts(", ptr: ");
        uart_hex(&cnt);
        uart_puts(", sp: ");
        uart_hex(cur_sp);
        uart_puts("\r\n");

        ++cnt;

        if ((ret = fork()) != 0){
            asm volatile("mov %0, sp" : "=r"(cur_sp));

            uart_puts("first child pid: ");
            uart_puts(itoa(get_pid()));
            uart_puts(", cnt: ");
            uart_puts(itoa(cnt));
            uart_puts(", ptr: ");
            uart_hex(&cnt);
            uart_puts(", sp: ");
            uart_hex(cur_sp);
            uart_puts("\r\n");
        }
        else{
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                
                uart_puts("second child pid: ");
                uart_puts(itoa(get_pid()));
                uart_puts(", cnt: ");
                uart_puts(itoa(cnt));
                uart_puts(", ptr: ");
                uart_hex(&cnt);
                uart_puts(", sp: ");
                uart_hex(cur_sp);
                uart_puts("\r\n");

                delay(50000000);
                ++cnt;
            }
            exit();
        }
        exit();
    }
    else {
        uart_puts("parent here, pid ");
        uart_puts(itoa(get_pid()));
        uart_puts(", child ");
        uart_puts(itoa(ret));
        uart_puts("\r\n");
    }
    exit();
}

void test_syscall() {
    /******** PID ********/
    // int pid = get_pid();
    // uart_puts("sys_getpid: ");
    // uart_puts(itoa(pid));
    // uart_puts("\n");

    /******** Read & Write ********/
    // char buf[32];
    // int ret = uart_read(buf, 10);
    // uart_puts("sys_uart_read: ");
    // if (ret > 0) {
    //     uart_puts(buf);
    //     uart_puts("\n");
    // }
    // else {
    //     uart_puts("Failed to read from UART\n");
    // }
    // ret = uart_write(buf, 10);
    // if (ret > 0) {
    //     uart_puts("sys_uart_write: ");
    //     uart_puts(buf);
    //     uart_puts("\n");
    // }
    // else {
    //     uart_puts("Failed to write to UART\n");
    // }
    // _exec();

    /******** Fork ********/
    struct ThreadTask* new_thread = thread_create(fork_test);

    asm volatile(
        "msr tpidr_el1, %0\n"
        "mov x5, 0x0\n"
        "msr spsr_el1, x5\n"
        "msr elr_el1, %1\n"
        "msr sp_el0, %2\n"
        "mov sp, %3\n"
        "eret"
        :
        : "r"(new_thread), "r"(new_thread->cpu_context.lr), "r"(new_thread->cpu_context.sp), 
          "r"(new_thread->kernel_stack + THREAD_STACK_SIZE)
        : "x5"
    );
}

// Create a shell thread that run in EL0
void create_shell_thread() {
    struct ThreadTask* new_thread = thread_create(shell);
    asm volatile(
        "msr tpidr_el1, %0\n"
        "mov x5, 0x0\n"
        "msr spsr_el1, x5\n"
        "msr elr_el1, %1\n"
        "msr sp_el0, %2\n"
        "mov sp, %3\n"
        "eret"
        :
        : "r"(new_thread), "r"(new_thread->cpu_context.lr), "r"(new_thread->cpu_context.sp), 
          "r"(new_thread->kernel_stack + THREAD_STACK_SIZE)
        : "x5"
    );
}

static void print_status_msg(const char* msg, int status_code) {
    uart_puts(msg);
    if (status_code == 0) {
        uart_puts("0 (SUCCESS)");
    } else {
        uart_puts("- (FAILURE, code: ");
        // 基本的負整數轉字串（VFS 錯誤碼通常為負）
        if (status_code < 0) {
            uart_puts("-");
            int s_abs = -status_code;
            char b[12]; // 足夠容納 int
            int i = 0;
            if (s_abs == 0) { b[i++] = '0'; }
            else {
                int temp_s = s_abs;
                while(temp_s > 0) { temp_s /= 10; i++;} // 計算位數
                b[i] = '\0';
                temp_s = s_abs;
                while(i > 0){ i--; b[i] = (temp_s % 10) + '0'; temp_s /= 10;}
            }
            uart_puts(b);
        } else {
             uart_puts("positive value"); // VFS 錯誤碼通常不為正
        }
        uart_puts(")");
    }
    uart_puts("\r\n");
}

void run_tmpfs_test_suite() {
    struct file* f = NULL;
    struct vnode* v_node = NULL;
    char buffer[100];
    int ret;
    int bytes_rw;
    const char* write_data = "Hello tmpfs!";
    size_t write_len = strlen(write_data);

    uart_puts("\r\n--- Starting tmpfs Test Suite ---\r\n");

    // 測試 1: 建立目錄
    uart_puts("\r\n▍ Test 1: Creating directory /mydir ...\r\n");
    ret = vfs_mkdir("/mydir");
    print_status_msg("  vfs_mkdir(\"/mydir\") result: ", ret);
    int test1_ok = (ret == 0);

    // 測試 2: 在新目錄中建立檔案
    uart_puts("\r\n▍ Test 2: Creating file /mydir/testfile.txt ...\r\n");
    if (test1_ok) {
        ret = vfs_open("/mydir/testfile.txt", O_CREAT, &f);
        print_status_msg("  vfs_open(\"/mydir/testfile.txt\", O_CREAT) result: ", ret);
        if (ret == 0 && f != NULL) {
            uart_puts("    File handle obtained.\r\n");
        } else {
            f = NULL;
            uart_puts("    Failed to obtain file handle.\r\n");
        }
    } else {
        uart_puts("  Skipped (dependent on Test 1 success).\r\n");
        ret = -1; 
    }
    int test2_ok = (ret == 0 && f != NULL);

    // 測試 3: 寫入檔案
    uart_puts("\r\n▍ Test 3: Writing to /mydir/testfile.txt ...\r\n");
    if (test2_ok) {
        bytes_rw = vfs_write(f, write_data, write_len);
        uart_puts("  vfs_write result (bytes written): ");
        if (bytes_rw == (int)write_len) { uart_puts("SUCCESS (expected length written)\r\n"); }
        else { uart_puts("FAILURE (bytes_rw != expected_len)\r\n"); }
    } else {
        uart_puts("  Skipped (dependent on Test 2 success).\r\n");
        bytes_rw = -1;
    }
    int test3_ok = (bytes_rw == (int)write_len);

    // 測試 4: 定位並讀取檔案
    uart_puts("\r\n▍ Test 4: Seeking and reading from /mydir/testfile.txt ...\r\n");
    if (test3_ok) {
        uart_puts("  Seeking to beginning (SEEK_SET, 0)...\r\n");
        long lseek_ret = f->f_ops->lseek64(f, 0, SEEK_SET); // 直接使用 f_ops->lseek64
        print_status_msg("    lseek64 result (new position): ", (int)lseek_ret);

        memset(buffer, 0, sizeof(buffer));
        uart_puts("  Reading back data...\r\n");
        bytes_rw = vfs_read(f, buffer, write_len);
        uart_puts("    vfs_read result (bytes read): ");
        if (bytes_rw == (int)write_len) { uart_puts("SUCCESS (expected length read)\r\n"); }
        else { uart_puts("FAILURE (bytes_rw != expected_len)\r\n"); }

        if (bytes_rw == (int)write_len && strcmp(buffer, write_data) == 0) {
            uart_puts("    Data verification: SUCCESS. Read: '"); uart_puts(buffer); uart_puts("'\r\n");
        } else {
            uart_puts("    Data verification: FAILURE. Read: '"); uart_puts(buffer); uart_puts("', Expected: '"); uart_puts(write_data); uart_puts("'\r\n");
        }
    } else {
        uart_puts("  Skipped (dependent on Test 3 success).\r\n");
    }

    // 測試 5: 關閉檔案
    uart_puts("\r\n▍ Test 5: Closing /mydir/testfile.txt ...\r\n");
    if (f != NULL) {
        ret = vfs_close(f);
        print_status_msg("  vfs_close result: ", ret);
        f = NULL; 
    } else {
        uart_puts("  Skipped (file not open).\r\n");
    }
    // 注意: 根據目前的 VFS 設計，與 f 關聯的 vnode (f->vnode) 在此處可能發生記憶體洩漏。

    // 測試 6: 查找目錄
    uart_puts("\r\n▍ Test 6: Looking up directory /mydir ...\r\n");
    if (test1_ok) {
        ret = vfs_lookup("/mydir", &v_node);
        print_status_msg("  vfs_lookup(\"/mydir\") result: ", ret);
        if (ret == 0 && v_node != NULL) {
            uart_puts("    Directory /mydir looked up successfully.\r\n");
        } else {
            uart_puts("    Failed to lookup /mydir.\r\n");
        }
    } else {
         uart_puts("  Skipped (dependent on Test 1 success for /mydir existence).\r\n");
    }

    // 測試 7: 查找檔案
    uart_puts("\r\n▍ Test 7: Looking up file /mydir/testfile.txt ...\r\n");
    if (test1_ok && test2_ok) { // 假設目錄和檔案已成功建立
        ret = vfs_lookup("/mydir/testfile.txt", &v_node);
        print_status_msg("  vfs_lookup(\"/mydir/testfile.txt\") result: ", ret);
        if (ret == 0 && v_node != NULL) {
            uart_puts("    File /mydir/testfile.txt looked up successfully.\r\n");
        } else {
            uart_puts("    Failed to lookup /mydir/testfile.txt.\r\n");
        }
    } else {
        uart_puts("  Skipped (dependent on Test 1 & 2 success for file existence).\r\n");
    }

    // 測試 8: 查找不存在的檔案
    uart_puts("\r\n▍ Test 8: Looking up non-existent file /nosuchfile.txt ...\r\n");
    ret = vfs_lookup("/nosuchfile.txt", &v_node);
    print_status_msg("  vfs_lookup(\"/nosuchfile.txt\") result: ", ret);
    if (ret == ENOENT_VFS) {
        uart_puts("    Correctly failed with ENOENT_VFS.\r\n");
    } else {
        uart_puts("    Failed with unexpected code or v_node was not NULL.\r\n");
        if (v_node != NULL) { free(v_node); v_node = NULL; }
    }

    // 測試 9: 使用 O_CREAT 開啟已存在的檔案
    uart_puts("\r\n▍ Test 9: Opening existing file /mydir/testfile.txt with O_CREAT ...\r\n");
    if (test1_ok && test2_ok) { // 檔案先前已成功建立
        struct file* f_reopen = NULL;
        ret = vfs_open("/mydir/testfile.txt", O_CREAT, &f_reopen);
        print_status_msg("  vfs_open (existing, O_CREAT) result: ", ret);
        if (ret == 0 && f_reopen != NULL) {
            uart_puts("    Successfully opened existing file with O_CREAT.\r\n");
            memset(buffer, 0, sizeof(buffer));
            bytes_rw = vfs_read(f_reopen, buffer, write_len);
            uart_puts("    vfs_read after reopen (bytes read): ");
            if (bytes_rw == (int)write_len) { uart_puts("SUCCESS\r\n"); } else { uart_puts("FAILURE\r\n"); }

            if (bytes_rw == (int)write_len && strcmp(buffer, write_data) == 0) {
                uart_puts("    Data verification after reopen: SUCCESS. Read: '"); uart_puts(buffer); uart_puts("'\r\n");
            } else {
                uart_puts("    Data verification after reopen: FAILURE. Read: '"); uart_puts(buffer); uart_puts("', Expected: '"); uart_puts(write_data); uart_puts("'\r\n");
            }
            vfs_close(f_reopen);
        } else {
            uart_puts("    Failed to open existing file with O_CREAT.\r\n");
        }
    } else {
        uart_puts("  Skipped (dependent on previous tests for file existence).\r\n");
    }

    uart_puts("--- tmpfs Test Suite Finished ---\r\n");
}

void run_mount_tests() {
    struct file* f = NULL;
    char buffer[100];
    int ret;
    int bytes_rw;
    const char* write_data_orig = "Original FS data!";
    size_t write_len_orig = strlen(write_data_orig);
    const char* write_data_mounted = "Mounted FS data!";
    size_t write_len_mounted = strlen(write_data_mounted);

    uart_puts("\r\n--- Starting Mount Test Suite ---\r\n");

    // 前置作業：確保根 tmpfs 已初始化，並建立一個掛載點目錄
    uart_puts("Mount Test Setup: Creating /mountpointdir ...\r\n");
    ret = vfs_mkdir("/mountpointdir");
    print_status_msg("  vfs_mkdir(\"/mountpointdir\") result: ", ret);
    if (ret != 0) {
        uart_puts("  Failed to create /mountpointdir, aborting mount tests.\r\n");
        return;
    }

    // 測試 M1: 在原始檔案系統的 /mountpointdir 中建立檔案
    uart_puts("\r\n▍ Test M1: Creating and writing to /mountpointdir/pre_mount.txt ...\r\n");
    ret = vfs_open("/mountpointdir/pre_mount.txt", O_CREAT, &f);
    print_status_msg("  vfs_open(\"/mountpointdir/pre_mount.txt\", O_CREAT) result: ", ret);
    if (ret == 0 && f != NULL) {
        bytes_rw = vfs_write(f, write_data_orig, write_len_orig);
        if (bytes_rw == (int)write_len_orig) {
            uart_puts("    Write to pre_mount.txt: SUCCESS\r\n");
        } else {
            uart_puts("    Write to pre_mount.txt: FAILURE\r\n");
        }
        vfs_close(f); f = NULL;
    } else {
        uart_puts("    Failed to create /mountpointdir/pre_mount.txt\r\n");
    }

    // 測試 M2: 掛載新的 tmpfs 到 /mountpointdir
    uart_puts("\r\n▍ Test M2: Mounting a new tmpfs onto /mountpointdir ...\r\n");
    ret = vfs_mount("/mountpointdir", "tmpfs");
    print_status_msg("  vfs_mount(\"/mountpointdir\", \"tmpfs\") result: ", ret);
    int mount_ok = (ret == 0);

    // 測試 M3: 在掛載的檔案系統中建立檔案
    uart_puts("\r\n▍ Test M3: Creating and writing to /mountpointdir/on_mounted.txt ...\r\n");
    if (mount_ok) {
        ret = vfs_open("/mountpointdir/on_mounted.txt", O_CREAT, &f);
        print_status_msg("  vfs_open(\"/mountpointdir/on_mounted.txt\", O_CREAT) result: ", ret);
        if (ret == 0 && f != NULL) {
            bytes_rw = vfs_write(f, write_data_mounted, write_len_mounted);
            if (bytes_rw == (int)write_len_mounted) {
                uart_puts("    Write to on_mounted.txt: SUCCESS\r\n");
            } else {
                uart_puts("    Write to on_mounted.txt: FAILURE\r\n");
            }
            vfs_close(f); f = NULL;
        } else {
            uart_puts("    Failed to create /mountpointdir/on_mounted.txt\r\n");
        }
    } else {
        uart_puts("  Skipped (dependent on Test M2 success).\r\n");
    }

    // 測試 M4: 讀取掛載檔案系統中的檔案
    uart_puts("\r\n▍ Test M4: Reading from /mountpointdir/on_mounted.txt ...\r\n");
    if (mount_ok) {
        ret = vfs_open("/mountpointdir/on_mounted.txt", 0, &f); // No O_CREAT
        print_status_msg("  vfs_open(\"/mountpointdir/on_mounted.txt\", 0) result: ", ret);
        if (ret == 0 && f != NULL) {
            memset(buffer, 0, sizeof(buffer));
            bytes_rw = vfs_read(f, buffer, write_len_mounted);
            if (bytes_rw == (int)write_len_mounted && strcmp(buffer, write_data_mounted) == 0) {
                uart_puts("    Read from on_mounted.txt: SUCCESS. Data: '"); uart_puts(buffer); uart_puts("'\r\n");
            } else {
                uart_puts("    Read from on_mounted.txt: FAILURE. Read: '"); uart_puts(buffer); uart_puts("', Expected: '"); uart_puts(write_data_mounted); uart_puts("'\r\n");
            }
            vfs_close(f); f = NULL;
        } else {
            uart_puts("    Failed to open /mountpointdir/on_mounted.txt for reading\r\n");
        }
    } else {
        uart_puts("  Skipped (dependent on Test M2 success).\r\n");
    }

    // 測試 M5: 嘗試讀取原始檔案系統中的 pre_mount.txt (應該不可見)
    uart_puts("\r\n▍ Test M5: Attempting to read /mountpointdir/pre_mount.txt (should fail or not exist) ...\r\n");
    if (mount_ok) {
        ret = vfs_open("/mountpointdir/pre_mount.txt", 0, &f);
        print_status_msg("  vfs_open(\"/mountpointdir/pre_mount.txt\", 0) result: ", ret);
        if (ret == ENOENT_VFS) {
            uart_puts("    Correctly failed with ENOENT_VFS (file is hidden by mount).\r\n");
        } else if (ret == 0 && f != NULL) {
            uart_puts("    ERROR: File pre_mount.txt is accessible after mount!\r\n");
            vfs_close(f); f = NULL;
        } else {
            uart_puts("    Unexpected result or error.\r\n");
        }
    } else {
        uart_puts("  Skipped (dependent on Test M2 success).\r\n");
    }
    
    // 測試 M6: 在掛載點內建立子目錄
    uart_puts("\r\n▍ Test M6: Creating directory /mountpointdir/mounted_subdir ...\r\n");
    if (mount_ok) {
        ret = vfs_mkdir("/mountpointdir/mounted_subdir");
        print_status_msg("  vfs_mkdir(\"/mountpointdir/mounted_subdir\") result: ", ret);
        if (ret == 0) {
            // 測試 M7: 在掛載點的子目錄中建立檔案
            uart_puts("\r\n▍ Test M7: Creating /mountpointdir/mounted_subdir/nested_file.txt ...\r\n");
            ret = vfs_open("/mountpointdir/mounted_subdir/nested_file.txt", O_CREAT, &f);
            print_status_msg("  vfs_open(\"/mountpointdir/mounted_subdir/nested_file.txt\", O_CREAT) result: ", ret);
            if (ret == 0 && f != NULL) {
                uart_puts("    Successfully created nested_file.txt.\r\n");
                vfs_close(f); f = NULL;
            } else {
                uart_puts("    Failed to create nested_file.txt.\r\n");
            }
        } else {
            uart_puts("    Failed to create /mountpointdir/mounted_subdir.\r\n");
        }
    } else {
        uart_puts("  Skipped (dependent on Test M2 success).\r\n");
    }

    // TODO: Add unmount tests if vfs_unmount is implemented.
    // For now, the mounted filesystem will persist.

    uart_puts("--- Mount Test Suite Finished ---\r\n");
}

void main() {
    // Get the address of the device tree blob
    uint32_t dtb_address;
    asm volatile ("mov %0, x20" : "=r" (dtb_address));

    int ret = fdt_init((void*)dtb_address);
    if (ret) {
        uart_puts("Failed to initialize the device tree blob!\n");
        return;
    }
    ret = fdt_traverse(initramfs_callback);
    if (ret) {
        uart_puts("Failed to traverse the device tree blob!\n");
        return;
    }

    mm_init();
    reserve(0x0000, 0x1000);                                    // Spin tables for multicore boot
    reserve(0x80000, (void*)&__stack_top);                      // Kernel image & startup allocator
    reserve((void*)cpio_addr, (void*)cpio_end);                 // Initramfs
    reserve((void*)dtb_address, (void*)dtb_address + be2le_u32(fdt_total_size));   // Devicetree 

    kmem_cache_init();

    vfs_init();

    enable_irq_el1();

    sched_init();

    timer_init();

    // run_tmpfs_test_suite();
    // run_mount_tests();

    // Lab5 Basic 1: Threads
    // sched_init();
    // for(int i = 0; i < 5; ++i) { // N should > 2
    //     thread_create(foo);
    // }
    // idle();

    // Lab5 Basic 2: Fork
    // thread_create(test_syscall);

    create_shell_thread();
    
    // Lab3 Basic 2: Print uptime every 2 seconds
    // add_timer(print_uptime, NULL, 2 * get_freq());

    // shell();
}