#include "Debug.h"

namespace EDA_CHALLENGE_Q4 { 

/**
 * @brief log file
 */
std::fstream g_log; 

void log_init() {
#ifndef G_LOG
return;
#endif
  g_log.open("../output/log.txt", std::ios::app | std::ios::out);
  assert(g_log.is_open());
}

// only close when flow ends
void log_close() {
#ifndef G_LOG
return;
#endif
  assert(g_log.is_open());
  g_log.close();
}

}