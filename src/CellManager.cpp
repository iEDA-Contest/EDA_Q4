#include "CellManager.hpp"

#include "Debug.h"

namespace EDA_CHALLENGE_Q4 {

CellManager::CellManager(ConfigManager* conf_man) {
  assert(conf_man != nullptr);
  init_cells(kCellTypeMem, conf_man->get_mem_list());
  init_cells(kCellTypeSoc, conf_man->get_soc_list());
  init_mems_aux();
  init_socs_aux();  
}

static int id_deviate = 1000;
/* initialize _mems or _socs */
void CellManager::init_cells(CellType type, std::vector<Config*> configs) {
  Cell* cell = nullptr;
  for (auto conf : configs) {
    for (int i = 0; i < conf->get_amount(); ++i) {
      cell = new Cell();
      cell->set_width(conf->get_width());
      cell->set_height(conf->get_height());
      cell->set_refer(conf->get_refer());
      cell->set_cell_id(conf->get_id_refer() * id_deviate + i);
      insert_cell(type, cell);
    }
  }
}

std::vector<Cell*> CellManager::choose_cells(bool aux, CellPriority prio, ...) {
  va_list ap;
  va_start(ap, prio);

  uint16_t args[4] = {};
  CellType tar_type = kCellTypeNull;
  Rectangle* box;

  switch (prio) {
    case kPrioMem:
      return aux ? _mems_aux : _mems;
    case kPrioSoc:
      return aux ? _socs_aux : _socs;
    case kRange:
      tar_type = (CellType)va_arg(ap, int);
      for (int i = 0; i < 4; ++i) {
        args[i] = (uint16_t)va_arg(ap, int);
      }
      return choose_range(aux, tar_type, args[0], args[1], args[2], args[3]);
    case kRatioMAX:
      tar_type = (CellType)va_arg(ap, int);
      return choose_ratioWH_max(aux, tar_type);
    case kDeathCol:
      tar_type = (CellType)va_arg(ap, int);
      box = (Rectangle*)va_arg(ap, Rectangle*);
      for (int i = 0; i < 2; ++i) {
        args[i] = (uint16_t)va_arg(ap, int);
      }
      return choose_death_y(aux, tar_type, *box, args[0], args[1]);
    case kDeathRow:
      tar_type = (CellType)va_arg(ap, int);
      box = (Rectangle*)va_arg(ap, Rectangle*);
      for (int i = 0; i < 2; ++i) {
        args[i] = (uint16_t)va_arg(ap, int);
      }
      return choose_death_x(aux, tar_type, *box, args[0], args[1]);

    default:
      PANIC("Unknown CellPriority = %d", prio);
      break;
  }

  va_end(ap);
}

std::vector<Cell*> CellManager::choose_range(bool aux, CellType tar_type, 
                                            uint16_t w_min, uint16_t w_max, 
                                            uint16_t h_min, uint16_t h_max) {
  std::vector<Cell*>* op_list = get_list(aux, tar_type);
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

void CellManager::delete_cell(CellType type, Cell* cell) {
  auto oplist = get_list(false, type);
  if (oplist) {
    for (auto c = oplist->begin(); c != oplist->end(); ++c) {
      if (*c == cell) {
        oplist->erase(c);
        break;
      }
    }
  }
}

/**
 * @brief choose cells according to minimal death area in y direction.
 * Here can optimize.
 * @param aux       visit all cells in correct cell type when true
 * @param target    target cell type
 * @param box       bounding box
 * @param min       min val that should be away from box
 * @param max       max val that should be away from box
 * @return std::vector<Cell*> All cells can directly be set without rotation
 * again.
 */
std::vector<Cell*> CellManager::choose_death_y(bool aux, CellType target, 
                                              Rectangle& box, int min, int max) {
  assert(min <= max);
  std::vector<Cell*>* op_list = get_list(aux, target);
  assert(op_list != nullptr);

  std::map<Cell*, int> area_map;
  for (auto cell : *op_list) {
    auto area_ori = std::max(box.get_width(), (int)cell->get_width()) *
                    (box.get_height() + min + cell->get_height());
    auto area_rotate = std::max(box.get_width(), (int)cell->get_height()) *
                       (box.get_height() + min + cell->get_width());
    if (area_rotate < area_ori) {
      cell->rotate();
    }

    auto tmp = box;
    tmp._c3._x = std::max(box.get_width(), (int)cell->get_width());
    tmp._c3._y = box.get_height() + min + cell->get_height();
    area_map[cell] = tmp.get_area() - box.get_area() - cell->get_area();
  }

  return get_min_area_sorted_chose_cells(area_map);
}

/**
 * @brief choose cells according to minimal death area in x direction.
 * Here can optimize.
 *
 * @param aux       visit all cells in correct cell type when true
 * @param target    target cell type
 * @param box       bounding box
 * @param min       min val that should be away from box
 * @param max       max val that should be away from box
 * @return std::vector<Cell*> All cells can directly be set without rotation
 * again.
 */
std::vector<Cell*> CellManager::choose_death_x(bool aux, CellType target, Rectangle& box,
                                               int min, int max) {
  assert(min <= max);
  std::vector<Cell*>* op_list = get_list(aux, target);
  assert(op_list != nullptr);

  std::map<Cell*, int> death;
  for (auto cell : *op_list) {
    auto area_ori = std::max(box.get_height(), (int)cell->get_height()) *
                    (box.get_width() + min + cell->get_width());
    auto area_rotate = std::max(box.get_height(), (int)cell->get_width()) *
                       (box.get_width() + min + cell->get_height());
    if (area_rotate < area_ori) {
      cell->rotate();
    }

    auto tmp = box;
    tmp._c3._x = box.get_width() + min + cell->get_width();
    tmp._c3._y = std::max(box.get_height(), (int)cell->get_height());
    death[cell] = tmp.get_area() - box.get_area() - cell->get_area();
  }

  return get_min_area_sorted_chose_cells(death);
}

std::vector<Cell*> CellManager::get_min_area_sorted_chose_cells(
    std::map<Cell*, int>& area_map) {
  std::multimap<int, Cell*> area_sorted;
  for (auto p : area_map) {
    area_sorted.insert(std::make_pair(p.second, p.first));
  }

  std::vector<Cell*> ret;
  for (auto p : area_sorted) {
    ret.push_back(p.second);
  }

  return ret;
}

CellManager::~CellManager() {
  for (auto m : _mems) {
    delete m;
  }
  _mems.clear();
  _mems_aux.clear();

  for (auto s : _socs) {
    delete s;
  }
  _socs.clear();
  _socs_aux.clear();

  _id_map.clear();
}

CellManager::CellManager(const CellManager& man) {
  ASSERT(man.get_mems_num() + man.get_socs_num() == man.get_id_map_num(), 
    "Please return cells into _mems and _socs before using copy constructor");

  size_t num = 0;
  _mems.resize(man.get_mems_num());
  for (auto c : man.get_mems()) {
    auto new_cell = new Cell(*c);
    _mems[num++] = new_cell;
    _id_map[new_cell->get_cell_id()] = new_cell;
  }

  num = 0;
  _socs.resize(man.get_socs_num());
  for (auto c : man.get_socs()) {
    auto new_cell = new Cell(*c);
    _socs[num++] = new_cell;
    _id_map[new_cell->get_cell_id()] = new_cell;
  }

  init_mems_aux();
  init_socs_aux();
}

int Cell::get_refer_id() { 
  return _cell_id / id_deviate;
}

CellType Cell::get_cell_type() {
  switch (_cell_id / id_deviate / id_base ) {
    case kCellTypeMem:  return kCellTypeMem;
    case kCellTypeSoc:  return kCellTypeSoc;
    default:            return kCellTypeNull;
  }
}

}  // namespace EDA_CHALLENGE_Q4
