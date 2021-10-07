#include "Flow.hpp"

#include <assert.h>
#include <getopt.h>
#include <string.h>

namespace EDA_CHALLENGE_Q4 {
void Flow::doStepTask() {
  switch (_step) {
    case kInit:
      printf("\n----- EDA_CHALLENGE_Q4 -----\n");
      set_step(kParseArgv);
      break;
    case kParseArgv:
      doTaskParseArgv();
      set_step(kParseResources);
      break;
    case kParseResources:
      doTaskParseResources();
      set_step(kFloorplan);
      break;
    case kFloorplan:
      doTaskFloorplan();
      set_step(kEnd);
      break;
    default:
      assert(0);
  }
}

void Flow::doTaskEnd() {
  assert(_step == kEnd);
  printf("\n----- EDA_CHALLENGE_Q4 END -----\n");
}

void Flow::doTaskParseArgv() {
  const struct option table[] = {
      {"conf", required_argument, nullptr, 'f'},
      {"constraint", required_argument, nullptr, 's'},
      {"help", no_argument, nullptr, 'h'},
      {nullptr, 0, nullptr, 0}};

  int option = 0;
  while ((option = getopt_long(_argc, _argv, "-hf:s:", table, nullptr)) != -1) {
    switch (option) {
      case 'f':
        _config_file = optarg;
        break;
      case 's':
        _constraint_file = optarg;
        break;
      default:
        printf("Usage: %s [OPTION...] \n\n", _argv[0]);
        printf("\t-f,--conf=FILE           input configure file\n");
        printf("\t-s,--constraint=FILE     input constraint file\n");
        printf("\n");
        exit(0);
        break;
    }
  }
}

void Flow::doTaskParseResources() {
  parseXml(_config_file);
  parseXml(_constraint_file);

  parseConf();
  parseConstraint();
}

void Flow::parseXml(char* file) {
  FILE* fp = fopen(file, "r");
  assert(fp != nullptr);

  _parser.reset_tokens();
  const uint16_t buffer_size = 256;
  char buffer[buffer_size];
  while (!feof(fp)) {
    memset(buffer, 0, buffer_size);
    if (fgets(buffer, buffer_size, fp) == nullptr) {
      continue;
    }

    //
    _parser.make_tokens(buffer);
  }
  fclose(fp);
}

void Flow::parseConstraint() { TODO(); }
void Flow::parseConf() { TODO(); }
void Flow::doTaskFloorplan() { TODO(); }
}  // namespace EDA_CHALLENGE_Q4