#ifndef MSDK_COMMON_H_
#define MSDK_COMMON_H_

typedef enum {
	INFO,
	DEBUG,
	ERROR,
} log_level_t;

typedef enum {
	MSDK_DATA,
	MSDK_EXIT,
} msg_type;

struct msdk_message {
	msg_type type;
	char cmd[100];
}__attribute__((aligned)); 

void log_level_init(void);

void msdk_log(log_level_t level, const char *fmt, ...);

int abstract_inet_open(int port);

void init_data(void);

int mainloop(int epfd, int sfd, int timeout_ms);
#endif
