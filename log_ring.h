#ifndef __log_ring__
#define __log_ring__

bool log_printf(const char * fmt, ...) __attribute__ ((format (printf, 1, 2)));

bool init_log(size_t max_log_entries);

void remove_log(void);


#endif // !__log_ring__