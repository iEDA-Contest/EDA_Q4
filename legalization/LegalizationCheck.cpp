/*
 * @Author: your name
 * @Date: 2021-12-10 21:00:11
 * @LastEditTime: 2021-12-11 23:24:01
 * @LastEditors: Please set LastEditors
 * @Description: 打开koroFileHeader查看配置 进行设置:
 * https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: /EDA_Q4/legalization/LegalizationCheck.cpp
 */

#include "LegalizationCheck.h"

#include <iostream>

namespace EDA_CHALLENGE_Q4 {

bool LegalizationCheck::isNumLegal(int num, Point range) {
  // range x y is the same represet the num must bigger.
  if (range._x == range._y) {
    if (num >= range._x) {
      return true;
    } else {
      return false;
    }
  }

  if (num >= range._x && num <= range._y) {
    return true;
  } else {
    return false;
  }
}

bool LegalizationCheck::isLeftBoundaryViolated(VCGNode* node) {
  auto cell = node->get_cell();

  auto node_it = _vertex_to_boundary_x_range.find(node);
  if (node_it == _vertex_to_boundary_x_range.end()) {
    std::cout << "Cell : " << cell->get_cell_id()
              << " x range has not been initilized." << std::endl;
    exit(1);
  }

  if (isNumLegal(cell->get_x(), node_it->second)) {
    return true;
  } else {
    return false;
  }
}

bool LegalizationCheck::isDownBoundaryViolated(VCGNode* node) {
  auto cell = node->get_cell();

  auto node_it = _vertex_to_boundary_y_range.find(node);
  if (node_it == _vertex_to_boundary_y_range.end()) {
    std::cout << "Cell : " << cell->get_cell_id()
              << " y range has not been initilized." << std::endl;
    exit(1);
  }

  if (isNumLegal(cell->get_y(), node_it->second)) {
    return true;
  } else {
    return false;
  }
}

Point LegalizationCheck::obtainLeftXBoundaryRange(VCGNode* node) {
  auto cell = node->get_cell();

  auto node_it = _vertex_to_boundary_x_range.find(node);
  if (node_it == _vertex_to_boundary_x_range.end()) {
    std::cout << "Cell : " << cell->get_cell_id()
              << " y range has not been initilized." << std::endl;
    exit(1);
  }
  return node_it->second;
}

Point LegalizationCheck::obtainDownYBoundaryRange(VCGNode* node) {
  auto cell = node->get_cell();

  auto node_it = _vertex_to_boundary_y_range.find(node);
  if (node_it == _vertex_to_boundary_y_range.end()) {
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
    _vertex_to_boundary_x_range[node] = x_range;
  } else {
    int box_right_x;  // TODO
    _vertex_to_boundary_x_range[node] = Point(box_right_x, box_right_x);
  }

  bool is_interposer_bottom =
      _vcg->get_pattern_tree()->is_interposer_bottom(node->get_vcg_id());
  if (is_interposer_bottom) {
    Point y_range;
    _vcg->get_cst_y(node->get_vcg_id(), y_range);
    _vertex_to_boundary_y_range[node] = y_range;
  } else {
    int box_upper_y;  // TODO
    _vertex_to_boundary_y_range[node] = Point(box_upper_y, box_upper_y);
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

void LegalizationCheck::initVertexLocInfo() {
  auto origin_nodes = _vcg->get_vcg_nodes();
  std::vector<VCGNode*> nodes;
  nodes.assign(origin_nodes.begin() + 1, origin_nodes.end() - 1);

  for (auto node : nodes) {
    auto cell = node->get_cell();
    int x_coord = cell->get_x();
    int y_coord = cell->get_y();

    addXToVertexes(x_coord, node);
    addYToVertexes(y_coord, node);
  }
}

void LegalizationCheck::set_left_x_boundary_range(VCGNode* node, Point range) {
  _vertex_to_boundary_x_range[node] = range;
}

void LegalizationCheck::set_down_y_boundary_range(VCGNode* node, Point range) {
  _vertex_to_boundary_y_range[node] = range;
}

}  // namespace EDA_CHALLENGE_Q4