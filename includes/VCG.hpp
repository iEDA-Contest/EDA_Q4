#ifndef __VCG_HPP_
#define __VCG_HPP_

#include <list>
#include <queue>
#include <set>

#include "CellManager.hpp"
#include "ConstraintManager.hpp"
#include "Debug.h"
#include "Regex.hpp"

namespace EDA_CHALLENGE_Q4 {

enum ModuleType { kMoColumn, kMoRow, kMoWheel, kMoSingle };

class ModuleHelper;

class MergeBox {
  public:
  // constructor
  MergeBox();
  ~MergeBox();

  // members
  MergeBox* _smaller;
  Rectangle* _box;
  ModuleHelper* _pre;
};

/**
 * @brief pick recommendation
 */
class ModuleHelper {
 public:
  // constructor
  ModuleHelper();
  ~ModuleHelper();

  // getter
  auto get_merge() const { return _merge; }

  // setter
  void set_merge_null() { _merge = nullptr; }
  void set_box_init() { _box->_c1 = {0, 0}; _box->_c3 = {0, 0}; }

  // function
  void insert_merge(MergeBox*);


  // members
  GridType _module;
  uint8_t _biggest;     // may be the biggest cell in this module
  ModuleType _type;     // indicate relative position between cells
  uint8_t _cell_num;    // cell num should be picked
  Rectangle* _box;      // bounding box of this module
  uint8_t _first_place; // cell place order topological    

 private:
  MergeBox* _merge;
};

enum VCGNodeType { kVCG_START, kVCG_MEM, kVCG_SOC, kVCG_END };

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
  auto get_froms_num() const { return _froms.size(); }
  auto get_tos_num() const { return _tos.size(); }
  VCGNode* get_from(size_t n = 1);
  VCGNode* get_to(size_t n = 1);
  std::vector<VCGNode*>& get_froms() { return _froms; }
  std::vector<VCGNode*>& get_tos() { return _tos; }
  float get_ratioHV_max();
  auto get_ratioHV() const { return 1.0 * _h_placeholder / _v_placeholder; }
  VCGNode* get_min_from();
  auto get_max_column_index() const { return _max_column_index; }
  auto get_column_index() const { return  _max_column_index - _h_placeholder + 1; }
  auto get_max_row_index() const { return _max_row_index; }
  auto get_row_index() const { return _max_row_index - _v_placeholder + 1; }

  // setter
  void set_id(uint8_t id) { _id = id; }
  void set_cell(Cell* cell) { _cell = cell; }
  void set_cell_null();
  void set_max_column_index(int index) { _max_column_index = index; }
  void set_max_row_index(int index) { _max_row_index = index; }

  // function
  void inc_v_placeholder() { ++_v_placeholder; }
  void inc_h_placeholder() { ++_h_placeholder; }
  void insert_from(VCGNode*);
  void insert_last_from(VCGNode*);
  void insert_start(VCGNode*);
  void insert_to(VCGNode*);
  void show_froms();
  void show_tos();
  bool is_from_start();
  bool is_first_column();

 private:
  // static
  static bool cmp_min_node(VCGNode*, VCGNode*);

  // members
  uint8_t _id;                   // VCG Node id
  VCGNodeType _type;             // VCG Node type
  uint8_t _v_placeholder;        // if "^" occur
  uint8_t _h_placeholder;        // if "<" occur
  std::vector<VCGNode*> _froms;  // in column order, top-down
  std::vector<VCGNode*> _tos;    // order in shortest path to END, bottom-up
  Cell* _cell;                   // not recommend to destory _cell here
  int _max_column_index;         // the max column index in _id_grid. Start and End is -1
  int _max_row_index;            // the max row index in _id_grid. Start and End is -1
};

struct CmpVCGNodeTopology {
  bool operator()(VCGNode*, VCGNode*);
};

struct CmpVCGNodeMoCol {
  bool operator()(VCGNode*, VCGNode*);
};

struct CmpVCGNodeMoRow {
  bool operator()(VCGNode*, VCGNode*);
};

class VCG {
 public:
  // constructor
  VCG(Token_List&);
  ~VCG();

  // getter
  auto get_vertex_num() const { return _adj_list.size(); }
  VCGNode* get_node(uint8_t);
  Cell* get_cell(uint8_t);
  CellType get_cell_type(uint8_t);
  VCGNodeType get_node_type(uint8_t);
  CellPriority get_priority(VCGNodeType);

