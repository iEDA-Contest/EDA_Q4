/*
 * @Author: sjchanson
 * @Date: 2021-12-10 21:01:02
 * @LastEditTime: 2021-12-12 15:10:16
 * @LastEditors: Please set LastEditors
 * @Description: 打开koroFileHeader查看配置 进行设置:
 * https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: /EDA_Q4/legalization/CellMovement.h
 */

#ifndef CELL_MOVEMENT_CHECK
#define CELL_MOVEMENT_CHECK

#include <map>
#include <stack>
#include <vector>

#include "LegalizationCheck.h"

namespace EDA_CHALLENGE_Q4 {

enum DISTANCE_TYPE { kDISTANCE_NORMAL, kDISTANCE_MAX, kDISTANCE_MIN };

struct VertexesXConstraint {
  VertexesXConstraint() = default;
  VertexesXConstraint(VCGNode* ln, VCGNode* rn, DISTANCE_TYPE t, int it,
                      Point xr)
      : left_node(ln), right_node(rn), type(t), interval(it), x_range(xr) {}

  VCGNode* left_node;
  VCGNode* right_node;
  DISTANCE_TYPE type;
  int interval;
  Point x_range;
};

class CellMovement {
 public:
  CellMovement() = delete;
  CellMovement(VCG* vcg);
  ~CellMovement();

  void executeCellMovement();

 private:
  VCG* _vcg;
  LegalizationCheck* _checker;

  void cellMovementByDFS(std::stack<VCGNode*>& vertex_stack);

  void recursiveMoveLeftCells(VCGNode* node);
  void moveCurrentCellToLegalLocation(VCGNode* node);
  VertexesXConstraint obtainVertexesXConstraint(VCGNode* left_node,
                                                VCGNode* right_node);
  bool checkLegalAndMoveCell(std::vector<VCGNode*> left_vertexes,
                             VCGNode* current_vertex);
  void moveLeftVertexX(VCGNode* left_node, VCGNode* right_node);
  void moveDownVertexY(VCGNode* down_node, VCGNode* up_node);
};

inline CellMovement::CellMovement(VCG* vcg) : _vcg(vcg) {
  _checker = new LegalizationCheck(vcg);
}

inline CellMovement::~CellMovement() { delete _checker; }

}  // namespace EDA_CHALLENGE_Q4

#endif