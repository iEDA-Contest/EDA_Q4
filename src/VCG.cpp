#include "VCG.hpp"

#include <algorithm>
#include <cmath>

namespace EDA_CHALLENGE_Q4 {

size_t VCG::_gds_file_num = 0;
static size_t max_picks_num = 5;

VCG::VCG(Token_List& tokens) : _cell_man(nullptr), _cst(nullptr),
_last_merge_id(0) {
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
  fill_id_grid();
  init_column_row_index();
  init_pattern_tree();
  // debug
  debug();
}

VCG::~VCG() {
  // before clear _cell_man, we should retrieve all cells
  retrieve_all_cells();
  delete _cell_man;
  _cell_man = nullptr;

  for (auto p : _adj_list) {
    delete p;
  }
  _adj_list.clear();

  for (auto v : _id_grid) {
    v.clear();
  }
  _id_grid.clear();

  _cst = nullptr;  // must not release here!
  
  _place_stack.clear();

  clear_Helper_map(); //

  delete _tree;
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
    g_log << "\n";
  }
  g_log << "------\n";
  g_log.flush();
}

void VCG::show_id_grid() {
  size_t max_row = get_max_row(_id_grid);

  for (size_t row = 0; row < max_row; ++row) {
    g_log << "row[" << row <<"]:";
    for (size_t column = 0; column < _id_grid.size(); ++column) {
      if (row < _id_grid[column].size()) {
        g_log << std::to_string(_id_grid[column][row]) << ", ";
      } else {
        PANIC("Does not fill _id_grid");
      }
    }
    g_log << "\n";
  }
  g_log << "------\n";
  g_log.flush();
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
  // slice2();
  // // slice
  // auto modules = slice();
  // // pick
  // auto helpers = make_pick_helpers(modules);
  // make_helper_map(helpers);
  // place();
  // // record
  // std::map<int, bool> first_try;
  // for (size_t node_id = 1; node_id <= _place_stack.size(); ++node_id) {
  //   auto cell = get_cell(node_id);
  //   assert(cell);
  //   first_try[cell->get_cell_id()] = cell->get_rotation();
  // }
  // // possibility
  // find_possible_combinations();
  // // recover
  // for (auto pair : first_try) {
  //   auto cell = _cell_man->get_cell(pair.first);
  //   assert(cell);
  //   if (cell->get_rotation() != pair.second) {
  //     cell->rotate();
  //   }
  // }
  // place_cells_again();
  // try_legalize();
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

    // make helper
    ModuleHelper* helper = new ModuleHelper();
    helper->_biggest = freq_sorted.rbegin()->second;
    helper->_module = m;
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

/**
 * @brief get two cells' constraint y
 * 
 * 
 * 
 * @param id1 
 * @param id2 
 * @return Point 
 */
Point VCG::get_cst_y(uint8_t id1, uint8_t id2) {
  CellType type1 = get_cell_type(id1);
  CellType type2 = get_cell_type(id2);
  assert(type1 != kCellTypeNull && type2 != kCellTypeNull);

  Point ret;
  static constexpr uint8_t shl = 2;
  switch (type1 << shl | type2) {
    case kCellTypeMem << shl | kCellTypeMem:
      ret._x = _cst->get_cst(kYMM_MIN);
      ret._y = _cst->get_cst(kYMM_MAX);
      break;
    case kCellTypeMem << shl | kCellTypeSoc:
    case kCellTypeSoc << shl | kCellTypeMem:
      ret._x = _cst->get_cst(kYMS_MIN);
      ret._y = _cst->get_cst(kYMS_MAX);
      break;
    case kCellTypeSoc << shl | kCellTypeSoc:
      ret._x = _cst->get_cst(kYSS_MIN);
      ret._y = _cst->get_cst(kYSS_MAX);
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
Point VCG::get_cst_y(uint8_t id) {
  CellType ctype = get_cell_type(id);
  assert(ctype != kCellTypeNull);

  Point ret;
  switch (ctype)
  {
    case kCellTypeMem: 
      ret._x = _cst->get_cst(kYMI_MIN);
      ret._y = _cst->get_cst(kYMI_MAX);
      break;
    case kCellTypeSoc:
      ret._x = _cst->get_cst(kYSI_MIN);
      ret._y = _cst->get_cst(kYSI_MAX);
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
Point VCG::get_cst_x(uint8_t id1, uint8_t id2) {
  CellType type1 = get_cell_type(id1);
  CellType type2 = get_cell_type(id2);
  assert(type1 != kCellTypeNull && type2 != kCellTypeNull);

  Point ret;
  static constexpr uint8_t shl = 2;
  switch (type1 << shl | type2) {
    case kCellTypeMem << shl | kCellTypeMem:
      ret._x = _cst->get_cst(kXMM_MIN);
      ret._y = _cst->get_cst(kXMM_MAX);
      break;
    case kCellTypeMem << shl | kCellTypeSoc:
    case kCellTypeSoc << shl | kCellTypeMem:
      ret._x = _cst->get_cst(kXMS_MIN);
      ret._y = _cst->get_cst(kXMS_MAX);
      break;
    case kCellTypeSoc << shl | kCellTypeSoc:
      ret._x = _cst->get_cst(kXSS_MIN);
      ret._y = _cst->get_cst(kXSS_MAX);
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
Point VCG::get_cst_x(uint8_t id) {
  CellType ctype = get_cell_type(id);
  assert(ctype != kCellTypeNull);

  Point ret;
  switch (ctype)
  {
    case kCellTypeMem: 
      ret._x = _cst->get_cst(kXMI_MIN);
      ret._y = _cst->get_cst(kXMI_MAX);
      break;
    case kCellTypeSoc:
      ret._x = _cst->get_cst(kXSI_MIN);
      ret._y = _cst->get_cst(kXSI_MAX);
      break;
    
    default: assert(0);
  }

  return ret;
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

  auto recommends = _cell_man->choose_cells(false, get_priority(get_node_type(id)));

  Cell* best = nullptr;
  for (auto cell : recommends) {
    do_cell_fits_node(cell, cur);

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

void VCG::gen_GDS() {
#ifndef GDS
  return;
#endif
  std::string fname = "../output/myresult" + std::to_string(_gds_file_num) + ".gds";
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
    gds << "STRNAME " << cell->get_refer() << std::to_string(node->get_id()) << "\n";
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

  auto c3_vec = cal_c3_of_interposer();
  auto c3_x = std::min(c3_vec[0], c3_vec[1]);
  auto c3_y = std::min(c3_vec[2], c3_vec[3]);
  // interposer
  gds << "BGNSTR\n";
  gds << "STRNAME interposer\n";
  gds << "BOUNDARY\n";
  gds << "LAYER 0\n";
  gds << "DATATYPE 0\n";
  gds << "XY\n";
  gds << "0 : 0\n";
  gds << "0 : " << c3_y << "\n";
  gds << c3_x << " : " << c3_y << "\n";
  gds << c3_x << " : 0\n";
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
    gds << "SNAME " << cell->get_refer() << std::to_string(node->get_id()) << "\n";
    gds << "XY " << std::to_string(cell->get_c1()._x) << ": "
        << std::to_string(cell->get_c1()._y) << "\n";  // c1 point
    gds << "ENDEL\n";
  }

  // interposer
  gds << "SREF\n";
  gds << "SNAME interposer\n";
  gds << "XY  0 : 0\n";  // c1 point
  gds << "ENDEL\n";

  // gds end
  gds << "\n\n";
  gds << "ENDSTR\n";
  gds << "ENDLIB\n";

  gds.close();
}

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
  _helper_map[0]->_biggest = 0;
  _helper_map[0]->_type = kMoSingle;
  _helper_map[0]->_module = g0;
  // start node 
  GridType g1(1);
  auto start = get_vertex_num() - 1;
  g1[0].push_back(start);
  _helper_map[start] = new ModuleHelper();
  _helper_map[start]->_biggest = start;
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
      visit_single(get_node(target->_biggest));
      visited[target->_biggest] = true;
      target->_order.push_back(target->_biggest);
      break;

    default: assert(0);
  }
  
  set_module_box(target->_biggest);
  merge_box(target);
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
  target->_order.push_back(first_node->get_id());

  Rectangle box(first_cell->get_c1(), first_cell->get_c3());
  
  try_place(first_node);

  auto last_node = first_node;
  // down -> top 
  while (queue.size()) {
    auto cur_node = queue.top();
    queue.pop();

    auto cell_type = get_cell_type(cur_node->get_id());
    auto constraint = get_cst_y(last_node->get_id(), cur_node->get_id());
    auto bests = _cell_man->choose_cells(false, kDeathCol, cell_type, box,
                                        constraint._x, constraint._y);
    ASSERT(bests.size(), "No cells to pick");
    do_pick_cell(cur_node->get_id(), bests[0]);

    visited[cur_node->get_id()] = true;
    _place_stack.push_back(cur_node->get_id());
    target->_order.push_back(cur_node->get_id());
    
    box._c3._x = std::max(box.get_width(), (int)bests[0]->get_width());
    box._c3._y = box.get_height() + constraint._x + bests[0]->get_height();

    try_place(cur_node);
    last_node = cur_node;
  }

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
    // interposer
    auto constraint = get_cst_y(cur_node->get_id());
    y_range = {constraint._x, constraint._y};
  } else {
    int min_y = 0;
    int max_y = 0;

    for (auto from : cur_node->get_froms()) {
      auto fcell = from->get_cell();
      assert(fcell);
      auto constraint = get_cst_y(cur_node->get_id(), from->get_id());
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

Point VCG::cal_x_range(VCGNode* cur_node, float flex) {
  assert(0 <= flex && flex <= 1);

  Point x_range(0, 0);
  if (cur_node == nullptr) return x_range;

  if (cur_node->is_first_column()){
    // interposer
    auto constraint = get_cst_x(cur_node->get_id());
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
      boxs.insert(helper->_biggest);
    }  
    for(auto id : boxs) {
      auto left_helper = _helper_map[id];
      auto merge = left_helper->get_merge();
      if (merge) {
        min_x = std::max(min_x, merge->_box->_c3._x + 1);
      } else {
        min_x = std::max(min_x, left_helper->_box->_c3._x + 1);
      }
    }
    
    // visual y overlap constraint
    auto cur_cell = cur_node->get_cell();
    assert(cur_cell);
    for (auto id : _place_stack) {
      auto node = get_node(id);
      auto cell = get_cell(id);
      assert(node && cell);
      if (!is_overlap_y(cell, cur_cell, false) ||
          node->get_max_column_index() >= cur_node->get_max_column_index() )
          continue;

      auto constraint = get_cst_x(cell->get_node_id(), cur_node->get_id());
      min_x = std::max(min_x, cell->get_c3()._x);
      if (node->get_max_column_index() == cur_node->get_column_index() - 1) {
        min_x = std::max(min_x, cell->get_c3()._x + constraint._x);
        max_x = max_x == 0 ? cell->get_c3()._x + constraint._y :
                             std::min(max_x, cell->get_c3()._x + constraint._y);
      }
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
  if (id == 0 || id == get_vertex_num() - 1) return;
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

  // death
  auto cell_area = _cell_man->cal_cell_area(get_cell_ids(helper));
  helper->set_death(1 - 1.0 * cell_area / box->get_area());

  // debug
  g_log << "helper[" << std::to_string(id) << "].box = {(" << 
  box->_c1._x << "," << box->_c1._y << "), (" <<
  box->_c3._x << "," << box->_c3._y << ")}" << "\n";
}

std::vector<Cell*> VCG::get_left_overlap_y_cells(VCGNode* cur_node) {
  assert(cur_node);
  auto cur_cell = cur_node->get_cell();
  assert(cur_cell);

  auto placed_cells = get_colomn_cells(cur_node->get_column_index() - 1);
  // remain cells that constitute constraint
  placed_cells.erase(std::remove_if(
    placed_cells.begin(), placed_cells.end(), [this, cur_cell](auto cell) {
      return !is_overlap_y(cell, cur_cell, false);
    }
  ), placed_cells.end());
  return placed_cells;
}

 void VCG::visit_single(VCGNode* target) {
  assert( target);

  if (target->get_type() == kVCG_START || target->get_type() == kVCG_END) return;
  auto cell = get_cell_fits_min_ratioWH(target->get_id());
  do_pick_cell(target->get_id(), cell);
  try_place(target);

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
  target->_order.push_back(first_node->get_id());

  Rectangle box(first_cell->get_c1(), first_cell->get_c3());
  
  try_place(first_node);
  
  auto last_node = first_node;
  // left -> right
  while (queue.size()) {
    auto cur_node = queue.top();
    queue.pop();

    auto cell_type = get_cell_type(cur_node->get_id());
    auto constraint = get_cst_x(last_node->get_id(), cur_node->get_id());
    auto bests = _cell_man->choose_cells(false, kDeathRow, cell_type, box, 
                                              constraint._x, constraint._y);
    ASSERT(bests.size(), "No cells to pick");
    do_pick_cell(cur_node->get_id(), bests[0]);

    visited[cur_node->get_id()] = true;
    _place_stack.push_back(cur_node->get_id());
    target->_order.push_back(cur_node->get_id());

    box._c3._x = box.get_width() + constraint._x + bests[0]->get_width();
    box._c3._y = std::max(box.get_height(), (int)bests[0]->get_height());

    try_place(cur_node);

    last_node = cur_node;
  }

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
    target->_order.push_back(cur_node->get_id());

    try_place(cur_node);
  }
  
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

void VCG::try_place(VCGNode* target, float flex_x, float flex_y) {
  assert(target);
  auto tar_cell = target->get_cell();
  assert(tar_cell);

  auto y_range = cal_y_range(target, flex_y);
  tar_cell->set_y_range(y_range);
  tar_cell->set_y(y_range._x + (1 - flex_y) * (y_range._y - y_range._x));

  auto x_range = cal_x_range(target, flex_x);
  tar_cell->set_x_range(x_range);
  tar_cell->set_x(x_range._x + (1 - flex_x)* (x_range._y - x_range._x));

  // debug
  g_log << tar_cell->get_refer() << 
  std::to_string(tar_cell->get_node_id()) << "_" << tar_cell->get_cell_id() 
  << " (" << tar_cell->get_x() << "," << tar_cell->get_y() << ")\n";
}

void VCG::merge_box(ModuleHelper* helper) {
  assert(helper);

  if (_place_stack.size() <= helper->_order.size()) return;

  auto last_merge_id = _last_merge_id == 0 ? _place_stack[0] : _last_merge_id;

  // _place_stack index
  size_t index = 0;
  for (; index < _place_stack.size(); ++index) {
    if (_place_stack[index] == last_merge_id) {
      break;
    }
  }

  for (;index < _place_stack.size();) {
    assert(_helper_map.count(_place_stack[index]));
    auto helper = _helper_map[_place_stack[index]];
    if (is_tos_placed(helper) ) {
      // vertical merge
      merge_all_tos(helper);
      merge_with_tos(helper);
    } else if (is_right_module_fusible(helper)) {
      merge_with_right(helper);
    }

    index += helper->_order.size();
  }

}

bool VCG::is_tos_placed(ModuleHelper* helper) {
  assert(helper);

  bool ret = true;
  for (auto& to_id : get_tos(helper)) {
    if (!is_placed(to_id)) {
      ret = false;
      break;
    }
  }

  return ret;
}

bool VCG::is_placed(uint8_t node_id) {
  bool ret = false;

  for (auto id : _place_stack) {
    if (id == node_id) {
      ret = true;
      break;
    }
  }

  return ret;
}

void VCG::merge_all_tos(ModuleHelper* target) {
  assert(target);

  auto tos = get_tos(target);
  std::set<uint8_t> tos_helper;
  for (auto to_id : tos) {
    assert(_helper_map.count(to_id));
    auto helper = _helper_map[to_id];
    if (tos_helper.count(helper->_biggest) == 0) {
      tos_helper.insert(helper->_biggest);
    }
  }

  if (tos_helper.size() <= 1) return;

  assert(_helper_map.count(*tos_helper.begin()));
  auto first = _helper_map[*tos_helper.begin()];
  auto merge = new MergeBox;
  merge->_pre = first;

  merge->_box->_c1 = first->_box->_c1;
  merge->_box->_c3 = first->_box->_c3;

  for (auto id : tos_helper) {
    assert(_helper_map.count(id));
    auto helper = _helper_map[id];

    merge->_box->_c1._x = std::min(merge->_box->_c1._x, helper->_box->_c1._x);
    merge->_box->_c1._y = std::min(merge->_box->_c1._y, helper->_box->_c1._y);
    merge->_box->_c3._x = std::max(merge->_box->_c3._x, helper->_box->_c3._x);
    merge->_box->_c3._y = std::max(merge->_box->_c3._y, helper->_box->_c3._y);
  }

  for (auto id : tos_helper) {
    assert(_helper_map.count(id));
    auto helper = _helper_map[id];

    helper->insert_merge(merge);
  }
  _merges.push_back(merge);

  // debug
  g_log << "merge some: ";
  for (auto id : tos_helper) {
    g_log << std::to_string(id) << " + ";
  }
  g_log << "\n";
  g_log.flush();
}

void VCG::merge_with_tos(ModuleHelper* helper) {
  assert(helper);

  auto merge = new MergeBox();
  merge->_pre = helper;

  auto last_merge = helper->get_merge();
  if (last_merge == nullptr) {
    merge->_box->_c1 = helper->_box->_c1;
    merge->_box->_c3 = helper->_box->_c3;
  } else { 
    merge->_box->_c1 = last_merge->_box->_c1;
    merge->_box->_c3 = last_merge->_box->_c3;
  }

  auto tos = get_tos(helper);
  assert(tos.size());
  auto to_id = *tos.begin();
  assert(_helper_map.count(to_id));
  auto to_helper = _helper_map[to_id];
  auto to_box = to_helper->get_merge();
  if (to_box == nullptr) {
    merge->_box->_c1._x = std::min(merge->_box->_c1._x, to_helper->_box->_c1._x);
    merge->_box->_c1._y = std::min(merge->_box->_c1._y, to_helper->_box->_c1._y);
    merge->_box->_c3._x = std::max(merge->_box->_c3._x, to_helper->_box->_c3._x);
    merge->_box->_c3._y = std::max(merge->_box->_c3._y, to_helper->_box->_c3._y);
  } else {
    merge->_box->_c1._x = std::min(merge->_box->_c1._x, to_box->_box->_c1._x);
    merge->_box->_c1._y = std::min(merge->_box->_c1._y, to_box->_box->_c1._y);
    merge->_box->_c3._x = std::max(merge->_box->_c3._x, to_box->_box->_c3._x);
    merge->_box->_c3._y = std::max(merge->_box->_c3._y, to_box->_box->_c3._y);
  }

  helper->insert_merge(merge);
  to_helper->insert_merge(merge);
  _merges.push_back(merge);
  _last_merge_id = to_helper->_biggest;

  // debug
  g_log << "merge down-top: " << std::to_string(helper->_biggest)
        << " + " << std::to_string(to_helper->_biggest) << "\n";
  g_log.flush();
}

std::set<uint8_t> VCG::get_tos(ModuleHelper* helper) {
  assert(helper);

  std::set<uint8_t> ret;
  for (auto column : helper->_module) {
    for (auto id : column) {
      auto node = get_node(id);
      if(node == nullptr) continue;

      for (auto to: node->get_tos()) {
        ret.insert(to->get_id());
      }
    }
  }

  return ret;
}

std::set<uint8_t> VCG::get_right_id_set(ModuleHelper* helper) {
  assert(helper);

  std::set<uint8_t> cur_right_id_set;
  for (auto id : helper->_module[helper->_module.size() - 1]) {
    cur_right_id_set.insert(id);
  }

  std::set<uint8_t> right_id_set;
  for (auto id : cur_right_id_set) {
    auto node = get_node(id);
    assert(node);
    auto col = node->get_max_column_index();
    auto row_min = node->get_row_index();
    auto row_max = node->get_max_row_index();
    for (auto row = row_min; row <= row_max; ++row) {
      if ((size_t)col + 1 < _id_grid.size() 
       && (size_t)row < _id_grid[col + 1].size()) {
        right_id_set.insert(_id_grid[col + 1][row]);
      }
    }
  }

  return right_id_set;
}

bool VCG::is_right_module_fusible(ModuleHelper* helper) {
  assert(helper);

  auto right_ids = get_right_id_set(helper);
  if (right_ids.size() == 0) return false;

  std::set<uint8_t> check_id_set;
  for (auto id : right_ids) {
    assert(_helper_map.count(id));
    auto ids = get_ids_in_helper(_helper_map[id]);
    for (auto id_in_helper : ids) {
      check_id_set.insert(id_in_helper);
    }
  }

  // bottom-right module should be merged before
  auto pre_merge = helper->get_merge();
  while (pre_merge) {
    auto bottom_rights = get_right_id_set(pre_merge->_pre);
    for (auto id : bottom_rights) {
      // bottom-right should consider those in check_id_set
      if (check_id_set.count(id)) continue;  

      assert(_helper_map.count(id));
      if (_helper_map[id]->get_merge() == nullptr) {
        return false;
      }
    }
    pre_merge = pre_merge->_smaller;
  }

  // right module should be palced
  bool ret = true;
  for (auto id : check_id_set) {
    if (!is_placed(id)) {
      ret = false;
      break;
    }
  }


  return ret;
}

void VCG::merge_with_right(ModuleHelper* helper) {
  assert(helper);

  auto right_id_set = get_right_id_set(helper);

  std::set<uint8_t> right_helper_set;
  for (auto id : right_id_set) {
    if (_helper_map.count(id)) {
      right_helper_set.insert(_helper_map[id]->_biggest);
    }
  }

  if (right_helper_set.size() < 1) return;

  auto merge = new MergeBox();
  merge->_pre = helper;

  auto last_merge = helper->get_merge();
  if (last_merge == nullptr) {
    merge->_box->_c1 = helper->_box->_c1;
    merge->_box->_c3 = helper->_box->_c3;
  } else {
    merge->_box->_c1 = last_merge->_box->_c1;
    merge->_box->_c3 = last_merge->_box->_c3;
  }

  for (auto id : right_helper_set) {
    auto right = _helper_map[id];

    merge->_box->_c1._x = std::min(merge->_box->_c1._x, right->_box->_c1._x);
    merge->_box->_c1._y = std::min(merge->_box->_c1._y, right->_box->_c1._y);
    merge->_box->_c3._x = std::max(merge->_box->_c3._x, right->_box->_c3._x);
    merge->_box->_c3._y = std::max(merge->_box->_c3._y, right->_box->_c3._y);
  }

  helper->insert_merge(merge);
  for (auto id : right_helper_set) {
    auto right = _helper_map[id];
    right->insert_merge(merge);
  }
  _merges.push_back(merge);
  _last_merge_id = *right_helper_set.begin();

  // debug
  g_log << "merge left-right: " << std::to_string(helper->_biggest)
        << " + " << std::to_string(*right_helper_set.begin()) << "\n";
  g_log.flush(); 
}

void VCG::place_cells_again() {
  // init
  set_Helpers_merge_null();
  _last_merge_id = 0;
  g_log << "\n--- updates cells position ---\n";
  //
  std::vector<uint8_t> place_stack;
  place_stack.swap(_place_stack);
  for (size_t index = 0; index < place_stack.size();) {
    assert(_helper_map.count(place_stack[index]));
    auto helper = _helper_map[place_stack[index]];
    helper->set_box_init();

    for (size_t order = 0; order < helper->_order.size(); ++order) {
      auto node = get_node(place_stack[index]);
      try_place(node);
      _place_stack.push_back(node->get_id());

      ++index;
    }

    set_module_box(helper->_biggest);
    merge_box(helper);
  }

}

std::vector<int> VCG::cal_c3_of_interposer() {
  int min_y = 0;
  int max_y = 0;
  // top
  for (auto column : _id_grid) {
    auto cell = get_cell(column[0]);
    assert(cell);
    auto constraint = get_cst_y(cell->get_node_id());
    min_y = std::max(min_y, cell->get_c3()._y + constraint._x);
    max_y = max_y == 0 ?
            cell->get_c3()._y + constraint._y :
            std::min(max_y, cell->get_c3()._y + constraint._y);
  }

  // right
  int min_x = 0;
  int max_x = 0;
  if (_id_grid.size()) {
    auto last_col = _id_grid[_id_grid.size() - 1];
    for (size_t row = 0; row < last_col.size(); ++row) {
      auto cell = get_cell(last_col[row]);
      assert(cell);
      auto constraint = get_cst_x(cell->get_node_id());
      min_x = std::max(min_x, cell->get_c3()._x + constraint._x);
      max_x = max_x == 0 ?
              cell->get_c3()._x + constraint._y :
              std::min(max_x, cell->get_c3()._x + constraint._y);
    }
  }

  // debug
  g_log << "\n";
  g_log << "##Interposer_x " << min_x << " ~ " << max_x << "\n";
  g_log << "##Interposer_y " << min_y << " ~ " << max_y << "\n";
  g_log.flush();

  return {min_x, max_x, min_y, max_y};
}

ModuleHelper::~ModuleHelper()  {
  for (auto v : _module) {
    v.clear();
  }
  _module.clear();

  delete _box;
  _box = nullptr;
  _merge = nullptr; // Must Not release here

  clear_picks();
}

void VCG::find_possible_combinations() {
  for (size_t order = 0; order < _place_stack.size();) {
    auto id = _place_stack[order];
    assert(_helper_map.count(id));
    auto helper = _helper_map[id];

    build_picks(helper);

    order += helper->_order.size();
  }
}

void VCG::build_picks(ModuleHelper* helper) {
  assert(helper);

  if (helper->_type == kMoWheel) return;

  // first
  build_first(helper);

  // left
  for (size_t order = 1; order < helper->_order.size(); ++order) {
    switch (helper->_type)
    {
      case kMoColumn:
        build_picks_col(helper);
        break;
      case kMoRow:
        build_picks_row(helper);
        break;
      case kMoWheel:
        break;
      
      case kMoSingle: PANIC("Module contains more than one cell"); 
      default: assert(0);
    }
  }
  
}

void VCG::build_first(ModuleHelper* helper) {
  assert(helper && helper->_order.size());

  std::set<int> refer_set;
  for (auto cell :
        _cell_man->choose_cells(true, 
          get_priority(get_node_type(helper->_order[0])))
      ) {
    
    if (refer_set.count(cell->get_refer_id())) continue;
    
    auto term = new PickTerm(cell);
    helper->_picks.push(term);

    if (!cell->is_square()) {
      cell->rotate();
      auto term_rotate = new PickTerm(cell);
      helper->_picks.push(term_rotate);
    }

    refer_set.insert(cell->get_refer_id());
  }
}

void VCG::build_picks_col(ModuleHelper* helper) {
  assert(helper && helper->_type == kMoColumn);

  Prio_Pick round_terms;
  while (helper->_picks.size()) {
    auto term = helper->_picks.top();
    helper->_picks.pop();

    auto has_picked_num = term->get_picks_num();
    assert(has_picked_num > 0 && helper->_order.size() > has_picked_num);
    auto last_node_id = helper->_order[has_picked_num - 1];
    auto tar_node_id = helper->_order[has_picked_num];
    auto tar_cell_type = get_cell_type(tar_node_id);
    auto constraint = get_cst_y(last_node_id, tar_node_id);
    auto bests = _cell_man->choose_cells(true, kDeathCol, tar_cell_type, 
                                        term->get_box(), constraint._x, constraint._y);
    ASSERT(bests.size(), "No Cells to pick");
  
    std::set<int> refer_set;
    for (auto cell : bests) {
      if (term->is_picked(cell) ||
          refer_set.count(cell->get_refer_id() << (cell->get_rotation() ? 0 : 1))
         ) continue;
        
      auto new_term = new PickTerm(*term);
      new_term->insert(cell);
      refer_set.insert(cell->get_refer_id() << (cell->get_rotation() ? 0 : 1));

      Rectangle box(*term->get_box());
      box._c3._x = std::max(box.get_width(), (int)cell->get_width());
      box._c3._y = box.get_height() + constraint._x + cell->get_height();
      new_term->set_box(box);
      new_term->set_death(1 -
        1.0 *  _cell_man->cal_cell_area(new_term->get_picks()) / box.get_area());
      
      if (round_terms.size() < max_picks_num) {
        round_terms.push(new_term);
      } else {

        auto worst_term = round_terms.top();
        auto worst_death = worst_term->get_death();
        auto new_death = new_term->get_death();
        if ( (worst_death > new_death) ||
             (worst_death == new_death &&
                worst_term->get_box()->get_area() > new_term->get_box()->get_area()
             )) {
          // new_term has smaller death or area
          round_terms.pop();
          round_terms.push(new_term);
          delete worst_term;
        } else {
          // new_term has bigger death or area
          delete new_term;
        }

      }

    } // end for: bests

    delete term;
  } // end while: helper->_picks

  helper->_picks.swap(round_terms);
}

void VCG::build_picks_row(ModuleHelper* helper) {
  assert(helper && helper->_type == kMoRow);

  Prio_Pick round_terms;
  while (helper->_picks.size()) {
    auto term = helper->_picks.top();
    helper->_picks.pop();

    auto has_picked_num = term->get_picks_num();
    assert(has_picked_num > 0 && helper->_order.size() > has_picked_num);
    auto last_node_id = helper->_order[has_picked_num - 1];
    auto tar_node_id = helper->_order[has_picked_num];
    auto tar_cell_type = get_cell_type(tar_node_id);
    auto constraint = get_cst_x(last_node_id, tar_node_id);
    auto bests = _cell_man->choose_cells(true, kDeathRow, tar_cell_type, 
                                        term->get_box(), constraint._x, constraint._y);
    ASSERT(bests.size(), "No Cells to pick");

    std::set<int> refer_set;
    for (auto cell : bests) {
      if (term->is_picked(cell) ||
          refer_set.count(cell->get_refer_id() << (cell->get_rotation() ? 0 : 1))
         ) continue;
        
      auto new_term = new PickTerm(*term);
      new_term->insert(cell);
      refer_set.insert(cell->get_refer_id() << (cell->get_rotation() ? 0 : 1));

      Rectangle box(*term->get_box());
      box._c3._x = box.get_width() + constraint._x + cell->get_width();
      box._c3._y = std::max(box.get_height(), (int)cell->get_height());
      new_term->set_box(box);
      new_term->set_death(1 -
        1.0 *  _cell_man->cal_cell_area(new_term->get_picks()) / box.get_area());
      
      if (round_terms.size() < max_picks_num) {
        round_terms.push(new_term);
      } else {
        auto worst_term = round_terms.top();
        auto worst_death = worst_term->get_death();
        auto new_death = new_term->get_death();

        if ( (worst_death > new_death) ||
             (worst_death == new_death &&
                worst_term->get_box()->get_area() > new_term->get_box()->get_area()
             )) {
          // new_term has smaller death or area
          round_terms.pop();
          round_terms.push(new_term);
          delete worst_term;
        } else {
          // new_term has bigger death or area
          delete new_term;
        }

      } // end if else: max_picks_num

    } // end for: bests

    delete term;
  } // end while: helper->_picks

  helper->_picks.swap(round_terms);
}

void VCG::try_legalize() {
  float cost = 1;
  while (cost >= 1) {
    gen_GDS(); 
    // auto bad_flex_cells = check_range_invalid();
    auto box_range = cal_c3_of_interposer();
    auto unpass_id = fail_cst_id();
    if (unpass_id != 0) {
      TODO();
    } else if (box_range[0] > box_range[1]) {
      fix_x_range(box_range[1], box_range[0] - box_range[1]);
    } else if (box_range[2] > box_range[3]) {
      fix_y_range(box_range[3], box_range[2] - box_range[3]);
    } else {
      cost = 0;
    }
  }
}

void VCG::fill_id_grid() {
  auto max_row = get_max_row(_id_grid);
  for(auto& column : _id_grid) {
    for (size_t row = column.size(); row < max_row; ++row) {
      column.push_back(column[row - 1]);
    }
  }
}

std::vector<int> VCG::check_range_invalid() {
  std::vector<int> ret;

  for (size_t order = 0; order < _place_stack.size(); ++order) {
    auto node_id = _place_stack[order];
    auto cell = get_cell(node_id);
    assert(cell);

    auto x_range = cell->get_x_range();
    auto y_range = cell->get_y_range();
    // debug
    g_log << cell->get_refer() << std::to_string(cell->get_node_id()) << ":\n";
    g_log << "x_range:" << x_range._x << "~" << x_range._y << "\n";
    g_log << "y_range:" << y_range._x << "~" << y_range._y << "\n";
    if (x_range._x <= x_range._y && 
        y_range._x <= y_range._y) {
      g_log << "Valid\n";
    } else {
      g_log << "!!!!! Invalid !!!!!\n";
      ret.push_back(cell->get_cell_id());
    }
    g_log.flush();
  }

  return ret;
}

void VCG::fix_x_range(int min_x, int to_fix) {
  std::set<uint8_t> rights;
  for (auto right_id : _id_grid[_id_grid.size() - 1]) {
    rights.insert(right_id);
  }

  auto last_col = _id_grid[_id_grid.size() - 1];
  for (size_t row = 0; row < last_col.size(); ++row) {
    auto cell = get_cell(last_col[row]);
    assert(cell);
  
    auto constraint = get_cst_x(cell->get_node_id());
    if (cell->get_c3()._x + constraint._x < min_x) {
      auto x_range = cell->get_x_range();
      if (x_range._y - x_range._x >= to_fix) {
        cell->set_x(cell->get_x() + to_fix);
        // float flex_x = 1 - (cell->get_x() - x_range._x) / (x_range._y - x_range._x);
        break;
      } else {
        TODO();
      }
    }
  }
  
}

void VCG::gen_result() {
  std::fstream result("../output/result.txt", std::ios::app | std::ios::out);
  assert(result.is_open());

  std::string pattern = _cst->get_pattern();
  std::string::size_type pos = 0;
  const std::string sub_str = "&#60;";
  while ((pos = pattern.find(sub_str)) != std::string::npos) {
    pattern.replace(pos, sub_str.length(), "<");
  }

  if (pattern[pattern.size() - 1] == ' ') {
    pattern.replace(pattern.size() - 1, 1, "\0");
  }
  result << "PATTERN \"" << pattern << "\" ";

  auto box_c3 = cal_c3_of_interposer();
  if (box_c3[0] > box_c3[1]
   || box_c3[2] > box_c3[3]) {
     result << "NA\n";
  } else {
    result << box_c3[0] << " * " << box_c3[2] << "\n";
    for (auto node : _adj_list) {
      auto type = node->get_type();
      if ( type == kVCG_START || type == kVCG_END) continue;

      auto cell = node->get_cell();
      assert(cell);

      result << cell->get_refer() << 
      "(" << cell->get_x() << ", " << cell->get_y() << ") " <<
      "R" << (cell->get_rotation() ? "90" : "0") << "\n";
    }
  } 
  result << "\n";

  result.close();
}

bool VCG::is_froms_cst_meet(uint8_t node_id) {
  auto node = get_node(node_id);
  assert(node);
  auto cell = node->get_cell();
  assert(cell);
  auto cell_c1 = cell->get_c1();

  bool ret = true;
  for (auto from : node->get_froms()) {
    auto from_cell = from->get_cell();
    if (from_cell == nullptr) continue;

    if (is_overlap_x(node->get_cell(), from->get_cell(), false)) {
      auto cst = get_cst_y(node_id, from->get_id());
      auto f_c3 = from_cell->get_c3();
      if (f_c3._y + cst._x <= cell_c1._y && cell_c1._y <= f_c3._y + cst._y) {
        continue;
      }
    } 

    ret = false;
    break;
    
  }  

  return ret;
}

bool VCG::is_tos_cst_meet(uint8_t node_id) {
  auto node = get_node(node_id);
  assert(node);
  auto cell = node->get_cell();
  assert(cell);
  auto cell_c3 = cell->get_c3();

  bool ret = true;
  for (auto to : node->get_tos()) {
    auto to_cell = to->get_cell();
    if (to_cell == nullptr) continue;

    if (is_overlap_x(node->get_cell(), to->get_cell(), false)) {
      auto cst = get_cst_y(node_id, to->get_id());
      auto to_c1 = to_cell->get_c1();
      if (cell_c3._y + cst._x <= to_c1._y && cell_c3._y + cst._y) {
        continue;
      }
    }

    ret = false;
    break;
  }

  return ret;
}

uint8_t VCG::fail_cst_id() {
  for (auto id : _place_stack) {
    if (!is_froms_cst_meet(id)
     || !is_tos_cst_meet(id)
     || !is_box_cst_meet(id) 
     )
      return id;
  }

  return 0;
}

bool VCG::is_box_cst_meet(uint8_t node_id) {
  auto node_check = get_node(node_id);
  assert(node_check);
  auto cell_check = node_check->get_cell();
  assert(cell_check);

  bool ret = true;
  for (auto id : _place_stack) {
    if (id == node_id) break;

    auto node = get_node(id);
    assert(node);
    auto cell = node->get_cell();
    assert(cell);

    auto cst_x = get_cst_x(node_id, id);
    auto cst_y = get_cst_y(node_id, id);
    Rectangle small;
    small._c1._x = cell_check->get_x() - cst_x._x;
    small._c1._y = cell_check->get_y() - cst_y._x;
    small._c3._x = cell_check->get_c3()._x + cst_x._x;
    small._c3._y = cell_check->get_c3()._y + cst_y._x;

    if (!is_overlap(small, cell->get_box(), false)) {
      continue;
    } else if (!is_overlap_y(cell_check, cell, false)) {
      continue;
    }

    ret = false;
    break;
  }

  return ret;
}

void VCG::fix_y_range(int min_y, int to_fix) {
  auto end = get_node(0);
  assert(end);
  std::set<int> out_of_box;
  std::set<int> inside_box;
  for (auto top : end->get_froms()) {
    auto cell = top->get_cell();
    assert(cell);

    if (cell->get_c3()._y >= min_y) {
      out_of_box.insert(cell->get_cell_id());
    } else {
      inside_box.insert(cell->get_cell_id());
    }
  }

  if (out_of_box.size() > inside_box.size()) {
    for (auto cell_id : inside_box) {
      auto cell = _cell_man->get_cell(cell_id);
      assert(cell);

      auto cell_range_y  = cell->get_y_range();
      if (min_y + to_fix <= cell_range_y._y + cell->get_height()) {
        cell->set_y(min_y + to_fix);
      } else {
        // assert(_helper_map.count(cell->get_node_id()));
        // auto helper = _helper_map[cell->get_node_id()];
        // for (auto id : helper->_order) {
        //   // auto h_cell = get_cell(id);
          
        // }
      }
    }
  } else {
    TODO();
  }
}

// void VCG::slice2() {
//   std::queue<GridType> queue;
//   queue.push(_id_grid);

//   std::queue<GridType> tmp;
//   std::queue<GridType> min_que;
//   while (queue.size()) {
//     auto module = queue.front();
//     queue.pop();

//     auto smallers = slice_module(module);
//     if (smallers.size() == 1) {
//       min_que.push(smallers.front());
//       smallers.pop();
//     } else {
//       while (smallers.size()) {
//         tmp.push(smallers.front());
//         smallers.pop();
//       }

//       if (queue.size() == 0 && tmp.size()) {
//         queue.swap(tmp);
//       }
//     }
//   }
// }

// /**
//  * @brief if there exists one vertical division line, 
//  *        cut along it only once. 
//  * 
//  * @return std::queue<GridType> size = 1 means failure, while 2 success
//  */
// std::queue<GridType> VCG::slice_vertical(GridType& grid) {
//   ASSERT(grid.size() && grid[0].size(), "Grid data polluted");
//   size_t max_row = grid[0].size();

//   std::queue<GridType> result;

//   if (grid.size() >= 1) {
//     // grid.size() >= 1

//     size_t col;
//     for (col = 0; col < grid.size() - 1; ++col) {
//       size_t row;
//       for (row = 0; row < max_row; ++row) {
//         ASSERT(grid[col].size() >= row + 1 
//             || grid[col + 1].size() >= row + 1,
//               "Fill grid first");

//         if (grid[col][row] == grid[col + 1][row]) {
//           break;
//         }
//       }

//       if (row == max_row) {
//         break;
//       }
//     }

//     if (col < grid.size()) {
//       GridType left;
//       for (size_t col_left = 0; col_left <= col; ++col_left) {
//         left.push_back(grid[col_left]);
//       }
//       result.push(left);
      
//       if (col + 1 < grid.size()) {
//         GridType right;
//         for (col = col + 1; col < grid.size(); ++col) {
//           right.push_back(grid[col]);
//         }
//         result.push(right);
//       }
//     }

//   } else {
//     // grid.size() == 1
//     result.push(grid);
//   }

//   return result;
// }

// /**
//  * @brief if there exists one horizontal division line, 
//  *        cut along it only once. 
//  * 
//  * @return std::queue<GridType> size = 1 means failure, while 2 success
//  */
// std::queue<GridType> VCG::slice_horizontal(GridType& grid) {
//   ASSERT(grid.size() && grid[0].size(), "Grid data polluted");
//   size_t max_row = grid[0].size();

//   size_t row = 0;
//   // for ()
// }

// std::queue<GridType> VCG::slice_module(GridType& grid) {
//   std::queue<GridType> ret;
//   ret = slice_vertical(grid);
//   if (ret.size() == 1) {
//     ret.pop();
//     ret = slice_horizontal(grid);
//   }

//   return ret;
// }

void VCG::init_pattern_tree() {
  auto map = make_id_type_map();
  _tree = new PatternTree(_id_grid, map);
}

PatternTree::PatternTree(GridType& grid, std::map<uint8_t, VCGNodeType>& map) {
  slice(grid, map);
  debug_show_pt_grid_map();
}

void PatternTree::slice(const GridType& grid, std::map<uint8_t, VCGNodeType>& map) {
  std::queue<std::pair<int, GridType>> queue;
  queue.push({0, grid});
  bool vertical;

  while (queue.size()) {
    auto pair = queue.front();
    queue.pop();

    auto smaller = slice_module(pair.second, vertical);
    if (smaller.size() == 1) {
      insert_leaves(pair.first, pair.second, map);
    } else {
      auto pt_node = new PTNode(_node_map.size(),
                     vertical ? kPTVertical : KPTHorizontal);
      ASSERT(_node_map.count(pt_node->get_pt_id()) == 0,
            "Have existed pt_id = %d", pt_node->get_pt_id());
      _node_map[pt_node->get_pt_id()] = pt_node;
      
      ASSERT(_node_map.count(pair.first), "Parent disappear");
      auto parent = _node_map[pair.first];
      parent->insert_child(pt_node);
      pt_node->set_parent(parent);

      while(smaller.size()) {
        queue.push({pt_node->get_pt_id() ,smaller.front()});
        smaller.pop();
      }

      // debug
      debug_create_node(pt_node);
    }
  }
}

std::queue<GridType> PatternTree::slice_module(const GridType& grid, bool& vertical) {
  std::queue<GridType> ret;
  ret = slice_vertical(grid);
  vertical = true;
  if (ret.size() == 1) {
    ret.pop();
    ret = slice_horizontal(grid);
    vertical = false;
  }

  return ret;
}

std::queue<GridType> PatternTree::slice_vertical(const GridType& grid) {
  ASSERT(grid.size() && grid[0].size(), "Grid data polluted");
  size_t max_row = grid[0].size();

  std::queue<GridType> result;

  if (grid.size() > 1) {
    // grid.size() > 1

    size_t col;
    for (col = 0; col < grid.size() - 1; ++col) {
      size_t row;
      for (row = 0; row < max_row; ++row) {
        ASSERT(grid[col].size() >= row + 1 
            || grid[col + 1].size() >= row + 1,
              "Fill grid first");

        if (grid[col][row] == grid[col + 1][row]) {
          break;
        }
      }

      if (row == max_row) {
        break;
      }
    }

    
    GridType left;
    for (size_t col_left = 0; col_left <= col; ++col_left) {
      left.push_back(grid[col_left]);
    }
    result.push(left);
    
    if (col + 1 < grid.size()) {
      GridType right;
      for (col = col + 1; col < grid.size(); ++col) {
        right.push_back(grid[col]);
      }
      result.push(right);
    }
    

  } else {
    // grid.size() == 1
    result.push(grid);
  }

  return result;
}

std::queue<GridType> PatternTree::slice_horizontal(const GridType& grid) {
  ASSERT(grid.size() && grid[0].size(), "Grid data polluted");
  size_t max_row = grid[0].size();

  std::queue<GridType> result;

  if (max_row > 1) {
    // max_row > 1

    size_t row;
    for (row = 0; row < max_row - 1; ++row) {
      size_t col;
      for (col = 0; col < grid.size(); ++col) {
        ASSERT(grid[col].size() >= row + 1 
              || grid[col + 1].size() >= row + 1,
                "Fill grid first");
        
        if (grid[col][row] == grid[col][row + 1]) {
          break;
        }
      }

      if (col == grid.size()) {
        break;
      }
    }

    
    GridType down;
    for (auto column : grid) {
      if (column.begin() + row + 1 < column.begin() + max_row) {
        std::vector<uint8_t> col_vec(column.begin() + row + 1, column.begin() + max_row);
        down.push_back(col_vec);
      }
    }
    if (down.size()) {
      result.push(down);
    }
    
    GridType top;
    for (auto column : grid) {
      if (column.begin() < column.begin() + row + 1) {
        std::vector<uint8_t> col_vec(column.begin(), column.begin() + row + 1);
        top.push_back(col_vec);
      }
    }
    if (top.size()) {
      result.push(top);
    }

  } else {
    // max_row == 1
    result.push(grid);
  }

  return result;
}

void PatternTree::insert_leaves( int parent_pt_id,
                    const GridType& grid, 
                    std::map<uint8_t, VCGNodeType>& map) {
  auto order = get_topological_sort(grid);

  if (order.size() == 1) {
    
    ASSERT(map.count(order[0]), "Can't find map between gird_id(%hhu) and VCGNodeType", order[0]);
    auto pt_node = new PTNode(_node_map.size(), get_pt_type(map[order[0]]));
    ASSERT(_node_map.count(pt_node->get_pt_id()) == 0,
          "Have existed pt_id = %d", pt_node->get_pt_id());
    _node_map[pt_node->get_pt_id()] = pt_node;
    
    ASSERT(_node_map.count(parent_pt_id), "Parent disappear");
    auto parent = _node_map[parent_pt_id];
    parent->insert_child(pt_node);
    pt_node->set_parent(parent);

    ASSERT(_pt_grid_map.count(pt_node->get_pt_id()) == 0,
           "_pt_grid_map data polluted");
    _pt_grid_map[pt_node->get_pt_id()] = order[0];

    // debug
    debug_create_node(pt_node);

  } else {

    auto pt_wheel = new PTNode(_node_map.size(), kPTWheel);
    ASSERT(_node_map.count(pt_wheel->get_pt_id()) == 0,
    "Have existed pt_id = %d", pt_wheel->get_pt_id());
    _node_map[pt_wheel->get_pt_id()] = pt_wheel;

    ASSERT(_node_map.count(parent_pt_id), "Parent disappear");
    auto parent = _node_map[parent_pt_id];
    parent->insert_child(pt_wheel);
    pt_wheel->set_parent(parent);

    // debug
    debug_create_node(pt_wheel);

    for (auto id : order) {
      ASSERT(map.count(id), "Can't find map between gird_id(%hhu) and VCGNodeType", id);
      auto pt_node = new PTNode(_node_map.size(), get_pt_type(map[id]));
      ASSERT(_node_map.count(pt_node->get_pt_id()) == 0,
      "Have existed pt_id = %d", pt_wheel->get_pt_id());
      _node_map[pt_node->get_pt_id()] = pt_node;

      pt_wheel->insert_child(pt_node);
      pt_node->set_parent(pt_wheel);

      ASSERT(_pt_grid_map.count(pt_node->get_pt_id()) == 0,
           "_pt_grid_map data polluted");
      _pt_grid_map[pt_node->get_pt_id()] = id;

      // debug
      debug_create_node(pt_node);
    }

  }
}

std::map<uint8_t, VCGNodeType> VCG::make_id_type_map() {
  std::map<uint8_t, VCGNodeType> map;
  for (auto column : _id_grid) {
    for (auto id : column) {
      auto node = get_node(id);
      assert(node);
      map[id] = node->get_type();
    }
  } 

  return map;
}

std::vector<uint8_t> PatternTree::get_topological_sort(const GridType& grid) {
  std::vector<uint8_t> order;

  auto in_edges = get_in_edges(grid);  
  std::queue<uint8_t> zero_in;
  get_zero_in_nodes(grid, in_edges, zero_in);
  while (zero_in.size()) {
    auto visit_id = zero_in.front();
    order.push_back(visit_id);
    zero_in.pop();

    in_edges.erase(visit_id);
    for (auto& pair : in_edges) {
      pair.second.erase(visit_id);
    }
    get_zero_in_nodes(grid, in_edges, zero_in);
  }

  return order;
}

void PatternTree::get_zero_in_nodes(const GridType& grid, 
                       const std::map<uint8_t, std::set<uint8_t>>& in_edges, 
                       std::queue<uint8_t>& zero_in) {
  // store before
  std::set<uint8_t> zero_in_set;
  while (zero_in.size()) {
    zero_in_set.insert(zero_in.front());
    zero_in.pop();
  }

  for (auto pair : in_edges) {
    if (pair.second.size() == 0 // zero in-edge
     && zero_in_set.count(pair.first) == 0 // not cover
       ) {
      zero_in_set.insert(pair.first);
    }
  }

  std::set<uint8_t> in_queue;
  std::priority_queue<std::pair<size_t, uint8_t>, 
                      std::vector<std::pair<size_t, uint8_t>>,
                      std::greater<std::pair<size_t, uint8_t>>> min_col_2_id;
  for (size_t col = 0; col < grid.size(); ++col) {
    for (size_t row = 0; row < grid[col].size(); ++row) {
      if (zero_in_set.count(grid[col][row])    // zero in-edge
       && in_queue.count(grid[col][row]) == 0 // not in queue
      ) {
        min_col_2_id.push({col, grid[col][row]});
        in_queue.insert(grid[col][row]);
      }
    }
  }

  while (min_col_2_id.size()) {
    zero_in.push(min_col_2_id.top().second);
    min_col_2_id.pop();
  }
}

PatternTree::~PatternTree() {
  _pt_grid_map.clear();

  for (auto& pair : _node_map) {
    delete pair.second;
  }
}

PTNode::~PTNode() {
  _parent = nullptr;
  _children.clear();
}

}  // namespace  EDA_CHALLENGE_Q4