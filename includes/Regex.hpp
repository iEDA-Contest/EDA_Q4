#ifndef __REGEX_HPP_
#define __REGEX_HPP_

#include <assert.h>
#include <regex.h>
#include <string.h>
#include <sys/types.h>

#include <string>
#include <utility>
#include <vector>

#include "Debug.h"

namespace EDA_CHALLENGE_Q4 {

enum RegexType {
  TK_NULL,
  TK_XML_HEAD,
  TK_SPACE,
  TK_TAG_BEG,
  TK_TAG_END,
  TK_TAG_DATA,
};

/* We use the POSIX regex functions to process regular expressions.*/
static struct RegexRule {
  std::string regex;
  RegexType token_type;
} rules[] = {
    {" +", TK_SPACE},
    {"<\\?[a-z0-9 =\".]+\\?>", TK_XML_HEAD},
    {"<([^<\\?/]*)>", TK_TAG_BEG},
    {"</([^<\\?/]*)>", TK_TAG_END},
    {"\\b([^<>]|&)+", TK_TAG_DATA},
};

class Regex {
 public:
  // constructor
  Regex(Regex&) = delete;

  // getter
  static Regex& getInstance();

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
  std::vector<std::pair<std::string, RegexType>> _tokens;
};

inline Regex& Regex::getInstance() {
  static Regex singleton;

  char error_msg[128];
  for (int i = 0, ret; i < _num_regex; ++i) {
    ret = regcomp(&singleton._regexs[i], rules[i].regex.c_str(),
                  REG_EXTENDED | REG_NEWLINE);
    if (ret != 0) {
      regerror(ret, &singleton._regexs[i], error_msg, sizeof(error_msg));
      panic("regex compilation failed: %s\n%s", error_msg,
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