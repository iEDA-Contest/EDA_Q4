#ifndef __REGEX_HPP_
#define __REGEX_HPP_

#include <regex.h>
#include <string.h>
#include <sys/types.h>

#include <string>
#include <vector>

#include "Debug.h"

namespace EDA_CHALLENGE_Q4 {

enum RegexType {
  kNULL,
  kSPACE,

  kTAG_XML_HEAD,
  kTAG_DATA_BEG,
  kTAG_DATA_END,

  // config tag beg //
  kTAG_MEM_BEG,
  kTAG_SOC_BEG,
  kTAG_REFERENCE_BEG,
  kTAG_AMOUNT_BEG,
  kTAG_WIDTH_BEG,
  kTAG_HEIGHT_BEG,
  kTAG_REFERENCE_END,
  kTAG_AMOUNT_END,
  kTAG_WIDTH_END,
  kTAG_HEIGHT_END,
  kTAG_SOC_END,
  kTAG_MEM_END,
  // config tag end //

  // constraint tag beg //
  kTAG_CONSTRAINT_BEG,
  kTAG_PATTERN_BEG,
  kTAG_XMM_BEG,
  kTAG_YMM_BEG,
  kTAG_XSS_BEG,
  kTAG_YSS_BEG,
  kTAG_XMS_BEG,
  kTAG_YMS_BEG,
  kTAG_XMI_BEG,
  kTAG_YMI_BEG,
  kTAG_XSI_BEG,
  kTAG_YSI_BEG,
  kTAG_CONSTRAINT_END,
  kTAG_PATTERN_END,
  kTAG_XMM_END,
  kTAG_YMM_END,
  kTAG_XSS_END,
  kTAG_YSS_END,
  kTAG_XMS_END,
  kTAG_YMS_END,
  kTAG_XMI_END,
  kTAG_YMI_END,
  kTAG_XSI_END,
  kTAG_YSI_END,
  // constraint tag end //

  kDATA,

  // unknown tag beg
  kTAG_BEG,
  kTAG_END,
  // unknown tag end
};

typedef std::vector<std::pair<std::string, RegexType>> Token_List;

/* We use the POSIX regex functions to process regular expressions.*/
static struct RegexRule {
  std::string regex;
  RegexType token_type;
} rules[] = {
    {" +", kSPACE},
    // xml head
    {"<\\?[a-z0-9 =\".]+\\?>", kTAG_XML_HEAD},
    {"<data>", kTAG_DATA_BEG},
    {"</data>", kTAG_DATA_END},
    // config tag beg
    {"<MEM>", kTAG_MEM_BEG},
    {"<SOC>", kTAG_SOC_BEG},
    {"<REFERENCE>", kTAG_REFERENCE_BEG},
    {"<AMOUNT>", kTAG_AMOUNT_BEG},
    {"<WIDTH>", kTAG_WIDTH_BEG},
    {"<HEIGHT>", kTAG_HEIGHT_BEG},
    {"</REFERENCE>", kTAG_REFERENCE_END},
    {"</AMOUNT>", kTAG_AMOUNT_END},
    {"</WIDTH>", kTAG_WIDTH_END},
    {"</HEIGHT>", kTAG_HEIGHT_END},
    {"</SOC>", kTAG_SOC_END},
    {"</MEM>", kTAG_MEM_END},
    // config tag end
    // constraint tag beg
    {"<CONSTRAINT>", kTAG_CONSTRAINT_BEG},
    {"<PATTERN>", kTAG_PATTERN_BEG},
    {"<SPACING_X_MEM_MEM>", kTAG_XMM_BEG},
    {"<SPACING_Y_MEM_MEM>", kTAG_YMM_BEG},
    {"<SPACING_X_SOC_SOC>", kTAG_XSS_BEG},
    {"<SPACING_Y_SOC_SOC>", kTAG_YSS_BEG},
    {"<SPACING_X_MEM_SOC>", kTAG_XMS_BEG},
    {"<SPACING_Y_MEM_SOC>", kTAG_YMS_BEG},
    {"<SPACING_X_MEM_INTERPOSER>", kTAG_XMI_BEG},
    {"<SPACING_Y_MEM_INTERPOSER>", kTAG_YMI_BEG},
    {"<SPACING_X_SOC_INTERPOSER>", kTAG_XSI_BEG},
    {"<SPACING_Y_SOC_INTERPOSER>", kTAG_YSI_BEG},
    {"</CONSTRAINT>", kTAG_CONSTRAINT_END},
    {"</PATTERN>", kTAG_PATTERN_END},
    {"</SPACING_X_MEM_MEM>", kTAG_XMM_END},
    {"</SPACING_Y_MEM_MEM>", kTAG_YMM_END},
    {"</SPACING_X_SOC_SOC>", kTAG_XSS_END},
    {"</SPACING_Y_SOC_SOC>", kTAG_YSS_END},
    {"</SPACING_X_MEM_SOC>", kTAG_XMS_END},
    {"</SPACING_Y_MEM_SOC>", kTAG_YMS_END},
    {"</SPACING_X_MEM_INTERPOSER>", kTAG_XMI_END},
    {"</SPACING_Y_MEM_INTERPOSER>", kTAG_YMI_END},
    {"</SPACING_X_SOC_INTERPOSER>", kTAG_XSI_END},
    {"</SPACING_Y_SOC_INTERPOSER>", kTAG_YSI_END},
    // constraint tag end

    {"\\b([^<>]|&)+", kDATA},

    {"<([^<\\?/]*)>", kTAG_BEG},
    {"</([^<\\?/]*)>", kTAG_END},
};

class Regex {
 public:
  // constructor
  Regex(Regex&) = delete;

  // getter
  static Regex& getInstance();
  Token_List& get_tokens() { return _tokens; }

  // setter

  // function
  bool make_tokens(char*);
  void reset_tokens();

 private:
  // constructor
  Regex() = default;
  ~Regex();

  // function
  void trim_nr(char*);

  // members
  static constexpr int _num_regex = sizeof(rules) / sizeof(rules[0]);
  regex_t _regexs[_num_regex];
  Token_List _tokens;
};

inline Regex& Regex::getInstance() {
  static Regex singleton;

  char error_msg[128];
  for (int i = 0, ret; i < _num_regex; ++i) {
    ret = regcomp(&singleton._regexs[i], rules[i].regex.c_str(),
                  REG_EXTENDED | REG_NEWLINE);
    if (ret != 0) {
      regerror(ret, &singleton._regexs[i], error_msg, sizeof(error_msg));
      PANIC("regex compilation failed: %s\n%s", error_msg,
            rules[i].regex.c_str());
    }
  }

  return singleton;
}

inline Regex::~Regex() {
  for (int i = 0; i < _num_regex; ++i) {
    regfree(_regexs + i);
  }
}

/**
 * @brief Because it seems impossible to match '\\n' and '\\r', Remove these
 * characters
 *
 *
 * @param str string to match
 */
inline void Regex::trim_nr(char* str) {
  char* p_end = str + strlen(str) - 1;
  for (; p_end >= str; --p_end) {
    if (*p_end == '\r' || *p_end == '\n') {
      *p_end = '\0';
    } else {
      break;
    }
  }
}

inline void Regex::reset_tokens() { _tokens.resize(0); }

}  // namespace EDA_CHALLENGE_Q4
#endif