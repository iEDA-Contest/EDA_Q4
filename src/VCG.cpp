#include "VCG.hpp"

#include <algorithm>
#include <cmath>

namespace EDA_CHALLENGE_Q4 {

size_t VCG::_gds_file_num = 0;

VCG::VCG(Token_List& tokens) : _cell_man(nullptr), _constraint(nullptr) {
  _adj_list.push_back(new VCGNode(kVCG_END));
  VCGNode* start = new VCGNode(kVCG_START);

  size_t column = 1;  //
  size_t row = 0;     // character's row, like SM|SS, row of M equals 2
  VCGNode* from = nullptr;
  auto to = _adj_list[0];  // from --> to

  for (auto token : tokens) {
    ASSERT(to != nullptr, "VCG Node parent is nullptr");

    switch (token.second) {
      // skip
      case kSPACE:
        break;

      // next column
      case kColumn:
        row = 0;
        ++column;
        to->insert_start(start);
        to = _adj_list[0];
        break;

      // module
      case kMEM:
        ++row;
        from = new VCGNode(kVCG_MEM, get_vertex_num());
        _adj_list.push_back(from);
        to->insert_from(from);
        to = from;
        set_id_grid(column, row, from->get_id());
        from = nullptr;
        break;
      case kSOC:
        ++row;
        from = new VCGNode(kVCG_SOC, get_vertex_num());
        _adj_list.push_back(from);
        to->insert_from(from);
        to = from;
        set_id_grid(column, row, from->get_id());
        from = nullptr;
        break;

      // placeholder
      case kVERTICAL:
        ASSERT(row != 0, "'^' occurs at first row");
        ++row;
        to->inc_v_placeholder();
        set_id_grid(column, row, to->get_id());
        break;
      case kHORIZONTAL:
        ASSERT(column > 1, "'<' occurs at first column");
        ++row;
        from = _adj_list[_id_grid[column - 2][row - 1]];
        set_id_grid(column, row, from->get_id());
        from->inc_h_placeholder();
        to->insert_last_from(from);
        to = from;
        break;

      // bugs
      default:
        PANIC("Unknown substring: %s", token.first.c_str());
        break;
    }
  }

  // insert start point
  start->set_id(get_vertex_num());
  to->insert_start(start);
  _adj_list.push_back(start);
  ASSERT(_adj_list.size() > 2, "VCG Node insufficient");

  // init
  init_tos();
  init_column_row_index();
  
  // debug
  // debug();
}

VCG::~VCG() {
  for (auto p : _adj_list) {
    if (p) {
      delete p;
    }
    p = nullptr;
  }
  _adj_list.clear();

  for (auto v : _id_grid) {
    v.clear();
  }
  _id_grid.clear();

  delete _cell_man;
  _constraint = nullptr;  // must not release here!

  std::set<ModuleHelper*> de_set;
  for (auto helper: _helper_map) {
    de_set.insert(helper.second);
  }
  _helper_map.clear();
  for (auto helper: de_set) {
    delete helper;
  }

}

/**
 * @brief build _tos of VCGNode according to VCG. Algorithm BFS.
 * Therefore the shorter path to start, the smaller order in _tos.
 *
 */
void VCG::init_tos() {
  if (get_vertex_num() == 0) return;
  std::vector<bool> line_up(_adj_list.size(), false);
  std::queue<VCGNode*> queue;
  queue.push(_adj_list[0]);
  line_up[0] = true;
  VCGNode* to = nullptr;
  while (queue.size()) {
    to = queue.front();
    queue.pop();
    for (auto from : to->get_froms()) {
      if (line_up[from->get_id()] == false) {
        queue.push(from);
        line_up[from->get_id()] = true;
      }
      from->insert_to(to);
    }
  }
}

void VCG::show_topology() {
  show_froms_tos();
  show_id_grid();
}

void VCG::undo_pick_cell(uint8_t id) {
  if (is_id_valid(id) && _adj_list[id]->get_cell()) {
    switch (_adj_list[id]->get_type()) {
      case kVCG_MEM:
        _cell_man->insert_cell(kCellTypeMem, _adj_list[id]->get_cell());
        _adj_list[id]->set_cell_null();
        break;
      case kVCG_SOC:
        _cell_man->insert_cell(kCellTypeSoc, _adj_list[id]->get_cell());
        _adj_list[id]->set_cell_null();
        break;

      default:
        PANIC("Fail to undo cell with VCG type = %d",
              _adj_list[id]->get_type());
    }
  }
}

void VCG::show_froms_tos() {
  for (auto p : _adj_list) {
    p->show_froms();
    p->show_tos();
    printf("\n");
  }
  printf("------\n");
}

void VCG::show_id_grid() {
  size_t max_row = get_max_row(_id_grid);

  for (size_t row = 0; row < max_row; ++row) {
    printf("row[%lu]:", row);
    for (size_t column = 0; column < _id_grid.size(); ++column) {
      if (row < _id_grid[column].size()) {
        printf("%2hhu, ", _id_grid[column][row]);
      } else {
        printf("  , ");
      }
    }
    printf("\b\b  \n");
  }
  printf("------\n");
}

/**
 * @brief
 *
 *
 *
 * @param grid grid to slice
 * @param min  whether slice into minimal cell.
 * @return std::vector<GridType>
 */
std::vector<GridType> VCG::get_smaller_module(GridType& grid, bool min) {
  std::vector<GridType> smaller;
  smaller = get_smaller_module_column(grid, min);
  if (smaller.size() == 1) {
    smaller = get_smaller_module_row(grid, min);
  }
  return smaller;
}

/**
 * @brief slice with column priority, but NOT slice when grid contains only one
 * row.
 *
 *
 *
 * @param grid
 * @param min  whether slice into minimal cell.
 * @return std::vector<GridType>
 */
std::vector<GridType> VCG::get_smaller_module_column(GridType& grid, bool min) {
  std::vector<GridType> result;

  size_t max_row = get_max_row(_id_grid);
  size_t grid_row = get_max_row(grid);

  if (max_row > 1) {
    size_t row;
    size_t begin_column = 0;

    if (min || grid_row > 1) {
      //
      for (size_t column = 0; column < grid.size() - 1; ++column) {
        for (row = 0; row < max_row; ++row) {
          if (row >= grid[column].size() || row >= grid[column + 1].size()) {
            continue;
          } else if (grid[column][row] == grid[column + 1][row]) {
            break;
          }
        }

        if (row == max_row) {
          GridType module;
          for (; begin_column <= column; ++begin_column) {
            module.push_back(grid[begin_column]);
          }
          result.push_back(module);
        }
      }
    }  // end if (grid_row > 1)

    GridType last_module;
    for (; begin_column < grid.size(); ++begin_column) {
      last_module.push_back(grid[begin_column]);
    }
    result.push_back(last_module);
  }

  return result;
}

/**
 * @brief slice with row priority, but NOT slice when grid contains only one
 * column.
 *
 *
 * @param grid
 * @param min  whether slice into minimal cell.
 * @return std::vector<GridType>
 */
std::vector<GridType> VCG::get_smaller_module_row(GridType& grid, bool min) {
  std::vector<GridType> result;

  size_t max_row = get_max_row(_id_grid);
  size_t begin_row = 0;
  size_t column;

  if (min || grid.size() > 1) {
    //
    for (size_t row = 0; row < max_row - 1; ++row) {
      for (column = 0; column < grid.size(); ++column) {
        if (row > grid[column].size() || row + 1 > grid[column].size()) {
          continue;
        } else if (grid[column][row] == grid[column][row + 1]) {
          break;
        }
      }

      if (column == grid.size()) {
        GridType module;
        for (size_t c = 0; c < grid.size(); ++c) {
          auto begin = grid[c].begin() + begin_row;
          auto end = grid[c].begin() + row + 1;
          end = grid[c].end() < end ? grid[c].end() : end;
          if (begin < end) {
            module.push_back(std::vector<uint8_t>(begin, end));
          }
        }

        if (module.size()) {
          result.push_back(module);
        }
        begin_row = row + 1;
      }
    }
  }  // end if (grid.size())

  GridType last_module;
  for (size_t c = 0; c < grid.size(); ++c) {
    auto begin = grid[c].begin() + begin_row;
    if (begin < grid[c].end()) {
      last_module.push_back(std::vector<uint8_t>(begin, grid[c].end()));
    }
  }
  if (last_module.size()) {
    result.push_back(last_module);
  }

  return result;
}

void VCG::find_best_place() {
  // slice
  auto modules = slice();
  // pick
  auto helpers = make_pick_helpers(modules);
  make_helper_map(helpers);
  place();
}

// get the smallest indivisible modules, and their order rely on slicing times
std::vector<GridType> VCG::slice() {
  std::queue<GridType> queue;
  std::vector<GridType> min_modules;
  auto smaller_grid = get_smaller_module(_id_grid);
  for (auto module : smaller_grid) {
    queue.push(module);
  }

  while (queue.size()) {
    auto module = queue.front();
    queue.pop();
    auto smaller_module = get_smaller_module(module);
    if (smaller_module.size() == 1) {
      min_modules.push_back(smaller_module.back());
    } else {
      for (auto smaller : smaller_module) {
        queue.push(smaller);
      }
    }
  }

  return min_modules;
}

bool VCG::cmp_module_priority(GridType& g1, GridType& g2) {
  // grid size check, which may be unnecessary
  if (g1.size() == 0) {
    return false;
  } else if (g2.size() == 0) {
    return true;
  }

  // set big cell later
  size_t types1 = get_type_num(g1);
  size_t types2 = get_type_num(g2);
  if (types1 == 1 && types2 != 1) {
    return false;
  } else if (types1 != 1 && types2 == 1) {
    return true;
  }

  // column priority
  bool is_column_module1 = g1.size() == 1;
  bool is_column_module2 = g2.size() == 1;
  if (is_column_module1 && !is_column_module2) {
    return true;
  } else if (!is_column_module1 && is_column_module2) {
    return false;
  }

  // row priority
  bool is_row_module1 = get_module_row(g1) == 1;
  bool is_row_module2 = get_module_row(g2) == 1;
  if (is_row_module1 && !is_row_module2) {
    return true;
  } else if (!is_row_module1 && is_row_module2) {
    return false;
  }

  // types
  if (types1 > types2) {
    return true;
  } else if (types1 < types2) {
    return false;
  }

  // slicing order
  return false;
}

std::vector<ModuleHelper*> VCG::make_pick_helpers(std::vector<GridType>& modules) {
  std::vector<ModuleHelper*> ret;

  for (auto m : modules) {
    // count frequency
    std::map<uint8_t, uint8_t> freq_map;
    for (auto c : m) {
      for (auto r : c) {
        ++freq_map[r];
      }
    }

    std::multimap<uint8_t, uint8_t> freq_sorted;
    for (auto fre : freq_map) {
      freq_sorted.insert(std::make_pair(fre.second, fre.first));
    }

    // make pick recommendation
    ModuleHelper* helper = new ModuleHelper();
    helper->_first_pick = freq_sorted.rbegin()->second;
    helper->_module = m;
    helper->_cell_num = freq_sorted.size();
    if (freq_sorted.size() == 1) {
      helper->_type = kMoSingle;
    } else if (m.size() == 1) {
      helper->_type = kMoColumn;
    } else if (get_module_row(m) == 1) {
      helper->_type = kMoRow;
    } else {
      helper->_type = kMoWheel;
    }

    ret.push_back(helper);
  }

  return ret;
}

void VCG::pick(ModuleHelper* helper) {
  assert(helper);
  pick_first(helper);

  switch (helper->_type) {
    case kMoColumn:
      pick_column(helper);
      break;
    case kMoRow:
      pick_row(helper);
      break;
    case kMoWheel:
      pick_wheel(helper);
      break;
    case kMoSingle:
      assert(get_cell(helper->_first_pick) != nullptr);
      break;
    default:
      assert(0);
  }
}

CellPriority VCG::get_priority(VCGNodeType type) {
  switch (type) {
    case kVCG_MEM:
      return kPrioMem;
    case kVCG_SOC:
      return kPrioSoc;

    default:
      return kPrioNull;
  }
}

void VCG::pick_first(ModuleHelper* helper) {
  assert(helper);
  auto best = get_cell_fits_min_ratioWH(helper->_first_pick);

  if (best) {
    do_pick_cell(helper->_first_pick, best);
  }
}

void VCG::pick_column(ModuleHelper* helper) {
  ASSERT(helper->_module.size() == 1, "Module should be column type");

  size_t up, down, cur;  // index
  size_t end = helper->_module[0].size();
  // find cur
  for (cur = 0; cur < end; ++cur) {
    if (helper->_module[0][cur] == helper->_first_pick) {
      break;
    }
  }
  up = cur;
  down = cur;

  Cell* first = get_cell(helper->_first_pick);
  ASSERT(first != nullptr, "first node should be pickd");
  Rectangle box({0, 0}, {first->get_width(), first->get_height()});
  std::map<uint8_t, bool> picked;
  picked[helper->_first_pick] = true;

  // BFS from cur, in a column
  auto& column = helper->_module[0];
  while (picked.size() < helper->_cell_num) {
    // up
    while (up > 0 && picked[column[up]] == true) --up;
    if (picked[column[up]] == false) {
      CellType target = get_cell_type(column[up]);
      auto constraint = get_constraint_y(column[up], column[up + 1]);
      auto bests = _cell_man->choose_cells(kDeathCol, target, box,
                                           constraint._x, constraint._y);
      ASSERT(bests.size(), "No cells to pick");
      do_pick_cell(column[up], bests[0]);
      picked[column[up]] = true;
      // box
      box._c3._x = std::max(box.get_width(), (int)bests[0]->get_width());
      box._c3._y = box.get_height() + constraint._x + bests[0]->get_height();
    }
    // down
    while (down < end - 1 && picked[column[down]] == true) ++down;
    if (picked[column[down]] == false) {
      CellType target = get_cell_type(column[down]);
      auto constraint = get_constraint_y(column[down - 1], column[down]);
      auto bests = _cell_man->choose_cells(kDeathCol, target, box,
                                           constraint._x, constraint._y);
      ASSERT(bests.size(), "No cells to pick");
      do_pick_cell(column[down], bests[0]);
      picked[column[down]] = true;
      // box
      box._c3._x = std::max(box.get_width(), (int)bests[0]->get_width());
      box._c3._y = box.get_height() + constraint._x + bests[0]->get_height();
    }
  }
}

/**
 * @brief get two cells' constraint y
 * 
 * 
 * 
 * @param id1 
 * @param id2 
 * @return Point 
 */
Point VCG::get_constraint_y(uint8_t id1, uint8_t id2) {
  CellType type1 = get_cell_type(id1);
  CellType type2 = get_cell_type(id2);
  assert(type1 != kCellTypeNull && type2 != kCellTypeNull);

  Point ret;
  static constexpr uint8_t shl = 2;
  switch (type1 << shl | type2) {
    case kCellTypeMem << shl | kCellTypeMem:
      ret._x = _constraint->get_constraint(kYMM_MIN);
      ret._y = _constraint->get_constraint(kYMM_MAX);
      break;
    case kCellTypeMem << shl | kCellTypeSoc:
    case kCellTypeSoc << shl | kCellTypeMem:
      ret._x = _constraint->get_constraint(kYMS_MIN);
      ret._y = _constraint->get_constraint(kYMS_MAX);
      break;
    case kCellTypeSoc << shl | kCellTypeSoc:
      ret._x = _constraint->get_constraint(kYSS_MIN);
      ret._y = _constraint->get_constraint(kYSS_MAX);
      break;

    default:
      assert(0);
  }

  return ret;
}

/**
 * @brief get interposer constraint y
 * 
 * 
 * 
 * @param id 
 * @return Point 
 */
Point VCG::get_constraint_y(uint8_t id) {
  CellType ctype = get_cell_type(id);
  assert(ctype != kCellTypeNull);

  Point ret;
  switch (ctype)
  {
    case kCellTypeMem: 
      ret._x = _constraint->get_constraint(kYMI_MIN);
      ret._y = _constraint->get_constraint(kYMI_MAX);
      break;
    case kCellTypeSoc:
      ret._x = _constraint->get_constraint(kYSI_MIN);
      ret._y = _constraint->get_constraint(kYSI_MAX);
      break;
    
    default: assert(0);
  }

  return ret;
}

/**
 * @brief get two cells' constraint x
 * 
 * 
 * 
 * @param id1 
 * @param id2 
 * @return Point 
 */
Point VCG::get_constraint_x(uint8_t id1, uint8_t id2) {
  CellType type1 = get_cell_type(id1);
  CellType type2 = get_cell_type(id2);
  assert(type1 != kCellTypeNull && type2 != kCellTypeNull);

  Point ret;
  static constexpr uint8_t shl = 2;
  switch (type1 << shl | type2) {
    case kCellTypeMem << shl | kCellTypeMem:
      ret._x = _constraint->get_constraint(kXMM_MIN);
      ret._y = _constraint->get_constraint(kXMM_MAX);
      break;
    case kCellTypeMem << shl | kCellTypeSoc:
    case kCellTypeSoc << shl | kCellTypeMem:
      ret._x = _constraint->get_constraint(kXMS_MIN);
      ret._y = _constraint->get_constraint(kXMS_MAX);
      break;
    case kCellTypeSoc << shl | kCellTypeSoc:
      ret._x = _constraint->get_constraint(kXSS_MIN);
      ret._y = _constraint->get_constraint(kXSS_MAX);
      break;

    default:
      assert(0);
  }

  return ret;
}

/**
 * @brief get interposer constraint x
 * 
 * 
 * 
 * @param id node id
 * @return Point (min, max) means constraint value lies in [min, max]
 */
Point VCG::get_constraint_x(uint8_t id) {
  CellType ctype = get_cell_type(id);
  assert(ctype != kCellTypeNull);

  Point ret;
  switch (ctype)
  {
    case kCellTypeMem: 
      ret._x = _constraint->get_constraint(kXMI_MIN);
      ret._y = _constraint->get_constraint(kXMI_MAX);
      break;
    case kCellTypeSoc:
      ret._x = _constraint->get_constraint(kXSI_MIN);
      ret._y = _constraint->get_constraint(kXSI_MAX);
      break;
    
    default: assert(0);
  }

  return ret;
}

void VCG::pick_row(ModuleHelper* helper) {
  ASSERT(get_max_row(helper->_module) == 1, "Module should be row type");

  size_t left, right, cur;  // index
  size_t end = helper->_module.size();
  // find cur
  for (cur = 0; cur < end; ++cur) {
    if (helper->_module[cur][0] == helper->_first_pick) {
      break;
    }
  }
  left = cur;
  right = cur;

  Cell* first = get_cell(helper->_first_pick);
  ASSERT(first != nullptr, "first node should be picked");
  Rectangle box({0, 0}, {first->get_width(), first->get_height()});
  std::map<uint8_t, bool> picked;
  picked[helper->_first_pick] = true;

  // BFS from cur, in a row
  auto& row = helper->_module;
  while (picked.size() < helper->_cell_num) {
    // left
    while (left > 0 && picked[row[left][0]] == true) --left;
    if (picked[row[left][0]] == false) {
      CellType target = get_cell_type(row[left][0]);
      auto constraint = get_constraint_x(row[left][0], row[left + 1][0]);
      auto bests = _cell_man->choose_cells(kDeathRow, target, box,
                                           constraint._x, constraint._y);
      ASSERT(bests.size(), "No cells to pick");
      do_pick_cell(row[left][0], bests[0]);
      picked[row[left][0]] = true;
      // box
      box._c3._x = box.get_width() + constraint._x + bests[0]->get_width();
      box._c3._y = std::max(box.get_height(), (int)bests[0]->get_height());
    }
    // right
    while (right < end - 1 && picked[row[right][0]] == true) ++right;
    if (picked[row[right][0]] == false) {
      CellType target = get_cell_type(row[right][0]);
      auto constraint = get_constraint_x(row[right - 1][0], row[right][0]);
      auto bests = _cell_man->choose_cells(kDeathRow, target, box,
                                           constraint._x, constraint._y);
      ASSERT(bests.size(), "No cells to pick");
      do_pick_cell(row[right][0], bests[0]);
      picked[row[right][0]] = true;
      // box
      box._c3._x = box.get_width() + constraint._x + bests[0]->get_width();
      box._c3._y = std::max(box.get_height(), (int)bests[0]->get_height());
    }
  }
}

/**
 * @brief pick cell that fits min ratioWH of node
 *
 *
 *
 * @param id node id
 * @return Cell* We can set it without rotation again.
 */
Cell* VCG::get_cell_fits_min_ratioWH(uint8_t id) {
  VCGNode* cur = get_node(id);
  assert(cur != nullptr);

  auto recommends = _cell_man->choose_cells(get_priority(get_node_type(id)));

  Cell* best = nullptr;
  for (auto cell : recommends) {
    auto f_cell_ori = fabs(cur->get_ratioHV() - cell->get_ratioWH());
    auto f_cell_rotate = fabs(cur->get_ratioHV() - 1 / cell->get_ratioWH());
    if (f_cell_rotate < f_cell_ori) {
      cell->rotate();
    }

    if (best == nullptr) {
      best = cell;
    } else {
      auto f_best = fabs(cur->get_ratioHV() - best->get_ratioWH());
      auto f_cell = fabs(cur->get_ratioHV() - 1.0 * cell->get_ratioWH());
      if (f_cell < f_best) {
        best = cell;
      }
    }
  }

  return best;
}

void VCG::pick_wheel(ModuleHelper* helper) {
  for (auto column : helper->_module) {
    for (auto id : column) {
      auto best = get_cell_fits_min_ratioWH(id);
      if (best) {
        do_pick_cell(id, best);
      }
    }
  }
}

bool VCG::pick_done() {
  bool ret = false;
  for (auto node : _adj_list) {
    auto node_type = node->get_type();
    if (node_type == kVCG_START || node_type == kVCG_END) continue;
    ret |= node->get_cell() == nullptr;
    if (ret) {
      return ret;
    }
  }

  ret |= _cell_man->get_mems_num() == 0 && _cell_man->get_socs_num() == 0;
  return ret;
}

/*
void VCG::place_bad() {
  // place_y_legalize();
  // place_x_legalize();
}
*/

/**
 * @brief place each cell and legalize their y coordinate
 *
 */
/*
void VCG::place_y_legalize() {
  std::queue<VCGNode*> queue;
  queue.push(_adj_list[_adj_list.size() - 1]);
  std::map<uint8_t, bool> visited;
  auto in_edges = get_in_edge_list();

  // topological sort
  while (queue.size()) {
    auto node = queue.front();
    queue.pop();

    // visit
    place_cell_y_legalize(node->get_id());
    visited[node->get_id()] = true;

    uint8_t id = 0;
    for (auto& ins : in_edges) {
      ins.erase(
          std::remove_if(ins.begin(), ins.end(),
                         [node](auto in) { return in == node->get_id(); }),
          ins.end());

      if (ins.size() == 0) {
        if (visited.count(id) == 0) {
          queue.push(get_node(id));
          visited[id] = false;
        }
      }

      ++id;
    }

  }  // end (queue.size())
}
*/
/*
void VCG::place_cell_y_legalize(uint8_t id) {
  auto target = get_node(id);
  auto obj_type = target->get_type();
  if (obj_type == kVCG_START || obj_type == kVCG_END || target->is_from_start())
    return;

  auto base = target->get_min_from();
  assert(base != nullptr);
  auto constraint = get_constraint_y(target->get_id(), base->get_id());
  auto tar_cell = target->get_cell();
  auto base_cell = base->get_cell();
  assert(tar_cell != nullptr && base_cell != nullptr);

  auto base_point = base_cell->get_positon();
  tar_cell->set_y(base_point._y + base_cell->get_height() + constraint._x);
}
*/

void VCG::gen_GDS() {
#ifndef GDS
  return;
#endif
  std::string fname = "/home/liangyuyao/EDA_CHALLENGE_Q4/output/myresult" +
                      std::to_string(_gds_file_num) + ".gds";
  ++_gds_file_num;
  std::fstream gds;
  gds.open(fname, std::ios::out);
  assert(gds.is_open());

  // gds head
  gds << "HEADER 600\n";
  gds << "BGNLIB\n";
  gds << "LIBNAME DensityLib\n";
  gds << "UNITS 1 1e-6\n";
  gds << "\n\n";

  size_t cell_num = 0;
  for (auto node : _adj_list) {
    if (node->get_type() == kVCG_START || node->get_type() == kVCG_END)
      continue;

    Cell* cell = node->get_cell();
    assert(cell != nullptr);
    ++cell_num;

    // rectangle's four relative coordinates
    // template
    gds << "BGNSTR\n";
    gds << "STRNAME " << cell->get_refer() << "\n";
    gds << "BOUNDARY\n";
    gds << "LAYER " << std::to_string(cell_num) << "\n";
    gds << "DATATYPE 0\n";
    gds << "XY\n";
    // this five coordinates should be clockwise or anti-clockwise
    gds << "0 : 0\n";
    gds << std::to_string(cell->get_width()) << " : 0\n";
    gds << std::to_string(cell->get_width()) << " : "
        << std::to_string(cell->get_height()) << "\n";
    gds << "0 : " << std::to_string(cell->get_height()) << "\n";
    gds << "0 : 0\n";
    gds << "ENDEL\n";
    gds << "ENDSTR\n\n";
  }

  //
  gds << "BGNSTR\n";
  gds << "STRNAME box\n";
  gds << "BOUNDARY\n";
  gds << "LAYER " << std::to_string(get_vertex_num() - 1) << "\n";
  gds << "DATATYPE 0\n";
  gds << "XY\n";
  gds << "0 : 0\n";
  gds << "0 : 1\n";
  gds << "1 : 1\n";
  gds << "1 : 0\n";
  gds << "0 : 0\n";
  gds << "ENDEL\n";
  gds << "ENDSTR\n\n";

  // add rectangles into top module
  gds << "\n\n";
  gds << "BGNSTR\n";
  gds << "STRNAME top\n";
  gds << "\n\n";

  for (auto node : _adj_list) {
    if (node->get_type() == kVCG_START || node->get_type() == kVCG_END)
      continue;

    Cell* cell = node->get_cell();
    assert(cell != nullptr);

    // template
    gds << "SREF\n";
    gds << "SNAME " << cell->get_refer() << "\n";
    gds << "XY " << std::to_string(cell->get_c1()._x) << ": "
        << std::to_string(cell->get_c1()._y) << "\n";  // c1 point
    gds << "ENDEL\n";
  }

  gds << "SREF\n";
  gds << "SNAME box\n";
  gds << "XY  0 : 0\n";  // c1 point
  gds << "ENDEL\n";


  // gds end
  gds << "\n\n";
  gds << "ENDSTR\n";
  gds << "ENDLIB\n";

  gds.close();
}
/*
void VCG::place_x_legalize() {
  std::queue<VCGNode*> queue;
  queue.push(_adj_list[_adj_list.size() - 1]);
  std::map<uint8_t, bool> visited;
  auto in_edges = get_in_edge_list();

  // topological sort
  while (queue.size()) {
    auto node = queue.front();
    queue.pop();

    // visit
    place_cell_x_legalize(node->get_id());
    visited[node->get_id()] = true;

    uint8_t id = 0;
    for (auto& ins : in_edges) {
      ins.erase(
          std::remove_if(ins.begin(), ins.end(),
                         [node](auto in) { return in == node->get_id(); }),
          ins.end());

      if (ins.size() == 0) {
        if (visited.count(id) == 0) {
          queue.push(get_node(id));
          visited[id] = false;
        }
      }

      ++id;
    }

  }  // end (queue.size())
}
*/

/*
void VCG::place_cell_x_legalize(uint8_t id) {
  auto target = get_node(id);
  auto obj_type = target->get_type();
  if (obj_type == kVCG_START || obj_type == kVCG_END || is_first_column(id))
    return;

  auto tar_cell = target->get_cell();
  assert(tar_cell);

  auto lefts = get_left_cells(id);
  for (auto left : lefts) {
    if (is_overlap_y(left, tar_cell)) {
      auto constraint =
          get_constraint_x(left->get_node_id(), tar_cell->get_node_id());
      auto x = std::max(tar_cell->get_x(),
                        left->get_x() + left->get_width() + constraint._x);
      tar_cell->set_x(x);
    }
  }
}
*/

/**
 * @brief get cells in left column next to current column
 *
 *
 * @param id ndoe_id
 * @return std::vector<Cell*>
 */
std::vector<Cell*> VCG::get_left_cells(uint8_t id) {
  std::vector<Cell*> ret;
  if (is_first_column(id)) return ret;

  int cur_col = -1;
  bool find = false;
  for (auto col : _id_grid) {
    ++cur_col;
    for (auto nid : col) {
      if (nid == id) {
        find = true;
        break;
      }
    }

    if (find) {
      break;
    }
  }

  assert(find && cur_col >= 1);

  for (auto id : _id_grid[cur_col - 1]) {
    ret.push_back(get_cell(id));
  }

  return ret;
}

bool VCG::is_overlap_x(Cell* c1, Cell* c2) {
  if (c1 == nullptr || c2 == nullptr) return true;

  auto c1_min_x = c1->get_x();
  auto c1_max_x = c1_min_x + c1->get_width();
  auto c2_min_x = c2->get_x();
  auto c2_max_x = c2_min_x + c2->get_width();

  return !((c1_min_x >= c2_max_x) || c1_max_x <= c2_min_x);
}

bool VCG::is_overlap_y(Cell* c1, Cell* c2) {
  if (c1 == nullptr || c2 == nullptr) return true;

  auto c1_min_y = c1->get_y();
  auto c1_max_y = c1_min_y + c1->get_height();
  auto c2_min_y = c2->get_y();
  auto c2_max_y = c2_min_y + c2->get_height();
  return !(c1_min_y >= c2_max_y || c1_max_y <= c2_min_y);
}

void VCG::place() {
  auto in_edges = get_in_edge_list();

  std::priority_queue<VCGNode*, std::vector<VCGNode*>, CmpVCGNodeTopology> queue;
  queue.push(get_node(get_vertex_num() - 1));
  std::set<uint8_t> in_queue_set;
  in_queue_set.insert(queue.top()->get_id());

  std::vector<uint8_t> place_stack;
  std::map<uint8_t, bool> visited;

  // topological sort
  while (queue.size()) {
    auto visit_node = get_node(queue.top()->get_id());
    assert(visit_node);
    queue.pop();
    in_queue_set.erase(visit_node->get_id());

    if (_helper_map.count(visit_node->get_id()) == 0) continue;

    // visit module
    auto helper = _helper_map[visit_node->get_id()];
    place_module(helper, visited);

    // clear queue elements visited
    std::priority_queue<VCGNode*, std::vector<VCGNode*>, CmpVCGNodeTopology> tmp;
    while (queue.size()) {
      auto node = queue.top();
      queue.pop();
      if (visited[node->get_id()] == false) {
        tmp.push(node);
      }
    }
    queue.swap(tmp);

    // cut in_edges
    for (auto& id_ins : in_edges) {
      auto& vec = id_ins.second;
      vec.erase(
        std::remove_if(vec.begin(), vec.end(),
          [visited](auto id) {
            auto it = visited.find(id);
            return it != visited.end() &&
                  it->second == true;
          }),
        vec.end());

      assert(_helper_map.count(id_ins.first));
      auto& id_helper = _helper_map[id_ins.first];
      if ( vec.size() == 0                                  // this judge reduce time costs and insure topological sort
           && is_froms_visited(id_helper->_module, visited)  // all froms visited
           && visited[id_ins.first] == false                // have not been visited
           && in_queue_set.count(id_ins.first) == 0         // not in the queue
         ) {
            queue.push(get_node(id_ins.first));
            in_queue_set.insert(id_ins.first);
      }
    } // end for (auto& id_ins : in_edges)

  } // end while (queue.size())
}

void VCG::make_helper_map(std::vector<ModuleHelper*>& helpers) {
  _helper_map.clear();

  for (auto& helper : helpers) {
    for (auto column : helper->_module) {
      for (auto id : column) {
        if (_helper_map.count(id) == 0) {
          _helper_map[id] = helper;
        }
      }
    }
  }

  // end node
  GridType g0(1);
  g0[0].push_back(0);
  _helper_map[0] = new ModuleHelper();
  _helper_map[0]->_cell_num = 1;
  _helper_map[0]->_first_pick = 0;
  _helper_map[0]->_type = kMoSingle;
  _helper_map[0]->_module = g0;
  // start node 
  GridType g1(1);
  auto start = get_vertex_num() - 1;
  g1[0].push_back(start);
  _helper_map[start] = new ModuleHelper();
  _helper_map[start]->_cell_num = 1;
  _helper_map[start]->_first_pick = start;
  _helper_map[start]->_type = kMoSingle;
  _helper_map[start]->_module = g1;
}

bool VCG::is_froms_visited(GridType& module, std::map<uint8_t, bool>& visited) {
  for (auto column : module) {
    auto check = get_node(column[column.size() - 1]);
    assert(check);
    for (auto node : check->get_froms()) {
      if (visited[node->get_id()] == false) {
        return false;
      }
    }
  }

  return true;
}

void VCG::place_module(ModuleHelper* target, std::map<uint8_t, bool>& visited) {
  assert(target);

  // in_edge check
  if (!is_froms_visited(target->_module, visited)) return;

  // visit module
  switch(target->_type) {
    case kMoColumn:
      place_col(target, visited);
      break;
    case kMoRow: 
      place_row(target, visited);
      break;
    case kMoWheel: 
      place_whe(target, visited);
      break;
    case kMoSingle: 
      visit_single(get_node(target->_first_pick) );
      visited[target->_first_pick] = true;
      break;

    default: assert(0);
  }
  
}

void VCG::init_column_row_index() {
  std::map<uint8_t, int> id_column_index_map;
  std::map<uint8_t, int> id_row_index_map;

  int column_index = -1;
  int row_index = -1;
  for (auto column : _id_grid) {
    ++column_index;
    row_index = -1;
    for (auto id : column) {
      ++row_index;
      id_column_index_map[id] = column_index;
      id_row_index_map[id] = std::max(row_index, id_row_index_map[id]);
    }
  }

  // except start and end
  assert(id_column_index_map.size() == _adj_list.size() - 2
         && id_row_index_map.size() == _adj_list.size() - 2);

  for (size_t id = 1; id < _adj_list.size() - 1; ++id) {
    _adj_list[id]->set_max_column_index(id_column_index_map[id]);
    _adj_list[id]->set_max_row_index(id_row_index_map[id]);
  }

  auto end = get_node(0);
  assert(end);
  end->set_max_column_index(-1);
  end->set_max_row_index(-1);

  auto start = get_node(_adj_list.size() - 1);
  assert(start);
  start->set_max_column_index(-1);
  start->set_max_row_index(-1);
}

/**
 * @brief decide the position of cell in the module
 * 
 * 
 * 
 * @param target 
 * @param helper_map 
 * @param visited 
 * @param place_stack 
 */
void VCG::place_sin(ModuleHelper* target, std::map<uint8_t, bool>& visited) {
  assert(target && target->_type == kMoSingle);

  if (target->_first_pick == 0 || target->_first_pick == get_vertex_num() - 1) return;
  
  auto node = get_node(target->_first_pick);
  visit_single(node);

  return;

  auto cur_node = get_node(target->_first_pick);
  auto cur_cell = cur_node->get_cell();
  assert(cur_node && cur_cell);

  // y first, because only y is asserted can x position be asserted
  if (cur_node->is_from_start()) {
    auto constraint = get_constraint_y(cur_node->get_id());
    cur_cell->set_y(constraint._x);
  } else {
    int min_y = 0;
    int max_y = 0;
    for (auto from : cur_node->get_froms()) {
      auto fcell = from->get_cell();
      assert(fcell);
      auto constraint = get_constraint_y(cur_node->get_id(), from->get_id());
      min_y = std::max(min_y, 
                       fcell->get_y() + fcell->get_height() + constraint._x);
      max_y = max_y == 0 ?
              fcell->get_y() + fcell->get_height() + constraint._y:
              std::min(max_y,
                       fcell->get_y() + fcell->get_height() + constraint._y);
    } // end for  

    if (min_y <= max_y) {
      cur_cell->set_y(min_y);
    } else {
      cur_cell->set_y(max_y);
      g_log << "Invalid y constraint, nodeid = " 
            << std::to_string(cur_node->get_id()) << "\n";
    }
  } // end if else : y constraint

  // x
  if (cur_node->is_first_column()){
    auto constraint = get_constraint_x(cur_node->get_id());
    cur_cell->set_x(constraint._x);
  } else {
    auto col_cells = get_colomn_cells(cur_node->get_column_index() - 1);
    // remain cells that constitute constraint
    col_cells.erase(std::remove_if(
      col_cells.begin(), col_cells.end(), [this, cur_cell](auto cell) {
        return !this->is_overlap_y(cell, cur_cell);
      }
    ), col_cells.end());

    int min_x = 0;
    int max_x = 0;
    for (auto cell: col_cells) {
      auto constraint = get_constraint_x(cell->get_node_id(), cur_node->get_id());
      min_x = std::max(min_x, 
                       cell->get_x() + cell->get_width() + constraint._x);
      max_x = max_x == 0 ?
              cell->get_x() + cell->get_width() + constraint._y :
              std::min(max_x, 
                       cell->get_x() + cell->get_width() + constraint._y);
    } // end for

    if (min_x <= max_x) {
      cur_cell->set_x(min_x);
    } else {
      cur_cell->set_x(max_x);
      g_log << "Invalid x constraint, nodeid = " 
            << std::to_string(cur_node->get_id()) << "\n";
    }
  } // end if else : x constraint

  // check froms overlap x
  bool adjust = false;
  if (!cur_node->is_from_start()){
    auto froms = cur_node->get_froms();
    auto first_from = froms[0];
    auto last_from = froms[froms.size() - 1];
    adjust = !is_overlap_x(first_from->get_cell(), cur_cell) 
          || !is_overlap_x(last_from->get_cell(), cur_cell);
  }
  
  if (adjust) {
    TODO(); // adjust froms
  }

}

void VCG::place_col(ModuleHelper* target, std::map<uint8_t, bool>& visited) {
  assert(target && target->_type == kMoColumn);

  auto queue = make_col_visit_queue(target);
  assert(queue.size());
  auto first_node = queue.top();
  queue.pop();

  auto first_cell = get_cell_fits_min_ratioWH(first_node->get_id());
  do_pick_cell(first_node->get_id(), first_cell);
  visited[first_node->get_id()] = true;
  _place_stack.push_back(first_node->get_id());

  Rectangle box(first_cell->get_c1(), first_cell->get_c3());
  
  legalize(first_node);

  auto last_node = first_node;
  // down -> top 
  while (queue.size()) {
    auto cur_node = queue.top();
    queue.pop();

    auto cell_type = get_cell_type(cur_node->get_id());
    auto constraint = get_constraint_y(last_node->get_id(), cur_node->get_id());
    auto bests = _cell_man->choose_cells(kDeathCol, cell_type, box,
                                        constraint._x, constraint._y);
    ASSERT(bests.size(), "No cells to pick");
    do_pick_cell(cur_node->get_id(), bests[0]);

    visited[cur_node->get_id()] = true;
    _place_stack.push_back(cur_node->get_id());
    
    box._c3._x = std::max(box.get_width(), (int)bests[0]->get_width());
    box._c3._y = box.get_height() + constraint._x + bests[0]->get_height();

    legalize(cur_node);
    last_node = cur_node;
  }

  set_module_box(target->_first_pick);
}

std::priority_queue<VCGNode*, std::vector<VCGNode*>, CmpVCGNodeMoCol> 
VCG::make_col_visit_queue(ModuleHelper* helper) {
  
  assert(helper && helper->_type == kMoColumn);

  std::priority_queue<VCGNode*, std::vector<VCGNode*>, CmpVCGNodeMoCol> ret;
  std::set<uint8_t> in_queue_set;
  
  for (auto id: helper->_module[0]) {
    if (in_queue_set.count(id) == 0) {
      ret.push(get_node(id));
      in_queue_set.insert(id);
    }
  }
  return ret;
}

Point VCG::cal_y_range(VCGNode* cur_node, float flex) {
  assert(0 <= flex && flex <= 1);

  Point y_range(0, 0);
  if (cur_node == nullptr) return y_range;
  
  if (cur_node->is_from_start()) {
    auto constraint = get_constraint_y(cur_node->get_id());
    y_range = {constraint._x, constraint._y};
  } else {
    int min_y = 0;
    int max_y = 0;

    for (auto from : cur_node->get_froms()) {
      auto fcell = from->get_cell();
      assert(fcell);
      auto constraint = get_constraint_y(cur_node->get_id(), from->get_id());
      min_y = std::max(min_y, 
                       fcell->get_c3()._y + constraint._x);
      max_y = max_y == 0 ?
              fcell->get_c3()._y + constraint._y:
              std::min(max_y,
                       fcell->get_c3()._y + constraint._y);
    } // end for  

    y_range = {min_y, max_y};
  } // end if else: cur_node->is_from_start()

  return y_range;
}

/**
 * @brief 
 * 
 * 
 * 
 * @param cur_node 
 * @param flex lies in [0, 1]. 0 means x position achieve max constraint while 1 means min
 * @return true 
 * @return false 
 */
Point VCG::cal_x_range(VCGNode* cur_node, float flex) {
  assert(0 <= flex && flex <= 1);

  Point x_range(0, 0);
  if (cur_node == nullptr) return x_range;

  if (cur_node->is_first_column()){
    auto constraint = get_constraint_x(cur_node->get_id());
    x_range = {constraint._x, constraint._y};
  } else {

    int min_x = 0;
    int max_x = 0;

    if (!cur_node->is_from_start()) {
      // froms' x constraint
      auto froms = cur_node->get_froms();
      assert(froms.size());

      // c1 should overlap with first from cell
      auto first_from = froms[0];
      auto first_from_cell = first_from->get_cell();
      assert(first_from_cell);
      max_x = first_from_cell->get_c3()._x;

      // c3 should overlap with last from cell
      auto last_from = froms[froms.size() - 1];
      auto last_from_cell = last_from->get_cell();
      auto cur_cell = cur_node->get_cell();
      assert(last_from_cell && cur_cell);
      min_x = last_from_cell->get_c1()._x - cur_cell->get_width();
    } // end froms' x constraint

    auto y_overlaps = get_left_overlap_y_cells(cur_node);
    
    // boxs' x constraint
    std::set<uint8_t> boxs;
    for (auto cell : y_overlaps) {
      assert(_helper_map.count(cell->get_node_id()));
      auto helper = _helper_map[cell->get_node_id()];
      boxs.insert(helper->_first_pick);
    }  
    for(auto id : boxs) {
      auto box = _helper_map[id]->_box;
      min_x = std::max(min_x, box->_c3._x);
    }
    
    // visual y overlap constraint
    for (auto cell: y_overlaps) {
      auto constraint = get_constraint_x(cell->get_node_id(), cur_node->get_id());
      min_x = std::max(min_x, 
                       cell->get_c3()._x + constraint._x);
      max_x = max_x == 0 ? cell->get_c3()._x + constraint._y :
              std::min(max_x, 
                       cell->get_c3()._x + constraint._y);
    }

    x_range = {min_x, max_x};
  } // end if else: cur_node->is_first_column()

  return x_range;
}

/**
 * @brief 
 * 
 * 
 * @param id _id_grid id
 */
void VCG::set_module_box(uint8_t id) {
  ASSERT(_helper_map.count(id),"No module contains node_id = %d", id);

  auto& helper = _helper_map[id];

  std::set<uint8_t> id_set;
  for (auto column : helper->_module) {
    for (auto id : column) {
      id_set.insert(id);
    }
  }

  auto cur_cell = get_cell(id);
  assert(cur_cell);
  auto box = helper->_box; 
  box->_c1 = cur_cell->get_c1();
  box->_c3 = cur_cell->get_c3();

  for (auto id: id_set) {
    auto cell = get_cell(id);
    assert(cell);
    
    box->_c1._x = std::min(box->_c1._x, cell->get_c1()._x);
    box->_c1._y = std::min(box->_c1._y, cell->get_c1()._y);
    box->_c3._x = std::max(box->_c3._x, cell->get_c3()._x);
    box->_c3._y = std::max(box->_c3._y, cell->get_c3()._y);
  }
}

std::vector<Cell*> VCG::get_left_overlap_y_cells(VCGNode* cur_node) {
  assert(cur_node);
  auto cur_cell = cur_node->get_cell();
  assert(cur_cell);

  auto col_cells = get_colomn_cells(cur_node->get_column_index() - 1);
    // remain cells that constitute constraint
    col_cells.erase(std::remove_if(
      col_cells.begin(), col_cells.end(), [this, cur_cell](auto cell) {
        return !this->is_overlap_y(cell, cur_cell);
      }
    ), col_cells.end());
  return col_cells;
}

