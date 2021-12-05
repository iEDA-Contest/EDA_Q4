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

class ModuleHelper;
class VCGNode;
class PickTerm;
struct CmpPickTerm;
typedef std::priority_queue<PickTerm*, std::vector<PickTerm*>, CmpPickTerm> Prio_Pick;

enum VCGNodeType { kVCG_START, kVCG_MEM, kVCG_SOC, kVCG_END };
enum ModuleType { kMoColumn, kMoRow, kMoWheel, kMoSingle };
enum PTNodeType { kPTNUll, kPTMem, kPTSoc, KPTHorizontal, kPTVertical, kPTWheel };

class PTNode { 
 public:
  // constructor
  PTNode() = default;
  PTNode(int, PTNodeType);
  ~PTNode();

  // getter
  auto get_type() const { return _type; }
  auto get_parent() const { return _parent; }
  auto get_pt_id() const { return _pt_id; }

  // setter
  void set_parent(PTNode*);

  // function
  void insert_child(PTNode*);
 
 private:
  // members
  int _pt_id;
  PTNodeType _type;
  PTNode* _parent;
  std::vector<PTNode*> _children;
};

class PatternTree {
 public:
  // constructor
  PatternTree(GridType&, std::map<uint8_t, VCGNodeType>&);
  ~PatternTree();

  // getter

  // setter

  // function

 private:
  // getter

  // setter

  // function
  void slice(const GridType&, std::map<uint8_t, VCGNodeType>&);
  std::queue<GridType> slice_module(const GridType&, bool&);
  std::queue<GridType> slice_vertical(const GridType&);
  std::queue<GridType> slice_horizontal(const GridType&);
  void insert_leaves(int, const GridType&, std::map<uint8_t, VCGNodeType>&);
  std::vector<uint8_t> get_topological_sort(const GridType&);
  std::map<uint8_t, std::set<uint8_t>> get_in_edges(const GridType&);
  void get_zero_in_nodes(const GridType&, const std::map<uint8_t, std::set<uint8_t>>&, std::queue<uint8_t>&);
  PTNodeType get_pt_type(VCGNodeType);
  void debug_create_node(PTNode*);
  void debug_show_pt_grid_map();

  // members
  std::map<int, PTNode*> _node_map;     // pt_id->pt_node
  std::map<int, uint8_t> _pt_grid_map;  // pt_id->grid_value
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

struct CmpPickTerm {
  bool operator()(PickTerm*, PickTerm*);
};

class PickTerm {
 typedef std::vector<std::pair<int, bool>> Picks_Type;
 public:
  // constructor
  PickTerm() = default;
  PickTerm(const PickTerm&);
  PickTerm(Cell*);
  ~PickTerm();

  // getter
  auto get_death() const { return _death; }
  auto get_picks_num() const { return _picks.size(); }
  auto get_box() const { return _box; }
  std::vector<int> get_picks();

  // setter
  void set_death(float death) { _death = death; }
  void set_box(int, int, int, int);
  void set_box(const Rectangle&);

  // function
  bool is_picked(Cell*);
  void insert(Cell*);

 private:
  // members
  Picks_Type _picks;
  float _death;
  Rectangle* _box;          // ignore position. Minimal bounding box in theory
};

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
  auto get_death() const { return _death; }

  // setter
  void set_merge_null() { _merge = nullptr; }
  void set_box_init() { _box->_c1 = {0, 0}; _box->_c3 = {0, 0}; }
  void set_death(float death) { _death = death; }

  // function
  void insert_merge(MergeBox*);
  void clear_picks();


  // members
  GridType _module;
  uint8_t _biggest;             // may be the biggest cell in this module
  ModuleType _type;             // indicate relative position between cells
  Rectangle* _box;              // bounding box of this module
  std::vector<uint8_t> _order;  // cell place order topological  
  Prio_Pick _picks;             // minimal death area picks