  // setter
  void set_cell_man(CellManager*);
  void set_constraint(Constraint*);

  // function
  void do_pick_cell(uint8_t, Cell*);
  void undo_pick_cell(uint8_t);
  bool is_id_valid(uint8_t);
  void show_topology();
  void show_froms_tos();
  void show_id_grid();
  void find_best_place();
  void gen_GDS();

 private:
  // getter
  size_t get_max_row(GridType&);
  std::vector<GridType> get_smaller_module(GridType&, bool min = false);
  std::vector<GridType> get_smaller_module_column(GridType&, bool);
  std::vector<GridType> get_smaller_module_row(GridType&, bool);
  Point get_constraint_x(uint8_t, uint8_t);
  Point get_constraint_y(uint8_t, uint8_t);
  Point get_constraint_x(uint8_t);
  Point get_constraint_y(uint8_t);
  Cell* get_cell_fits_min_ratioWH(uint8_t);
  std::vector<Cell*> get_left_cells(uint8_t);
  std::map<uint8_t, std::vector<uint8_t>> get_in_edge_list();
  std::vector<Cell*> get_colomn_cells(int);
  std::vector<Cell*> get_left_overlap_y_cells(VCGNode*);
  std::set<uint8_t> get_tos(ModuleHelper*);
  std::set<uint8_t> get_right_id_set(ModuleHelper*);
  std::set<uint8_t> get_ids_in_helper(ModuleHelper*);

  // setter
  void set_id_grid(size_t, size_t, uint8_t);
  void set_module_box(uint8_t);

  // function
  void init_tos();
  void init_column_row_index();
  void debug();
  std::vector<GridType> slice();
  std::vector<ModuleHelper*> make_pick_helpers(std::vector<GridType>&);
  bool is_first_column(uint8_t);
  bool is_overlap_y(Cell*, Cell*);
  bool is_overlap_x(Cell*, Cell*);
  void place();
  void place_module(ModuleHelper*, std::map<uint8_t, bool>&);
  void place_col(ModuleHelper*, std::map<uint8_t, bool>&);
  void place_row(ModuleHelper*, std::map<uint8_t, bool>&);
  void place_whe(ModuleHelper*, std::map<uint8_t, bool>&);
  void visit_single(VCGNode*);
  void make_helper_map(std::vector<ModuleHelper*>&);
  bool is_froms_visited(GridType&, std::map<uint8_t, bool>&);
  std::priority_queue<VCGNode*, std::vector<VCGNode*>, CmpVCGNodeMoCol> make_col_visit_queue(ModuleHelper*);
  std::priority_queue<VCGNode*, std::vector<VCGNode*>, CmpVCGNodeMoRow> make_row_visit_queue(ModuleHelper*);
  std::queue<uint8_t> make_whe_visit_queue(ModuleHelper*);
  Point cal_y_range(VCGNode*, float);
  Point cal_x_range(VCGNode*, float);
  std::map<uint8_t, std::set<uint8_t>> make_in_edge_list(GridType&);
  void try_place(VCGNode*);
  void merge_box(ModuleHelper*);
  bool is_tos_placed(ModuleHelper*);
  bool is_placed(uint8_t);
  void merge_all_tos(ModuleHelper*);
  void merge_with_tos(ModuleHelper*);
  bool is_right_module_fusible(ModuleHelper*);
  void merge_with_right(ModuleHelper*);
  void place_cells_again();
  void clear_merges();
  void set_Helpers_merge_null();
  void clear_Helper_map();
  void retrieve_all_cells();
  void do_cell_fits_node(Cell*, VCGNode*);
  void swap_cell(VCGNode*, VCGNode*);
  Point cal_c3_of_interposer();

  // static
  static bool cmp_module_priority(GridType&, GridType&);
  static size_t get_module_row(GridType&);
  static size_t get_type_num(GridType&);
  static size_t _gds_file_num;

  // members
  std::vector<VCGNode*> _adj_list;  // Node0 is end, final Node is start
  GridType _id_grid;                // [column][row]
  CellManager* _cell_man;
  Constraint* _constraint;
  std::map<uint8_t, ModuleHelper*> _helper_map;
  std::vector<uint8_t> _place_stack;
  std::list<MergeBox*> _merges;     // mainly for release
  size_t _last_merge_id;
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
  _cell = nullptr;  // must not clear _cell here
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
  _cell->set_node_id(0);
  _cell = nullptr;
}

