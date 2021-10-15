#ifndef __POINT_HPP_
#define __POINT_HPP_

namespace EDA_CHALLENGE_Q4 {
class Point {
 public:
  // constructor
  Point() = default;
  Point(int, int);
  ~Point() = default;

  // getter
  auto get_x() const { return _x; }
  auto get_y() const { return _y; }
  // setter

  // operator
  Point& operator=(const Point& p);

  // function

 private:
  // members
  int _x;
  int _y;
};

inline Point::Point(int x, int y) : _x(x), _y(y) {}
inline Point& Point::operator=(const Point& p) {
  _x = p.get_x();
  _y = p.get_y();
  return *this;
}

}  // namespace EDA_CHALLENGE_Q4
#endif