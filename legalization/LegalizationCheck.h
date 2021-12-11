/*
 * @Author: sjchanson
 * @Date: 2021-12-10 21:00:35
 * @LastEditTime: 2021-12-11 14:36:01
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

  bool isLeftViolated(VCGNode* node);
  bool isDownViolated(VCGNode* node);

  std::vector<VCGNode*> obtainXLeftNodes(Point lower_y, Point upper_y);
  std::vector<VCGNode*> obtainYDownNodes(Point lower_x, Point upper_x);

  void updateVertexXYCoordRange(VCGNode* node);

 private:
  VCG* _vcg;

  // record x y range.
  std::map<VCGNode*, Point> _vertex_to_x_range;
  std::map<VCGNode*, Point> _vertex_to_y_range;

};  // namespace LegalizationCheck

inline LegalizationCheck::LegalizationCheck(VCG* vcg) : _vcg(vcg) {}

}  // namespace EDA_CHALLENGE_Q4
#endif