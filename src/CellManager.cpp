#include "CellManager.hpp"

#include "Debug.h"
namespace EDA_CHALLENGE_Q4 {

CellManager::CellManager(ConfigManager* conf_man) {
  assert(conf_man != nullptr);
  init_configs(conf_man->get_mem_list());
  init_configs(conf_man->get_soc_list());
}

void CellManager::init_configs(std::vector<Config> configs) {
  Cell* cell = nullptr;
  for (auto conf : configs) {
    for (int i = 0; i < conf.get_amount(); ++i) {
      cell = new Cell();
      cell->set_id((conf.get_id_refer() << 8) + _cell_list.size());
      cell->set_width(conf.get_width());
      cell->set_height(conf.get_height());
      insert_cell(cell);
    }
  }
}

auto CellManager::get_cell(uint16_t id) { TODO(); }

}  // namespace EDA_CHALLENGE_Q4
