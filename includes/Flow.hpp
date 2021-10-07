#ifndef __FLOW_HPP_
#define __FLOW_HPP_

#include "Regex.hpp"

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
  void parseConf();
  void parseConstraint();
  void parseXml(char*);

  // member variables
  FlowStepType _step;      // flow step
  int _argc;               // argc of main function
  char** _argv;            // argv of main function
  char* _config_file;      // configure.xml
  char* _constraint_file;  // constraint.xml
  Regex& _parser = Regex::getInstance();
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

  return singleton;
}

}  // namespace EDA_CHALLENGE_Q4
#endif