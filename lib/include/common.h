/*****************************************************************************
  *
  * Monitoring Plugins common include file
  *
  *****************************************************************************/

#ifndef _COMMON_H_
#define _COMMON_H_

#define COPYRIGHT "  Copyright (c) %s Bodo Schulz <%s>\n\n"

#define UT_HLP_VRS ("\
       %s (-h | --help) for detailed help\n\
       %s (-V | --version) for version information\n")

#define UT_HELP_VRSN ("\
\nOptions:\n\
 -h, --help\n\
    Print detailed help screen\n\
 -V, --version\n\
    Print version information\n")

#define UT_SUPPORT ("\n\
Send email to help@monitoring-plugins.org if you have questions regarding\n\
use of this software. To submit patches or suggest improvements, send email\n\
to devel@monitoring-plugins.org\n\n")

#define PACKAGE "monitoring-plugins"
#define VERSION "0.0.0"

/*
 *
 * Standard Values
 *
 */

enum {
  OK = 0,
  ERROR = -1
};

/* AIX seems to have this defined somewhere else */
#ifndef FALSE
enum {
  FALSE,
  TRUE
};
#endif

enum {
  STATE_OK,
  STATE_WARNING,
  STATE_CRITICAL,
  STATE_UNKNOWN,
  STATE_DEPENDENT
};

enum {
  DEFAULT_SOCKET_TIMEOUT = 10,   /* timeout after 10 seconds */
  MAX_INPUT_BUFFER = 8192,       /* max size of most buffers we use */
  MAX_HOST_ADDRESS_LENGTH = 256   /* max size of a host address */
};

void print_revision(const char *, const char *);
void usage(const char *msg);
void usage_va(const char *fmt, ...) __attribute__((noreturn));
int is_integer (char *number);

#endif /* _COMMON_H_ */