inline VCGNode* VCGNode::get_to(size_t n) {
  return 0 < n && n <= _tos.size() ? _tos[n - 1] : nullptr;
}

inline float VCGNode::get_ratioHV_max() {
  float ret1 = 1.0 * _v_placeholder / _h_placeholder;
  float ret2 = 1.0 * _h_placeholder / _v_placeholder;
  return ret1 > ret2 ? ret1 : ret2;
}

inline bool VCGNode::is_from_start() {
  for (auto node : _froms) {
    if (node->get_type() == kVCG_START) {
      return true;
    }
    return false; // the first node should be 'start' if it exists
  }
  // 'start' has no froms
  return false;
}

inline VCGNode* VCGNode::get_min_from() {
  std::vector<VCGNode*> tmp = _froms;
  std::sort(tmp.begin(), tmp.end(), cmp_min_node);
  return tmp.size() ? tmp[0] : nullptr;
}

inline bool VCGNode::cmp_min_node(VCGNode* n1, VCGNode* n2) {
  if (n1 == nullptr || n2 == nullptr) {
    return false;
  } else {
    return n1->get_id() < n2->get_id();
  }
}

inline bool VCGNode::is_first_column() {
  return get_column_index() == 0;
}

// ModuleHelper

inline ModuleHelper::ModuleHelper()
  :_biggest(0), _type(kMoSingle), _cell_num(0) {
  _module.resize(0);
  _box = new Rectangle();
  _merge = nullptr;
}

