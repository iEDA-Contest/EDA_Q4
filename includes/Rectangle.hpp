#ifndef __RECTANGLE_HPP_
#define __RECTANGLE_HPP_

#include "Point.hpp"
#include "cmath"

namespace EDA_CHALLENGE_Q4 { 

class Rectangle {
 public:
  // constructor
  Rectangle() = default;
  Rectangle(Point, Point);
  Rectangle(const Rectangle&);

  // getter
  auto get_width() { return std::abs(_c1._x - _c3._x); }
  auto get_height() { return std::abs(_c1._y - _c3._y); }
  auto get_area() { return get_width() * get_height(); }

  // setter

  // operator
  bool operator==(const Rectangle&);
  Rectangle& operator=(const Rectangle&);

  // function
  void reset();

  // members
  Point _c1; // left bottom
  Point _c3; // right top
};

inline Rectangle::Rectangle(Point c1, Point c3){
  _c1 = c1;
  _c3 = c3;
}

inline Rectangle::Rectangle(const Rectangle& r) {
    _c1 = r._c1;
    _c3 = r._c3;
}

inline bool Rectangle::operator==(const Rectangle& r) {
  return _c1 == r._c1 && _c3 == r._c3;
}

inline Rectangle& Rectangle::operator=(const Rectangle& r) {
  _c1 = r._c1;
  _c3 = r._c3;
  return *this;
}

inline void Rectangle::reset() {
  _c1._x = 0;
  _c1._y = 0;
  _c3._x = 0;
  _c3._y = 0;
}

} // namespace EDA_CHALLENGE_Q4
#endif