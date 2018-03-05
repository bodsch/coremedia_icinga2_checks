
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <string>
#include <cctype>
#include <vector>
#include <limits>
#include <algorithm>
#include <map>
#include <hiredis/hiredis.h>

#include <common.h>
#include <resolver.h>
#include <md5.h>

const char *progname = "check_test";
const char *NP_VERSION = "0.0.1";
const char *copyright = "2018";
const char *email = "bodo@boone-schulz-de";

void print_help (void);
void print_usage (void);

int main(int argc, char **argv) {

  int result = STATE_UNKNOWN;

  if (argc < 2)
    std::cout << progname << " Could not parse arguments" << std::endl;
  else
  if (strcmp (argv[1], "-V") == 0 || strcmp (argv[1], "--version") == 0) {
    print_revision (progname, NP_VERSION);
    exit (STATE_UNKNOWN);
  }
  else
  if (strcmp (argv[1], "-h") == 0 || strcmp (argv[1], "--help") == 0) {
    print_help();
    exit (STATE_UNKNOWN);
  }
  else
  if (!is_integer (argv[1]))
    std::cout << progname << " Arguments to check_dummy must be an integer" << std::endl;
  else
    result = atoi (argv[1]);

  switch (result) {
    case STATE_OK:
      printf("OK");
      break;
    case STATE_WARNING:
      printf("WARNING");
      break;
    case STATE_CRITICAL:
      printf("CRITICAL");
      break;
    case STATE_UNKNOWN:
      printf("UNKNOWN");
      break;
    default:
      printf("UNKNOWN");
      printf(": ");
      printf("Status %d is not a supported error state\n", result);
      return STATE_UNKNOWN;
  }

  if (argc >= 3)
    printf (": %s", argv[2]);

  printf("\n");

  return result;
}

void print_help (void) {

  printf ("\n%s v%s (%s %s)\n", progname, NP_VERSION, PACKAGE, VERSION);

//   print_revision (progname, NP_VERSION);
  printf (COPYRIGHT, copyright, email);

  printf ("%s\n", ("This plugin will simply return the state corresponding to the numeric value"));
  printf ("%s\n", ("of the <state> argument with optional text"));
  printf ("\n\n");
  print_usage ();
  printf (UT_HELP_VRSN);
}

void print_usage (void) {
  printf ("%s\n", ("Usage:"));
  printf (" %s <integer state> [optional text]\n", progname);
}
