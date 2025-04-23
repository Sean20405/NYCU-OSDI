#include "timer.h"

#define TIMER_MSG_SIZE 64

struct Timer {
    struct Timer* prev;
    struct Timer* next;
    timer_callback callback;
    char msg[TIMER_MSG_SIZE];
    int expiration;  // Unit: tick
};

static struct Timer* timer_head = NULL;

void timer_enable_irq() {
    asm volatile("msr cntp_ctl_el0, %0"::"r"(1));
    *CORE0_TIMER_IRQ_CTRL = (1 << 1);
}

void timer_disable_irq() {
    asm volatile("msr cntp_ctl_el0, %0"::"r"(0));
    *CORE0_TIMER_IRQ_CTRL &= ~(1 << 1);
}

void set_timer_irq(unsigned long long tick) {
    unsigned long long expiration = tick;
    asm volatile("msr cntp_tval_el0, %0"::"r"(expiration));
}

void print_msg(char* msg) {
    unsigned long long curr_tick = get_tick();
    unsigned long long freq = get_freq();

    uart_puts("Timer expired: ");
    uart_puts(msg);
    uart_puts(" at time: ");
    uart_hex(curr_tick / freq);
    uart_puts(" sec.\r\n");
}

void print_uptime(char* _) {
    unsigned long long curr_tick = get_tick();
    unsigned long long freq = get_freq();

    uart_puts("Uptime: ");
    uart_hex(curr_tick / freq);
    uart_puts(" sec.\r\n");

    add_timer(print_uptime, NULL, 2 * freq);
}

void core_timer_handler() {
    int curr_tick = get_tick();

    timer_disable_irq();
    enable_irq_el1();  // Can enable IRQ in advance for other interrupts

    // Clear all expired timers
    int removed = 0;

    while (timer_head != NULL && timer_head->expiration <= curr_tick) {
        struct Timer* curr = timer_head;
        timer_head = curr->next;
        if (curr->next) curr->next->prev = NULL;
        removed = 1;

        curr->callback(curr->msg);
        // free(curr);
    }

    // Reset the timer
    if (removed && timer_head) {
        set_timer_irq(timer_head->expiration - curr_tick);
        timer_enable_irq();
    }
}

unsigned long long get_tick() {
    unsigned long long cntpct_el0 = 0;
    asm volatile("mrs %0, cntpct_el0" : "=r"(cntpct_el0));
    return cntpct_el0;
}

unsigned long long get_freq() {
    unsigned long long cntfrq_el0 = 0;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq_el0));
    return cntfrq_el0;
}

unsigned long long get_time() {
    unsigned long long cntpct_el0 = 0;
    asm volatile("mrs %0, cntpct_el0" : "=r"(cntpct_el0));
    unsigned long long cntfrq_el0 = 0;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq_el0));
    return cntpct_el0 / cntfrq_el0;
}

void set_timeout(char* msg, int sec) {
    unsigned long long cntfrq_el0 = 0;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq_el0));

    add_timer(print_msg, msg, (unsigned long long)sec * cntfrq_el0);
}

void add_timer(timer_callback callback, char* msg, unsigned long long tick) {
    struct Timer* new_timer = (struct Timer*)simple_alloc(sizeof(struct Timer));
    if (new_timer == NULL) {
        uart_puts("Failed to allocate memory for timer\r\n");
        return;
    }

    unsigned long long curr_tick = get_tick();
    memcpy(new_timer->msg, msg, strlen(msg) + 1);
    new_timer->expiration = curr_tick + tick;
    new_timer->callback = callback;
    

    int reset = 0;

    // Add the new timer to the list
    timer_disable_irq();
    if (timer_head == NULL) {  // List is empty
        new_timer->prev = NULL;
        new_timer->next = NULL;
        timer_head = new_timer;
        reset = 1;
    }
    else if (timer_head->expiration >= new_timer->expiration) {  // Insert at head
        new_timer->next = timer_head;
        timer_head->prev = new_timer;
        new_timer->prev = NULL;
        timer_head = new_timer;
        reset = 1;
    }
    else {
        struct Timer* curr = timer_head;
        while (curr->next != NULL && curr->next->expiration < new_timer->expiration) {
            curr = curr->next;
        }
        new_timer->next = curr->next;
        new_timer->prev = curr;
        curr->next = new_timer;
        if (curr->next != NULL) {
            curr->next->prev = new_timer;
        }
    }

    // Reset the timer
    if (reset) {
        set_timer_irq(timer_head->expiration - curr_tick);
    }
    timer_enable_irq();
}