/// @file log.c
/// @brief logging functions

#include "log.h"
#include <time.h>
#include "string.h"

// file global logger configuration. Change log target via log_init() & log_exit()
logLevel_e log_level = ELogError;
logStyle_e log_style = ELogStyleMinimal;
FILE *log_stdout = NULL;

/// @brief Initialize logger. Must be called before first logging attempt
/// @param level a loglevel
/// @param style a logstye
/// @param logfile path to file for logging, or NULL for default (stdout)
/// @return ENoErr on success, EErr on failure
int log_init(logLevel_e level, logStyle_e style, const char* logfile) {

	if (!log_exit()) {
		if (fprintf(stdout, "Failed to discard previous logger settings") < 0) {
			fprintf(stderr, "Failed to print to stdout");
		}
		return EErr;
	}

	if (level <= ELogDebug)
	log_level = level;

	if (style <= ELogStyleVerbose)
	log_style = style;

	if (logfile == NULL) {
		log_stdout = stdout;
	} else {
		log_stdout = fopen(logfile, "w");
		if (log_stdout == NULL) {
			log_stdout = stdout;
			log_error("Failed to reroute logging to file @ %s: %s", logfile, errno);
			return EErr;
		}
	}

	return ENoErr;
}

/// @brief Cleanup logger. Close any open file descriptors and reset log_stdout to NULL
/// @return ENoErr on success, EErr otherwise
int log_exit() {

	int ret = 0;

	if (log_stdout == NULL)
	return ENoErr;

	log_level = ELogError;
	log_style = ELogStyleMinimal;

	// close log_stdout
	if (log_stdout == stdout) {
		log_stdout = NULL;
	} else {
		ret = fclose(log_stdout);
		if (ret != 0) {
			log_error("Failed to close stdout logfile");
		} else {
			log_debug("Successfully closed stdout logfile");
		}
		log_stdout = NULL;
	}

	return (ret == 0) ? ENoErr : EErr;
}

/// @brief print an error message to stderr
/// @param fmt a formatted string
/// @param ... option specifiers for string
void log_error(const char *fmt, ...) {

	if (log_stdout == NULL)
	return;

	char buf[MAX_LINE_LEN];
	va_list args;

	switch(log_level) {
		case ELogDisable: return;
		case ELogError:   break;
		case ELogWarn   : break;
		case ELogVerbose: break;
		case ELogDebug:   break;
		default: return;
	}

	switch(log_style) {
		case ELogStyleNone: sprintf(buf, "%s\n", fmt); break;
		case ELogStyleMinimal: sprintf(buf, "error   | %s\n", fmt); break;
		case ELogStyleVerbose: {
			//get time
			time_t t;
			struct tm *tm;
			time(&t);
			tm = localtime(&t);
			sprintf(buf, "(PID %d) | %02d:%02d:%02d |  ERROR   | %s\n", \
			getpid(), tm->tm_hour, tm->tm_min, tm->tm_sec, fmt);
		} break;
		default: return;
	}

	va_start(args, fmt);
	vfprintf(log_stdout, buf, args);  // intentionally do not check return value, since there's nothing we can do about it
	va_end(args);
	fflush(log_stdout);
}

/// @brief print a warning message to stdout
/// @param fmt a formatted string
/// @param ... option specifiers for string
void log_warning(const char *fmt, ...) {

	if (log_stdout == NULL)
	return;

	char buf[MAX_LINE_LEN];
	va_list args;

	switch(log_level) {
		case ELogDisable: return;
		case ELogError:   return;
		case ELogWarn   : break;
		case ELogVerbose: break;
		case ELogDebug:   break;
		default: return;
	}

	switch(log_style) {
		case ELogStyleNone: sprintf(buf, "%s\n", fmt); break;
		case ELogStyleMinimal: sprintf(buf, "warning | %s\n", fmt); break;
		case ELogStyleVerbose: {
			//get time
			time_t t;
			struct tm *tm;
			time(&t);
			tm = localtime(&t);
			sprintf(buf, "(PID %d) | %02d:%02d:%02d |  WARNING | %s\n", \
			getpid(), tm->tm_hour, tm->tm_min, tm->tm_sec, fmt);
		} break;
		default: return;
	}

	va_start(args, fmt);
	vfprintf(log_stdout, buf, args);  // intentionally do not check return value, since there's nothing we can do about it
	va_end(args);
	fflush(log_stdout);
}

