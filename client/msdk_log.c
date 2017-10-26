#include <stdio.h>
#include <stdarg.h>
#include "common.h"

static log_level_t first_level;

void log_level_init(void)
{
#ifdef DBG
	first_level = DEBUG;
#else
	first_level = ERROR;
#endif
}

void msdk_log(log_level_t level, const char *fmt, ...)
{
	if (level < first_level)
		return;
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}
