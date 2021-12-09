#include "ConfigManager.hpp"

#include "Debug.h"

namespace EDA_CHALLENGE_Q4 {

constexpr static uint8_t id_base_mem = id_base * kCellTypeMem;
constexpr static uint8_t id_base_soc = id_base * kCellTypeSoc;

ConfigManager::ConfigManager(Token_List& tokens) {
  std::vector<Config*>* op_list = nullptr;
  std::vector<RegexType> stack;  // maybe change to an uint8[] better
  Config* conf = nullptr;
  uint8_t id_base_tmp = 0;

  for (auto token : tokens) {
    switch (token.second) {
      // skip
      case kTAG_XML_HEAD:
      case kTAG_DATA_BEG:
      case kTAG_DATA_END:
        break;

      // push with MEM/SOC tag
      case kTAG_MEM_BEG:
        stack.push_back(token.second);
        op_list = &_mem_config_list;
        conf = new Config();
        break;
      case kTAG_SOC_BEG:
        stack.push_back(token.second);
        op_list = &_soc_config_list;
        conf = new Config();
        break;

      // pop with MEM/SOC tag
      case kTAG_MEM_END:
        assert(stack.back() == kTAG_MEM_BEG);
        stack.pop_back();
        op_list->push_back(conf);
        conf = nullptr;
        op_list = nullptr;
        break;
      case kTAG_SOC_END:
        assert(stack.back() == kTAG_SOC_BEG);
        stack.pop_back();
        op_list->push_back(conf);
        conf = nullptr;
        op_list = nullptr;
        break;

      // push with property tag
      case kTAG_REFERENCE_BEG:
        id_base_tmp = stack.back() == kTAG_MEM_BEG ? id_base_mem : id_base_soc;
      case kTAG_AMOUNT_BEG:
      case kTAG_WIDTH_BEG:
      case kTAG_HEIGHT_BEG:
        assert(stack.back() == kTAG_MEM_BEG || stack.back() == kTAG_SOC_BEG);
        stack.push_back(token.second);
        break;

      // pop with property tag
      case kTAG_REFERENCE_END:
        assert(stack.back() == kTAG_REFERENCE_BEG);
        stack.pop_back();
        break;
      case kTAG_AMOUNT_END:
        assert(stack.back() == kTAG_AMOUNT_BEG);
        stack.pop_back();
        break;
      case kTAG_WIDTH_END:
        assert(stack.back() == kTAG_WIDTH_BEG);
        stack.pop_back();
        break;
      case kTAG_HEIGHT_END:
        assert(stack.back() == kTAG_HEIGHT_BEG);
        stack.pop_back();
        break;

      // set data
      case kDATA:
        if (stack.back() == kTAG_REFERENCE_BEG) {
          auto id = id_base_tmp + op_list->size();
          conf->set_id_refer(id);
          conf->set_refer(token.first);
          _id_refers_map[id] = token.first;
        } else {
          conf->set_value(stack.back(), atoi(token.first.c_str()));
        }
        break;

      // bugs
      default:
        PANIC("No operation tag: %s", token.first.c_str());
        break;
    }
  }

  ASSERT(stack.size() == 0, "Some tokens should be processed");
}

ConfigManager::~ConfigManager() {
  _id_refers_map.clear();
  for (auto m : _mem_config_list) {
    delete m;
  }
  _mem_config_list.clear();
  
  for (auto s : _soc_config_list) {
    delete s;
  }
  _soc_config_list.clear();
}

}  // namespace EDA_CHALLENGE_Q4
