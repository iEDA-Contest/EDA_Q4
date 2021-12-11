/*
 * @Author: sjchanson
 * @Date: 2021-12-10 21:00:35
 * @LastEditTime: 2021-12-11 23:23:02
 * @LastEditors: Please set LastEditors
 * @Description: 打开koroFileHeader查看配置 进行设置:
 * https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: /EDA_Q4/legalization/LegalizationCheck.h
 */

#ifndef LEGALIZATION_CHECK
#define LEGALIZATION_CHECK

#include <map>
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
  bool isLeftBoundaryViolated(VCGNode* node);
  bool isDownBoundaryViolated(VCGNode* node);

  std::vector<VCGNode*> obtainXLeftNodes(int x_coord, Point y_range);
  std::vector<VCGNode*> obtainYDownNodes(int y_coord, Point x_range);

  Point obtainLeftXBoundaryRange(VCGNode* node);
  Point obtainDownYBoundaryRange(VCGNode* node);

  void updateVertexLocInfo(VCGNode* node, Point new_coord);
  void updateVertexXYCoordRange(VCGNode* node);

  void eraseXToVertexes(int x_coord, VCGNode* node);
  void eraseYToVertexes(int y_coord, VCGNode* node);
  void addXToVertexes(int x_coord, VCGNode* node);
  void addYToVertexes(int y_coord, VCGNode* node);

  void set_left_x_boundary_range(VCGNode* node, Point range);
  void set_down_y_boundary_range(VCGNode* node, Point range);

 private:
  VCG* _vcg;

  // record x y range.
  std::map<VCGNode*, Point> _vertex_to_boundary_x_range;
  std::map<VCGNode*, Point> _vertex_to_boundary_y_range;

  // record x y coordinates.
  std::map<int, std::vector<VCGNode*>> _x_to_vertexes;
  std::map<int, std::vector<VCGNode*>> _y_to_vertexes;

  void initVertexLocInfo();

};  // namespace LegalizationCheck

inline LegalizationCheck::LegalizationCheck(VCG* vcg) : _vcg(vcg) {
  initVertexLocInfo();
}

}  // namespace EDA_CHALLENGE_Q4
#endif