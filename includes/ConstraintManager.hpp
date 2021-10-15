#ifndef _CONSTRAINT_MANAGER_HPP
#define _CONSTRAINT_MANAGER_HPP

#include <stdint.h>

#include <map>
#include <utility>
#include <vector>

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
  auto get_id_pattern() const { return _id_pattern; }
  auto get_constraint(ConstraintType);

  // setter
  void set_id_pattern(uint16_t id_pattern) { _id_pattern = id_pattern; }
  void set_constraint(ConstraintType, uint16_t);
  void set_constraint(RegexType, const char*);

  // function

 private:
  // members
  uint8_t _id_pattern;
  uint16_t _constraint[kTotal];
};

class ConstraintManager {
 public:
  ConstraintManager(Token_List&);
  ~ConstraintManager() = default;

 private:
  std::vector<Constraint> _pattern_list;
  std::map<uint8_t, std::string> _id_pattern_map;
};

// Constraint

inline auto Constraint::get_constraint(ConstraintType type) {
  ASSERT(0 <= type && type < kTotal, "Unknow constriant id:%d", type);
  return _constraint[type];
}

inline void Constraint::set_constraint(ConstraintType type, uint16_t data) {
  ASSERT(0 <= type && type < kTotal, "Unknow constriant id:%d", type);
  _constraint[type] = data;
}

// ConstraintManager

}  // namespace EDA_CHALLENGE_Q4
#endif