/// @brief print a notice to stdout
/// @param fmt a formatted string
/// @param ... option specifiers for string
void log_notice(const char *fmt, ...) {

	if (log_stdout == NULL)
	return;

	char buf[MAX_LINE_LEN];
	va_list args;

	switch(log_level) {
		case ELogDisable: return;
		case ELogError:   return;
		case ELogWarn   : return;
		case ELogVerbose: break;
		case ELogDebug:   break;
		default: return;
	}

	switch(log_style) {
		case ELogStyleNone: sprintf(buf, "%s\n", fmt); break;
		case ELogStyleMinimal: sprintf(buf, "notice  | %s\n", fmt); break;
		case ELogStyleVerbose: {
			//get time
			time_t t;
			struct tm *tm;
			time(&t);
			tm = localtime(&t);
			sprintf(buf, "(PID %d) | %02d:%02d:%02d |  NOTICE  | %s\n", \
			getpid(), tm->tm_hour, tm->tm_min, tm->tm_sec, fmt);
		} break;
		default: return;
	}

	va_start(args, fmt);
	vfprintf(log_stdout, buf, args);  // intentionally do not check return value, since there's nothing we can do about it
	va_end(args);
	fflush(log_stdout);
}

/// @brief print a always to stdout
/// @param fmt a formatted string
/// @param ... option specifiers for string
void log_always(const char *fmt, ...) {

	if (log_stdout == NULL)
	return;

	char buf[MAX_LINE_LEN];
	va_list args;

	switch(log_level) {
		case ELogDisable: return;
		case ELogError:   break;
		case ELogWarn   : break;
		case ELogVerbose: break;
		case ELogDebug:   break;
		default: return;
	}

	switch(log_style) {
		case ELogStyleNone: sprintf(buf, "%s\n", fmt); break;
		case ELogStyleMinimal: sprintf(buf, "always  | %s\n", fmt); break;
		case ELogStyleVerbose: {
			//get time
			time_t t;
			struct tm *tm;
			time(&t);
			tm = localtime(&t);
			sprintf(buf, "(PID %d) | %02d:%02d:%02d |  ALWAYS  | %s\n", \
			getpid(), tm->tm_hour, tm->tm_min, tm->tm_sec, fmt);
		} break;
		default: return;
	}

	va_start(args, fmt);
	vfprintf(log_stdout, buf, args);  // intentionally do not check return value, since there's nothing we can do about it
	va_end(args);
	fflush(log_stdout);
}

/// @brief print a debug message to stdout
/// @param fmt a formatted string
/// @param ... option specifiers for string
void log_debug(const char *fmt, ...) {

	if (log_stdout == NULL)
	return;

	char buf[MAX_LINE_LEN];
	va_list args;

	switch(log_level) {
		case ELogDisable: return;
		case ELogError:   return;
		case ELogWarn   : return;
		case ELogVerbose: return;
		case ELogDebug:   break;
		default: return;
	}

	switch(log_style) {
		case ELogStyleNone: sprintf(buf, "%s\n", fmt); break;
		case ELogStyleMinimal: sprintf(buf, "debug   | %s\n", fmt); break;
		case ELogStyleVerbose: {
			//get time
			time_t t;
			struct tm *tm;
			time(&t);
			tm = localtime(&t);
			sprintf(buf, "(PID %d) | %02d:%02d:%02d |  DEBUG   | %s\n", \
			getpid(), tm->tm_hour, tm->tm_min, tm->tm_sec, fmt);
		} break;
		default: return;
	}

	//TODO: test this code && if it works: copy to all other functions
	va_start(args, fmt);
	if (vfprintf(log_stdout, buf, args) < 0) {
		va_end(args);
		if (log_stdout != stdout) {
			fprintf(stdout, "error | Logging to file failed. Rerouting to stdout...");
			fclose(log_stdout);
			log_stdout = stdout;
		}
	}
	va_end(args);
	fflush(log_stdout);
}

char* get_logstring(char *buf, logStyle_e log_style, const char *log_level, const char *fmt) {
	switch(log_style) {
		case ELogStyleNone: sprintf(buf, "%s\n", fmt); break;
		case ELogStyleMinimal: sprintf(buf, "%s  | %s\n", log_level, fmt); break;
		case ELogStyleVerbose: {
			//get time
			char level[30] = {'\0'};
			time_t t;
			struct tm *tm;
			time(&t);
			tm = localtime(&t);
			sprintf(buf, "(PID %d) | %02d:%02d:%02d |  %s  | %s\n", \
			getpid(), tm->tm_hour, tm->tm_min, tm->tm_sec, \
			to_upper(log_level, level, strlen(log_level)), fmt);
		} break;
		default: break;
	}
	return buf;
}

char* to_upper(const char *in, char *buf, int len) {

	for (int i = 0; i < len; i++) {
		if ((in[i] < ASCII_LOWER_START) || (in[i] > ASCII_LOWER_END)) continue;
		buf[i] = (in[i] - 32);
	}
	return buf;
}
