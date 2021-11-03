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
      set_step(kGDSGen);
      break;
    case kGDSGen:
      doTaskGDSGen();
      set_step(kEnd);
      break;
    default:
      assert(0);
  }
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
  // parse xml
  _parser = new Regex(kXML);
  parseXml(_config_file);
  _conf_man = new ConfigManager(_parser->get_tokens());
  parseXml(_constraint_file);
  _constraint_man = new ConstraintManager(_parser->get_tokens());
  _cell_man = new CellManager(_conf_man);
  delete _parser;
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
    _parser->make_tokens(const_cast<char*>(constraint->get_pattern().c_str()));
    VCG g(_parser->get_tokens());
    g.set_cell_man(_cell_man);
    g.set_constraint(constraint);
    // !!!!! floorplan >>>>> !!!!!
    // TODO();
    g.find_best_place();
    // !!!!! <<<<< floorplan !!!!!
    _parser->reset_tokens();
  }

  delete _parser;
}

void Flow::doTaskGDSGen() {
#ifndef GDS
  return;
#endif
  std::ofstream gds("/home/liangyuyao/EDA_CHALLENGE_Q4/output/myresult.gds");
  assert(gds.is_open());

  // gds head
  gds << "HEADER 600\n";
  gds << "BGNLIB\n";
  gds << "LIBNAME DensityLib\n";
  gds << "UNITS 1 1e-6\n";
  gds << "\n\n";

  // rectangle's four relative coordinates
  // template
  gds << "BGNSTR\n";
  gds << "STRNAME area\n";
  gds << "BOUNDARY\n";
  gds << "LAYER 1\n";
  gds << "DATATYPE 0\n";
  gds << "XY\n";
  // this five coordinates should be clockwise or anti-clockwise
  gds << "0 : 0\n";
  gds << "1000 : 0\n";
  gds << "1000 : 2000\n";
  gds << "0 : 2000\n";
  gds << "0 : 0\n";
  gds << "ENDEL\n";
  gds << "ENDSTR\n";

  // add rectangles into top module
  gds << "\n\n";
  gds << "BGNSTR\n";
  gds << "STRNAME top\n";
  gds << "\n\n";

  // template
  gds << "SREF\n";
  gds << "SNAME area\n";
  gds << "XY 0:0\n";  // c1 point
  gds << "ENDEL\n";

  //
  gds << "\n\n";
  gds << "ENDSTR\n";
  gds << "ENDLIB\n";

  gds.close();
}

}  // namespace EDA_CHALLENGE_Q4