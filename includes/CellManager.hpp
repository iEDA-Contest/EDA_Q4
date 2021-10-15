#ifndef __CELL_HPP_
#define __CELL_HPP_

#include <stdint.h>

#include <vector>

#include "ConfigManager.hpp"
#include "Point.hpp"

namespace EDA_CHALLENGE_Q4 {

class Cell {
 public:
  // constructor
  Cell() = default;
  ~Cell() = default;

  // getter
  auto get_id() const { return _id; }
  auto get_positon() const { return _c1; }
  auto get_rotation() const { return _rotation; }
  auto get_width() const { return _width; }
  auto get_height() const { return _height; }

  // setter
  void set_id(uint16_t id) { _id = id; }
  void set_positon(int x, int y) { _c1 = Point(x, y); }
  void set_rotation(uint16_t r) { _rotation = r; }
  void set_width(uint16_t width) { _width = width; }
  void set_height(uint16_t height) { _height = height; }

  // function

 private:
  // members
  uint16_t _id;    // _id = (id_base * cell_type << 8) + cell_list No.
  Point _c1;       // left bottom coordinate of the rectangle
  bool _rotation;  // 90 degree or not
  uint16_t _width;
  uint16_t _height;
};

class CellManager {
 public:
  // constructor
  CellManager(ConfigManager*);
  ~CellManager();

  // getter
  auto get_cell(uint16_t);

  // setter

  // function
  void init_configs(std::vector<Config>);
  void insert_cell(Cell*);

 private:
  // members
  std::vector<Cell> _cell_list;
};

// Cell

// CellManager
inline void CellManager::insert_cell(Cell* cell) {
  assert(cell != nullptr);
  _cell_list.push_back(*cell);
}

}  // namespace EDA_CHALLENGE_Q4
#endif