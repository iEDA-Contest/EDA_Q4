#include "VCG.hpp"

#include <algorithm>
#include <cmath>

namespace EDA_CHALLENGE_Q4 {

size_t VCG::_gds_file_num = 0;
// static size_t max_picks_num = 5;

VCG::VCG(Token_List& tokens) : _cell_man(nullptr), _cst(nullptr) {
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
void VCG::fill_id_grid() {
  auto max_row = get_max_row(_id_grid);
  for(auto& column : _id_grid) {
    for (size_t row = column.size(); row < max_row; ++row) {
      column.push_back(column[row - 1]);
    }
  }
}

void VCG::find_best_place() {
  traverse_tree();
}

// void VCG::gen_result() {
//   std::fstream result("../output/result.txt", std::ios::app | std::ios::out);
//   assert(result.is_open());

//   std::string pattern = _cst->get_pattern();
//   std::string::size_type pos = 0;
//   const std::string sub_str = "&#60;";
//   while ((pos = pattern.find(sub_str)) != std::string::npos) {
//     pattern.replace(pos, sub_str.length(), "<");
//   }

//   if (pattern[pattern.size() - 1] == ' ') {
//     pattern.replace(pattern.size() - 1, 1, "\0");
//   }
//   result << "PATTERN \"" << pattern << "\" ";

//   auto box_c3 = cal_c3_of_interposer();
//   if (box_c3[0] > box_c3[1]
//    || box_c3[2] > box_c3[3]) {
//      result << "NA\n";
//   } else {
//     result << box_c3[0] << " * " << box_c3[2] << "\n";
//     for (auto node : _adj_list) {
//       auto type = node->get_type();
//       if ( type == kVCG_START || type == kVCG_END) continue;

//       auto cell = node->get_cell();
//       assert(cell);

//       result << cell->get_refer() << 
//       "(" << cell->get_x() << ", " << cell->get_y() << ") " <<
//       "R" << (cell->get_rotation() ? "90" : "0") << "\n";
//     }
//   } 
//   result << "\n";

//   result.close();
// }

void VCG::init_pattern_tree() {
  auto map = make_id_type_map();
  _tree = new PatternTree(_id_grid, map);
}

PatternTree::PatternTree(GridType& grid, std::map<uint8_t, VCGNodeType>& map):
  _cm(nullptr), _cst(nullptr) {
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

    std::queue<GridType> smaller;
    slice_module(pair.second, vertical, smaller);
    if (smaller.size() == 1) {
      insert_leaves(pair.first, pair.second, map);
    } else {
      auto pt_node = new PTNode(_node_map.size(),
                                vertical ? kPTVertical : KPTHorizontal,
                                pair.second);
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

void PatternTree::slice_module( const GridType& grid /*in*/,
                                bool& vertical /*in*/,
                                std::queue<GridType>& ret /*out*/) {
  clear_queue(ret);

  slice_vertical(grid, ret);
  vertical = true;
  if (ret.size() == 1) {
    ret.pop();
    slice_horizontal(grid, ret);
    vertical = false;
  }
}

void PatternTree::slice_vertical( const GridType& grid /*in*/,
                                  std::queue<GridType>& ret /*out*/) {
  ASSERT(grid.size() && grid[0].size(), "Grid data polluted");
  size_t max_row = grid[0].size();

  clear_queue(ret);

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
    ret.push(left);
    
    if (col + 1 < grid.size()) {
      GridType right;
      for (col = col + 1; col < grid.size(); ++col) {
        right.push_back(grid[col]);
      }
      ret.push(right);
    }
    

  } else {
    // grid.size() == 1
    ret.push(grid);
  }
}

void PatternTree::slice_horizontal( const GridType& grid /*in*/,
                                    std::queue<GridType>& ret /*out*/) {
  ASSERT(grid.size() && grid[0].size(), "Grid data polluted");
  size_t max_row = grid[0].size();

  clear_queue(ret);

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
      ret.push(down);
    }
    
    GridType top;
    for (auto column : grid) {
      if (column.begin() < column.begin() + row + 1) {
        std::vector<uint8_t> col_vec(column.begin(), column.begin() + row + 1);
        top.push_back(col_vec);
      }
    }
    if (top.size()) {
      ret.push(top);
    }

  } else {
    // max_row == 1
    ret.push(grid);
  }
}

void PatternTree::insert_leaves( int parent_pt_id,
                    const GridType& grid, 
                    std::map<uint8_t, VCGNodeType>& map) {
  std::vector<uint8_t> order; 
  get_topological_sort(grid, order);

  if (order.size() == 1) {
    
    ASSERT(map.count(order[0]), "Can't find map between gird_id(%hhu) and VCGNodeType", order[0]);
    auto pt_node = new PTNode(_node_map.size(), get_pt_type(map[order[0]]), grid);
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

    auto pt_wheel = new PTNode(_node_map.size(), kPTWheel, grid);
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
      auto pt_node = new PTNode(_node_map.size(), get_pt_type(map[id]), {{id}});
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

void PatternTree::get_topological_sort( const GridType& grid /*in*/,
                                        std::vector<uint8_t>& order /*out*/) {
  order.clear();

  std::map<uint8_t, std::set<uint8_t>> in_edges;
  get_in_edges(grid, in_edges);

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

}

void PatternTree::get_zero_in_nodes(const GridType& grid /*in*/,
                                    const std::map<uint8_t, std::set<uint8_t>>& in_edges /*in*/, 
                                    std::queue<uint8_t>& zero_in /*out*/) {
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
      if (zero_in_set.count(grid[col][row])   // zero in-edge
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

  _cm = nullptr;  // not release here
  _cst = nullptr;       // not release here
}

PTNode::~PTNode() {
  _parent = nullptr;
  _children.clear();
  _grid.clear();

  for (auto p : _picks) {
    delete p;
  }
  _picks.clear();
}

void VCG::traverse_tree() {
  _tree->postorder_traverse();
}

void PatternTree::postorder_traverse() {
  std::vector<int> preorder;
  std::vector<int> postorder;

  preorder.push_back(0);
  while (preorder.size()) {
    auto pre = preorder.back();
    preorder.pop_back();
    postorder.push_back(pre);

    ASSERT(_node_map.count(pre), "pt_id = %d invalid", pre);
    auto pt_node = _node_map[pre];
    for (auto child : pt_node->get_children()) {
      preorder.push_back(child->get_pt_id());
    }
  }

  for (auto it = postorder.rbegin(); it != postorder.rend(); ++it) {
    visit_pt_node(*it);
    // printf("%2d, ", *it);
  }
  
}

void PatternTree::visit_pt_node(int pt_id) {
  ASSERT(_node_map.count(pt_id), "pt_id = %d invalid", pt_id);

  auto pt_node = _node_map[pt_id];
  switch (pt_node->get_type()) {
    case kPTMem:
    case kPTSoc:        list_possibility(pt_node);  break;
    case kPTVertical:   merge_hrz(pt_node);         break;
    case KPTHorizontal: TODO();
    case kPTWheel:      TODO();
    
    default: PANIC("Unhandled pt_node type = %d", pt_node->get_type());
  }
}

void PatternTree::list_possibility(PTNode* pt_node) {
  if (pt_node == nullptr) return;

  CellType c_type = kCellTypeNull;
  get_celltype(pt_node->get_type(), c_type);
  auto cells = _cm->choose_cells(c_type);

  ASSERT(_pt_grid_map.count(pt_node->get_pt_id()),
          "no map of pt_id = %d", pt_node->get_pt_id());
  auto grid_value = _pt_grid_map[pt_node->get_pt_id()];

  PickHelper* p = nullptr;
  for (auto cell : cells) {
    p = new PickHelper(grid_value, cell->get_cell_id(), cell->get_rotation());
    p->set_box(0, 0, cell->get_width(), cell->get_height());
    pt_node->insert_pick(p);
    
    p = new PickHelper(grid_value, cell->get_cell_id(), !cell->get_rotation());
    p->set_box(0, 0, cell->get_height(), cell->get_width());
    pt_node->insert_pick(p);
  }
}

void PatternTree::merge_hrz(PTNode* pt_node) {
  assert(pt_node && pt_node->get_type() == kPTVertical);
  auto children = pt_node->get_children();
  ASSERT(children.size() == 2, "Topology error");
  auto lchild = children[0];
  auto rchild = children[1];

  std::set<uint8_t> lefts_set1;
  std::set<uint8_t> bottoms_set1;
  std::vector<PickItem*> lefts_items;
    for (auto lpick : lchild->get_picks()) {
    lchild->get_grid_lefts(lefts_set1);
    lchild->get_grid_bottoms(bottoms_set1);
    get_cell_ids(lpick, lefts_set1, lefts_items);
    adjust_interposer_left(lefts_items);

    for (auto rpick : rchild->get_picks()) {
      if (is_pick_repeat(lpick, rpick)) continue;
      // place
      
    }
  }
}

/**
 * @brief Construct a new Pick Helper:: Pick Helper object 
 * 
 * @param first   cells chosen first
 * @param second  cells chosen later
 */
PickHelper::PickHelper(PickHelper* first, PickHelper* second) {
  ASSERT(first && second, "Please enter valid data");

  for (auto item : first->get_items()) {
    _items.push_back(new PickItem(*item));
  }
  for (auto item : second->get_items()) {
    _items.push_back(new PickItem(*item));
  }
  for (auto item : _items) {
    _id_item_map[item->_grid_value] = item;
  }
}

bool PatternTree::is_pick_repeat(PickHelper* pick1, PickHelper* pick2) {
  if (!pick1 || !pick2) return false;

  for (auto item1 : pick1->get_items()) {
    for (auto item2 : pick2->get_items()) {
      if (item1->_cell_id == item2->_cell_id) {
        return true;
      }
    }
  }

  return false;
}

void PatternTree::adjust_interposer_left(std::vector<PickItem*>& lefts_items) {
  for (auto item : lefts_items) {
    if (!is_interposer_left(item->_grid_value)) continue;

    auto cell = _cm->get_cell(item->_cell_id);
    ASSERT(cell, "Missing cell whose c_id = %d", item->_cell_id);

    auto celltype = cell->get_cell_type();
    Point range;
    get_cst_x(celltype, range);
    
  }
}

}  // namespace  EDA_CHALLENGE_Q4