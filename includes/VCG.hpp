#ifndef __VCG_HPP_
#define __VCG_HPP_

#include "CellManager.hpp"
#include "Debug.h"
#include "Regex.hpp"

namespace EDA_CHALLENGE_Q4 {

enum VCGNodeType { kVCG_START, kVCG_MEM, kVCG_SOC, kVCG_END };
typedef std::vector<std::vector<uint8_t>> GridType;

class VCGNode {
 public:
  // constructor
  VCGNode(VCGNodeType);
  VCGNode(VCGNodeType, uint8_t);
  ~VCGNode();

  // getter
  auto get_id() const { return _id; }
  auto get_type() const { return _type; }
  auto get_v_placeholder() const { return _v_placeholder; }
  auto get_h_placeholder() const { return _h_placeholder; }
  auto get_cell() const { return _cell; }
  auto get_froms_num() const {return _froms.size(); }
  auto get_tos_num() const {return _tos.size(); }
  VCGNode* get_from(size_t n = 1);
  VCGNode* get_to(size_t n = 1);
  std::vector<VCGNode*>& get_froms() { return _froms; }
  std::vector<VCGNode*>& get_tos() { return _tos; }

  // setter
  void set_id(uint8_t id) { _id = id; }
  void set_cell(Cell* cell) { _cell = cell; }
  void set_cell_null();

  // function
  void inc_v_placeholder() { ++_v_placeholder; }
  void inc_h_placeholder() { ++_h_placeholder; }
  void insert_from(VCGNode*);
  void insert_last_from(VCGNode*);
  void insert_start(VCGNode*);
  void insert_to(VCGNode*);
  void show_froms();
  void show_tos();

 private:
  // members
  uint8_t _id;                   // VCG Node id
  VCGNodeType _type;             // VCG Node type
  uint8_t _v_placeholder;        // if "^" occur
  uint8_t _h_placeholder;        // if "<" occur
  std::vector<VCGNode*> _froms;  // in column order, top-down
  std::vector<VCGNode*> _tos;    // order in shortest path to END, bottom-up
  Cell* _cell;                   // not recommend to destory _cell here
};

class VCG {
 public:
  // constructor
  VCG(Token_List&);
  ~VCG();

  // getter
  auto get_vertex_num() const { return _adj_list.size(); }
  std::vector<Cell*> get_cells(CellPriority, uint8_t, ...);
  VCGNode* get_cell(uint8_t);
  CellType get_cell_type(uint8_t);

  // setter
  void set_cell_man(CellManager*);

  // function
  void do_place_cell(uint8_t, Cell*);
  void undo_place_cell(uint8_t);
  bool is_id_valid(uint8_t);
  void show_topology();
  void show_froms_tos();
  void show_id_grid();
  void find_best_place();

 private:
  // getter
  size_t get_max_row(GridType&);
  std::vector<GridType> get_smaller_module(GridType&);
  std::vector<GridType> get_smaller_module_column(GridType&);
  std::vector<GridType> get_smaller_module_row(GridType&);

  // setter
  void set_id_grid(size_t, size_t, uint8_t);

  // function
  void build_tos();
  void debug();
  std::vector<GridType> slice();

  // members
  std::vector<VCGNode*> _adj_list;  // Node0 is end, final Node is start
  GridType _id_grid;                // [column][row]
  CellManager* _cell_man;
};

// VCGNode
inline VCGNode::VCGNode(VCGNodeType type)
    : _id(0), _v_placeholder(1), _h_placeholder(1), _cell(nullptr) {
  _type = type;
}

inline VCGNode::VCGNode(VCGNodeType type, uint8_t id)
    : _id(id), _v_placeholder(1), _h_placeholder(1), _cell(nullptr) {
  _type = type;
}

// n is a natural
inline VCGNode* VCGNode::get_from(size_t n) {
  return 0 < n && n <= _froms.size() ? _froms[n - 1] : nullptr;
}

inline VCGNode::~VCGNode() {
  _froms.clear();
  _tos.clear();
  _cell = nullptr;
}

inline void VCGNode::insert_from(VCGNode* from) {
  if (from) {
    _froms.push_back(from);
  }
}

/**
 * @brief insert start point of VCG
 *
 *
 *
 * @param start
 */
inline void VCGNode::insert_start(VCGNode* start) {
  if (start && _froms.size() == 0) {
    _froms.push_back(start);
  }
}

/**
 * @brief insert node into the end of _froms
 *
 *
 *
 * @param from
 */
inline void VCGNode::insert_last_from(VCGNode* from) {
  if (from) {
    if (_froms.size() == 0) {
      _froms.push_back(from);
    } else if (_froms[_froms.size() - 1] != from) {
      _froms.push_back(from);
    }
  }
}

inline void VCGNode::insert_to(VCGNode* to) {
  if (to) {
    _tos.push_back(to);
  }
}

inline void VCGNode::show_froms() {
  printf("froms[%d]:", get_id());
  for (auto from : _froms) {
    printf("%2d, ", from->get_id());
  }
  printf("\n");
}

inline void VCGNode::show_tos() {
  printf("tos[%d]:", get_id());
  for (auto to : _tos) {
    printf("%2d, ", to->get_id());
  }
  printf("\n");
}

/*Not release memory, please manage it manly!*/
inline void VCGNode::set_cell_null() {
  _cell = nullptr;
}

inline VCGNode* VCGNode::get_to(size_t n) {
  return 0 < n && n <= _tos.size() ? _tos[n - 1] : nullptr;
}

// VCG

inline void VCG::set_id_grid(size_t column, size_t row, uint8_t id) {
  if (column == _id_grid.size() + 1 && row == 1) {
    _id_grid.resize(_id_grid.size() + 1);
    _id_grid[column - 1].push_back(id);
  } else if (column == _id_grid.size() &&
             row == _id_grid[column - 1].size() + 1) {
    _id_grid[column - 1].push_back(id);
  }
}

inline void VCG::debug() { 
  show_topology(); 
}

inline void VCG::set_cell_man(CellManager* cm) {
  if (_cell_man == nullptr && cm != nullptr) {
    _cell_man = new CellManager(*cm);
  }
}

inline VCGNode* VCG::get_cell(uint8_t id) {
  return id < _adj_list.size() ? _adj_list[id] : nullptr;
}

inline CellType VCG::get_cell_type(uint8_t id) {
  VCGNode* p = get_cell(id);
  if (p) {
    switch (p->get_type()) {
      case kVCG_MEM:
        return kCellTypeMem;
      case kVCG_SOC:
        return kCellTypeSoc;
      default:
        return kCellTypeNull;
    }
  } else {
    return kCellTypeNull;
  }
}

inline bool  VCG::is_id_valid(uint8_t id) { return id < _adj_list.size(); }

inline size_t VCG::get_max_row(GridType& grid) {
  size_t max_row = 0;
  for (auto column : _id_grid) {
    max_row = max_row == 0 || max_row < column.size() ? column.size() : max_row;
  }
  return max_row;
}

}  // namespace EDA_CHALLENGE_Q4
#endif