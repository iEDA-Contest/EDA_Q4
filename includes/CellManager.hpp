#ifndef __CELL_HPP_
#define __CELL_HPP_

#include <stdint.h>

#include <algorithm>
#include <vector>

#include "ConfigManager.hpp"
#include "Rectangle.hpp"

namespace EDA_CHALLENGE_Q4 {

enum CellPriority {
  kPrioMem,
  kPrioSoc,
  kRange,
  kRatioMAX,
  kDeathCol,
  kDeathRow,
  kPrioNull
};

class Cell {
 public:
  // constructor
  Cell() = default;
  Cell(const Cell&);
  ~Cell() = default;

  // getter
  auto get_c1() const { return _c1; }
  auto get_c3() const { return Point(_c1._x + _width, _c1._y + _height); }
  auto get_rotation() const { return _rotation; }
  auto get_width() const { return _width; }
  auto get_height() const { return _height; }
  auto get_max_edge() const { return _height > _width ? _height : _width; }
  auto get_min_edge() const { return _height < _width ? _height : _width; }
  auto get_ratioWH() const { return 1.0 * _width / _height; }
  float get_ratioWH_max();
  auto get_refer() const { return _refer; }
  int get_area() const { return _width * _height; }
  auto get_x() const { return _c1._x; }
  auto get_y() const { return _c1._y; }
  auto get_node_id() const { return _node_id; }
  auto get_x_range() const { return _x_range; }
  auto get_y_range() const { return _y_range; }
  float get_flex_x() const;
  float get_flex_y() const;

  // setter
  void set_positon(int x, int y) { _c1._x = x, _c1._y = y; }
  void set_rotation(uint16_t r) { _rotation = r; }
  void set_width(uint16_t width) { _width = width; }
  void set_height(uint16_t height) { _height = height; }
  void set_refer(std::string refer) { _refer = refer; }
  void set_x(int x) { _c1._x = x; }
  void set_y(int y) { _c1._y = y; }
  void set_node_id(uint8_t id) { _node_id = id; }
  void set_x_range(Point range) { _x_range = range; }
  void set_y_range(Point range) { _y_range = range; }
  
  // function
  void rotate();

 private:
  // members
  Point _c1;          // left bottom coordinate of the rectangle
  bool _rotation;     // 90 degree or not
  uint16_t _width;
  uint16_t _height;
  std::string _refer;
  uint8_t _node_id;   // this should not be 0, as 0 is end node
  Point _x_range;      // if c1 in this range, then x meets constraint
  Point _y_range;      // if c1 in this range, then y meets constraint
};

class CellManager {
 public:
  // constructor
  CellManager(ConfigManager*);
  CellManager(const CellManager&);
  ~CellManager();

  // getter
  auto get_mems() const { return _mems; }
  auto get_mems_num() const { return _mems.size(); }
  auto get_socs() const { return _socs; }
  auto get_socs_num() const { return _socs.size(); }

  // setter

  // function
  void insert_cell(CellType, Cell*);
  std::vector<Cell*> choose_cells(CellPriority, ...);
  void delete_cell(CellType, Cell*);

 private:
  // getter
  std::vector<Cell*>* get_list(CellType);
  std::vector<Cell*> get_min_area_sorted_chose_cells(std::map<Cell*, int>&);

  // function
  void init_cells(CellType, std::vector<Config*>);
  std::vector<Cell*> choose_range(CellType, uint16_t, uint16_t, uint16_t,
                                  uint16_t);
  std::vector<Cell*> choose_ratioWH_max(CellType);
  std::vector<Cell*> choose_death_y(CellType, Rectangle&, int, int);
  std::vector<Cell*> choose_death_x(CellType, Rectangle&, int, int);

  // static
  static bool cmp_ratioWH_max(Cell* a, Cell* b);

  // members
  std::vector<Cell*> _mems;
  std::vector<Cell*> _socs;
};

// Cell
inline Cell::Cell(const Cell& c) {
  _c1 = c.get_c1();
  _rotation = c.get_rotation();
  _width = c.get_width();
  _height = c.get_height();
  _refer = c.get_refer();
  _node_id = c.get_node_id();
}

/* rotate the cell and exchange width with height*/
inline void Cell::rotate() {
  _rotation = !_rotation;
  _width = _width ^ _height;
  _height = _width ^ _height;
  _width = _width ^ _height;
}

inline float Cell::get_ratioWH_max() {
  float r1 = 1.0 * _width / _height;
  float r2 = 1.0 * _height / _width;
  return r1 > r2 ? r1 : r2;
}

inline float Cell::get_flex_x() const {
  return 1 - (
              (_c1._x - _x_range._x) / 
              (_x_range._y - _x_range._x)
            ); 
}

inline float Cell::get_flex_y() const {
  return 1 - (
              (_c1._y - _y_range._x) / 
              (_y_range._y - _y_range._x)
            ); 
}

// CellManager
inline void CellManager::insert_cell(CellType type, Cell* cell) {
  assert(cell != nullptr);
  auto list = get_list(type);
  if (list) {
    list->push_back(cell);
  }
}

inline CellManager::CellManager(const CellManager& man) {
  size_t num = 0;
  _mems.resize(man.get_mems_num());
  for (auto c : man.get_mems()) {
    _mems[num++] = new Cell(*c);
  }

  num = 0;
  _socs.resize(man.get_socs_num());
  for (auto c : man.get_socs()) {
    _socs[num++] = new Cell(*c);
  }
}

inline CellManager::~CellManager() {
  for (auto m : _mems) {
    if (m) {
      delete m;
      m = nullptr;
    }
  }
  _mems.clear();

  for (auto s : _socs) {
    if (s) {
      delete s;
      s = nullptr;
    }
  }
  _socs.clear();
}

inline std::vector<Cell*>* CellManager::get_list(CellType type) {
  std::vector<Cell*>* op_list = nullptr;
  switch (type) {
    case kCellTypeMem:
      op_list = &_mems;
      break;
    case kCellTypeSoc:
      op_list = &_socs;
      break;
    default:
      PANIC("Illegal cell_type = %d", type);
      break;
  }
  return op_list;
}

inline std::vector<Cell*> CellManager::choose_ratioWH_max(CellType obj_type) {
  std::vector<Cell*>* op_list = get_list(obj_type);
  assert(op_list != nullptr);

  std::vector<Cell*> result = *op_list;
  std::sort(result.begin(), result.end(), cmp_ratioWH_max);
  return result;
}

inline bool CellManager::cmp_ratioWH_max(Cell* a, Cell* b) {
  if (a == nullptr || b == nullptr) return false;
  if (a->get_ratioWH_max() > b->get_ratioWH_max()) {
    return true;
  } else if (a->get_ratioWH_max() == b->get_ratioWH_max()) {
    return a->get_max_edge() > b->get_max_edge() ? true : false;
  } else {
    return false;
  }
}

}  // namespace EDA_CHALLENGE_Q4
#endif