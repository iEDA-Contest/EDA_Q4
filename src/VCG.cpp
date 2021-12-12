#include "VCG.hpp"

#include <algorithm>
#include <cmath>

namespace EDA_CHALLENGE_Q4 {

size_t VCG::_gds_file_num = 0;
static constexpr size_t max_combination = 80;

VCG::VCG(Token_List& tokens) : _cm(nullptr), _cst(nullptr), _helper(nullptr) {
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
        set_id_grid(column, row, from->get_vcg_id());
        from = nullptr;
        break;
      case kSOC:
        ++row;
        from = new VCGNode(kVCG_SOC, get_vertex_num());
        _adj_list.push_back(from);
        to->insert_from(from);
        to = from;
        set_id_grid(column, row, from->get_vcg_id());
        from = nullptr;
        break;

      // placeholder
      case kVERTICAL:
        ASSERT(row != 0, "'^' occurs at first row");
        ++row;
        to->inc_v_placeholder();
        set_id_grid(column, row, to->get_vcg_id());
        break;
      case kHORIZONTAL:
        ASSERT(column > 1, "'<' occurs at first column");
        ++row;
        from = _adj_list[_id_grid[column - 2][row - 1]];
        set_id_grid(column, row, from->get_vcg_id());
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
  // debug();
}

VCG::~VCG() {
  // before clear _cell_man, we should retrieve all cells
  retrieve_all_cells();
  delete _cm;
  _cm = nullptr;

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

  _helper = nullptr;  // must not release here!
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
      if (line_up[from->get_vcg_id()] == false) {
        queue.push(from);
        line_up[from->get_vcg_id()] = true;
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
        _cm->insert_cell(kCellTypeMem, _adj_list[id]->get_cell());
        _adj_list[id]->set_cell_null();
        break;
      case kVCG_SOC:
        _cm->insert_cell(kCellTypeSoc, _adj_list[id]->get_cell());
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
    g_log << "row[" << row << "]:";
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
  assert(id_column_index_map.size() == _adj_list.size() - 2 &&
         id_row_index_map.size() == _adj_list.size() - 2);

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
  for (auto& column : _id_grid) {
    for (size_t row = column.size(); row < max_row; ++row) {
      column.push_back(column[row - 1]);
    }
  }
}

void VCG::find_best_place() {
  traverse_tree();
  // root
  auto root = _tree->get_pt_node(0);
  ASSERT(root, "Root of Pattern Tree missing");
  auto root_picks = root->get_picks();
  ASSERT(root_picks.size(), "No Picks in root node");
  auto best = root_picks[root_picks.size() - 1];
  set_cells_by_helper(best);

  // get_biggest_column_helper(4);

  // debug
  // for (auto helper : root_picks) {
  //   undo_all_picks();
  //   set_cells_by_helper(helper);
  //   gen_GDS();

  //   debug_picks();
  // }
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

  int c3_arr[4];
  get_interposer_c3(c3_arr);
  if (c3_arr[0] > c3_arr[1] || c3_arr[2] > c3_arr[3]) {
    result << "NA\n";
  } else {
    result << c3_arr[0] << " * " << c3_arr[2] << "\n";
    for (auto node : _adj_list) {
      auto type = node->get_type();
      if (type == kVCG_START || type == kVCG_END) continue;

      auto cell = node->get_cell();
      assert(cell);

      result << cell->get_refer() << "(" << cell->get_x() << ", "
             << cell->get_y() << ") "
             << "R" << (cell->get_rotation() ? "90" : "0") << "\n";
    }
  }
  result << "\n";

  result.close();
}

void VCG::init_pattern_tree() {
  auto map = make_id_type_map();
  _tree = new PatternTree(_id_grid, map);
  _tree->set_vcg(this);
}

PatternTree::PatternTree(GridType& grid, std::map<uint8_t, VCGNodeType>& map)
    : _cm(nullptr), _cst(nullptr), _vcg(nullptr) {
  slice(grid, map);
  // debug_show_pt_grid_map();
}

void PatternTree::slice(const GridType& grid,
                        std::map<uint8_t, VCGNodeType>& map) {
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
      auto pt_node =
          new PTNode(_node_map.size(), vertical ? kPTVertical : KPTHorizontal,
                     pair.second);
      ASSERT(_node_map.count(pt_node->get_pt_id()) == 0,
             "Have existed pt_id = %d", pt_node->get_pt_id());
      _node_map[pt_node->get_pt_id()] = pt_node;

      ASSERT(_node_map.count(pair.first), "Parent disappear");
      auto parent = _node_map[pair.first];
      parent->insert_child(pt_node);
      pt_node->set_parent(parent);

      while (smaller.size()) {
        queue.push({pt_node->get_pt_id(), smaller.front()});
        smaller.pop();
      }

      // debug
      // debug_create_node(pt_node);
    }
  }
}

