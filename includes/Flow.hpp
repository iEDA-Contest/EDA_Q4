/*
 * @Author: your name
 * @Date: 2021-12-10 20:49:16
 * @LastEditTime: 2021-12-12 16:22:39
 * @LastEditors: your name
 * @Description: 打开koroFileHeader查看配置 进行设置:
 * https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: /EDA_Q4/includes/Flow.hpp
 */
#ifndef __FLOW_HPP_
#define __FLOW_HPP_

#include "../legalization/CellMovement.h"
#include "CellManager.hpp"
#include "ConfigManager.hpp"
#include "ConstraintManager.hpp"

namespace EDA_CHALLENGE_Q4 {

enum FlowStepType {
  kInit,
  kParseArgv,
  kParseResources,
  kFloorplan,
  kEnd,
};

class Flow {
 public:
  // constructor
  Flow(Flow&) = delete;

  // getter
  static Flow& getInstance(const int, char**);
  auto get_step() const { return _step; }
  auto get_argc() const { return _argc; }
  auto get_argv() const { return _argv; }

  // setter
  void set_step(const FlowStepType step) { _step = step; }
  void set_argc(const int argc) { _argc = argc; }
  void set_argv(char** argv) { _argv = argv; }

  // function
  void doStepTask();
  void doTaskEnd();

 private:
  // constructor
  Flow() = default;
  ~Flow() = default;

  // function
  void doTaskParseArgv();
  void doTaskParseResources();
  void doTaskFloorplan();
  void parseXml(char*);

  // member
  FlowStepType _step;      // flow step
  int _argc;               // argc of main function
  char** _argv;            // argv of main function
  char* _config_file;      // configure.xml
  char* _constraint_file;  // constraint.xml
  Regex* _parser;
  ConfigManager* _conf_man;
  ConstraintManager* _constraint_man;
  CellManager* _cell_man;
};

/**
 * @brief singleton model
 *
 *
 *
 * @param argc main function's argc
 * @param argv main function's argv
 * @return Flow& singleton object
 */
inline Flow& Flow::getInstance(const int argc, char** argv) {
  static Flow singleton;

  singleton.set_step(FlowStepType::kInit);
  singleton.set_argc(argc);
  singleton.set_argv(argv);

  log_init();

  return singleton;
}

inline void Flow::doTaskEnd() {
  assert(_step == kEnd);
  g_log << "\n----- EDA_CHALLENGE_Q4 END -----\n";
  log_close();
}

}  // namespace EDA_CHALLENGE_Q4
#endif