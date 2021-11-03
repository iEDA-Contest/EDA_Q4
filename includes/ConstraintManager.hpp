#ifndef _CONSTRAINT_MANAGER_HPP
#define _CONSTRAINT_MANAGER_HPP

#include <stdint.h>

#include <map>
#include <utility>

#include "Debug.h"
#include "Regex.hpp"

namespace EDA_CHALLENGE_Q4 {

enum ConstraintType {
  // min
  kXMM_MIN,
  kYMM_MIN,
  kXSS_MIN,
  kYSS_MIN,
  kXMS_MIN,
  kYMS_MIN,
  kXMI_MIN,
  kYMI_MIN,
  kXSI_MIN,
  kYSI_MIN,
  // max
  kXMM_MAX,
  kYMM_MAX,
  kXSS_MAX,
  kYSS_MAX,
  kXMS_MAX,
  kYMS_MAX,
  kXMI_MAX,
  kYMI_MAX,
  kXSI_MAX,
  kYSI_MAX,
  // to indicate enum total
  kTotal,
};

class Constraint {
 public:
  // constructor
  Constraint() = default;
  ~Constraint() = default;

  // getter
  auto get_constraint(ConstraintType) const;
  auto get_pattern() { return _pattern; }

  // setter
  void set_constraint(ConstraintType, uint16_t);
  void set_constraint(RegexType, const char*);
  void set_pattern(std::string pattern) { _pattern = pattern; }

  // function

 private:
  // members
  std::string _pattern;
  uint16_t _constraint[kTotal];
};

class ConstraintManager {
 public:
  // constructor
  ConstraintManager(Token_List&);
  ~ConstraintManager();

  // getter
  auto get_pattern_list() const { return _pattern_list; }

  // setter

  // function

 private:
  std::vector<Constraint*> _pattern_list;
};

// Constraint

inline auto Constraint::get_constraint(ConstraintType type) const {
  ASSERT(0 <= type && type < kTotal, "Unknow constriant id:%d", type);
  return _constraint[type];
}

inline void Constraint::set_constraint(ConstraintType type, uint16_t data) {
  ASSERT(0 <= type && type < kTotal, "Unknow constriant id:%d", type);
  _constraint[type] = data;
}

// ConstraintManager
inline ConstraintManager::~ConstraintManager() {
  for (auto p : _pattern_list) {
    if (p) {
      delete p;
      p = nullptr;
    }
  }
  _pattern_list.clear();
}

}  // namespace EDA_CHALLENGE_Q4
#endif