inline ModuleHelper::~ModuleHelper()  {
  for (auto v : _module) {
    v.clear();
  }
  delete _box;
  _box = nullptr;
  _merge = nullptr; // Must Not release here
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

inline void VCG::debug() { show_topology(); }

inline void VCG::set_cell_man(CellManager* cm) {
  if (_cell_man == nullptr && cm != nullptr) {
    _cell_man = new CellManager(*cm);
  }
}

inline VCGNode* VCG::get_node(uint8_t id) {
  return id < _adj_list.size() ? _adj_list[id] : nullptr;
}

inline CellType VCG::get_cell_type(uint8_t id) {
  VCGNode* p = get_node(id);
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

inline VCGNodeType VCG::get_node_type(uint8_t id) {
  assert(is_id_valid(id));
  return _adj_list[id]->get_type();
}

inline bool VCG::is_id_valid(uint8_t id) { return id < _adj_list.size(); }

inline size_t VCG::get_max_row(GridType& grid) {
  size_t max_row = 0;
  for (auto column : grid) {
    max_row = max_row == 0 || max_row < column.size() ? column.size() : max_row;
  }
  return max_row;
}

inline size_t VCG::get_module_row(GridType& grid) {
  size_t max_row = 0;
  for (auto column : grid) {
    max_row = max_row == 0 || max_row < column.size() ? column.size() : max_row;
  }
  return max_row;
}

/**
 * @brief Get number of types that grid contains
 *
 *
 *
 * @param grid
 * @return size_t
 */
inline size_t VCG::get_type_num(GridType& grid) {
  std::set<uint8_t> set;
  for (auto column : grid) {
    for (auto row : column) {
      set.insert(row);
    }
  }

  return set.size();
}

inline void VCG::do_pick_cell(uint8_t id, Cell* cell) {
  if (is_id_valid(id) && cell != nullptr &&
      _adj_list[id]->get_cell() == nullptr) {
    //
    cell->set_node_id(id);
    _adj_list[id]->set_cell(cell);
    _cell_man->delete_cell(get_cell_type(id), cell);

    // debug
    g_log << "nodeid = " << std::to_string(id) << ", cell_id = " << cell->get_refer() << "\n";
    g_log.flush();
  }
}

inline Cell* VCG::get_cell(uint8_t id) {
  VCGNode* node = get_node(id);
  return node ? node->get_cell() : nullptr;
}

inline void VCG::set_constraint(Constraint* c) {
  if (c != nullptr && _constraint == nullptr) {
    _constraint = c;
  }
}

inline bool VCG::is_first_column(uint8_t id) {
  ASSERT(_id_grid.size(), "_id_grid is empty!");
  for (auto id_first_column : _id_grid[0]) {
    if (id_first_column == id) {
      return true;
    }
  }

  return false;
}

inline std::map<uint8_t, std::vector<uint8_t>> VCG::get_in_edge_list() {
  std::map<uint8_t, std::vector<uint8_t>> ret;
  for (auto node : _adj_list) {
    std::vector<uint8_t> ins;
    for (auto from : node->get_froms()) {
      ins.push_back(from->get_id());
    }
    ret[node->get_id()] = ins;
  }
  return ret;
}

inline bool CmpVCGNodeTopology::operator()(VCGNode* n1, VCGNode* n2) {
  if (n1 == nullptr || n2 == nullptr) return false;

  return n1->get_max_column_index() > n2->get_max_column_index();
}

inline std::vector<Cell*> VCG::get_colomn_cells(int co_id) {
  std::vector<Cell*> ret;
  if(co_id >= 0 && (size_t)co_id < _id_grid.size()) {
    std::set<uint8_t> in_list_set;
    for(auto id : _id_grid[co_id]){
      if(in_list_set.count(id) == 0) {
        auto cell = get_cell(id);
        if (cell && is_placed(cell->get_node_id())) {
          ret.push_back(cell);
          in_list_set.insert(id);
        }
      }
    }
  }

  return ret;
}

inline bool CmpVCGNodeMoCol::operator()(VCGNode* n1, VCGNode* n2) {
  if (n1 == nullptr || n2 == nullptr) return false;

  return n1->get_max_row_index() < n2->get_max_row_index();
}

inline bool CmpVCGNodeMoRow::operator()(VCGNode* n1, VCGNode* n2) {
  if (n1 == nullptr || n2 == nullptr) return false;

  return n1->get_column_index() > n2->get_column_index();
}

inline MergeBox::MergeBox(): _smaller(nullptr), _pre(nullptr) {
  _box = new Rectangle();
}

inline MergeBox::~MergeBox() {
  delete _box;
  _box = nullptr;

  // Must Not release here.
  _smaller = nullptr; 
  _pre = nullptr;     
}

inline void ModuleHelper::insert_merge(MergeBox* merge) {
  if (merge == nullptr) return;

  if (_merge == nullptr) {
    _merge = merge;
  } else {
    MergeBox* p = _merge;
    _merge = merge;
    _merge->_smaller = p;
  }
}

inline std::set<uint8_t> VCG::get_ids_in_helper(ModuleHelper* helper) {
  assert(helper);

  std::set<uint8_t> ret;
  for (auto column : helper->_module) {
    for (auto id : column) {
      ret.insert(id);
    }
  }

  return ret;
}

inline void VCG::clear_merges() {
  for (auto merge : _merges) {
    delete merge;

    // Must not release memory here
    merge->_smaller = nullptr;
    merge->_pre = nullptr;
  }
  _merges.clear();
}

/**
 * @brief Here helps to release memory of _merges
 * 
 */
inline void VCG::set_Helpers_merge_null() {
  for (auto helper : _helper_map) {
    helper.second->set_merge_null();
  }

  clear_merges();
}

/**
 * @brief  Here helps to release memory of _merges
 * 
 */
inline void VCG::clear_Helper_map() {
  set_Helpers_merge_null();
  std::set<ModuleHelper*> de_set;
  for (auto helper: _helper_map) {
    de_set.insert(helper.second);
  }
  _helper_map.clear();
  for (auto helper: de_set) {
    delete helper;
  }
  de_set.clear();
}

inline void VCG::retrieve_all_cells() {
  for (auto node : _adj_list) {
    undo_pick_cell(node->get_id());
  }
}

inline void VCG::do_cell_fits_node(Cell* cell, VCGNode* node) {
  if (!cell || !node) return;

  auto f_cell_ori = fabs(node->get_ratioHV() - cell->get_ratioWH());
  auto f_cell_rotate = fabs(node->get_ratioHV() - 1 / cell->get_ratioWH());
  if (f_cell_rotate < f_cell_ori) {
    cell->rotate();
  }
}

inline void VCG::swap_cell(VCGNode* n1, VCGNode* n2) {
  if (!n1 || !n2) return;
  auto cell_1 = n1->get_cell();
  auto cell_2 = n2->get_cell();
  undo_pick_cell(n1->get_id());
  undo_pick_cell(n2->get_id());

  do_cell_fits_node(cell_1, n2);
  do_pick_cell(n2->get_id(), cell_1);

  do_cell_fits_node(cell_2, n1);
  do_pick_cell(n1->get_id(), cell_2);
}

}  // namespace EDA_CHALLENGE_Q4
#endif