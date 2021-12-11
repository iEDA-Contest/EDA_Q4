/*
 * @Author: sjchanson
 * @Date: 2021-12-10 21:01:02
 * @LastEditTime: 2021-12-11 14:34:29
 * @LastEditors: Please set LastEditors
 * @Description: 打开koroFileHeader查看配置 进行设置:
 * https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: /EDA_Q4/legalization/CellMovement.h
 */

#ifndef CELL_MOVEMENT_CHECK
#define CELL_MOVEMENT_CHECK

#include <map>
#include <vector>

#include "LegalizationCheck.h"

namespace EDA_CHALLENGE_Q4 {

class CellMovement {
 public:
  CellMovement() = delete;
  CellMovement(VCG* vcg);
  ~CellMovement() = default;

 private:
  VCG* _vcg;

  LegalizationCheck* _checker;
};

}  // namespace EDA_CHALLENGE_Q4

#endif