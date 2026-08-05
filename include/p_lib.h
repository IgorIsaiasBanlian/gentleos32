/* lib/errors.c */
extern const char *error_message_for(int code);
/* lib/file.c */
extern const char *file_type_names[FILE_TYPE_COUNT];
extern file_st *file_lookup(const char *name);
/* lib/heap.c */
extern void heap_dump(void);
extern size_t heap_get_avail_mem(void);
extern size_t heap_get_used_mem(void);
extern void heap_add_region(uintptr_t start, uintptr_t end);
extern void *heap_alloc(size_t size, const char *desc, int assert);
extern void heap_free(void *ptr);
/* lib/key.c */
extern char key_char_for_code(uint8_t code, uint8_t mods);
extern int key_number_for_code(uint8_t code);
/* lib/printf.c */
extern int vsnprintf(char *buf, size_t nbyte, const char *fmt, va_list va);
extern int snprintf(char *buf, size_t nbyte, const char *fmt, ...);
/* lib/rand.c */
extern void rand_add_entropy(uint32_t seed);
extern uint32_t rand(void);
/* lib/sleep.c */
extern void sleep(uint32_t msecs);
extern void halt(void);
/* lib/string.c */
extern void *memcpy(void *dest, const void *src, size_t n);
extern void *memset(void *dest, int c, size_t n);
extern int32_t strcmp(const char *s1, const char *s2);
extern int32_t strncmp(const char *s1, const char *s2, size_t n);
extern size_t strlen(const char *s1);
extern char *strncpy(char *dest, const char *src, size_t n);
/* lib/time.c */
extern const char *TIME_MONTH_NAMES_SHORT[];
extern const char *TIME_DAY_NAMES_SHORT[];
extern const char *TIME_DAY_NAMES_LONG[];
extern int time_get_day_of_week(int day, int month, int year);
extern int time_get_days_in_month(int month, int year);
extern int time_equals(time_st *t1, time_st *t2);
extern void time_init(time_st *t, uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
extern void time_clear(time_st *t);
extern void time_copy(time_st *dst, time_st *src);
extern void time_add_seconds(time_st *t, uint32_t secs);
extern void time_get(time_st *t);
extern void time_set(time_st *t, int set_rtc);
