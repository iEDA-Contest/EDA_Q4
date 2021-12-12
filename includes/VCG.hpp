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

class VCG;

enum VCGNodeType { kVCG_START, kVCG_MEM, kVCG_SOC, kVCG_END };
enum PTNodeType { kPTNUll, kPTMem, kPTSoc, KPTHorizontal, kPTVertical, kPTWheel };

class PickItem {
 public:
  // constructor
  PickItem(uint8_t, int, bool, int, int);
  PickItem(const PickItem&);
  ~PickItem() = default;

  // member 
  uint8_t _vcg_id;
  int _cell_id;
  bool _rotation;
  int _c1_x;
  int _c1_y;
};

class PickHelper {
 public:
  // constructor
  PickHelper(uint8_t, int, bool);
  PickHelper(PickHelper*, PickHelper*);
  PickHelper(PickHelper*);
  ~PickHelper();

  // getter
  auto get_items() const { return _items; }
  auto get_box() const { return _box; }
  auto get_death() const { return _death; }
  auto get_items_num() const { return _items.size(); }

  // setter
  void set_box(const Rectangle& r) { _box = r; }
  void set_box(int, int, int, int);
  void set_death(float d) { _death = d; }
  
  // function
  PickItem* get_item(uint8_t);
  void get_items(const std::set<uint8_t>&, std::vector<PickItem*>&);
  PickHelper* get_pt_node_helper(uint8_t, int);
  void insert_source(PickHelper*);

 private:
  // members
  std::vector<PickItem*> _items;
  std::map<uint8_t, PickItem*> _id_item_map; // vcg_id->item
  Rectangle _box;
  float _death;
};

struct CmpPickHelperDeath {
  bool operator()(PickHelper*, PickHelper*);
};

class PTNode { 
 typedef std::vector<PickHelper*> Picks; 
 public:
  // constructor
  PTNode() = default;
  PTNode(int, PTNodeType, const GridType&);
  ~PTNode();

  // getter
  auto get_type() const { return _type; }
  auto get_parent() const { return _parent; }
  auto get_pt_id() const { return _pt_id; }
  auto get_children() const { return _children; }
  auto get_picks() const { return _picks; }
  auto get_grid() const { return _grid; }

  // setter
  void set_parent(PTNode*);

  // function
  void insert_child(PTNode*);
  void insert_pick(PickHelper*);
  void get_grid_lefts(std::set<uint8_t>&);
  void get_grid_rights(std::set<uint8_t>&);
  void get_grid_tops(std::set<uint8_t>&);
  void get_grid_bottoms(std::set<uint8_t>&);
 
 private:
  // members
  int _pt_id;
  PTNodeType _type;
  PTNode* _parent;
  std::vector<PTNode*> _children;
  GridType _grid;
  Picks _picks;
};

class PatternTree {
 typedef std::priority_queue< PickHelper*, std::vector<PickHelper*>, CmpPickHelperDeath> DeathQue;

 public:
  // constructor
  PatternTree(GridType&, std::map<uint8_t, VCGNodeType>&);
  ~PatternTree();

  // getter
  PTNode* get_pt_node(int);

  // setter
  void set_cst(Constraint*);
  void set_cm(CellManager*);
  void set_vcg(VCG*);

  // function
  void postorder_traverse();
  void get_cst_x(CellType, Point&);
  void get_cst_x(CellType, CellType, Point&);
  void get_cst_y(CellType, Point&);
  void get_cst_y(CellType, CellType, Point&);
  void set_cell_status(Cell*, PickItem*);
  PTNode* get_biggest_column(uint8_t);

 private:
  // getter

  // setter