 private:
  MergeBox* _merge;
  float _death;                 
};


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
  void gen_result();

  // members
  static size_t _gds_file_num;

 private:
  // version 2
  void init_pattern_tree();  
  std::map<uint8_t, VCGNodeType> make_id_type_map();

  // getter
  size_t get_max_row(GridType&);
  std::vector<GridType> get_smaller_module(GridType&, bool min = false);
  std::vector<GridType> get_smaller_module_column(GridType&, bool);
  std::vector<GridType> get_smaller_module_row(GridType&, bool);
  Point get_cst_x(uint8_t, uint8_t);
  Point get_cst_y(uint8_t, uint8_t);
  Point get_cst_x(uint8_t);
  Point get_cst_y(uint8_t);
  Cell* get_cell_fits_min_ratioWH(uint8_t);
  std::vector<Cell*> get_left_cells(uint8_t);
  std::map<uint8_t, std::vector<uint8_t>> get_in_edge_list();
  std::vector<Cell*> get_colomn_cells(int);
  std::vector<Cell*> get_left_overlap_y_cells(VCGNode*);
  std::set<uint8_t> get_tos(ModuleHelper*);
  std::set<uint8_t> get_right_id_set(ModuleHelper*);
  std::set<uint8_t> get_ids_in_helper(ModuleHelper*);
  std::vector<int> get_cell_ids(ModuleHelper*);

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
  bool is_overlap_y(Cell*, Cell*, bool);
  bool is_overlap_x(Cell*, Cell*, bool);
  bool is_overlap(int, int, int, int, bool);
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
  void try_place(VCGNode*, float flex_x = 1, float flex_y = 1);
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
  std::vector<int> cal_c3_of_interposer();
  void find_possible_combinations();
  void build_picks(ModuleHelper*);
  void build_first(ModuleHelper*);
  void build_picks_col(ModuleHelper*);
  void build_picks_row(ModuleHelper*);
  void try_legalize();
  void fill_id_grid();
  std::vector<int> check_range_invalid();
  void fix_x_range(int, int);
  bool is_froms_cst_meet(uint8_t);
  bool is_tos_cst_meet(uint8_t);
  uint8_t fail_cst_id();
  bool is_box_cst_meet(uint8_t);
  bool is_overlap(Rectangle, Rectangle, bool);
  void fix_y_range(int, int);

  // static
  static bool cmp_module_priority(GridType&, GridType&);
  static size_t get_module_row(GridType&);
  static size_t get_type_num(GridType&);

  // members
  std::vector<VCGNode*> _adj_list;  // Node0 is end, final Node is start
  GridType _id_grid;                // pattern matrix [column][row]
  CellManager* _cell_man;
  Constraint* _cst;
  std::map<uint8_t, ModuleHelper*> _helper_map;
  std::vector<uint8_t> _place_stack;
  std::list<MergeBox*> _merges;     // mainly for release
  size_t _last_merge_id;
  PatternTree* _tree;
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
  g_log << "froms[" << std::to_string(get_id()) << "]:";
  for (auto from : _froms) {
    g_log << std::to_string(from->get_id()) << ", ";
  }
  g_log << "\n";
  g_log.flush();
}

