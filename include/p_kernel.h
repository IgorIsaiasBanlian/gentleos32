/* kernel/core_c.c */
extern uint32_t krn_core_mboot_header[];
extern void krn_core_c_main(void);
extern __attribute__((force_align_arg_pointer)) void krn_core_c_isr_handle(isr_stack_st *isr_stack);
/* kernel/debug.c */
extern void (*krn_debug_status_cb)(const char *, ...);
extern void krn_debug_putc(char c);
extern void krn_debug_printf(const char *fmt, ...);
extern void krn_debug_assert(int expr, const char *file, unsigned line);
extern void krn_debug_beep(unsigned hz, unsigned msecs, unsigned count);
extern void krn_debug_dump_kernel_location(void);
/* kernel/event.c */
extern int krn_event_ipush(event_st event);
extern int krn_event_push(event_st event);
extern int krn_event_pop(event_st *event);
extern uint16_t krn_event_count(void);
/* kernel/intr.c */
extern void krn_intr_handle(isr_stack_st *isr_stack);
extern void krn_intr_set_handler(uint8_t int_no, isr_handler_fn handler);
extern void krn_intr_init(void);
/* kernel/keyboard.c */
extern void krn_keyboard_init(void);
/* kernel/main.c */
extern system_info_st krn_system_info;
extern void krn_main(void);
/* kernel/mboot.c */
extern mboot_info_st *krn_core_mboot_info;
extern void krn_mboot_init(void);
/* kernel/mouse.c */
extern void krn_mouse_handle_uart_data(uint8_t data);
extern void krn_mouse_handle_ps2_data(uint8_t data);
/* kernel/pic.c */
extern void krn_pic_init(void);
/* kernel/ps2.c */
extern uint8_t krn_ps2_read_data(int wait);
extern void krn_ps2_reboot(void);
extern void krn_ps2_init(void);
/* kernel/rtc.c */
extern int krn_rtc_are_times_equal(time_st *t1, time_st *t2);
extern void krn_rtc_get_time(time_st *t);
/* kernel/speaker.c */
extern void krn_speaker_stop(void);
extern void krn_speaker_play(unsigned hz);
/* kernel/system.c */
extern uint32_t krn_system_get_total_mem(void);
extern uint32_t krn_system_get_used_mem(void);
extern uint32_t krn_system_get_avail_mem(void);
/* kernel/timer.c */
extern volatile uint8_t krn_timer_is_cpu_idle;
extern uint32_t krn_timer_get_msecs(void);
extern uint8_t krn_timer_get_cpu_usage(void);
extern void krn_timer_init(void);
/* kernel/uart.c */
extern void krn_uart_write_data(uint8_t data);
extern void krn_uart_init(void);
/* kernel/vga.c */
extern void krn_vga_set_color(int index, uint32_t rgb);
extern void krn_vga_init(void);
