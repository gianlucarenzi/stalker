/*
 * $Id: debug.h,v 1.1 2016/01/12 09:17:22 gianluca Exp $
 */
#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/* ANSI Eye-Candy ;-) */
#define ANSI_RESET   "\x1b[0m"
#define ANSI_BLACK   "\x1b[30m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_WHITE   "\x1b[37m"

#define ANSI_LIGHT_BLACK   "\x1b[1;30m"
#define ANSI_LIGHT_RED     "\x1b[1;31m"
#define ANSI_LIGHT_GREEN   "\x1b[1;32m"
#define ANSI_LIGHT_YELLOW  "\x1b[1;33m"
#define ANSI_LIGHT_BLUE    "\x1b[1;34m"
#define ANSI_LIGHT_MAGENTA "\x1b[1;35m"
#define ANSI_LIGHT_CYAN    "\x1b[1;36m"
#define ANSI_LIGHT_WHITE   "\x1b[1;37m"

#define printR(fmt, args...) \
	{\
		printf(fmt, ## args); \
	}

#define printRaw(type, fmt, args...) \
	{\
		printf("%s " type " (%s): " fmt, __FILE__, __func__, ## args); \
	}

#define printRaw_E(type, fmt, args...) \
	{\
		printf("%s " type " (%s): " fmt, __FILE__, __func__, ## args); \
	}

#define DBG_N(fmt, args...) \
  { if (debuglevel >= DBG_NOISY) {\
		printf(ANSI_YELLOW "%s NOISY (%s): " fmt ANSI_RESET, __FILE__, __func__, ## args); \
	} \
  }

#define DBG_V(fmt, args...) \
  { if (debuglevel >= DBG_VERBOSE) {\
		printf(ANSI_BLUE "%s VERBOSE (%s): " fmt ANSI_RESET, __FILE__, __func__, ## args); \
	} \
  }

#define DBG_I(fmt, args...) \
  { if (debuglevel >= DBG_INFO) {\
		printf(ANSI_GREEN "%s INFO (%s): " fmt ANSI_RESET, __FILE__, __func__, ## args); \
	} \
  }

#define DBG_W(fmt, args...) \
  { if (debuglevel >= DBG_WARNING) {\
	printf(ANSI_MAGENTA "%s WARN (%s): " fmt ANSI_RESET, __FILE__, __func__, ## args); \
	} \
  }

#define DBG_E(fmt, args...) \
  { \
	printf(ANSI_RED "%s Err (%s): " fmt ANSI_RESET, __FILE__, __func__, ## args); \
  }

#define DBG_ERROR   0
#define DBG_INFO    1
#define DBG_WARNING 2
#define DBG_VERBOSE 3
#define DBG_NOISY   4

#ifdef __cplusplus
}
#endif

#endif
