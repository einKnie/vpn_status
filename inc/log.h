/// @file log.h
/// @brief header file of logger

#ifndef _LOG_H_
#define _LOG_H_

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <linux/limits.h>

#define MAX_FILE_PATH_LEN   PATH_MAX  ///< max logfile path length
#define MAX_LINE_LEN        700 ///< max logging line length. Set to absurdly hight length

#define ASCII_LOWER_START 97
#define ASCII_LOWER_END   122

#define LOGLEVEL_DEFAULT ELogVerbose
#define LOGSTYLE_DEFAULT ELogStyleVerbose

enum { EErr = 0, ENoErr }; ///< Return values
typedef enum {
	ELogDisable = 0, ///< no logging
	ELogError,       ///< just errors
	ELogWarn,        ///< errors and warnings
	ELogVerbose,     ///< errors, warnings, and notices
	ELogDebug        ///< errors, warnings, notices, and debug messages
} logLevel_e;      ///< log level

typedef enum {
	ELogStyleNone,   ///< print just the plain message
	ELogStyleMinimal,///< output minimalistic logging. example: "warning | <message>"
	ELogStyleVerbose ///< output verbose logging. example: "PID <pid> | WARNING | <message>"
} logStyle_e;      ///< log style

int  log_init(logLevel_e level, logStyle_e style, const char* logfile);
int  log_exit(void);
void log_error(const char *fmt, ...);
void log_warning(const char *fmt, ...);
void log_notice(const char *fmt, ...);
void log_debug(const char *fmt, ...);
void log_always(const char *fmt, ...);

char* get_logstring(char *buf, logStyle_e log_style, const char *log_level, const char *fmt);
char* to_upper(const char *in, char *buf, int len);

#endif // !_LOG_H_
