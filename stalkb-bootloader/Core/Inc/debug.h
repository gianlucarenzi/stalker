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
#define ANSI_RED    "\x1b[31m"
#define ANSI_GREEN  "\x1b[32m"
#define ANSI_YELLOW "\x1b[1;33m"
#define ANSI_BLUE   "\x1b[1;34m"
#define ANSI_RESET  "\x1b[0m"

#define printR(fmt, args...) \
	{\
		printf(fmt, ## args); \
	}

#define printRaw(type, fmt, args...) \
	{\
		printf("%s " type " (%s): " fmt, __FILE__, __func__, ## args); \
	}

#define DBG_N(fmt, args...) \
  { if (debuglevel >= DBG_NOISY) {\
		printf(ANSI_YELLOW); \
		printRaw("NOISY", fmt,## args); \
		printf(ANSI_RESET); \
	} \
  }

#define DBG_V(fmt, args...) \
  { if (debuglevel >= DBG_VERBOSE) {\
		printf(ANSI_BLUE); \
		printRaw("VERBOSE", fmt,## args); \
		printf(ANSI_RESET); \
	} \
  }

#define DBG_I(fmt, args...) \
  { if (debuglevel >= DBG_INFO) {\
		printf(ANSI_GREEN); \
		printRaw("INFO", fmt,## args); \
		printf(ANSI_RESET); \
	} \
  }

#define DBG_E(fmt, args...) \
  { \
	printf(ANSI_RED); \
	printRaw("ERROR", fmt, ## args); \
	printf(ANSI_RESET); \
  }

#define DBG_ERROR   0
#define DBG_INFO    1
#define DBG_VERBOSE 2
#define DBG_NOISY   3

#ifdef __cplusplus
}
#endif

#endif
