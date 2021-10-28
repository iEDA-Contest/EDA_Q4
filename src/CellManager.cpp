#include "CellManager.hpp"

#include "Debug.h"

namespace EDA_CHALLENGE_Q4 {

CellManager::CellManager(ConfigManager* conf_man) {
  assert(conf_man != nullptr);
  init_cells(kCellTypeMem, conf_man->get_mem_list());
  init_cells(kCellTypeSoc, conf_man->get_soc_list());
}

/* initialize _mems or _socs */
void CellManager::init_cells(CellType type, std::vector<Config*> configs) {
  Cell* cell = nullptr;
  for (auto conf : configs) {
    for (int i = 0; i < conf->get_amount(); ++i) {
      cell = new Cell();
      cell->set_width(conf->get_width());
      cell->set_height(conf->get_height());
      insert_cell(type, cell);
    }
  }
}

std::vector<Cell*> CellManager::choose_cells(CellPriority prio,
                                             CellType obj_type, va_list ap) {
  uint16_t args[4] = {};
  switch (prio) {
    case kRange:
      for (int i = 0; i < 4; ++i) {
        args[i] = (uint16_t)va_arg(ap, int);
      }
      return choose_range(obj_type, args[0], args[1], args[2], args[3]);
    case kRatio_MAX:
      return choose_ratioWH_max(obj_type);
    default:
      PANIC("Unknown CellPriority = %d", prio);
      break;
  }
}

std::vector<Cell*> CellManager::choose_range(CellType obj_type, uint16_t w_min,
                                             uint16_t w_max, uint16_t h_min,
                                             uint16_t h_max) {
  std::vector<Cell*>* op_list = choose_oplist(obj_type);
  assert(op_list != nullptr);

  std::vector<Cell*> result;
  for (auto cell : *op_list) {
    if (w_min <= cell->get_width() && cell->get_width() <= w_max &&
        h_min <= cell->get_height() && cell->get_height() <= h_max) {
      result.push_back(cell);
    } else if (h_min <= cell->get_width() && cell->get_width() <= h_max &&
               w_min <= cell->get_height() && cell->get_height() <= w_max) {
      cell->rotate();
      result.push_back(cell);
    }
  }

  return result;
}

}  // namespace EDA_CHALLENGE_Q4
