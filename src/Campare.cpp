#include "CellManager.hpp"

namespace EDA_CHALLENGE_Q4 {

bool compare_ratioWH_max(Cell* a, Cell* b) {
  if (a == nullptr || b == nullptr) return false;
  if (a->get_ratioWH_max() > b->get_ratioWH_max()) {
    return true;
  } else if (a->get_ratioWH_max() == b->get_ratioWH_max()) {
    return a->get_max_edge() > b->get_max_edge() ? true : false;
  } else {
    return false;
  }
}

}  // namespace EDA_CHALLENGE_Q4