#ifndef __MACRO_H_
#define __MACRO_H_

#include <stdio.h>
#include <stdlib.h>

namespace EDA_CHALLENGE_Q4 {

#define Assert(cond, ...)           \
  do {                              \
    if (!(cond)) {                  \
      fflush(stdout);               \
      fprintf(stderr, "\33[1;31m"); \
      fprintf(stderr, __VA_ARGS__); \
      fprintf(stderr, "\33[0m\n");  \
      assert(cond);                 \
    }                               \
  } while (0)

#define panic(...) Assert(0, __VA_ARGS__)

#define TODO() panic("TO IMPLEMENT %s()\n", __func__)

}  // namespace EDA_CHALLENGE_Q4
#endif