#include "Flow.hpp"

using namespace EDA_CHALLENGE_Q4;
int main(int argc, char** argv) {
  Flow& flow = Flow::getInstance(argc, argv);
  while (flow.get_step() != FlowStepType::kEnd) {
    flow.doStepTask();
  }
  flow.doTaskEnd();
  return 0;
}