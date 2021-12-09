#ifndef __REGEX_HPP_
#define __REGEX_HPP_

#include <regex.h>
#include <string.h>
#include <sys/types.h>

#include <string>

#include "Debug.h"

namespace EDA_CHALLENGE_Q4 {

enum RegexMode { kMODE_NULL, kXML, kPATTERN };

enum RegexType {
  kRegex_NULL,
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

  // pattern beg
  kColumn,
  kHORIZONTAL,
  kVERTICAL,
  kSOC,
  kMEM,
  // pattern end
};

typedef std::vector<std::pair<std::string, RegexType>> Token_List;

/* We use the POSIX regex functions to process regular expressions.*/
static struct RegexRule {
  std::string regex;
  RegexType token_type;
} xml_rules[] =
    {
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
},
  pattern_rules[] = {
      {" +", kSPACE}, {"\\^", kVERTICAL}, {"&#60;", kHORIZONTAL},
      {"S", kSOC},    {"M", kMEM},        {"\\|", kColumn},
};

class Regex {
 public:
  // constructor
  Regex(RegexMode);
  ~Regex();

  // getter
  auto get_num_regex() const { return _num_regex; }
  auto get_regexs() const { return _regexs; }
  Token_List& get_tokens() { return _tokens; }
  auto get_rules() const { return _rules; }
  auto get_mode() const { return _mode; }

  // setter
  void set_num_regex(int num) { _num_regex = num; }
  void set_rules(RegexRule* rules) { _rules = rules; }
  void set_mode(RegexMode mode) { _mode = mode; }

  // function
  bool make_tokens(char*);
  void reset_tokens();

 private:
  // function
  void trim_nr(char*);
  void init_mode();

  // members
  int _num_regex;
  regex_t* _regexs;    // regexs compiled successfully
  Token_List _tokens;  // tokens that matched by regexs
  RegexRule* _rules;   // compiling rules
  RegexMode _mode;
};

inline void Regex::init_mode() {
  ASSERT(_mode != kMODE_NULL, "Regex Mode should not be null!");

  char error_msg[128];
  _regexs = (regex_t*)malloc(sizeof(regex_t) * _num_regex);
  for (int i = 0, ret; i < _num_regex; ++i) {
    ret = regcomp(&_regexs[i], _rules[i].regex.c_str(),
                  REG_EXTENDED | REG_NEWLINE);
    if (ret != 0) {
      regerror(ret, &_regexs[i], error_msg, sizeof(error_msg));
      PANIC("regex compilation failed: %s\n%s", error_msg,
            _rules[i].regex.c_str());
    }
  }
}

inline Regex::~Regex() {
  for (int i = 0; i < _num_regex; ++i) {
    regfree(_regexs + i);
  }
  free(_regexs);
  _regexs = nullptr;
  
  _tokens.clear();
  _rules = nullptr;
  _num_regex = 0;
  _mode = kMODE_NULL;
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