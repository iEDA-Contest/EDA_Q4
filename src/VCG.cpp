#include "VCG.hpp"

#include <cmath>
#include <queue>

namespace EDA_CHALLENGE_Q4 {

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

  // init
  build_tos();

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
}

/**
 * @brief build _tos of VCGNode according to VCG. Algorithm BFS.
 * Therefore the shorter path to start, the smaller order in _tos.
 *
 */
void VCG::build_tos() {
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
        PANIC("Fail to undo cell with cell_type = %d",
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
  std::sort(modules.begin(), modules.end(), cmp_module_priority);
  // pick
  auto helpers = make_pick_helpers(modules);
  for (auto helper : helpers) {
    pick(helper);
  }
  assert(pick_done());
  // place
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

std::vector<PickHelper> VCG::make_pick_helpers(std::vector<GridType>& modules) {
  std::vector<PickHelper> ret;

  for (auto m : modules) {
    // count frequency
    std::map<uint8_t, uint8_t> freq_map;
    for (auto c : m) {
      for (auto r : c) {
        freq_map[r]++;
      }
    }

    std::multimap<uint8_t, uint8_t> freq_sorted;
    for (auto fre : freq_map) {
      freq_sorted.insert(std::make_pair(fre.second, fre.first));
    }

    // make pick recommendation
    PickHelper helper;
    helper._first = freq_sorted.rbegin()->second;
    helper._module = m;
    helper._cell_num = freq_sorted.size();
    if (freq_sorted.size() == 1) {
      helper._type = kPickSingle;
    } else if (m.size() == 1) {
      helper._type = kPickColumn;
    } else if (get_module_row(m) == 1) {
      helper._type = kPickRow;
    } else {
      helper._type = kPickWheel;
    }

    ret.push_back(helper);
  }

  return ret;
}

void VCG::pick(PickHelper& helper) {
  pick_first(helper);

  switch (helper._type) {
    case kPickColumn:
      pick_column(helper);
      break;
    case kPickRow:
      pick_row(helper);
      break;
    case kPickWheel:
      pick_wheel(helper);
      break;
    case kPickSingle:
      assert(get_cell(helper._first) != nullptr);
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

void VCG::pick_first(PickHelper& helper) {
  auto best = get_cell_fits_min_ratioWH(helper._first);

  if (best) {
    do_pick_cell(helper._first, best);
  }
}

void VCG::pick_column(PickHelper& helper) {
  ASSERT(helper._module.size() == 1, "Module should be column type");

  size_t up, down, cur;  // index
  size_t end = helper._module[0].size();
  // find cur
  for (cur = 0; cur < end; ++cur) {
    if (helper._module[0][cur] == helper._first) {
      break;
    }
  }
  up = cur;
  down = cur;

  Cell* first = get_cell(helper._first);
  ASSERT(first != nullptr, "first node should be pickd");
  Rectangle box({0, 0}, {first->get_width(), first->get_height()});
  std::map<uint8_t, bool> picked;
  picked[helper._first] = true;

  // BFS from cur, in a column
  auto& column = helper._module[0];
  while (picked.size() < helper._cell_num) {
    // up
    while (up > 0 && picked[column[up]] == true) up--;
    if (picked[column[up]] == false) {
      CellType target = get_cell_type(column[up]);
      auto constraint = get_min_constraint_y(column[up], column[up + 1]);
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
    while (down < end - 1 && picked[column[down]] == true) down++;
    if (picked[column[down]] == false) {
      CellType target = get_cell_type(column[down]);
      auto constraint = get_min_constraint_y(column[down - 1], column[down]);
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

Point VCG::get_min_constraint_y(uint8_t id1, uint8_t id2) {
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

Point VCG::get_min_constraint_x(uint8_t id1, uint8_t id2) {
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

void VCG::pick_row(PickHelper& helper) {
  ASSERT(get_max_row(helper._module) == 1, "Module should be row type");

  size_t left, right, cur;  // index
  size_t end = helper._module.size();
  // find cur
  for (cur = 0; cur < end; ++cur) {
    if (helper._module[cur][0] == helper._first) {
      break;
    }
  }
  left = cur;
  right = cur;

  Cell* first = get_cell(helper._first);
  ASSERT(first != nullptr, "first node should be picked");
  Rectangle box({0, 0}, {first->get_width(), first->get_height()});
  std::map<uint8_t, bool> picked;
  picked[helper._first] = true;

  // BFS from cur, in a row
  auto& row = helper._module;
  while (picked.size() < helper._cell_num) {
    // left
    while (left > 0 && picked[row[left][0]] == true) left--;
    if (picked[row[left][0]] == false) {
      CellType target = get_cell_type(row[left][0]);
      auto constraint = get_min_constraint_x(row[left][0], row[left + 1][0]);
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
    while (right < end - 1 && picked[row[right][0]] == true) right++;
    if (picked[row[right][0]] == false) {
      CellType target = get_cell_type(row[right][0]);
      auto constraint = get_min_constraint_x(row[right - 1][0], row[right][0]);
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

void VCG::pick_wheel(PickHelper& helper) {
  for (auto column : helper._module) {
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

void VCG::place() {}

}  // namespace  EDA_CHALLENGE_Q4