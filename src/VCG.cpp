#include "VCG.hpp"

#include <queue>

namespace EDA_CHALLENGE_Q4 {

VCG::VCG(Token_List& tokens) : _cell_man(nullptr) {
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

/**
 * @brief
 *
 *
 *
 * @param prio   priority
 * @param pos_id to determine which position
 * @param ...    parameters in needed
 * @return std::vector<Cell*>
 */
std::vector<Cell*> VCG::get_cells(CellPriority prio, uint8_t pos_id, ...) {
  va_list ap;
  va_start(ap, pos_id);
  std::vector<Cell*> result;
  switch (prio) {
    case kRange:
    case kRatio_MAX:
      result = _cell_man->choose_cells(prio, get_cell_type(pos_id), ap);
      break;

    default:
      PANIC("Unknown CellPriority = %d", prio);
      break;
  }
  va_end(ap);

  return result;
}

void VCG::do_place_cell(uint8_t id, Cell* cell) { 
  if (is_id_valid(id) && cell != nullptr) {
    _adj_list[id]->set_cell(cell);
  }
 }

void VCG::undo_place_cell(uint8_t id) {
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
        PANIC("Fail to undo cell with cell_type = %d", _adj_list[id]->get_type());
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
      if(row  < _id_grid[column].size()) {
        printf("%2hhu, ", _id_grid[column][row]);
      } else {
        printf("  , ");
      }
    }
    printf("\b\b  \n");
  }
  printf("------\n");
}

std::vector<GridType> VCG::get_smaller_module(GridType& grid) {
  std::vector<GridType> smaller;
  smaller = get_smaller_module_column(grid);
  if (smaller.size() == 1) {
    smaller = get_smaller_module_row(grid);
  }
  return smaller;
}


// column priority
std::vector<GridType> VCG::get_smaller_module_column(GridType& grid) {
  std::vector<GridType> result;

  size_t max_row = get_max_row(grid);

  if (max_row > 1) {

    size_t row;
    size_t begin_column = 0;

    for (size_t column  = 0; column < grid.size() - 1; ++column) {
      for(row = 0; row < max_row; ++row){
        if (row >= grid[column].size() || row >= grid[column + 1].size()) {
          continue;
        } else if(grid[column][row] == grid[column + 1][row]) {
          break;
        }
      }

      if (row == max_row) {
        GridType module;
        for(;begin_column <= column; ++begin_column) {
          module.push_back(grid[begin_column]);
        }
        result.push_back(module);
      }
    }

    GridType last_module;
    for (; begin_column < grid.size(); ++begin_column) {
      last_module.push_back(grid[begin_column]);
    }
    result.push_back(last_module);
  } 

  return result;
}

// row priority
std::vector<GridType> VCG::get_smaller_module_row(GridType& grid) {
  std::vector<GridType> result;
  
  size_t max_row = get_max_row(grid);
  size_t begin_row = 0;
  size_t column;
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
        if ( begin < end) {
          module.push_back(std::vector<uint8_t>(begin, end));
        }
      }

      if (module.size()) {
        result.push_back(module);
      }
      begin_row = row + 1;
    }
  }

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
  auto modules = slice();
  modules.size();
}

// get the smallest indivisible modules, and their order rely on slicing times
std::vector<GridType> VCG::slice() {
  std::queue<GridType> queue;
  std::vector<GridType> min_modules;
  auto smaller_grid = get_smaller_module(_id_grid);
  for (auto module: smaller_grid) {
    queue.push(module);
  }

  while (queue.size()) {
    auto module = queue.front();
    queue.pop();
    auto smaller_module = get_smaller_module(module);
    if (smaller_module.size() == 1) {
      min_modules.push_back(smaller_module.back());
    } else{
      for (auto smaller: smaller_module) {
        queue.push(smaller);
      }
    }
  }

  return min_modules;
}

}  // namespace  EDA_CHALLENGE_Q4