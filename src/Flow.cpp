#include "Flow.hpp"

#include <assert.h>
#include <getopt.h>
#include <string.h>

#include <fstream>

#include "VCG.hpp"

namespace EDA_CHALLENGE_Q4 {
void Flow::doStepTask() {
  switch (_step) {
    case kInit:
      g_log << "----- EDA_CHALLENGE_Q4 -----\n";
      g_log.flush();
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

void Flow::doTaskParseArgv() {
  const struct option table[] = {{"cfg", required_argument, nullptr, 'f'},
                                 {"cst", required_argument, nullptr, 's'},
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
        printf("\t-f,--cfg=FILE     input configure file\n");
        printf("\t-s,--cst=FILE     input constraint file\n");
        printf("\n");
        exit(0);
        break;
    }
  }
}

void Flow::doTaskParseResources() {
  // parse xml
  _parser = new Regex(kXML);
  parseXml(_config_file);
  _conf_man = new ConfigManager(_parser->get_tokens());
  parseXml(_constraint_file);
  _constraint_man = new ConstraintManager(_parser->get_tokens());
  _cell_man = new CellManager(_conf_man);
  delete _parser;
  _parser = nullptr;
}

void Flow::parseXml(char* file) {
  FILE* fp = fopen(file, "r");
  assert(fp != nullptr && _parser != nullptr);

  _parser->reset_tokens();
  const uint16_t buffer_size = 256;
  char buffer[buffer_size];
  while (!feof(fp)) {
    memset(buffer, 0, buffer_size);
    if (fgets(buffer, buffer_size, fp) == nullptr) {
      continue;
    }
    //
    _parser->make_tokens(buffer);
  }
  fclose(fp);
}

void Flow::doTaskFloorplan() {
  // parse pattern to VCG
  _parser = new Regex(kPATTERN);
  for (auto constraint : _constraint_man->get_pattern_list()) {
    g_log << "\n## " << constraint->get_pattern() << " >>\n";
    g_log.flush();

    _parser->make_tokens(const_cast<char*>(constraint->get_pattern().c_str()));
    VCG g(_parser->get_tokens());
    g.set_cell_man(_cell_man);
    g.set_constraint(constraint);
    // // !!!!! floorplan >>>>> !!!!!
    g.find_best_place();
    // // !!!!! <<<<< floorplan !!!!!
    g.gen_GDS();

    _parser->reset_tokens();

    // do cell movement.
    // CellMovement* cell_move = new CellMovement(&g);
    // cell_move->executeCellMovement();
    // delete cell_move;

    ++g._gds_file_num;
    g.gen_result();

    g_log << " << end\n";
    g_log.flush();

    // g.gen_result();
  }

  delete _parser;
  _parser = nullptr;
}

}  // namespace EDA_CHALLENGE_Q4