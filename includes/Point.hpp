#ifndef __POINT_HPP_
#define __POINT_HPP_

namespace EDA_CHALLENGE_Q4 {
struct Point {
 public:
  // constructor
  Point() = default;
  Point(int x, int y);

  // operator
  Point& operator=(const Point&); 
  bool operator==(const Point&);

  // members
  int _x;
  int _y;
};

inline Point::Point(int x, int y) : _x(x), _y(y) {}

inline Point& Point::operator=(const Point& p) {
  _x = p._x;
  _y = p._y;
  return *this;
}

inline bool Point::operator==(const Point& p) {
  return _x == p._x && _y == p._y;
}

}  // namespace EDA_CHALLENGE_Q4
#endif