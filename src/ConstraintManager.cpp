#include "ConstraintManager.hpp"
namespace EDA_CHALLENGE_Q4 {

ConstraintManager::ConstraintManager(Token_List& tokens) {
  std::vector<RegexType> stack;  // maybe change to an uint8[] better
  Constraint* constraint = nullptr;

  for (auto token : tokens) {
    switch (token.second) {
      // skip
      case kTAG_XML_HEAD:
      case kTAG_DATA_BEG:
      case kTAG_DATA_END:
        break;

      // constraint tag
      case kTAG_CONSTRAINT_BEG:
        stack.push_back(token.second);
        constraint = new Constraint();
        break;
      case kTAG_CONSTRAINT_END:
        assert(stack.back() == kTAG_CONSTRAINT_BEG);
        stack.pop_back();
        _pattern_list.push_back(*constraint);
        constraint = nullptr;
        break;

      // pattern tag
      case kTAG_PATTERN_BEG:
        assert(stack.back() == kTAG_CONSTRAINT_BEG);
        stack.push_back(token.second);
        break;
      case kTAG_PATTERN_END:
        assert(stack.back() == kTAG_PATTERN_BEG);
        stack.pop_back();
        break;

      // push with constraint property tag
      case kTAG_XMM_BEG:
      case kTAG_YMM_BEG:
      case kTAG_XSS_BEG:
      case kTAG_YSS_BEG:
      case kTAG_XMS_BEG:
      case kTAG_YMS_BEG:
      case kTAG_XMI_BEG:
      case kTAG_YMI_BEG:
      case kTAG_XSI_BEG:
      case kTAG_YSI_BEG:
        assert(stack.back() == kTAG_CONSTRAINT_BEG);
        stack.push_back(token.second);
        break;

      // pop with constraint property tag
      case kTAG_XMM_END:
        assert(stack.back() == kTAG_XMM_BEG);
        stack.pop_back();
        break;
      case kTAG_YMM_END:
        assert(stack.back() == kTAG_YMM_BEG);
        stack.pop_back();
        break;
      case kTAG_XSS_END:
        assert(stack.back() == kTAG_XSS_BEG);
        stack.pop_back();
        break;
      case kTAG_YSS_END:
        assert(stack.back() == kTAG_YSS_BEG);
        stack.pop_back();
        break;
      case kTAG_XMS_END:
        assert(stack.back() == kTAG_XMS_BEG);
        stack.pop_back();
        break;
      case kTAG_YMS_END:
        assert(stack.back() == kTAG_YMS_BEG);
        stack.pop_back();
        break;
      case kTAG_XMI_END:
        assert(stack.back() == kTAG_XMI_BEG);
        stack.pop_back();
        break;
      case kTAG_YMI_END:
        assert(stack.back() == kTAG_YMI_BEG);
        stack.pop_back();
        break;
      case kTAG_XSI_END:
        assert(stack.back() == kTAG_XSI_BEG);
        stack.pop_back();
        break;
      case kTAG_YSI_END:
        assert(stack.back() == kTAG_YSI_BEG);
        stack.pop_back();
        break;

      // set data
      case kDATA:
        if (stack.back() == kTAG_PATTERN_BEG) {
          constraint->set_pattern(token.first);
        } else {
          constraint->set_constraint(stack.back(), token.first.c_str());
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

void Constraint::set_constraint(RegexType type, const char* data) {
  uint16_t min;
  uint16_t max;
  sscanf(data, "%hd %hd", &min, &max);
  assert(min < max);

  switch (type) {
    case kTAG_XMM_BEG:
      set_constraint(kXMM_MIN, min);
      set_constraint(kXMM_MAX, max);
      break;
    case kTAG_YMM_BEG:
      set_constraint(kYMM_MIN, min);
      set_constraint(kYMM_MAX, max);
      break;
    case kTAG_XSS_BEG:
      set_constraint(kXSS_MIN, min);
      set_constraint(kXSS_MAX, max);
      break;
    case kTAG_YSS_BEG:
      set_constraint(kYSS_MIN, min);
      set_constraint(kYSS_MAX, max);
      break;
    case kTAG_XMS_BEG:
      set_constraint(kXMS_MIN, min);
      set_constraint(kXMS_MAX, max);
      break;
    case kTAG_YMS_BEG:
      set_constraint(kYMS_MIN, min);
      set_constraint(kYMS_MAX, max);
      break;
    case kTAG_XMI_BEG:
      set_constraint(kXMI_MIN, min);
      set_constraint(kXMI_MAX, max);
      break;
    case kTAG_YMI_BEG:
      set_constraint(kYMI_MIN, min);
      set_constraint(kYMI_MAX, max);
      break;
    case kTAG_XSI_BEG:
      set_constraint(kXSI_MIN, min);
      set_constraint(kXSI_MAX, max);
      break;
    case kTAG_YSI_BEG:
      set_constraint(kYSI_MIN, min);
      set_constraint(kYSI_MAX, max);
      break;
    default:
      PANIC("Invalid tagid:%d", type);
  }
}

}  // namespace EDA_CHALLENGE_Q4
