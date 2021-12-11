/*
 * @Author: sjchanson
 * @Date: 2021-12-10 21:00:35
 * @LastEditTime: 2021-12-11 12:59:07
 * @LastEditors: Please set LastEditors
 * @Description: 打开koroFileHeader查看配置 进行设置:
 * https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: /EDA_Q4/legalization/LegalizationCheck.h
 */

#ifndef LEGALIZATION_CHECK
#define LEGALIZATION_CHECK

#include <map>
#include <vector>

#include "../includes/VCG.hpp"

namespace EDA_CHALLENGE_Q4 {
class LegalizationCheck {
 public:
  LegalizationCheck() = delete;
  LegalizationCheck(VCG* vcg);
  ~LegalizationCheck() = default;

 private:
  VCG* _vcg;
};  // namespace LegalizationCheck

inline LegalizationCheck::LegalizationCheck(VCG* vcg) : _vcg(vcg) {}

}  // namespace EDA_CHALLENGE_Q4
#endif