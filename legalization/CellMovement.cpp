/*
 * @Author: sjchanson
 * @Date: 2021-12-10 21:01:11
 * @LastEditTime: 2021-12-11 23:31:58
 * @LastEditors: Please set LastEditors
 * @Description: 打开koroFileHeader查看配置 进行设置:
 * https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: /EDA_Q4/legalization/CellMovement.cpp
 */

#include "CellMovement.h"

#include <queue>

namespace EDA_CHALLENGE_Q4 {

void CellMovement::executeCellMovement() {
  auto start_vertex = *(_vcg->get_vcg_nodes().rbegin());
  auto end_vertex = *(_vcg->get_vcg_nodes().begin());

  std::queue<VCGNode*> vertex_queue;
  vertex_queue.push(start_vertex);

  while (!vertex_queue.empty()) {
    auto current_vertex = vertex_queue.front();

    // terminate condition.
    if (current_vertex->get_vcg_id() == end_vertex->get_vcg_id()) {
      vertex_queue.pop();
      continue;
    }

    auto next_vertexes = current_vertex->get_froms();

    // make sure the vertexes is ordinal.
    std::map<int, VCGNode*> ordinal_vertexes;
    for (auto next_vertex : next_vertexes) {
      ordinal_vertexes.emplace(next_vertex->get_vcg_id(), next_vertex);
    }

    for (auto pair : ordinal_vertexes) {
      auto vertex = pair.second;
      auto cell = vertex->get_cell();
      _checker->updateVertexXYCoordRange(vertex);

      // step 1 : fit the boundary condition.
      if (_checker->isLeftBoundaryViolated(vertex)) {
        auto range = _checker->obtainLeftXBoundaryRange(vertex);
        Point new_coord(range._x, vertex->get_cell()->get_y());
        _checker->updateVertexLocInfo(current_vertex, new_coord);
      }
      if (_checker->isDownBoundaryViolated(vertex)) {
        auto range = _checker->obtainDownYBoundaryRange(vertex);
        Point new_coord(vertex->get_cell()->get_x(), range._x);
        _checker->updateVertexLocInfo(current_vertex, new_coord);
      }

      // step 2 : fit the left-right constraint condition.
      auto left_vertexes = _checker->obtainXLeftNodes(
          cell->get_x(), Point(cell->get_c1()._y, cell->get_c3()._y));

      checkLegalAndMoveCell(left_vertexes, vertex);
    }
  }
}

VertexesXConstraint CellMovement::obtainVertexesXConstraint(
    VCGNode* left_node, VCGNode* right_node) {
  VertexesXConstraint* vertexes_x_cst;
  Point x_range;
  DISTANCE_TYPE current_type;

  _vcg->get_cst_x(left_node->get_vcg_id(), right_node->get_vcg_id(), x_range);

  int left_x_upper = left_node->get_cell()->get_c3()._x;
  int right_x_lower = right_node->get_cell()->get_c1()._x;
  int value = right_x_lower - left_x_upper;

  if (value < x_range._x) {
    current_type = kDISTANCE_MIN;
  } else if (value > x_range._y) {
    current_type = kDISTANCE_MAX;
  } else {
    current_type = kDISTANCE_NORMAL;
  }

  return VertexesXConstraint(left_node, right_node, current_type, value,
                             x_range);
}

bool CellMovement::checkLegalAndMoveCell(std::vector<VCGNode*> left_vertexes,
                                         VCGNode* current_vertex) {
  std::vector<VertexesXConstraint> vertexes_x_csts;
  for (auto left_vertex : left_vertexes) {
    auto vertexes_x_cst =
        obtainVertexesXConstraint(left_vertex, current_vertex);
    vertexes_x_csts.push_back(vertexes_x_cst);
  }

  bool both_normal = true;
  bool both_max = false;
  bool occur_min = false;
  VertexesXConstraint min_type_cst;

  for (auto cst : vertexes_x_csts) {
    if (cst.type == kDISTANCE_MIN) {
      occur_min = true;
      both_max = false;
      both_normal = false;
      min_type_cst = cst;
      break;
    }
    if (cst.type == kDISTANCE_MAX) {
      both_max = true;
      both_normal = false;
    }
  }

  if (both_normal) {
    return true;
  }

  if (occur_min) {
    int move_right_value = min_type_cst.x_range._x - min_type_cst.interval;
    Point new_coord(current_vertex->get_cell()->get_x() + move_right_value,
                    current_vertex->get_cell()->get_y());
    _checker->updateVertexLocInfo(current_vertex, new_coord);
    int max_move_value =
        new_coord._x + (min_type_cst.x_range._y - min_type_cst.x_range._x);
    Point move_range(new_coord._x, max_move_value);
    _checker->set_left_x_boundary_range(current_vertex, move_range);
    return false;
  }

  if (both_max) {
    // dectet if can solve by moving cell.
    for (auto cst : vertexes_x_csts) {
      int move_right_value = cst.interval - cst.x_range._y;
      Point new_coord(cst.left_node->get_cell()->get_x() + move_right_value,
                      cst.left_node->get_cell()->get_y());
      _checker->updateVertexLocInfo(cst.left_node, new_coord);
    }

    return false;
  }
}

}  // namespace EDA_CHALLENGE_Q4