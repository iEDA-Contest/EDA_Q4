#ifndef __CELL_HPP_
#define __CELL_HPP_

#include <stdint.h>

#include "Point.hpp"

namespace EDA_CHALLENGE_Q4 {

class Cell {
 protected:
  uint32_t _reference;
  Point _c1;  // left bottom coordinate of the rectangle
  Point _c3;  // right top coordinate of the rectangle
};

}  // namespace EDA_CHALLENGE_Q4
#endif