  // function
  void slice(const GridType&, std::map<uint8_t, VCGNodeType>&);
  void slice_module(const GridType&, bool&, std::queue<GridType>&);
  void slice_vertical(const GridType&, std::queue<GridType>&);
  void slice_horizontal(const GridType&, std::queue<GridType>&);
  void insert_leaves(int, const GridType&, std::map<uint8_t, VCGNodeType>&);
  void get_topological_sort(const GridType&, std::vector<uint8_t>&);
  void get_in_edges(const GridType&, std::map<uint8_t, std::set<uint8_t>>&);
  void get_zero_in_nodes(const GridType&, const std::map<uint8_t, std::set<uint8_t>>&, std::queue<uint8_t>&);
  PTNodeType get_pt_type(VCGNodeType);
  void debug_create_node(PTNode*);
  void debug_show_pt_grid_map();
  void visit_pt_node(int);
  void list_possibility(PTNode*);
  void get_celltype(PTNodeType, CellType&);
  void merge_hrz(PTNode*);
  bool is_pick_repeat(PickHelper*, PickHelper*);
  void adjust_interposer_left(std::vector<PickItem*>&);
  void adjust_interposer_bottom(std::vector<PickItem*>&);
  bool is_interposer_left(uint8_t);
  bool is_interposer_bottom(uint8_t);
  void clear_queue(std::queue<GridType>&);
  void get_box_range_fit_litems(const std::vector<PickItem*>&, const std::vector<PickItem*>&, int, Point&);
  void get_box_range_fit_bitems(const std::vector<PickItem*>&, const std::vector<PickItem*>&, int, Point&);
  bool is_overlap_y(Cell*, Cell*, bool);
  bool is_overlap_x(Cell*, Cell*, bool);
  bool is_overlap(int, int, int, int, bool);
  void debug_GDS(PickHelper*);
  void get_helper_box(PickHelper*, Rectangle&);
  int get_cells_area(PickHelper*);
  void merge_vtc(PTNode*);
  bool insert_death_que(DeathQue&, PickHelper*);
  int get_pt_id(uint8_t);

  // members
  std::map<int, PTNode*> _node_map;     // pt_id->pt_node
  std::map<int, uint8_t> _pt_grid_map;  // pt_id->vcg_id
  CellManager* _cm;
  Constraint* _cst;
  VCG* _vcg;
};

class VCGNode {
 public:
  // constructor
  VCGNode(VCGNodeType);
  VCGNode(VCGNodeType, uint8_t);
  ~VCGNode();

  // getter
  auto get_vcg_id() const { return _vcg_id; }
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
  CellType get_cell_type();

  // setter
  void set_id(uint8_t id) { _vcg_id = id; }
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
  uint8_t _vcg_id;               // VCG Node id
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
  void undo_all_picks();
  bool is_id_valid(uint8_t);
  void show_topology();
  void show_froms_tos();
  void show_id_grid();
  void find_best_place();
  void gen_GDS();
  void gen_result();
  void update_pitem(Cell*);

  // members
  static size_t _gds_file_num;

 private:
  // version 2
  void init_pattern_tree();  
  std::map<uint8_t, VCGNodeType> make_id_type_map();
  void traverse_tree();
  void fill_id_grid();
  void retrieve_all_cells();

  // getter
  size_t get_max_row(GridType&);

  // setter
  void set_id_grid(size_t, size_t, uint8_t);

  // function
  void init_tos();
  void init_column_row_index();
  void debug();
  void get_interposer_c3(int[4]);
  void get_cst_x(uint8_t, Point&);
  void get_cst_x(uint8_t, uint8_t, Point&);
  void get_cst_y(uint8_t, Point&);
  void get_cst_y(uint8_t, uint8_t, Point&);
  void set_cells_by_helper(PickHelper*);
  void debug_picks();
 
  // members
  std::vector<VCGNode*> _adj_list;  // Node0 is end, final Node is start
  GridType _id_grid;                // pattern matrix [column][row]
  CellManager* _cm;
  Constraint* _cst;
  PatternTree* _tree;
  PickHelper* _helper;
};

