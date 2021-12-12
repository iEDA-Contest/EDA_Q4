/*
 * @Author: sjchanson
 * @Date: 2021-12-10 21:01:11
 * @LastEditTime: 2021-12-12 16:08:12
 * @LastEditors: Please set LastEditors
 * @Description: 打开koroFileHeader查看配置 进行设置:
 * https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: /EDA_Q4/legalization/CellMovement.cpp
 */

#include "CellMovement.h"

#include <limits.h>

#include <iostream>
#include <queue>

namespace EDA_CHALLENGE_Q4 {

void CellMovement::executeCellMovement() {
  std::stack<VCGNode*> vertex_stack;

  vertex_stack.push(_vcg->get_node(_vcg->get_vertex_num() - 1));
  cellMovementByDFS(vertex_stack);

  // auto start_vertex = *(_vcg->get_vcg_nodes().rbegin());
  // auto end_vertex = *(_vcg->get_vcg_nodes().begin());

  // std::queue<VCGNode*> vertex_queue;
  // vertex_queue.push(start_vertex);

  // while (!vertex_queue.empty()) {
  //   auto current_vertex = vertex_queue.front();

  //   // terminate condition.
  //   if (current_vertex->get_vcg_id() == end_vertex->get_vcg_id()) {
  //     vertex_queue.pop();
  //     continue;
  //   }

  //   auto next_vertexes = current_vertex->get_froms();

  //   // make sure the vertexes is ordinal.
  //   std::map<int, VCGNode*> ordinal_vertexes;
  //   for (auto next_vertex : next_vertexes) {
  //     ordinal_vertexes.emplace(next_vertex->get_vcg_id(), next_vertex);
  //   }

  //   for (auto pair : ordinal_vertexes) {
  //     auto vertex = pair.second;
  //     auto cell = vertex->get_cell();

  //     _checker->updateVertexXYCoordRange(vertex);

  //     // step 1 : fit the boundary condition.
  //     if (_checker->isLeftBoundaryViolated(vertex)) {
  //       auto range = _checker->obtainLeftXRange(vertex);
  //       Point new_coord(range._x, vertex->get_cell()->get_y());
  //       _checker->updateVertexLocInfo(current_vertex, new_coord);
  //     }
  //     if (_checker->isDownBoundaryViolated(vertex)) {
  //       auto range = _checker->obtainDownYRange(vertex);
  //       Point new_coord(vertex->get_cell()->get_x(), range._x);
  //       _checker->updateVertexLocInfo(current_vertex, new_coord);
  //     }

  //     // step 2 : fit the left-right constraint condition.
  //     auto left_vertexes = _checker->obtainXLeftNodes(
  //         cell->get_x(), Point(cell->get_c1()._y, cell->get_c3()._y));

  //     checkLegalAndMoveCell(left_vertexes, vertex);
  //   }
  // }
}

void CellMovement::cellMovementByDFS(std::stack<VCGNode*>& vertex_stack) {
  if (vertex_stack.empty()) {
    std::cout << "Error In Stack" << std::endl;
    exit(1);
  }

  auto current_vertex = vertex_stack.top();
  auto current_cell = current_vertex->get_cell();

  // termination condition.
  if (!current_cell) {
    vertex_stack.pop();
    return;
  }

  // cell movement.
  bool is_interposer_bottom =
      _checker->isBottomInterposerVertex(current_vertex);
  if (is_interposer_bottom) {
    int boundary_value = 0 + _checker->obtainDownYRange(current_vertex)._x;

    if (_checker->isDownBoundaryViolated(current_vertex, boundary_value)) {
      _checker->updateVertexYCoord(current_vertex, boundary_value);
    }
  } else {
    auto down_vertexes = _checker->obtainYDownNodes(
        current_cell->get_y(),
        Point(current_cell->get_c1()._x, current_cell->get_c3()._x));

    for (auto down_vertex : down_vertexes) {
      int box_up_y =
          _checker->obtainBoundaryUpYCoord(down_vertex, current_vertex);
      auto constraint_range =
          _checker->obtainUpVertexYConstraintRange(down_vertex, current_vertex);

      int boundary_value =
          box_up_y > constraint_range._x ? box_up_y : constraint_range._x;
      _checker->updateVertexYCoord(current_vertex, boundary_value);

      if (_checker->isDownVertexViolated(down_vertex, current_vertex)) {
        moveDownVertexY(down_vertex, current_vertex);
      }
    }
  }

  bool is_interposer_left = _checker->isLeftInterposerVertex(current_vertex);
  if (is_interposer_left) {
    int boundary_value = 0 + _checker->obtainLeftXRange(current_vertex)._x;

    if (_checker->isLeftBoundaryViolated(current_vertex, boundary_value)) {
      _checker->updateVertexXCoord(current_vertex, boundary_value);
    }
  } else {
    auto left_vertexes = _checker->obtainXLeftNodes(
        current_cell->get_x(),
        Point(current_cell->get_c1()._y, current_cell->get_c3()._y));

    for (auto left_vertex : left_vertexes) {
      int box_right_x =
          _checker->obtainBoundaryRightXCoord(left_vertex, current_vertex);
      auto constraint_range = _checker->obtainRightVertexXConstraintRange(
          left_vertex, current_vertex);

      int boundary_value =
          box_right_x > constraint_range._x ? box_right_x : constraint_range._x;
      _checker->updateVertexXCoord(current_vertex, boundary_value);

      if (_checker->isLeftVertexViolated(left_vertex, current_vertex)) {
        moveLeftVertexX(left_vertex, current_vertex);
      }
    }
  }

  auto next_vertexes = current_vertex->get_froms();
  // make sure the vertexes is ordinal.
  std::map<int, VCGNode*> ordinal_vertexes;
  for (auto next_vertex : next_vertexes) {
    ordinal_vertexes.emplace(next_vertex->get_vcg_id(), next_vertex);
  }

  for (auto pair : ordinal_vertexes) {
    auto next_vertex = pair.second;

    vertex_stack.push(next_vertex);
    cellMovementByDFS(vertex_stack);
  }
  vertex_stack.pop();
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
    _checker->set_left_x_range(current_vertex, move_range);
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

void CellMovement::moveLeftVertexX(VCGNode* left_node, VCGNode* right_node) {
  auto movable_range = _checker->obtainLeftVertexXMovableRange(left_node);
  auto top_down_range = _checker->obtainTopDownVertexesXRange(left_node);
  auto constraint_range =
      _checker->obtainLeftVertexXConstraintRange(left_node, right_node);

  int left_max = INT_MIN;
  int right_min = INT_MAX;

  movable_range._x > left_max ? left_max = movable_range._x : left_max;
  top_down_range._x > left_max ? left_max = top_down_range._x : left_max;
  constraint_range._x > left_max ? left_max = constraint_range._x : left_max;

  movable_range._y < right_min ? right_min = movable_range._y : right_min;
  top_down_range._y < right_min ? right_min = top_down_range._y : right_min;
  constraint_range._y < right_min ? right_min = constraint_range._y : right_min;

  if (right_min < left_max) {
    std::cout << "Error In Range : moveLeftVertexX" << std::endl;
    exit(1);
  }

  _checker->updateVertexXCoord(left_node, right_min);

  _checker->set_left_x_range(right_node, Point(0, right_min - left_max));
}

}  // namespace EDA_CHALLENGE_Q4