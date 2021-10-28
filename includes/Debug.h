#ifndef __MACRO_H_
#define __MACRO_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "stdarg.h"

namespace EDA_CHALLENGE_Q4 {

#define ASSERT(cond, ...)           \
  do {                              \
    if (!(cond)) {                  \
      fflush(stdout);               \
      fprintf(stderr, "\33[1;31m"); \
      fprintf(stderr, __VA_ARGS__); \
      fprintf(stderr, "\33[0m\n");  \
      assert(cond);                 \
    }                               \
  } while (0)

#define PANIC(...) ASSERT(0, __VA_ARGS__)

#define TODO() PANIC("TO IMPLEMENT %s()\n", __func__)

// #define GDS

}  // namespace EDA_CHALLENGE_Q4
#endif