#include "Regex.hpp"

namespace EDA_CHALLENGE_Q4 {
Regex::Regex(RegexMode mode) {
  ASSERT(mode != kMODE_NULL, "Regex Mode should not be null!");
  set_mode(mode);
  switch (mode) {
    case kXML:
      set_rules(xml_rules);
      set_num_regex(sizeof(xml_rules) / sizeof(xml_rules[0]));
      break;
    case kPATTERN:
      set_rules(pattern_rules);
      set_num_regex(sizeof(pattern_rules) / sizeof(pattern_rules[0]));
      break;
    default:
      assert(0);
  }
  init_mode();
}

bool Regex::make_tokens(char* buff) {
  if (buff == nullptr) return false;
  trim_nr(buff);

  int i;
  int position = 0;
  regmatch_t pos_match;  // position of match

  while (buff[position] != '\0') {
    // Try all runles one by one
    for (i = 0; i < _num_regex; ++i) {
      if (regexec(&_regexs[i], buff + position, 1, &pos_match, 0) == 0 &&
          pos_match.rm_so == 0) {
        char* sub_str_star = buff + position;
        position += pos_match.rm_eo;

        switch (_rules[i].token_type) {
          case kRegex_NULL:
            PANIC("\"%s\" should not match kRegex_NULL",
                  std::string(buff + position, pos_match.rm_so).c_str());
            break;
          case kSPACE:
            break;
          case kTAG_BEG:
          case kTAG_END:
            PANIC("Unknown tag: %s",
                  std::string(sub_str_star, pos_match.rm_eo).c_str());
            break;
          default:
            _tokens.push_back({std::string(sub_str_star, pos_match.rm_eo),
                               _rules[i].token_type});
            break;
        }

        break;
      }
    }

    if (i == _num_regex) {
      printf("no match at position %d\n%s\n%*.s^\n", position, buff, position,
             "");
      return false;
    }
  }

  return true;
}

}  // namespace EDA_CHALLENGE_Q4