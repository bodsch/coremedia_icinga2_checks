
#include <stdio.h>							/* obligatory includes */
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>      /* printf */
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */

#include <getopt.h>

#include "../include/common.h"

extern void print_usage (void);
extern const char *progname;

void usage(const char *msg) {
  printf ("%s\n", msg);
  print_usage ();
  exit (STATE_UNKNOWN);
}

void usage_va (const char *fmt, ...)
{
	va_list ap;
	printf("%s: ", progname);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");
	exit (STATE_UNKNOWN);
}

// void usage4 (const char *msg) {
// 	printf ("%s: %s\n", progname, msg);
// 	print_usage();
// 	exit (STATE_UNKNOWN);
// }

void print_revision (const char *command_name, const char *revision) {
  printf ("%s v%s (%s %s)\n", command_name, revision, PACKAGE, VERSION);
}

const char *state_text (int result) {

	switch (result) {
	case STATE_OK:
		return "OK";
	case STATE_WARNING:
		return "WARNING";
	case STATE_CRITICAL:
		return "CRITICAL";
	case STATE_DEPENDENT:
		return "DEPENDENT";
	default:
		return "UNKNOWN";
	}
}

int is_integer (char *number) {
	long int n;

	if (!number || (strspn (number, "-0123456789 ") != strlen (number)))
		return FALSE;

	n = strtol (number, NULL, 10);

	if (errno != ERANGE && n >= INT_MIN && n <= INT_MAX)
		return TRUE;
	else
		return FALSE;
}



int is_intneg (char *number){
	if (is_integer (number) && atoi (number) < 0)
		return TRUE;
	else
		return FALSE;
}

int is_intnonneg (char *number) {
	if (is_integer (number) && atoi (number) >= 0)
		return TRUE;
	else
		return FALSE;
}

int is_intpercent (char *number) {
	int i;
	if (is_integer (number) && (i = atoi (number)) >= 0 && i <= 100)
		return TRUE;
	else
		return FALSE;
}


char* human_readable(double size/*in bytes*/, char *buf) {

  int i = 0;
  const char* units[] = {"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
  while (size > 1024) {
    size /= 1024;
    i++;
  }
  sprintf(buf, "%.*f %s", i, size, units[i]);
  return buf;
}

/**
 *
 */
bool in_array(const std::string &value, const std::vector<std::string> &array) {
  return std::find(array.begin(), array.end(), value) != array.end();
}


int service_state( std::string state_string, int state_numeric ) {

  int state = STATE_UNKNOWN;

  if( state_numeric != -1 ) {

    if( state_numeric == STOPPED ) {
      std::cout << "Service are in <b>stopped</b> state." << std::endl;
      state = STATE_WARNING;
    } else
    if( state_numeric == STARTING ) {
      std::cout << "Service are in <b>starting</b> state." << std::endl;
      state = STATE_UNKNOWN;
    } else
    if( state_numeric == INITIALIZING ) {
      std::cout << "Service are in <b>initializing</b> state." << std::endl;
      state = STATE_UNKNOWN;
    } else
    if( state_numeric == RUNNING) {
      state = STATE_OK;
    }
    else {
      std::cout << "Service are in <b>failed</b> state." << std::endl;
      state  = STATE_CRITICAL;
    }

  } else {

    std::transform(state_string.begin(), state_string.end(), state_string.begin(), ::tolower);

    if( state_string == "initializing") {
      std::cout << "Service are in <b>initializing</b> state." << std::endl;
      state  = STATE_WARNING;
    } else
    if( state_string == "running") {
      state = STATE_OK;
    }
    else {
      std::cout << "Service are in <b>unknown</b> state." << std::endl;
      state = STATE_CRITICAL;
    }
  }

  return state;
}