 void VCG::visit_single(VCGNode* target) {
  assert( target);

  if (target->get_type() == kVCG_START || target->get_type() == kVCG_END) return;
  auto cell = get_cell_fits_min_ratioWH(target->get_id());
  do_pick_cell(target->get_id(), cell);
  legalize(target);

  _place_stack.push_back(target->get_id());
 }

void VCG::place_row(ModuleHelper* target, std::map<uint8_t, bool>& visited) {
  assert(target && target->_type == kMoRow);

  auto queue = make_row_visit_queue(target);
  assert(queue.size());
  auto first_node = queue.top();
  queue.pop();

  auto first_cell = get_cell_fits_min_ratioWH(first_node->get_id());
  do_pick_cell(first_node->get_id(), first_cell);
  visited[first_node->get_id()] = true;
  _place_stack.push_back(first_node->get_id());

  Rectangle box(first_cell->get_c1(), first_cell->get_c3());
  
  legalize(first_node);
  
  auto last_node = first_node;
  // left -> right
  while (queue.size()) {
    auto cur_node = queue.top();
    queue.pop();

    auto cell_type = get_cell_type(cur_node->get_id());
    auto constraint = get_constraint_x(last_node->get_id(), cur_node->get_id());
    auto bests = _cell_man->choose_cells(kDeathRow, cell_type, box, 
                                              constraint._x, constraint._y);
    ASSERT(bests.size(), "No cells to pick");
    do_pick_cell(cur_node->get_id(), bests[0]);

    visited[cur_node->get_id()] = true;
    _place_stack.push_back(cur_node->get_id());

    box._c3._x = box.get_width() + constraint._x + bests[0]->get_width();
    box._c3._y = std::max(box.get_height(), (int)bests[0]->get_height());

    legalize(cur_node);

    last_node = cur_node;
  }

  set_module_box(target->_first_pick);
}

std::priority_queue<VCGNode*, std::vector<VCGNode*>, CmpVCGNodeMoRow> 
VCG::make_row_visit_queue(ModuleHelper* helper) {

  assert(helper && helper->_type == kMoRow);

  std::priority_queue<VCGNode*, std::vector<VCGNode*>, CmpVCGNodeMoRow> ret;
  std::set<uint8_t> in_queue_set;
  
  for (auto& column: helper->_module) {
    auto id = column[0];
    if (in_queue_set.count(id) == 0) {
      ret.push(get_node(id));
      in_queue_set.insert(id);
    }
  }

  return ret;
}

void VCG::place_whe(ModuleHelper* target, std::map<uint8_t, bool>& visited) {
  assert(target && target->_type == kMoWheel);

  auto queue = make_whe_visit_queue(target); 
  while (queue.size()) {
    auto cur_node = get_node(queue.front());
    queue.pop();

    auto best = get_cell_fits_min_ratioWH(cur_node->get_id());
    ASSERT(best, "No cells to pick");
    do_pick_cell(cur_node->get_id(), best);
    
    visited[cur_node->get_id()] = true;
    _place_stack.push_back(cur_node->get_id());

    legalize(cur_node);
  }
  
  set_module_box(target->_first_pick);
}

std::queue<uint8_t> VCG::make_whe_visit_queue(ModuleHelper* helper) {

  assert(helper && helper->_type == kMoWheel);

  std::queue<uint8_t> ret;
  std::priority_queue<VCGNode*, std::vector<VCGNode*>, CmpVCGNodeTopology> queue;
  std::set<uint8_t> in_queue_set;

  auto in_edges = make_in_edge_list(helper->_module);

  for (auto pair : in_edges) {
    if (pair.second.size() == 0) {
      queue.push(get_node(pair.first));
      in_queue_set.insert(pair.first);
    }
  }

  while (queue.size()) {
    auto node = queue.top();

    // visit
    ret.push(node->get_id());
    in_edges.erase(node->get_id());
    queue.pop();

    // cut in_edge
    for (auto& pair : in_edges) {
      if (pair.second.count(node->get_id())) {
        pair.second.erase(node->get_id());
      }

      if (pair.second.size() == 0 
       && in_queue_set.count(pair.first) == 0) {
         queue.push(get_node(pair.first));
         in_queue_set.insert(pair.first);
       }
    }
  }

  assert(ret.size() == helper->_cell_num);
  return ret;
}

std::map<uint8_t, std::set<uint8_t>> VCG::make_in_edge_list(GridType& module) {
  auto max_row = get_module_row(module);
  std::map<uint8_t, std::set<uint8_t>> ret;

  for (size_t col = 0; col < module.size(); ++col) {
    for (size_t row = 0; row < max_row - 1; ++row) {
      if ( (row > module[col].size() - 1)
        || (row + 1 > module[col].size() - 1) ) continue;

      if (module[col][row] != module[col][row + 1]) {
        ret[module[col][row]].insert(module[col][row + 1]);
      }
    }
  }

  for (auto col : module) {
    if (ret.count(col[col.size() - 1]) == 0) {
      ret[col[col.size() - 1]] = {};
    }
  }

  return ret;
}

void VCG::legalize(VCGNode* target) {
  assert(target);
  auto tar_cell = target->get_cell();
  assert(tar_cell);

  float flex_x = 1;
  float flex_y = 1;
  auto y_range = cal_y_range(target, flex_y);
  if (y_range._x < y_range._y) {
    tar_cell->set_y_range(y_range);
    tar_cell->set_y(y_range._x + (1 - flex_y) * (y_range._y - y_range._x));
  } else {
    // debug
    tar_cell->set_y(y_range._x + (1 - flex_y) * (y_range._y - y_range._x));
    // gen_GDS();
    // TODO();
  }

  auto x_range = cal_x_range(target, flex_x);
  if (x_range._x < x_range._y) {
    tar_cell->set_x_range(x_range);
    tar_cell->set_x(x_range._x + (1 - flex_x)* (x_range._y - x_range._x));
  } else {
    // debug
    tar_cell->set_x(x_range._x + (1 - flex_x)* (x_range._y - x_range._x));
    // gen_GDS();
    // TODO();
}
}

}  // namespace  EDA_CHALLENGE_Q4