/*
 * @Author: your name
 * @Date: 2021-12-10 21:00:11
 * @LastEditTime: 2021-12-12 16:04:15
 * @LastEditors: Please set LastEditors
 * @Description: 打开koroFileHeader查看配置 进行设置:
 * https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: /EDA_Q4/legalization/LegalizationCheck.cpp
 */

#include "LegalizationCheck.h"

#include <limits.h>

#include <iostream>

namespace EDA_CHALLENGE_Q4 {

bool LegalizationCheck::isNumLegal(int num, Point range) {
  if (num >= range._x && num <= range._y) {
    return true;
  } else {
    return false;
  }
}

bool LegalizationCheck::isLeftBoundaryViolated(VCGNode* node,
                                               int boundary_value) {
  auto cell = node->get_cell();

  if (cell->get_x() < boundary_value) {
    return true;
  } else {
    return false;
  }
}

bool LegalizationCheck::isDownBoundaryViolated(VCGNode* node,
                                               int boundary_value) {
  auto cell = node->get_cell();

  if (cell->get_y() < boundary_value) {
    return true;
  } else {
    return false;
  }
}

bool LegalizationCheck::isLeftVertexViolated(VCGNode* left_node,
                                             VCGNode* right_node) {
  auto constraint_range =
      obtainLeftVertexXConstraintRange(left_node, right_node);

  if (isNumLegal(left_node->get_cell()->get_x(), constraint_range)) {
    return true;
  } else {
    return false;
  }
}

bool LegalizationCheck::isDownVertexViolated(VCGNode* down_node,
                                             VCGNode* up_node) {
  auto constraint_range = obtainDownVertexYConstraintRange(down_node, up_node);

  if (isNumLegal(down_node->get_cell()->get_y(), constraint_range)) {
    return true;
  } else {
    return false;
  }
}

Point LegalizationCheck::obtainLeftXRange(VCGNode* node) {
  auto cell = node->get_cell();

  auto node_it = _vertex_to_x_range.find(node);
  if (node_it == _vertex_to_x_range.end()) {
    std::cout << "Cell : " << cell->get_cell_id()
              << " y range has not been initilized." << std::endl;
    exit(1);
  }
  return node_it->second;
}

Point LegalizationCheck::obtainDownYRange(VCGNode* node) {
  auto cell = node->get_cell();

  auto node_it = _vertex_to_y_range.find(node);
  if (node_it == _vertex_to_y_range.end()) {
    std::cout << "Cell : " << cell->get_cell_id()
              << " y range has not been initilized." << std::endl;
    exit(1);
  }

  return node_it->second;
}

std::vector<VCGNode*> LegalizationCheck::obtainXLeftNodes(int x_coord,
                                                          Point y_range) {
  //
  std::vector<VCGNode*> candidate_nodes;
  for (auto pair : _x_to_vertexes) {
    if (pair.first <= x_coord) {
      continue;
    }
    if (pair.first >= x_coord) {
      break;
    }

    candidate_nodes.insert(candidate_nodes.end(), pair.second.begin(),
                           pair.second.end());
  }

  std::vector<VCGNode*> left_nodes;
  for (auto node : candidate_nodes) {
    auto cell = node->get_cell();
    if (cell->get_x() < x_coord) {
      left_nodes.push_back(node);
    }
  }

  return left_nodes;
}

std::vector<VCGNode*> LegalizationCheck::obtainYDownNodes(int y_coord,
                                                          Point x_range) {
  //
  std::vector<VCGNode*> candidate_nodes;
  for (auto pair : _y_to_vertexes) {
    if (pair.first <= y_coord) {
      continue;
    }
    if (pair.first >= y_coord) {
      break;
    }

    candidate_nodes.insert(candidate_nodes.end(), pair.second.begin(),
                           pair.second.end());
  }

  std::vector<VCGNode*> down_nodes;
  for (auto node : candidate_nodes) {
    auto cell = node->get_cell();
    if (cell->get_y() < y_coord) {
      down_nodes.push_back(node);
    }
  }

  return down_nodes;
}

void LegalizationCheck::updateVertexXYCoordRange(VCGNode* node) {
  auto cell = node->get_cell();

  bool is_interposer_left =
      _vcg->get_pattern_tree()->is_interposer_left(node->get_vcg_id());
  if (is_interposer_left) {
    Point x_range;
    _vcg->get_cst_x(node->get_vcg_id(), x_range);
    _vertex_to_x_range[node] = x_range;
  } else {
    int box_right_x;  // TODO
    _vertex_to_x_range[node] = Point(box_right_x, box_right_x);
  }

  bool is_interposer_bottom =
      _vcg->get_pattern_tree()->is_interposer_bottom(node->get_vcg_id());
  if (is_interposer_bottom) {
    Point y_range;
    _vcg->get_cst_y(node->get_vcg_id(), y_range);
    _vertex_to_y_range[node] = y_range;
  } else {
    int box_upper_y;  // TODO
    _vertex_to_y_range[node] = Point(box_upper_y, box_upper_y);
  }
}

void LegalizationCheck::updateVertexLocInfo(VCGNode* node, Point new_coord) {
  auto cell = node->get_cell();
  eraseXToVertexes(cell->get_x(), node);
  eraseYToVertexes(cell->get_y(), node);

  cell->set_positon(new_coord._x, new_coord._y);

  addXToVertexes(cell->get_x(), node);
  addYToVertexes(cell->get_y(), node);
  updateVertexXYCoordRange(node);
}

void LegalizationCheck::updateVertexXCoord(VCGNode* node, int x_coord) {
  auto cell = node->get_cell();
  eraseXToVertexes(cell->get_x(), node);

  cell->set_x(x_coord);

  addXToVertexes(cell->get_x(), node);
}

void LegalizationCheck::updateVertexYCoord(VCGNode* node, int y_coord) {
  auto cell = node->get_cell();
  eraseYToVertexes(cell->get_y(), node);

  cell->set_y(y_coord);

  addYToVertexes(cell->get_y(), node);
}

void LegalizationCheck::eraseXToVertexes(int x_coord, VCGNode* node) {
  auto it = _x_to_vertexes.find(x_coord);

  if (it == _x_to_vertexes.end()) {
    std::cout << "Error In delete node coordinate map." << std::endl;
    exit(1);
  }

  auto node_it = std::find_if(
      it->second.begin(), it->second.end(), [node](VCGNode* target_node) {
        return node->get_vcg_id() == target_node->get_vcg_id();
      });
  if (node_it == it->second.end()) {
    std::cout << "Error In delete node coordinate map." << std::endl;
    exit(1);
  }
  it->second.erase(node_it);
}

void LegalizationCheck::eraseYToVertexes(int y_coord, VCGNode* node) {
  auto it = _y_to_vertexes.find(y_coord);

  if (it == _y_to_vertexes.end()) {
    std::cout << "Error In delete node coordinate map." << std::endl;
    exit(1);
  }

  auto node_it = std::find_if(
      it->second.begin(), it->second.end(), [node](VCGNode* target_node) {
        return node->get_vcg_id() == target_node->get_vcg_id();
      });
  if (node_it == it->second.end()) {
    std::cout << "Error In delete node coordinate map." << std::endl;
    exit(1);
  }
  it->second.erase(node_it);
}

void LegalizationCheck::addXToVertexes(int x_coord, VCGNode* node) {
  auto it = _x_to_vertexes.find(x_coord);
  if (it == _x_to_vertexes.end()) {
    std::vector<VCGNode*> nodes{node};
    _x_to_vertexes.emplace(x_coord, nodes);
  } else {
    auto& nodes = it->second;
    nodes.push_back(node);
  }
}

void LegalizationCheck::addYToVertexes(int y_coord, VCGNode* node) {
  auto it = _y_to_vertexes.find(y_coord);
  if (it == _y_to_vertexes.end()) {
    std::vector<VCGNode*> nodes{node};
    _x_to_vertexes.emplace(y_coord, nodes);
  } else {
    auto& nodes = it->second;
    nodes.push_back(node);
  }
}

void LegalizationCheck::initVertexesInfo() {
  auto origin_nodes = _vcg->get_vcg_nodes();
  std::vector<VCGNode*> nodes;
  nodes.assign(origin_nodes.begin() + 1, origin_nodes.end() - 1);

  for (auto node : nodes) {
    // set the interposer info.
    if (isLeftInterposerVertex(node)) {
      Point x_range;
      _vcg->get_cst_x(node->get_vcg_id(), x_range);
      _vertex_to_x_range[node] = x_range;
    }

    if (isBottomInterposerVertex(node)) {
      Point y_range;
      _vcg->get_cst_y(node->get_vcg_id(), y_range);
      _vertex_to_y_range[node] = y_range;
    }

    // set the location info.
    auto cell = node->get_cell();
    int x_coord = cell->get_x();
    int y_coord = cell->get_y();

    addXToVertexes(x_coord, node);
    addYToVertexes(y_coord, node);
  }
}

void LegalizationCheck::set_left_x_range(VCGNode* node, Point range) {
  _vertex_to_x_range[node] = range;
}

void LegalizationCheck::set_down_y_range(VCGNode* node, Point range) {
  _vertex_to_y_range[node] = range;
}

bool LegalizationCheck::isLeftInterposerVertex(VCGNode* node) {
  bool is_interposer_left =
      _vcg->get_pattern_tree()->is_interposer_left(node->get_vcg_id());
  return is_interposer_left;
}

bool LegalizationCheck::isBottomInterposerVertex(VCGNode* node) {
  bool is_interposer_bottom =
      _vcg->get_pattern_tree()->is_interposer_bottom(node->get_vcg_id());
  return is_interposer_bottom;
}

Point LegalizationCheck::obtainTopDownVertexesXRange(VCGNode* vertex) {
  auto cell = vertex->get_cell();

  int left_max = INT_MIN;
  int right_min = INT_MAX;

  std::vector<VCGNode*> one_hop_childs = vertex->get_tos();
  std::vector<VCGNode*> one_hop_parent = vertex->get_froms();

  for (auto child : one_hop_childs) {
    auto child_cell = child->get_cell();
    if (child_cell->get_x() > left_max) {
      left_max = child_cell->get_x();
    }

    if (child_cell->get_c3()._x < right_min) {
      right_min = child_cell->get_c3()._x;
    }
  }

  for (auto master : one_hop_parent) {
    auto master_cell = master->get_cell();
    if (master_cell->get_x() > left_max) {
      left_max = master_cell->get_x();
    }

    if (master_cell->get_c3()._x < right_min) {
      right_min = master_cell->get_c3()._x;
    }
  }

  if (right_min < left_max) {
    std::cout << "Error In Range : obtainTopDownVertexesXRange" << std::endl;
    exit(1);
  }

  return Point(left_max, right_min);
}

Point LegalizationCheck::obtainLeftVertexXConstraintRange(VCGNode* left_node,
                                                          VCGNode* right_node) {
  auto left_cell = left_node->get_cell();
  auto right_cell = right_node->get_cell();

  Point cells_range;
  _vcg->get_cst_x(left_node->get_vcg_id(), right_node->get_vcg_id(),
                  cells_range);

  int left_cell_width = left_cell->get_width();
  int right_cell_x = right_cell->get_x();

  int left_max = right_cell_x - cells_range._y - left_cell_width;
  int right_min = right_cell_x - cells_range._x - left_cell_width;

  if (right_min < left_max) {
    std::cout << "Error In Range : obtainLeftVertexXConstraintRange"
              << std::endl;
    exit(1);
  }

  return Point(left_max, right_min);
}

Point LegalizationCheck::obtainDownVertexYConstraintRange(VCGNode* down_node,
                                                          VCGNode* up_node) {
  auto left_cell = down_node->get_cell();
  auto right_cell = up_node->get_cell();

  Point cells_range;
  _vcg->get_cst_y(down_node->get_vcg_id(), up_node->get_vcg_id(), cells_range);

  int down_cell_height = left_cell->get_height();
  int up_cell_y = right_cell->get_y();

  int down_max = up_cell_y - cells_range._y - down_cell_height;
  int up_min = up_cell_y - cells_range._x - down_cell_height;

  if (up_min < down_max) {
    std::cout << "Error In Range : obtainDownVertexYConstraintRange"
              << std::endl;
    exit(1);
  }

  return Point(down_max, up_min);
}

Point LegalizationCheck::obtainRightVertexXConstraintRange(
    VCGNode* left_node, VCGNode* right_node) {
  auto left_cell = left_node->get_cell();
  auto right_cell = right_node->get_cell();

  Point cells_range;
  _vcg->get_cst_x(left_node->get_vcg_id(), right_node->get_vcg_id(),
                  cells_range);

  int left_cell_right_x = left_cell->get_c3()._x;

  int left_max = left_cell_right_x + cells_range._x;
  int right_min = left_cell_right_x + cells_range._x;

  if (right_min < left_max) {
    std::cout << "Error In Range : obtainRightVertexXConstraintRange"
              << std::endl;
    exit(1);
  }

  return Point(left_max, right_min);
}

Point LegalizationCheck::obtainUpVertexYConstraintRange(VCGNode* down_node,
                                                        VCGNode* up_node) {
  auto left_cell = down_node->get_cell();
  auto right_cell = up_node->get_cell();

  Point cells_range;
  _vcg->get_cst_y(down_node->get_vcg_id(), up_node->get_vcg_id(), cells_range);

  int down_cell_up_y = left_cell->get_c3()._y;

  int down_max = down_cell_up_y + cells_range._x;
  int up_min = down_cell_up_y + cells_range._x;

  if (up_min < down_max) {
    std::cout << "Error In Range : obtainUpVertexYConstraintRange" << std::endl;
    exit(1);
  }

  return Point(down_max, up_min);
}

Point LegalizationCheck::obtainLeftVertexXMovableRange(VCGNode* left_node) {
  auto left_cell = left_node->get_cell();
  Point movable_range = _vertex_to_x_range[left_node];

  int left_cell_x = left_cell->get_x();

  int left_max = left_cell_x + movable_range._x;
  int right_min = left_cell_x + movable_range._y;

  if (right_min < left_max) {
    std::cout << "Error In Range : obtainLeftVertexXMovableRange" << std::endl;
    exit(1);
  }

  return Point(left_max, right_min);
}

Point LegalizationCheck::obtainDownVertexYMovableRange(VCGNode* down_node) {
  auto down_cell = down_node->get_cell();
  Point movable_range = _vertex_to_y_range[down_node];

  int down_cell_y = down_cell->get_y();

  int down_max = down_cell_y + movable_range._x;
  int up_min = down_cell_y + movable_range._y;

  if (up_min < down_max) {
    std::cout << "Error In Range : obtainDownVertexYMovableRange" << std::endl;
    exit(1);
  }

  return Point(down_max, up_min);
}

int LegalizationCheck::obtainBoundaryRightXCoord(VCGNode* left_node,
                                                 VCGNode* right_node) {
  // LCA.
  std::set<VCGNode*> right_node_froms;
  std::set<VCGNode*> right_node_tos;

  VCGNode* right_current_from_node = right_node;
  while (right_current_from_node->get_cell()) {
    // pick the left side froms.
    auto next_vertexes = right_current_from_node->get_froms();
    // make sure the vertexes is ordinal.
    std::map<int, VCGNode*> ordinal_vertexes;
    for (auto next_vertex : next_vertexes) {
      ordinal_vertexes.emplace(next_vertex->get_vcg_id(), next_vertex);
    }

    right_current_from_node = ordinal_vertexes.begin()->second;
    right_node_froms.emplace(right_current_from_node);
  }

  VCGNode* right_current_to_node = right_node;
  while (right_current_to_node->get_cell()) {
    // pick the left side froms.
    auto next_vertexes = right_current_to_node->get_tos();
    // make sure the vertexes is ordinal.
    std::map<int, VCGNode*> ordinal_vertexes;
    for (auto next_vertex : next_vertexes) {
      ordinal_vertexes.emplace(next_vertex->get_vcg_id(), next_vertex);
    }

    right_current_to_node = ordinal_vertexes.begin()->second;
    right_node_tos.emplace(right_current_to_node);
  }

  std::vector<VCGNode*> candidate_vertexes;

  VCGNode* left_current_from_node = left_node;
  while (left_current_from_node->get_cell()) {
    // pick the right side froms.
    auto next_vertexes = left_current_from_node->get_froms();
    // make sure the vertexes is ordinal.
    std::map<int, VCGNode*> ordinal_vertexes;
    for (auto next_vertex : next_vertexes) {
      ordinal_vertexes.emplace(next_vertex->get_vcg_id(), next_vertex);
    }

    left_current_from_node = ordinal_vertexes.rbegin()->second;

    auto it = right_node_froms.find(left_current_from_node);
    if (it != right_node_froms.end()) {
      break;
    }

    candidate_vertexes.push_back(left_current_from_node);
  }

  VCGNode* left_current_to_node = left_node;
  while (left_current_to_node->get_cell()) {
    // pick the right side froms.
    auto next_vertexes = left_current_to_node->get_tos();
    // make sure the vertexes is ordinal.
    std::map<int, VCGNode*> ordinal_vertexes;
    for (auto next_vertex : next_vertexes) {
      ordinal_vertexes.emplace(next_vertex->get_vcg_id(), next_vertex);
    }

    left_current_to_node = ordinal_vertexes.rbegin()->second;

    auto it = right_node_tos.find(left_current_to_node);
    if (it != right_node_tos.end()) {
      break;
    }

    candidate_vertexes.push_back(left_current_to_node);
  }

  int boundary_right_max = INT_MIN;
  for (auto vertex : candidate_vertexes) {
    auto right_x = vertex->get_cell()->get_c3()._x;
    if (right_x > boundary_right_max) {
      boundary_right_max = right_x;
    }
  }

  return boundary_right_max;
}

int LegalizationCheck::obtainBoundaryUpYCoord(VCGNode* down_node,
                                              VCGNode* up_node) {
  auto up_node_tos = up_node->get_tos();

  int boundary_up_max = INT_MIN;
  for (auto to : up_node_tos) {
    auto up_y = to->get_cell()->get_c3()._y;
    if (up_y > boundary_up_max) {
      boundary_up_max = up_y;
    }
  }

  return boundary_up_max;
}

}  // namespace EDA_CHALLENGE_Q4