inline void VCGNode::show_tos() {
  g_log << "tos[" << std::to_string(get_id()) << "]:";
  for (auto to : _tos) {
    g_log << std::to_string(to->get_id()) << ", ";
  }
  g_log << "\n";
  g_log.flush();
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
  :_biggest(0), _type(kMoSingle), _death(0) {
  _module.resize(0);
  _box = new Rectangle();
  _merge = nullptr;
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
  if (c != nullptr && _cst == nullptr) {
    _cst = c;
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

inline bool PickTerm::is_picked(Cell* c) {
  assert(c);

  bool ret = false;

  for (auto pair : _picks) {
    if (c->get_cell_id() == pair.first &&
        c->get_rotation() == pair.second) {
      ret = true;
      break;
    }
  }

  return ret;
}

inline PickTerm::PickTerm(const PickTerm& p) {
  for (auto id : p._picks) {
    _picks.push_back(id);
  }
  _box = new Rectangle(*p.get_box());
  _death = p.get_death();
}

inline void PickTerm::insert(Cell* c) {
  assert(c);

  if (!is_picked(c)) {
    _picks.push_back({c->get_cell_id() , c->get_rotation()});
  }
}

inline PickTerm::PickTerm(Cell* cell) : _death(0){
  assert(cell);
  _picks.push_back({cell->get_cell_id(), cell->get_rotation()});
  _box = new Rectangle({0, 0}, {cell->get_width(), cell->get_height()});
}

inline void PickTerm::set_box(const Rectangle& box) {
  *_box = box;
}

inline bool CmpPickTerm::operator()(PickTerm* t1, PickTerm* t2) {
  if (t1 == nullptr || t2 == nullptr) return false;

  auto d1 = t1->get_death();
  auto d2 = t2->get_death();

  if (d1 < d2) {
    return true;
  } else if (d1 > d2) {
    return false;
  } else {
    return t1->get_box()->get_area() < t2->get_box()->get_area();
  }
}

inline void ModuleHelper::clear_picks() {
  while (_picks.size()) {
    auto term = _picks.top();
    _picks.pop();
    delete term;
  }
}

inline PickTerm::~PickTerm() {
  _picks.clear();

  delete _box;
  _box = nullptr;
}

inline std::vector<int> PickTerm::get_picks() {
  std::vector<int> ret;

  for (auto pair : _picks) {
    ret.push_back(pair.first);
  }

  return ret;
}

inline std::vector<int> VCG::get_cell_ids(ModuleHelper* helper) {
  std::vector<int> ret;
  if (helper == nullptr) return ret;
  
  for (auto node_id : helper->_order) {
    auto cell = get_cell(node_id);
    assert(cell);
    ret.push_back(cell->get_cell_id());
  }

  return ret;
}

inline bool VCG::is_overlap_y(Cell* c1, Cell* c2, bool edge) {
  if (c1 == nullptr || c2 == nullptr) return true;

  auto c1_min_y = c1->get_y();
  auto c1_max_y = c1_min_y + c1->get_height();
  auto c2_min_y = c2->get_y();
  auto c2_max_y = c2_min_y + c2->get_height();
  return is_overlap(c1_min_y, c1_max_y, c2_min_y, c2_max_y, edge);
}

inline bool VCG::is_overlap_x(Cell* c1, Cell* c2, bool egde) {
  if (c1 == nullptr || c2 == nullptr) return true;

  auto c1_min_x = c1->get_x();
  auto c1_max_x = c1_min_x + c1->get_width();
  auto c2_min_x = c2->get_x();
  auto c2_max_x = c2_min_x + c2->get_width();

  return is_overlap(c1_min_x, c1_max_x, c2_min_x, c2_max_x, egde);
}

inline bool VCG::is_overlap(int c1_min, int c1_max, int c2_min, int c2_max, bool edge) {
  ASSERT(c1_min <= c1_max && c2_min <= c2_max, "Misuse function");

  return edge ? 
        !(c1_min > c2_max || c1_max < c2_min) :
        !(c1_min >= c2_max || c1_max <= c2_min); 
}

inline bool VCG::is_overlap(Rectangle r1, Rectangle r2, bool edge) {
  return is_overlap(r1._c1._x, r1._c3._x, r2._c1._x, r2._c3._x, edge)
      && is_overlap(r1._c1._y, r1._c3._y, r2._c1._y, r2._c3._y, edge);
}

inline PTNode::PTNode(int pt_id, PTNodeType type):
  _pt_id(pt_id), _type(type), _parent(nullptr) {
    
}

inline void PTNode::insert_child(PTNode* pt_node) {
  if (pt_node && pt_node->get_pt_id() != _pt_id) {
    _children.push_back(pt_node);
  }
}

inline void PTNode::set_parent(PTNode* parent) {
  if (parent && parent != this) {
    _parent = parent;
  }
}

inline std::map<uint8_t, std::set<uint8_t>> PatternTree::get_in_edges(const GridType& grid) {
  std::map<uint8_t, std::set<uint8_t>> visited;
  for (auto column : grid) {
    for (size_t row = 0; row < column.size(); ++row) {

      if (visited.count(column[row]) == 0) {
        visited[column[row]] = {};
      } 
      if ( row + 1 < column.size()
        && column[row] != column[row + 1]
        && visited[column[row]].count(column[row + 1]) == 0) {
        visited[column[row]].insert(column[row+ 1]);
      } 
      
    }
  }

  return visited;
}

inline PTNodeType PatternTree::get_pt_type(VCGNodeType vcg_type) {
  switch (vcg_type) {
    case kVCG_MEM: return kPTMem; 
    case kVCG_SOC: return kPTSoc;

    default: PANIC("Fail to change to PTNodeType by VCGNodeType = %d", vcg_type);
  }
}

inline void PatternTree::debug_create_node(PTNode* pt_node) {
#ifndef G_LOG
  return;
#endif
  if (pt_node == nullptr) return;

  g_log << "Create PTNode_" << pt_node->get_pt_id() << " ";
  switch (pt_node->get_type()) {
    case kPTSoc:        g_log << "Soc ";  break;
    case kPTMem:        g_log << "Mem ";  break;
    case KPTHorizontal: g_log << "Hrz ";  break;
    case kPTVertical:   g_log << "Vtc ";  break;
    case kPTWheel:      g_log << "Whe ";  break;

    default: PANIC("Invalid create PTNodeType = %d", pt_node->get_type());
  }
  g_log << " parent_id = " << 
      (pt_node->get_parent() ? pt_node->get_parent()->get_pt_id() : 0) << "\n";
  g_log.flush();
}

inline void PatternTree::debug_show_pt_grid_map() {
#ifndef G_LOG
  return;
#endif
  for (auto pair : _pt_grid_map) {
    g_log << "pt_id = " << pair.first
          << ", grid_id = " << std::to_string(pair.second) << "\n";
  }
  g_log.flush();
}

}  // namespace EDA_CHALLENGE_Q4
#endif