void PatternTree::slice_module(const GridType& grid /*in*/,
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

void PatternTree::slice_vertical(const GridType& grid /*in*/,
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
        ASSERT(grid[col].size() >= row + 1 || grid[col + 1].size() >= row + 1,
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

void PatternTree::slice_horizontal(const GridType& grid /*in*/,
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
        ASSERT(grid[col].size() >= row + 1 || grid[col + 1].size() >= row + 1,
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
        std::vector<uint8_t> col_vec(column.begin() + row + 1,
                                     column.begin() + max_row);
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

void PatternTree::insert_leaves(int parent_pt_id, const GridType& grid,
                                std::map<uint8_t, VCGNodeType>& map) {
  std::vector<uint8_t> order;
  get_topological_sort(grid, order);

  if (order.size() == 1) {
    ASSERT(map.count(order[0]),
           "Can't find map between gird_id(%hhu) and VCGNodeType", order[0]);
    auto pt_node =
        new PTNode(_node_map.size(), get_pt_type(map[order[0]]), grid);
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
    // debug_create_node(pt_node);

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
    // debug_create_node(pt_wheel);

    for (auto id : order) {
      ASSERT(map.count(id),
             "Can't find map between gird_id(%hhu) and VCGNodeType", id);
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
      // debug_create_node(pt_node);
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

void PatternTree::get_topological_sort(const GridType& grid /*in*/,
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

void PatternTree::get_zero_in_nodes(
    const GridType& grid /*in*/,
    const std::map<uint8_t, std::set<uint8_t>>& in_edges /*in*/,
    std::queue<uint8_t>& zero_in /*out*/) {
  // store before
  std::set<uint8_t> zero_in_set;
  while (zero_in.size()) {
    zero_in_set.insert(zero_in.front());
    zero_in.pop();
  }

  for (auto pair : in_edges) {
    if (pair.second.size() == 0                // zero in-edge
        && zero_in_set.count(pair.first) == 0  // not cover
    ) {
      zero_in_set.insert(pair.first);
    }
  }

  std::set<uint8_t> in_queue;
  std::priority_queue<std::pair<size_t, uint8_t>,
                      std::vector<std::pair<size_t, uint8_t>>,
                      std::greater<std::pair<size_t, uint8_t>>>
      min_col_2_id;
  for (size_t col = 0; col < grid.size(); ++col) {
    for (size_t row = 0; row < grid[col].size(); ++row) {
      if (zero_in_set.count(grid[col][row])       // zero in-edge
          && in_queue.count(grid[col][row]) == 0  // not in queue
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
  for (auto& pair : _node_map) {
    delete pair.second;
  }
  _node_map.clear();

  _pt_grid_map.clear();

  _cm = nullptr;   // not release here
  _cst = nullptr;  // not release here
  _vcg = nullptr;  // not release here
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

void VCG::traverse_tree() { _tree->postorder_traverse(); }

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
    // debug
    // printf("%2d, ", *it);
  }
}

void PatternTree::visit_pt_node(int pt_id) {
  ASSERT(_node_map.count(pt_id), "pt_id = %d invalid", pt_id);

  auto pt_node = _node_map[pt_id];
  switch (pt_node->get_type()) {
    case kPTMem:
    case kPTSoc:
      list_possibility(pt_node);
      break;
    case kPTVertical:
      merge_hrz(pt_node);
      break;
    case KPTHorizontal:
      merge_vtc(pt_node);
      break;
    case kPTWheel:
      merge_wheel(pt_node);
      break;

    default:
      PANIC("Unhandled pt_node type = %d", pt_node->get_type());
  }

  if (pt_node->get_picks().size() == 0) {
    g_log << "No picks generate in pt_id = " << pt_node->get_pt_id() << "\n";
  }

  // debug
  if (pt_node->get_pt_id() == 16) {
    for (auto pick : pt_node->get_picks()) {
      debug_GDS(pick);
    }
  }
}

void PatternTree::list_possibility(PTNode* pt_node) {
  if (pt_node == nullptr) return;

  CellType c_type = kCellTypeNull;
  get_celltype(pt_node->get_type(), c_type);
  auto cells = _cm->choose_cells(c_type);

  ASSERT(_pt_grid_map.count(pt_node->get_pt_id()), "no map of pt_id = %d",
         pt_node->get_pt_id());
  auto grid_value = _pt_grid_map[pt_node->get_pt_id()];

  PickHelper* p = nullptr;
  for (auto cell : cells) {
    p = new PickHelper(grid_value, cell->get_cell_id(), cell->get_rotation());
    p->set_box(0, 0, cell->get_width(), cell->get_height());
    pt_node->insert_pick(p);

    if (cell->is_square()) continue;
    cell->rotate();

    p = new PickHelper(grid_value, cell->get_cell_id(), cell->get_rotation());
    p->set_box(0, 0, cell->get_width(), cell->get_height());
    pt_node->insert_pick(p);
  }
}

void PatternTree::merge_hrz(PTNode* pt_node) {
  assert(pt_node && pt_node->get_type() == kPTVertical);
  auto children = pt_node->get_children();
  ASSERT(children.size() == 2, "Topology error");
  auto lchild = children[0];
  auto rchild = children[1];

  std::set<uint8_t> lpick_vcg_id_set;
  std::set<uint8_t> rpick_vcg_id_set;
  std::vector<PickItem*> l_items;
  std::vector<PickItem*> r_items;
  DeathQue death_queue;
  Rectangle box;
  for (auto lpick : lchild->get_picks()) {
    PickHelper* lpick_new = new PickHelper(lpick);

    // interposer
    lchild->get_grid_lefts(lpick_vcg_id_set);
    lpick_new->get_items(lpick_vcg_id_set, l_items);
    adjust_interposer_left(l_items);

    lchild->get_grid_bottoms(lpick_vcg_id_set);
    lpick_new->get_items(lpick_vcg_id_set, l_items);
    adjust_interposer_bottom(l_items);

    // update left box
    get_helper_box(lpick_new, box);
    lpick_new->set_box(box);

    lchild->get_grid_rights(lpick_vcg_id_set);
    lpick_new->get_items(lpick_vcg_id_set, l_items);

    for (auto rpick : rchild->get_picks()) {
      if (is_pick_repeat(lpick_new, rpick)) continue;

      PickHelper* rpick_new = new PickHelper(rpick);

      // interposer
      rchild->get_grid_bottoms(rpick_vcg_id_set);
      rpick_new->get_items(rpick_vcg_id_set, r_items);
      adjust_interposer_bottom(r_items);

      // update right box
      get_helper_box(rpick_new, box);
      rpick_new->set_box(box);

      // two pt_node
      rchild->get_grid_lefts(rpick_vcg_id_set);
      rpick_new->get_items(rpick_vcg_id_set, r_items);

      Point range;
      get_box_range_fit_litems(r_items, l_items, lpick_new->get_box()._c3._x,
                               range);
      int x_move = range._x;
      if (range._x > range._y) {
        x_move = range._x;

        // g_log << "[HRZ merging violate] occur in pt_node = "
        //       << std::to_string(pt_node->get_pt_id())  << ", "
        //       << "range.min_x = " << range._x << ", "
        //       << "range.max_x = " << range._y << "\n";
      }

      for (auto r_item : rpick_new->get_items()) {
        r_item->_c1_x += x_move;
      }

      PickHelper* new_helper = new PickHelper(lpick_new, rpick_new);

      get_helper_box(new_helper, box);
      new_helper->set_box(box);
      int cell_area = get_cells_area(new_helper);
      new_helper->set_death(1.0 * (box.get_area() - cell_area) /
                            box.get_area());

      // debug
      // debug_GDS(new_helper);

      if (!insert_death_que(death_queue, new_helper)) {
        delete new_helper;
      }

      delete rpick_new;

    }  // end for auto rpick

    delete lpick_new;
  }  // end for auto lpick

  if (death_queue.size() == 0) {
    second_pick_replace(lchild, rchild, death_queue);
  }

  while (death_queue.size()) {
    auto pick = death_queue.top();
    death_queue.pop();
    pt_node->insert_pick(pick);

    // debug
    // debug_GDS(pick);
  }
}

/**
 * @brief Construct a new Pick Helper:: Pick Helper object
 *
 * @param first   cells chosen first
 * @param second  cells chosen later
 */
PickHelper::PickHelper(PickHelper* first, PickHelper* second) : _death(0) {
  ASSERT(first && second, "Please enter valid data");

  for (auto item : first->get_items()) {
    _items.push_back(new PickItem(*item));
  }
  for (auto item : second->get_items()) {
    _items.push_back(new PickItem(*item));
  }
  for (auto item : _items) {
    _id_item_map[item->_vcg_id] = item;
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

void PatternTree::adjust_interposer_left(std::vector<PickItem*>& left_items) {
  for (auto item : left_items) {
    if (!is_interposer_left(item->_vcg_id)) continue;

    auto cell = _cm->get_cell(item->_cell_id);
    ASSERT(cell, "Missing cell whose c_id = %d", item->_cell_id);

    set_cell_status(cell, item);

    auto celltype = cell->get_cell_type();
    Point range;
    get_cst_x(celltype, range);
    item->_c1_x = range._x;
  }
}

void PatternTree::adjust_interposer_bottom(
    std::vector<PickItem*>& bottom_items) {
  for (auto item : bottom_items) {
    if (!is_interposer_bottom(item->_vcg_id)) continue;

    auto cell = _cm->get_cell(item->_cell_id);
    ASSERT(cell, "Missing cell whose c_id = %d", item->_cell_id);

    set_cell_status(cell, item);

    auto celltype = cell->get_cell_type();
    Point range;
    get_cst_y(celltype, range);
    cell->set_y(range._x);
    item->_c1_y = range._x;
  }
}

void VCG::gen_GDS() {
#ifndef GDS
  return;
#endif
  std::string fname =
      "../output/myresult" + std::to_string(_gds_file_num) + ".gds";
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
    if (cell == nullptr) continue;
    ++cell_num;

    // rectangle's four relative coordinates
    // template
    gds << "BGNSTR\n";
    gds << "STRNAME " << cell->get_refer() << std::to_string(node->get_vcg_id())
        << "\n";
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

  int c3_arr[4];
  get_interposer_c3(c3_arr);
  auto c3_x = std::min(c3_arr[0], c3_arr[1]);
  auto c3_y = std::min(c3_arr[2], c3_arr[3]);
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
    if (cell == nullptr) continue;

    // template
    gds << "SREF\n";
    gds << "SNAME " << cell->get_refer() << std::to_string(node->get_vcg_id())
        << "\n";
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

void VCG::get_interposer_c3(int ret_arr[4] /*out*/) {
  Point constraint;

  int min_y = 0;
  int max_y = 0;
  // top
  for (auto column : _id_grid) {
    auto cell = get_cell(column[0]);
    if (cell == nullptr) continue;

    get_cst_y(cell->get_vcg_id(), constraint);
    min_y = std::max(min_y, cell->get_c3()._y + constraint._x);
    max_y = max_y == 0 ? cell->get_c3()._y + constraint._y
                       : std::min(max_y, cell->get_c3()._y + constraint._y);
  }

  // right
  int min_x = 0;
  int max_x = 0;
  if (_id_grid.size()) {
    auto last_col = _id_grid[_id_grid.size() - 1];
    for (size_t row = 0; row < last_col.size(); ++row) {
      auto cell = get_cell(last_col[row]);
      if (cell == nullptr) continue;

      get_cst_x(cell->get_vcg_id(), constraint);
      min_x = std::max(min_x, cell->get_c3()._x + constraint._x);
      max_x = max_x == 0 ? cell->get_c3()._x + constraint._y
                         : std::min(max_x, cell->get_c3()._x + constraint._y);
    }
  }

  // debug
  // g_log << "\n";
  // g_log << "##Interposer_x " << min_x << " ~ " << max_x << "\n";
  // g_log << "##Interposer_y " << min_y << " ~ " << max_y << "\n";
  // g_log.flush();

  ret_arr[0] = min_x;
  ret_arr[1] = max_x;
  ret_arr[2] = min_y;
  ret_arr[3] = max_y;
}

void PatternTree::get_box_range_fit_litems(
    const std::vector<PickItem*>& r_items /*in*/,
    const std::vector<PickItem*>& l_items /*in*/, int l_box_c3_x /*in*/,
    Point& r_box_left /*out*/) {
  int min_x = l_box_c3_x;
  int max_x = 0;

  for (auto r_item : r_items) {
    auto r_cell = _cm->get_cell(r_item->_cell_id);
    assert(r_cell);

    // restore right cell's status
    set_cell_status(r_cell, r_item);

    for (auto l_item : l_items) {
      auto l_cell = _cm->get_cell(l_item->_cell_id);
      assert(l_cell);

      // restore left cell's status
      set_cell_status(l_cell, l_item);

      if (is_overlap_y(r_cell, l_cell, false)) {
        get_cst_x(r_cell->get_cell_type(), l_cell->get_cell_type(), r_box_left);

        min_x = std::max(l_cell->get_c3()._x + r_box_left._x, min_x);
        max_x = max_x == 0
                    ? l_cell->get_c3()._x + r_box_left._y
                    : std::min(l_cell->get_c3()._x + r_box_left._y, max_x);
      }
    }  // end auto l_item
  }    // end auto r_item

  r_box_left._x = min_x;
  r_box_left._y = max_x;
}

void PatternTree::get_box_range_fit_bitems(
    const std::vector<PickItem*>& t_items /*in*/,
    const std::vector<PickItem*>& b_items /*in*/, int b_box_c3_y /*in*/,
    Point& t_box_bottom /*out*/) {
  int min_y = b_box_c3_y;
  int max_y = 0;

  for (auto t_item : t_items) {
    auto t_cell = _cm->get_cell(t_item->_cell_id);
    assert(t_cell);

    // restore top cell's status
    set_cell_status(t_cell, t_item);

    for (auto b_item : b_items) {
      auto b_cell = _cm->get_cell(b_item->_cell_id);
      assert(b_cell);

      // restore bottom cell's status
      set_cell_status(b_cell, b_item);

      if (is_overlap_x(t_cell, b_cell, false)) {
        get_cst_y(t_cell->get_cell_type(), b_cell->get_cell_type(),
                  t_box_bottom);

        min_y = std::max(b_cell->get_c3()._y + t_box_bottom._x, min_y);
        max_y = max_y == 0
                    ? b_cell->get_c3()._y + t_box_bottom._y
                    : std::min(b_cell->get_c3()._y + t_box_bottom._y, max_y);
      }
    }  // end auto b_item
  }    // end auto t_item

  t_box_bottom._x = min_y;
  t_box_bottom._y = max_y;
}

void PatternTree::debug_GDS(PickHelper* helper) {
  if (!helper) return;

  for (auto item : helper->get_items()) {
    auto cell = _cm->get_cell(item->_cell_id);
    if (cell == nullptr) continue;

    set_cell_status(cell, item);
    _vcg->do_pick_cell(item->_vcg_id, cell);
  }
  _vcg->gen_GDS();
  _vcg->undo_all_picks();
}

/**
 *  @param helper
 *  @param box    box will be reset, and then form bounding box of helper
 */
void PatternTree::get_helper_box(PickHelper* helper /*in*/,
                                 Rectangle& box /*out*/) {
  if (!helper) return;

  // first
  box.reset();
  auto items = helper->get_items();
  ASSERT(items.size(), "No PickItem");
  auto item0 = items[0];
  box._c1._x = item0->_c1_x;
  box._c1._y = item0->_c1_y;

  // later
  for (auto item : items) {
    auto cell = _cm->get_cell(item->_cell_id);
    if (cell == nullptr) {
      g_log << "cell_id = " << item->_cell_id << "missing\n";
      continue;
    }

    set_cell_status(cell, item);

    box._c1._x = std::min(cell->get_x(), box._c1._x);
    box._c1._y = std::min(cell->get_y(), box._c1._x);
    box._c3._x = std::max(cell->get_c3()._x, box._c3._x);
    box._c3._y = std::max(cell->get_c3()._y, box._c3._y);
  }
}

int PatternTree::get_cells_area(PickHelper* helper) {
  if (!helper) return 0;

  int area = 0;
  for (auto item : helper->get_items()) {
    auto cell = _cm->get_cell(item->_cell_id);
    if (cell == nullptr) {
      g_log << "cell_id = " << item->_cell_id << "missing\n";
      continue;
    }

    area += cell->get_area();
  }

  return area;
}

void VCG::undo_all_picks() {
  for (auto node : _adj_list) {
    undo_pick_cell(node->get_vcg_id());
  }
}

void PatternTree::merge_vtc(PTNode* pt_node) {
  assert(pt_node && pt_node->get_type() == KPTHorizontal);
  auto children = pt_node->get_children();
  ASSERT(children.size() == 2, "Topology error");
  auto bchild = children[0];
  auto tchild = children[1];

  std::set<uint8_t> bpick_vcg_id_set;
  std::set<uint8_t> tpick_vcg_id_set;
  std::vector<PickItem*> b_items;
  std::vector<PickItem*> t_items;
  std::priority_queue<PickHelper*, std::vector<PickHelper*>, CmpPickHelperDeath>
      death_queue;
  Rectangle box;
  for (auto bpick : bchild->get_picks()) {
    PickHelper* bpick_new = new PickHelper(bpick);

    // interposer
    bchild->get_grid_lefts(bpick_vcg_id_set);
    bpick_new->get_items(bpick_vcg_id_set, b_items);
    adjust_interposer_left(b_items);

    bchild->get_grid_bottoms(bpick_vcg_id_set);
    bpick_new->get_items(bpick_vcg_id_set, b_items);
    adjust_interposer_bottom(b_items);

    // update bottom box
    get_helper_box(bpick_new, box);
    bpick_new->set_box(box);

    bchild->get_grid_tops(bpick_vcg_id_set);
    bpick_new->get_items(bpick_vcg_id_set, b_items);

    // debug
    // debug_GDS(bpick_new);

    for (auto tpick : tchild->get_picks()) {
      if (is_pick_repeat(bpick_new, tpick)) continue;

      PickHelper* tpick_new = new PickHelper(tpick);

      // interposer
      tchild->get_grid_lefts(tpick_vcg_id_set);
      tpick_new->get_items(tpick_vcg_id_set, t_items);
      adjust_interposer_left(t_items);

      // update top box
      get_helper_box(tpick_new, box);
      tpick_new->set_box(box);

      // two pt_node
      tchild->get_grid_bottoms(tpick_vcg_id_set);
      tpick_new->get_items(tpick_vcg_id_set, t_items);

      Point range;
      get_box_range_fit_bitems(t_items, b_items, bpick_new->get_box()._c3._y,
                               range);
      int y_move = range._x;
      if (range._x > range._y) {
        y_move = range._x;

        // g_log << "[VTC merging violate] occur in pt_node = "
        //       << std::to_string(pt_node->get_pt_id()) << ", "
        //       << "range.min_y = " << range._x << ", "
        //       << "range.max_y = " << range._y << "\n";
      }

      for (auto t_item : tpick_new->get_items()) {
        t_item->_c1_y += y_move;
      }

      PickHelper* new_helper = new PickHelper(bpick_new, tpick_new);

      get_helper_box(new_helper, box);
      new_helper->set_box(box);
      int cell_area = get_cells_area(new_helper);
      new_helper->set_death(1.0 * (box.get_area() - cell_area) /
                            box.get_area());

      // debug
      // debug_GDS(new_helper);

      if (!insert_death_que(death_queue, new_helper)) {
        delete new_helper;
      }

      delete tpick_new;

    }  // end for auto tpick

    delete bpick_new;
  }  // end for auto bpick

  if (death_queue.size() == 0) {
    second_pick_replace(bchild, tchild, death_queue);
  }

  while (death_queue.size()) {
    auto pick = death_queue.top();
    death_queue.pop();
    pt_node->insert_pick(pick);

    // debug
    // debug_GDS(pick);
  }
}

/**
 * @brief
 *
 * @param queue   container
 * @param helper  helper to insert
 * @return false  please manage helper(eg: release it)
 * @return true   insert successfully or helper is nullptr
 */
bool PatternTree::insert_death_que(DeathQue& queue, PickHelper* helper) {
  if (!helper) return true;

  // bool repreat = false;
  // DeathQue tmp;
  // while (queue.size()) {
  //   auto top = queue.top();
  //   queue.pop();
  //   tmp.push(top);

  //   repreat |= is_pick_same(top, helper);
  // }
  // queue.swap(tmp);
  // if (repreat) return false;

  // choose minimal death
  if (queue.size() < max_combination) {
    queue.push(helper);
    return true;
  } else {
    auto worst = queue.top();
    if (worst->get_death() > helper->get_death()) {
      queue.pop();
      delete worst;
      queue.push(helper);
      return true;
    }
  }  // end choose minimal death

  return false;
}

void VCG::set_cells_by_helper(PickHelper* helper) {
  _helper = helper;

  for (auto item : _helper->get_items()) {
    auto cell = _cm->get_cell(item->_cell_id);
    assert(cell);

    _tree->set_cell_status(cell, item);
    do_pick_cell(item->_vcg_id, cell);
  }
}

void VCG::update_pitem(Cell* cell) {
  if (!cell || !_helper) return;

  TODO();
}

PTNode* PatternTree::get_biggest_column(uint8_t vcg_id) {
  auto pt_id = get_pt_id(vcg_id);
  ASSERT(pt_id >= 0, "No leaves-pt_node contains vcg_id = %d", vcg_id);
  auto pt_node = get_pt_node(pt_id);
  ASSERT(pt_node, "No leaves-pt_node whose pt_id = %d", pt_id);

  PTNode* parent = pt_node->get_parent();
  while (parent && parent->get_type() != kPTVertical) {
    pt_node = parent;
    parent = pt_node->get_parent();
  }

  return pt_node;
}

int PatternTree::get_pt_id(uint8_t vcg_id) {
  for (auto pair : _pt_grid_map) {
    if (pair.second == vcg_id) {
      return pair.first;
    }
  }

  return -1;
}

// void VCG::debug_picks() {
//   g_log << "Picks:\n";
//   for (auto node : _adj_list) {
//     auto cell = node->get_cell();
//     if (cell == nullptr) continue;

//     g_log << "nodeid = " << std::to_string(node->get_vcg_id())
//           << ", cell_id = " << cell->get_cell_id() << "\n";
//     g_log.flush();
//   }
//   g_log << "----------\n";
// }

void PatternTree::second_pick_replace(PickHelper* first /*in*/,
                                      PickHelper* second /*out*/) {
  if (!first || !second) return;

  _vcg->set_cells_by_helper(first);

  // find repeat
  std::set<uint8_t> repeat_set;
  for (auto item : second->get_items()) {
    auto cell = _cm->get_cell(item->_cell_id);
    ASSERT(cell, "Cell Manager miss cell_id = %d", item->_cell_id);

    if (cell->get_vcg_id() != 0 /*cell has been placed*/) {
      repeat_set.insert(item->_vcg_id);
      item->_cell_id = 0;
    }
  }

  for (auto vcg_id : repeat_set) {
    auto item = second->get_item(vcg_id);

    auto cellprio = _vcg->get_priority(item->_vcg_id);
    auto cells = _cm->choose_cells(false, cellprio);
    for (auto cell : cells) {
      if (second->is_picked(cell->get_cell_id())) continue;

      item->_cell_id = cell->get_cell_id();
      break;
    }
  }

  _vcg->undo_all_picks();

  // replace second helper
  replace(second);
}

void PatternTree::replace(PickHelper* helper) {
  helper->reset_pos();

  Point range;
  std::vector<uint8_t> placed;
  for (auto item : helper->get_items()) {
    auto cell = _cm->get_cell(item->_cell_id);
    assert(cell);

    int min_y = 0;
    int max_y = 0;
    // y : interposer bottom
    if (is_interposer_bottom(item->_vcg_id)) {
      get_cst_y(cell->get_cell_type(), range);
      min_y = range._x;
      max_y = range._y;
    }
    // y : from
    std::set<uint8_t> froms_vcg_id_set;
    get_from_vcg_ids(item->_vcg_id, froms_vcg_id_set);
    for (auto from_id : froms_vcg_id_set) {
      auto from_item = helper->get_item(from_id);
      if (from_item == nullptr) continue;

      auto from_cell = _cm->get_cell(from_item->_cell_id);
      assert(from_cell);

      if (from_cell->get_rotation() != from_item->_rotation) {
        from_cell->rotate();
      }

      if (from_item) {
        auto from_cell_type = _vcg->get_cell_type(from_id);
        get_cst_y(cell->get_cell_type(), from_cell_type, range);

        min_y = std::max(from_item->_c1_y + from_cell->get_height() + range._x,
                         min_y);
        max_y = max_y == 0
                    ? from_item->_c1_y + from_cell->get_height() + range._y
                    : std::min(
                          from_item->_c1_y + from_cell->get_height() + range._y,
                          max_y);
      }
    }

    item->_c1_y = min_y <= max_y ? min_y : max_y;
    set_cell_status(cell, item);

    int min_x = 0;
    int max_x = 0;
    // x : interposer
    if (is_interposer_left(item->_vcg_id)) {
      get_cst_x(cell->get_cell_type(), range);
      min_x = range._x;
      max_x = range._y;
    }
    // x : left
    for (auto vcg_id : placed) {
      auto item_placed = helper->get_item(vcg_id);
      assert(item_placed);

      auto cell_placed = _cm->get_cell(item_placed->_cell_id);
      assert(cell_placed);

      if (is_overlap_y(cell, cell_placed, false)) {
        get_cst_y(cell->get_cell_type(), cell_placed->get_cell_type(), range);

        min_x = std::max(cell_placed->get_c3()._x + range._x, min_x);
        max_x = max_x == 0
                    ? cell_placed->get_c3()._x + range._y
                    : std::min(cell_placed->get_c3()._x + range._y, max_x);
      }
    }

    item->_c1_x = min_x <= max_x ? min_x : max_x;

    placed.push_back(item->_vcg_id);

  }  // end auto item
}

void PatternTree::get_from_vcg_ids(uint8_t vcg_id /*in*/,
                                   std::set<uint8_t>& set /*out*/) {
  set.clear();

  auto node = _vcg->get_node(vcg_id);
  if (!node) return;

  for (auto from : node->get_froms()) {
    set.insert(from->get_vcg_id());
  }
}

bool PatternTree::is_pick_same(PickHelper* p1, PickHelper* p2) {
  if (!p1 || !p2) return false;

  auto items1 = p1->get_items();
  auto items2 = p2->get_items();

  auto item_num1 = items1.size();
  auto item_num2 = items2.size();
  auto num = std::min(item_num1, item_num2);

  for (size_t i = 0; i < num; ++i) {
    if (items1[i]->_cell_id != items2[i]->_cell_id) {
      return false;
    }
  }

  return true;
}

void PatternTree::second_pick_replace(PTNode* first /*in*/,
                                      PTNode* second /*in*/, DeathQue& queue) {
  if (!first || !second) return;
  return;
  // release
  while (queue.size()) {
    auto ele = queue.top();
    delete ele;
    queue.pop();
  }

  auto fpick = first->get_picks();
  auto spick = second->get_picks();
  Rectangle box;
  for (auto f_it = fpick.rbegin(); f_it != fpick.rend(); ++f_it) {
    for (auto s_it = spick.rbegin(); s_it != spick.rend(); ++s_it) {
      second_pick_replace(*f_it, *s_it);

      PickHelper* new_helper = new PickHelper(*f_it, *s_it);

      get_helper_box(new_helper, box);
      new_helper->set_box(box);
      int cell_area = get_cells_area(new_helper);
      new_helper->set_death(1.0 * (box.get_area() - cell_area) /
                            box.get_area());

      // debug
      // debug_GDS(new_helper);

      if (!insert_death_que(queue, new_helper)) {
        delete new_helper;
      }
    }
  }
}

// wu wen_rui >>>>>>>

void PatternTree::merge_wheel(PTNode *pt_node)
{
  assert(pt_node && pt_node->get_type() == kPTWheel);
  auto children = pt_node->get_children();
  ASSERT(children.size() == 5, "Topology error");
  std::vector<uint8_t> orders;
  get_topological_sort(pt_node->get_grid(), orders);
  PTNode *child0;
  PTNode *child1;
  PTNode *child2;
  PTNode *child3;
  PTNode *child4;

  for (auto child : children)
  {
    auto grid = child->get_grid();
    if (grid[0][0] == orders[0])
      child0 = child;
    if (grid[0][0] == orders[1])
      child1 = child;
    if (grid[0][0] == orders[2])
      child2 = child;
    if (grid[0][0] == orders[3])
      child3 = child;
    if (grid[0][0] == orders[4])
      child4 = child;
  }

  std::set<uint8_t> pick_vcg_id_set0;
  std::set<uint8_t> pick_vcg_id_set1;
  std::set<uint8_t> pick_vcg_id_set2;
  std::set<uint8_t> pick_vcg_id_set3;
  std::set<uint8_t> pick_vcg_id_set4;
  std::vector<PickItem *> items0;
  std::vector<PickItem *> items1;
  std::vector<PickItem *> items2;
  std::vector<PickItem *> items3;
  std::vector<PickItem *> items4;

  std::priority_queue<PickHelper *, std::vector<PickHelper *>,
                      CmpPickHelperDeath> death_queue;
  Rectangle box;
  Point range;
  auto grid0 = child0->get_grid();
  if (child0->get_grid().size() > grid0[0].size())
  {
    for (auto pick0 : child0->get_picks())
    {
      PickHelper *pick_new0 = new PickHelper(pick0);
      // interposer
      child0->get_grid_lefts(pick_vcg_id_set0);
      pick_new0->get_items(pick_vcg_id_set0, items0);
      adjust_interposer_left(items0);

      child0->get_grid_bottoms(pick_vcg_id_set0);
      pick_new0->get_items(pick_vcg_id_set0, items0);
      adjust_interposer_bottom(items0);

      // update bottom box
      get_helper_box(pick_new0, box);
      pick_new0->set_box(box);

      child0->get_grid_tops(pick_vcg_id_set0);
      pick_new0->get_items(pick_vcg_id_set0, items0);

      for (auto pick1 : child1->get_picks())
      {
        if (is_pick_repeat(pick0, pick1))
          continue;

        PickHelper *pick_new1 = new PickHelper(pick1);
        // interposer
        child1->get_grid_lefts(pick_vcg_id_set1);
        pick_new1->get_items(pick_vcg_id_set1, items1);
        adjust_interposer_left(items1);

        // update top box
        get_helper_box(pick_new1, box);
        pick_new1->set_box(box);

        child1->get_grid_bottoms(pick_vcg_id_set1);
        pick_new1->get_items(pick_vcg_id_set1, items1);

        
        get_box_range_fit_bitems(items1, items0, pick_new0->get_box()._c3._y, range);
        int y_move = range._x;
        if (range._x > range._y)
        {
          y_move = range._y;

          g_log << "[VRT merging violate] occur in pt_node = "
                << std::to_string(pt_node->get_pt_id())
                << "range.min_x = " << range._x << ", "
                << "range.max_x = " << range._y << "\n";
        }

        for (auto r_item : pick_new1->get_items())
        {
          r_item->_c1_y += y_move;
        }
        // interposer
        items1.clear();
        pick_vcg_id_set1.clear();

        child1->get_grid_lefts(pick_vcg_id_set1);
        pick_new1->get_items(pick_vcg_id_set1, items1);
        adjust_interposer_left(items1);
        child1->get_grid_bottoms(pick_vcg_id_set1);
        pick_new1->get_items(pick_vcg_id_set1, items1);
        adjust_interposer_bottom(items1);

        for (auto pick2 : child2->get_picks())
        {
          if (is_pick_repeat(pick0, pick1) || is_pick_repeat(pick1, pick2) || is_pick_repeat(pick0, pick2))
            continue;

          PickHelper *pick_new2 = new PickHelper(pick2);

          // interposer
          child2->get_grid_bottoms(pick_vcg_id_set2);
          pick_new2->get_items(pick_vcg_id_set2, items2);
          adjust_interposer_bottom(items2);

          // update top box
          get_helper_box(pick_new2, box);
          pick_new2->set_box(box);

          child2->get_grid_lefts(pick_vcg_id_set2);
          pick_new2->get_items(pick_vcg_id_set2, items2);

          get_box_range_fit_litems(items2, items1, pick_new0->get_box()._c3._x, range);
          int x_move = range._x;
          if (range._x > range._y)
          {
            x_move = range._y;

            g_log << "[HRZ merging violate] occur in pt_node = "
                  << std::to_string(pt_node->get_pt_id())
                  << "range.min_x = " << range._x << ", "
                  << "range.max_x = " << range._y << "\n";
          }

          for (auto r_item : pick_new2->get_items())
          {
            r_item->_c1_x += x_move;
          }
          // interposer
          items2.clear();
          pick_vcg_id_set2.clear();
          child2->get_grid_lefts(pick_vcg_id_set2);
          pick_new2->get_items(pick_vcg_id_set2, items2);
          adjust_interposer_left(items2);

          // update top box
          get_helper_box(pick_new2, box);
          pick_new2->set_box(box);

          child2->get_grid_bottoms(pick_vcg_id_set2);
          pick_new2->get_items(pick_vcg_id_set2, items2);

          get_box_range_fit_bitems(items2, items0, pick_new0->get_box()._c3._y, range);
          int y_move = range._x;
          if (range._x > range._y)
          {
            y_move = range._y;

            g_log << "[VRT merging violate] occur in pt_node = "
                  << std::to_string(pt_node->get_pt_id())
                  << "range.min_x = " << range._x << ", "
                  << "range.max_x = " << range._y << "\n";
          }

          for (auto r_item : pick_new2->get_items())
          {
            r_item->_c1_y += y_move;
          }

          // interposer
          items2.clear();
          pick_vcg_id_set2.clear();

          child2->get_grid_lefts(pick_vcg_id_set2);
          pick_new2->get_items(pick_vcg_id_set2, items2);
          adjust_interposer_left(items2);
          child2->get_grid_bottoms(pick_vcg_id_set2);
          pick_new2->get_items(pick_vcg_id_set2, items2);
          adjust_interposer_bottom(items2);

          for (auto pick3 : child3->get_picks())
          {
            if (is_pick_repeat(pick0, pick1) || is_pick_repeat(pick1, pick2) || is_pick_repeat(pick2, pick3) || is_pick_repeat(pick0, pick2) || is_pick_repeat(pick0, pick3) || is_pick_repeat(pick1, pick3))
              continue;

            PickHelper *pick_new3 = new PickHelper(pick3);

            // interposer
            child3->get_grid_bottoms(pick_vcg_id_set3);
            pick_new3->get_items(pick_vcg_id_set3, items3);
            adjust_interposer_bottom(items3);

            // update top box
            get_helper_box(pick_new3, box);
            pick_new3->set_box(box);

            child3->get_grid_lefts(pick_vcg_id_set3);
            pick_new3->get_items(pick_vcg_id_set3, items3);

            get_box_range_fit_litems(items3, items1, pick_new0->get_box()._c3._x, range);
            int x_move = range._x;
            if (range._x > range._y)
            {
              x_move = range._y;

              g_log << "[HRZ merging violate] occur in pt_node = "
                    << std::to_string(pt_node->get_pt_id())
                    << "range.min_x = " << range._x << ", "
                    << "range.max_x = " << range._y << "\n";
            }

            for (auto r_item : pick_new3->get_items())
            {
              r_item->_c1_x += x_move;
            }

            // interposer
            items3.clear();
            pick_vcg_id_set3.clear();

            child3->get_grid_lefts(pick_vcg_id_set3);
            pick_new3->get_items(pick_vcg_id_set3, items3);
            adjust_interposer_left(items3);
            child3->get_grid_bottoms(pick_vcg_id_set3);
            pick_new3->get_items(pick_vcg_id_set3, items3);
            adjust_interposer_bottom(items3);

            for (auto pick4 : child4->get_picks())
            {
              if (is_pick_repeat(pick0, pick1) || is_pick_repeat(pick1, pick2) || is_pick_repeat(pick2, pick3) || is_pick_repeat(pick3, pick4) || is_pick_repeat(pick0, pick2) || is_pick_repeat(pick0, pick3) || is_pick_repeat(pick1, pick3) || is_pick_repeat(pick0, pick4) || is_pick_repeat(pick1, pick4) || is_pick_repeat(pick2, pick4))
                continue;

              PickHelper *pick_new4 = new PickHelper(pick4);

              // interposer
              child4->get_grid_lefts(pick_vcg_id_set4);
              pick_new4->get_items(pick_vcg_id_set4, items4);
              adjust_interposer_left(items4);

              // update top box
              get_helper_box(pick_new4, box);
              pick_new4->set_box(box);

              child4->get_grid_bottoms(pick_vcg_id_set4);
              pick_new4->get_items(pick_vcg_id_set4, items4);

              get_box_range_fit_bitems(items4, items3, pick_new0->get_box()._c3._y, range);
              int y_move = range._x;
              if (range._x > range._y)
              {
                y_move = range._y;

                g_log << "[VRT merging violate] occur in pt_node = "
                      << std::to_string(pt_node->get_pt_id())
                      << "range.min_x = " << range._x << ", "
                      << "range.max_x = " << range._y << "\n";
              }

              for (auto r_item : pick_new4->get_items())
              {
                r_item->_c1_y += y_move;
              }
              PickHelper *new_helper = new PickHelper(pick_new0, pick_new1, pick_new2, pick_new3, pick_new4);

              get_helper_box(new_helper, box);
              new_helper->set_box(box);
              int cell_area = get_cells_area(new_helper);
              new_helper->set_death(1.0 * (box.get_area() - cell_area) / box.get_area());

              // debug
              // debug_GDS(new_helper);

              if (!insert_death_que(death_queue, new_helper))
              {
                delete new_helper;
              }
              delete pick_new4;
            }
            delete pick_new3;
          }
          delete pick_new2;
        }
        delete pick_new1;
      }
      delete pick_new0;
    }
  }
  else
  {
    for (auto pick0 : child0->get_picks())
    {
      PickHelper *pick_new0 = new PickHelper(pick0);
      // interposer
      child0->get_grid_lefts(pick_vcg_id_set0);
      pick_new0->get_items(pick_vcg_id_set0, items0);
      adjust_interposer_left(items0);

      child0->get_grid_bottoms(pick_vcg_id_set0);
      pick_new0->get_items(pick_vcg_id_set0, items0);
      adjust_interposer_bottom(items0);

      // update bottom box
      get_helper_box(pick_new0, box);
      pick_new0->set_box(box);

      child0->get_grid_tops(pick_vcg_id_set0);
      pick_new0->get_items(pick_vcg_id_set0, items0);

      for (auto pick1 : child1->get_picks())
      {
        if (is_pick_repeat(pick0, pick1))
          continue;

        PickHelper *pick_new1 = new PickHelper(pick1);
        // interposer
        child1->get_grid_bottoms(pick_vcg_id_set1);
        pick_new1->get_items(pick_vcg_id_set1, items1);
        adjust_interposer_bottom(items1);

        // update top box
        get_helper_box(pick_new1, box);
        pick_new1->set_box(box);

        child1->get_grid_lefts(pick_vcg_id_set1);
        pick_new1->get_items(pick_vcg_id_set1, items1);

        Point range;
        get_box_range_fit_litems(items1, items0, pick_new0->get_box()._c3._x, range);
        int x_move = range._x;
        if (range._x > range._y)
        {
          x_move = range._y;

          g_log << "[HRZ merging violate] occur in pt_node = "
                << std::to_string(pt_node->get_pt_id())
                << "range.min_x = " << range._x << ", "
                << "range.max_x = " << range._y << "\n";
        }

        for (auto r_item : pick_new1->get_items())
        {
          r_item->_c1_x += x_move;
        }
        // interposer
        items1.clear();
        pick_vcg_id_set1.clear();

        child1->get_grid_lefts(pick_vcg_id_set1);
        pick_new1->get_items(pick_vcg_id_set1, items1);
        adjust_interposer_left(items1);
        child1->get_grid_bottoms(pick_vcg_id_set1);
        pick_new1->get_items(pick_vcg_id_set1, items1);
        adjust_interposer_bottom(items1);

        for (auto pick2 : child2->get_picks())
        {
          if (is_pick_repeat(pick0, pick1) || is_pick_repeat(pick1, pick2) || is_pick_repeat(pick0, pick2))
            continue;
          PickHelper *pick_new2 = new PickHelper(pick2);

          // interposer
          child2->get_grid_bottoms(pick_vcg_id_set2);
          pick_new2->get_items(pick_vcg_id_set2, items2);
          adjust_interposer_bottom(items2);

          // update top box
          get_helper_box(pick_new2, box);
          pick_new2->set_box(box);

          child2->get_grid_lefts(pick_vcg_id_set2);
          pick_new2->get_items(pick_vcg_id_set2, items2);

          Point range;
          get_box_range_fit_litems(items2, items1, pick_new0->get_box()._c3._x, range);
          int x_move = range._x;
          if (range._x > range._y)
          {
            x_move = range._y;

            g_log << "[HRZ merging violate] occur in pt_node = "
                  << std::to_string(pt_node->get_pt_id())
                  << "range.min_x = " << range._x << ", "
                  << "range.max_x = " << range._y << "\n";
          }

          for (auto r_item : pick_new2->get_items())
          {
            r_item->_c1_x += x_move;
          }
          // interposer
          items2.clear();
          pick_vcg_id_set2.clear();
          child2->get_grid_lefts(pick_vcg_id_set2);
          pick_new2->get_items(pick_vcg_id_set2, items2);
          adjust_interposer_left(items2);

          // update top box
          get_helper_box(pick_new2, box);
          pick_new2->set_box(box);

          child2->get_grid_bottoms(pick_vcg_id_set2);
          pick_new2->get_items(pick_vcg_id_set2, items2);

          get_box_range_fit_bitems(items2, items0, pick_new0->get_box()._c3._y, range);
          int y_move = range._x;
          if (range._x > range._y)
          {
            y_move = range._y;

            g_log << "[VRT merging violate] occur in pt_node = "
                  << std::to_string(pt_node->get_pt_id())
                  << "range.min_x = " << range._x << ", "
                  << "range.max_x = " << range._y << "\n";
          }

          for (auto r_item : pick_new2->get_items())
          {
            r_item->_c1_y += y_move;
          }

          // interposer
          items2.clear();
          pick_vcg_id_set2.clear();

          child2->get_grid_lefts(pick_vcg_id_set2);
          pick_new2->get_items(pick_vcg_id_set2, items2);
          adjust_interposer_left(items2);
          child2->get_grid_bottoms(pick_vcg_id_set2);
          pick_new2->get_items(pick_vcg_id_set2, items2);
          adjust_interposer_bottom(items2);

          for (auto pick3 : child3->get_picks())
          {
            if (is_pick_repeat(pick0, pick1) || is_pick_repeat(pick1, pick2) || is_pick_repeat(pick2, pick3) || is_pick_repeat(pick0, pick2) || is_pick_repeat(pick0, pick3) || is_pick_repeat(pick1, pick3))
              continue;

            PickHelper *pick_new3 = new PickHelper(pick3);

            // interposer
            child3->get_grid_lefts(pick_vcg_id_set3);
            pick_new3->get_items(pick_vcg_id_set3, items3);
            adjust_interposer_left(items3);

            // update top box
            get_helper_box(pick_new3, box);
            pick_new3->set_box(box);

            child3->get_grid_bottoms(pick_vcg_id_set3);
            pick_new3->get_items(pick_vcg_id_set3, items3);

            Point range;
            get_box_range_fit_bitems(items3, items1, pick_new0->get_box()._c3._y, range);
            int y_move = range._x;
            if (range._x > range._y)
            {
              y_move = range._y;

              g_log << "[VRT merging violate] occur in pt_node = "
                    << std::to_string(pt_node->get_pt_id())
                    << "range.min_x = " << range._x << ", "
                    << "range.max_x = " << range._y << "\n";
            }

            for (auto r_item : pick_new3->get_items())
            {
              r_item->_c1_y += y_move;
            }

            // interposer
            items3.clear();
            pick_vcg_id_set3.clear();

            child3->get_grid_lefts(pick_vcg_id_set3);
            pick_new3->get_items(pick_vcg_id_set3, items3);
            adjust_interposer_left(items3);
            child3->get_grid_bottoms(pick_vcg_id_set3);
            pick_new3->get_items(pick_vcg_id_set3, items3);
            adjust_interposer_bottom(items3);

            for (auto pick4 : child4->get_picks())
            {
              if (is_pick_repeat(pick0, pick1) || is_pick_repeat(pick1, pick2) || is_pick_repeat(pick2, pick3) || is_pick_repeat(pick3, pick4) || is_pick_repeat(pick0, pick2) || is_pick_repeat(pick0, pick3) || is_pick_repeat(pick1, pick3) || is_pick_repeat(pick0, pick4) || is_pick_repeat(pick1, pick4) || is_pick_repeat(pick2, pick4))
                continue;

              PickHelper *pick_new4 = new PickHelper(pick4);

              // interposer
              child4->get_grid_bottoms(pick_vcg_id_set4);
              pick_new4->get_items(pick_vcg_id_set4, items4);
              adjust_interposer_bottom(items4);

              // update top box
              get_helper_box(pick_new4, box);
              pick_new4->set_box(box);

              child4->get_grid_lefts(pick_vcg_id_set4);
              pick_new4->get_items(pick_vcg_id_set4, items4);

              Point range;
              get_box_range_fit_litems(items4, items3, pick_new0->get_box()._c3._x, range);
              int x_move = range._x;
              if (range._x > range._y)
              {
                x_move = range._y;

                g_log << "[HRZ merging violate] occur in pt_node = "
                      << std::to_string(pt_node->get_pt_id())
                      << "range.min_x = " << range._x << ", "
                      << "range.max_x = " << range._y << "\n";
              }

              for (auto r_item : pick_new4->get_items())
              {
                r_item->_c1_y += x_move;
              }
              PickHelper *new_helper = new PickHelper(pick_new0, pick_new1, pick_new2, pick_new3, pick_new4);

              get_helper_box(new_helper, box);
              new_helper->set_box(box);
              int cell_area = get_cells_area(new_helper);
              new_helper->set_death(1.0 * (box.get_area() - cell_area) / box.get_area());

              // debug
              // debug_GDS(new_helper);

              if (!insert_death_que(death_queue, new_helper))
              {
                delete new_helper;
              }
              delete pick_new4;
            }
            delete pick_new3;
          }
          delete pick_new2;
        }
        delete pick_new1;
      }
      delete pick_new0;
    }
  }
}

 PickHelper::PickHelper(PickHelper *h1, PickHelper *h2, PickHelper *h3, PickHelper *h4, PickHelper *h5)
{
  ASSERT(h1 && h2 && h3 && h4 && h5, "Please enter valid data");

  for (auto item : h1->get_items())
  {
    _items.push_back(new PickItem(*item));
  }
  for (auto item : h2->get_items())
  {
    _items.push_back(new PickItem(*item));
  }
  for (auto item : h3->get_items())
  {
    _items.push_back(new PickItem(*item));
  }
  for (auto item : h4->get_items())
  {
    _items.push_back(new PickItem(*item));
  }
  for (auto item : h5->get_items())
  {
    _items.push_back(new PickItem(*item));
  }
  for (auto item : _items)
  {
    _id_item_map[item->_vcg_id] = item;
  }
}

// <<<<<<<


}  // namespace  EDA_CHALLENGE_Q4