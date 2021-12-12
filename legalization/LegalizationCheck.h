/*
 * @Author: sjchanson
 * @Date: 2021-12-10 21:00:35
 * @LastEditTime: 2021-12-12 15:08:25
 * @LastEditors: Please set LastEditors
 * @Description: 打开koroFileHeader查看配置 进行设置:
 * https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: /EDA_Q4/legalization/LegalizationCheck.h
 */

#ifndef LEGALIZATION_CHECK
#define LEGALIZATION_CHECK

#include <map>
#include <set>
#include <vector>

#include "../includes/Point.hpp"
#include "../includes/VCG.hpp"

namespace EDA_CHALLENGE_Q4 {
class LegalizationCheck {
 public:
  LegalizationCheck() = delete;
  LegalizationCheck(VCG* vcg);
  ~LegalizationCheck() = default;

  bool isNumLegal(int num, Point range);
  bool isLeftBoundaryViolated(VCGNode* node, int boundary_value);
  bool isDownBoundaryViolated(VCGNode* node, int boundary_value);
  bool isLeftVertexViolated(VCGNode* left_node, VCGNode* right_node);
  bool isDownVertexViolated(VCGNode* down_node, VCGNode* up_node);

  std::vector<VCGNode*> obtainXLeftNodes(int x_coord, Point y_range);
  std::vector<VCGNode*> obtainYDownNodes(int y_coord, Point x_range);

  Point obtainLeftXRange(VCGNode* node);
  Point obtainDownYRange(VCGNode* node);

  void updateVertexLocInfo(VCGNode* node, Point new_coord);
  void updateVertexXCoord(VCGNode* node, int x_coord);
  void updateVertexYCoord(VCGNode* node, int y_coord);

  void updateVertexXYCoordRange(VCGNode* node);

  void eraseXToVertexes(int x_coord, VCGNode* node);
  void eraseYToVertexes(int y_coord, VCGNode* node);
  void addXToVertexes(int x_coord, VCGNode* node);
  void addYToVertexes(int y_coord, VCGNode* node);

  void set_left_x_range(VCGNode* node, Point range);
  void set_down_y_range(VCGNode* node, Point range);

  bool isLeftInterposerVertex(VCGNode* node);
  bool isBottomInterposerVertex(VCGNode* node);

  // range.
  Point obtainTopDownVertexesXRange(VCGNode* vertex);
  Point obtainLeftVertexXConstraintRange(VCGNode* left_node,
                                         VCGNode* right_node);
  Point obtainRightVertexXConstraintRange(VCGNode* left_node,
                                          VCGNode* right_node);
  Point obtainDownVertexYConstraintRange(VCGNode* down_node, VCGNode* up_node);
  Point obtainUpVertexYConstraintRange(VCGNode* down_node, VCGNode* up_node);

  Point obtainLeftVertexXMovableRange(VCGNode* left_node);
  Point obtainDownVertexYMovableRange(VCGNode* down_node);

  // boundary value.
  int obtainBoundaryRightXCoord(VCGNode* left_node, VCGNode* right_node);
  int obtainBoundaryUpYCoord(VCGNode* down_node, VCGNode* up_node);

 private:
  VCG* _vcg;

  // record x y range.
  std::map<VCGNode*, Point> _vertex_to_x_range;
  std::map<VCGNode*, Point> _vertex_to_y_range;

  // record x y coordinates.
  std::map<int, std::vector<VCGNode*>> _x_to_vertexes;
  std::map<int, std::vector<VCGNode*>> _y_to_vertexes;

  void initVertexesInfo();

};  // namespace LegalizationCheck

inline LegalizationCheck::LegalizationCheck(VCG* vcg) : _vcg(vcg) {
  initVertexesInfo();
}

}  // namespace EDA_CHALLENGE_Q4
#endif