// VCGNode
inline VCGNode::VCGNode(VCGNodeType type)
    : _vcg_id(0), _v_placeholder(1), _h_placeholder(1), _cell(nullptr) {
  _type = type;
}

inline VCGNode::VCGNode(VCGNodeType type, uint8_t id)
    : _vcg_id(id), _v_placeholder(1), _h_placeholder(1), _cell(nullptr) {
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
  g_log << "froms[" << std::to_string(get_vcg_id()) << "]:";
  for (auto from : _froms) {
    g_log << std::to_string(from->get_vcg_id()) << ", ";
  }
  g_log << "\n";
  g_log.flush();
}

inline void VCGNode::show_tos() {
  g_log << "tos[" << std::to_string(get_vcg_id()) << "]:";
  for (auto to : _tos) {
    g_log << std::to_string(to->get_vcg_id()) << ", ";
  }
  g_log << "\n";
  g_log.flush();
}

/*Not release memory, please manage it manly!*/
inline void VCGNode::set_cell_null() {
  _cell->set_vcg_id(0);
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
    return n1->get_vcg_id() < n2->get_vcg_id();
  }
}

inline bool VCGNode::is_first_column() {
  return get_column_index() == 0;
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
  if (_cm == nullptr && cm != nullptr) {
    _cm = new CellManager(*cm);
    _tree->set_cm(_cm);
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

inline void VCG::do_pick_cell(uint8_t vcg_id, Cell* cell) {
  if (is_id_valid(vcg_id) && cell != nullptr &&
      _adj_list[vcg_id]->get_cell() == nullptr) {
    //
    cell->set_vcg_id(vcg_id);
    _adj_list[vcg_id]->set_cell(cell);
    _cm->delete_cell(get_cell_type(vcg_id), cell);

    // debug
    // g_log << "nodeid = " << std::to_string(vcg_id) << ", cell_id = " << cell->get_refer() << "\n";
    // g_log.flush();
  }
}

inline Cell* VCG::get_cell(uint8_t vcg_id) {
  VCGNode* node = get_node(vcg_id);
  return node ? node->get_cell() : nullptr;
}

inline void VCG::set_constraint(Constraint* c) {
  if (c != nullptr && _cst == nullptr) {
    _cst = c;
    _tree->set_cst(c);
  }
}

inline void VCG::retrieve_all_cells() {
  for (auto node : _adj_list) {
    undo_pick_cell(node->get_vcg_id());
  }
}

inline bool PatternTree::is_overlap_y(Cell* c1, Cell* c2, bool edge) {
  if (c1 == nullptr || c2 == nullptr) return true;

  auto c1_min_y = c1->get_y();
  auto c1_max_y = c1_min_y + c1->get_height();
  auto c2_min_y = c2->get_y();
  auto c2_max_y = c2_min_y + c2->get_height();
  return is_overlap(c1_min_y, c1_max_y, c2_min_y, c2_max_y, edge);
}

inline bool PatternTree::is_overlap_x(Cell* c1, Cell* c2, bool egde) {
  if (c1 == nullptr || c2 == nullptr) return true;

  auto c1_min_x = c1->get_x();
  auto c1_max_x = c1_min_x + c1->get_width();
  auto c2_min_x = c2->get_x();
  auto c2_max_x = c2_min_x + c2->get_width();

  return is_overlap(c1_min_x, c1_max_x, c2_min_x, c2_max_x, egde);
}

inline bool PatternTree::is_overlap(int c1_min, int c1_max, int c2_min, int c2_max, bool edge) {
  ASSERT(c1_min <= c1_max && c2_min <= c2_max, "Misuse function");

  return edge ? 
        !(c1_min > c2_max || c1_max < c2_min) :
        !(c1_min >= c2_max || c1_max <= c2_min); 
}

// inline bool VCG::is_overlap(Rectangle r1, Rectangle r2, bool edge) {
//   return is_overlap(r1._c1._x, r1._c3._x, r2._c1._x, r2._c3._x, edge)
//       && is_overlap(r1._c1._y, r1._c3._y, r2._c1._y, r2._c3._y, edge);
// }

inline PTNode::PTNode(int pt_id, PTNodeType type, const GridType& grid):
  _pt_id(pt_id), _type(type), _parent(nullptr), _grid(grid) {
    
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

inline void PatternTree::get_in_edges(const GridType& grid /*in*/,
                                      std::map<uint8_t, std::set<uint8_t>>& in_edges /*out*/) {
  for (auto column : grid) {
    for (size_t row = 0; row < column.size(); ++row) {

      if (in_edges.count(column[row]) == 0) {
        in_edges[column[row]] = {};
      } 
      if ( row + 1 < column.size()
        && column[row] != column[row + 1]
        && in_edges[column[row]].count(column[row + 1]) == 0) {
        in_edges[column[row]].insert(column[row+ 1]);
      } 
      
    }
  }
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
          << ", grid_value = " << std::to_string(pair.second) << "\n";
  }
  g_log.flush();
}

inline void PatternTree::set_cm(CellManager* cm) {
  if (cm && _cm == nullptr) {
    _cm = cm;
  }
}

inline void PatternTree::set_cst(Constraint* cst) {
  if (cst && _cst == nullptr) {
    _cst = cst;
  }
}

inline void PatternTree::set_vcg(VCG* vcg) {
  if (vcg && _vcg == nullptr) {
    _vcg = vcg;
  }
}

inline PickHelper::PickHelper(uint8_t grid_value, int cell_id, bool rotation)
  :_death(0) {
  auto p = new PickItem(grid_value, cell_id, rotation, 0, 0);
  _items.push_back(p);
  _id_item_map[grid_value] = p;
}

inline void PTNode::insert_pick(PickHelper* pick) {
  if (pick) {
    _picks.push_back(pick);
  }
}

inline void PatternTree::get_celltype(PTNodeType pt_type /*in*/, 
                                      CellType& c_type /*out*/ ) {
  switch (pt_type) {
    case kPTMem: c_type = kCellTypeMem; break;
    case kPTSoc: c_type = kCellTypeSoc; break;
    default: PANIC("No exchange from pt_type = %d to cell_type", pt_type);
  }
}

inline PickItem::PickItem(uint8_t grid_value, int cell_id, bool rotation, int c1_x, int c1_y) 
  : _vcg_id(grid_value), _cell_id(cell_id), _rotation(rotation), _c1_x(c1_x), _c1_y(c1_y) {
}

inline PickHelper::~PickHelper() {
  for (auto i : _items) {
    delete i;
  }
  _items.clear();

  _id_item_map.clear();
}

inline PickItem::PickItem(const PickItem& p) :
 _vcg_id(p._vcg_id),
 _cell_id(p._cell_id),
 _rotation(p._rotation),
 _c1_x(p._c1_x),
 _c1_y(p._c1_y) {
}

inline void PickHelper::set_box(int c1_x, int c1_y, int c3_x, int c3_y) {
  _box._c1._x = c1_x;
  _box._c1._y = c1_y;
  _box._c3._x = c3_x;
  _box._c3._y = c3_y;
}

inline void PTNode::get_grid_lefts(std::set<uint8_t>& set /*out*/) {
  set.clear();
  for (auto id : _grid[0]) {
    set.insert(id);
  }
}

inline void PTNode::get_grid_rights(std::set<uint8_t>& set /*out*/) {
  set.clear();
  ASSERT(_grid.size(), "Data error");
  for (auto id : _grid[_grid.size() - 1]) {
    set.insert(id);
  }
}

inline void PTNode::get_grid_tops(std::set<uint8_t>& set /*out*/) {
  set.clear();
  for (auto column : _grid) {
    set.insert(column[0]);
  }
}

inline void PTNode::get_grid_bottoms(std::set<uint8_t>& set /*out*/) {
  set.clear();
  for (auto column : _grid) {
    set.insert(column[column.size() - 1]);
  }
}

inline void PickHelper::get_items(const std::set<uint8_t>& grid_set /*in*/, 
                                  std::vector<PickItem*>& items /*out*/) {
  items.clear();
  for (auto id : grid_set) {
    auto item = get_item(id);
    if(item) {
      items.push_back(item);
    }
  }
}

inline PickItem* PickHelper::get_item(uint8_t vcg_id) {
  return _id_item_map.count(vcg_id) ? _id_item_map[vcg_id] : nullptr;
}

inline bool PatternTree::is_interposer_left(uint8_t id) {
  ASSERT(_node_map.count(0), "Missing data, pt_id = 0");
  auto grid = _node_map[0]->get_grid();
  ASSERT(grid.size(), "Data error");
  
  for (auto id_in_col : grid[0]) {
    if ( id_in_col == id) {
      return true;
    }
  }

  return false;
}

inline bool PatternTree::is_interposer_bottom(uint8_t id) {
  ASSERT(_node_map.count(0), "Missing data, pt_id = 0");
  auto grid = _node_map[0]->get_grid();
  ASSERT(grid.size(), "Data error");

  for (auto col : grid) {
    if(col[col.size() - 1] == id) {
      return true;
    }
  }

  return false;
}

inline void PatternTree::get_cst_x( CellType c_type /*in*/,
                                    Point& range /*out*/) {
  switch (c_type) {
    case kCellTypeMem: 
      range._x = _cst->get_cst(kXMI_MIN); 
      range._y = _cst->get_cst(kXMI_MAX); 
      break;
    case kCellTypeSoc:
      range._x = _cst->get_cst(kXSI_MIN); 
      range._y = _cst->get_cst(kXSI_MAX); 
      break;
    default: PANIC("Unknown interposer constraint for cell type = %d", c_type);
  }
}

inline void PatternTree::get_cst_x( CellType c_type1 /*in*/,
                                    CellType c_type2 /*in*/,
                                    Point& range /*out*/) {
  static constexpr uint8_t shl = 2;
  switch(c_type1 << shl | c_type2) {
    case kCellTypeMem << shl | kCellTypeMem: 
      range._x = _cst->get_cst(kXMM_MIN);
      range._y = _cst->get_cst(kXMM_MAX);
      break;
    case kCellTypeSoc << shl | kCellTypeSoc: 
      range._x = _cst->get_cst(kXSS_MIN);
      range._y = _cst->get_cst(kXSS_MAX);
      break;
    case kCellTypeSoc << shl | kCellTypeMem: 
    case kCellTypeMem << shl | kCellTypeSoc: 
      range._x = _cst->get_cst(kXMS_MIN);
      range._y = _cst->get_cst(kXMS_MAX);
      break;
    
    default: PANIC("Unknown cell type constraint between type1 = %d and type2 = %d", c_type1, c_type2 );
  }
}

inline void PatternTree::get_cst_y( CellType c_type /*in*/,
                                    Point& range /*out*/) {
  switch (c_type) {
    case kCellTypeMem: 
      range._x = _cst->get_cst(kYMI_MIN); 
      range._y = _cst->get_cst(kYMI_MAX); 
      break;
    case kCellTypeSoc:
      range._x = _cst->get_cst(kYSI_MIN); 
      range._y = _cst->get_cst(kYSI_MAX); 
      break;
    default: PANIC("Unknown interposer constraint for cell type = %d", c_type);
  }
}

inline void PatternTree::get_cst_y( CellType c_type1 /*in*/,
                                    CellType c_type2 /*in*/,
                                    Point& range /*out*/) {
  static constexpr uint8_t shl = 2;
  switch(c_type1 << shl | c_type2) {
    case kCellTypeMem << shl | kCellTypeMem: 
      range._x = _cst->get_cst(kYMM_MIN);
      range._y = _cst->get_cst(kYMM_MAX);
      break;
    case kCellTypeSoc << shl | kCellTypeSoc: 
      range._x = _cst->get_cst(kYSS_MIN);
      range._y = _cst->get_cst(kYSS_MAX);
      break;
    case kCellTypeSoc << shl | kCellTypeMem: 
    case kCellTypeMem << shl | kCellTypeSoc: 
      range._x = _cst->get_cst(kYMS_MIN);
      range._y = _cst->get_cst(kYMS_MAX);
      break;
    
    default: PANIC("Unknown cell type constraint between type1 = %d and type2 = %d", c_type1, c_type2 );
  }
}

inline void PatternTree::clear_queue(std::queue<GridType>& q) {
  std::queue<GridType> empty;
  q.swap(empty);
}

inline void VCG::get_cst_x( uint8_t vcg_id /*in*/,
                            Point& range /*out*/) {
  ASSERT(is_id_valid(vcg_id), "No vcg_id = %d", vcg_id);
  auto node = _adj_list[vcg_id];
  _tree->get_cst_x(node->get_cell_type(), range);
}

inline void VCG::get_cst_x( uint8_t vcg_id1 /*in*/,
                            uint8_t vcg_id2 /*in*/,
                            Point& range /*out*/) {
  ASSERT(is_id_valid(vcg_id1), "No vcg_id = %d", vcg_id1);
  ASSERT(is_id_valid(vcg_id2), "No vcg_id = %d", vcg_id2);
  auto node1 = _adj_list[vcg_id1];
  auto node2 = _adj_list[vcg_id2];
  _tree->get_cst_x( node1->get_cell_type(),
                    node2->get_cell_type(),
                    range);
}

inline void VCG::get_cst_y( uint8_t vcg_id /*in*/,
                            Point& range /*out*/) {
  ASSERT(is_id_valid(vcg_id), "No vcg_id = %d", vcg_id);
  auto node = _adj_list[vcg_id];
  _tree->get_cst_y(node->get_cell_type(), range);
}

inline void VCG::get_cst_y( uint8_t vcg_id1 /*in*/,
                            uint8_t vcg_id2 /*in*/,
                            Point& range /*out*/) {
  ASSERT(is_id_valid(vcg_id1), "No vcg_id = %d", vcg_id1);
  ASSERT(is_id_valid(vcg_id2), "No vcg_id = %d", vcg_id2);
  auto node1 = _adj_list[vcg_id1];
  auto node2 = _adj_list[vcg_id2];
  _tree->get_cst_y( node1->get_cell_type(),
                    node2->get_cell_type(),
                    range);
}

inline CellType VCGNode::get_cell_type() {
  switch (_type) {
    case kVCG_MEM: return kCellTypeMem;
    case kVCG_SOC: return kCellTypeSoc;

    default: PANIC("No exchange from vcg_type = %d to cell_type", _type);
  }
}

inline bool CmpPickHelperDeath::operator()(PickHelper* p1, PickHelper* p2) {
  if (!p1 || !p2) return false;

  return p1->get_death() < p2->get_death();
}

inline void PatternTree::set_cell_status(Cell* cell /*out*/, PickItem* item /*in*/) {
  if (cell && item && cell->get_cell_id() == item->_cell_id) {
    cell->set_x(item->_c1_x);
    cell->set_y(item->_c1_y);
    if (cell->get_rotation() != item->_rotation) {
      cell->rotate();
    }
  }
}

inline PickHelper::PickHelper(PickHelper* helper)
  :_death(0) {
  for (auto item : helper->get_items()) {
    _items.push_back(new PickItem(*item));
  }
  for (auto item : _items) {
    _id_item_map[item->_vcg_id] = item;
  }
}

inline PTNode* PatternTree::get_pt_node(int pt_id) {
  return _node_map.count(pt_id) ? _node_map[pt_id] : nullptr;
}

}  // namespace EDA_CHALLENGE_Q4
#endif