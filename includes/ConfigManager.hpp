#ifndef __CONFIGURE_MANAGER_HPP_
#define __CONFIGURE_MANAGER_HPP_

#include <map>
#include <string>

#include "Regex.hpp"

namespace EDA_CHALLENGE_Q4 {

const static uint16_t id_base = 100;
enum CellType { kCellTypeNull, kCellTypeMem, kCellTypeSoc };

class Config {
 public:
  // constructor
  Config() = default;
  Config(uint8_t, uint16_t, uint16_t, uint16_t);
  ~Config() = default;

  // getter
  auto get_id_refer() const { return _id_refer; }
  auto get_amount() const { return _amount; }
  auto get_width() const { return _width; }
  auto get_height() const { return _height; }
  auto get_refer() const { return _refer; }

  // setter
  void set_id_refer(const uint8_t id_refer) { _id_refer = id_refer; }
  void set_amount(const uint16_t amount) { _amount = amount; }
  void set_width(const uint16_t width) { _width = width; }
  void set_height(const uint16_t height) { _height = height; }
  void set_value(RegexType, uint16_t);
  void set_refer(const std::string refer) { _refer = refer; }

  // function

 private:
  // members
  uint8_t _id_refer;
  uint16_t _amount;
  uint16_t _width;
  uint16_t _height;
  std::string _refer;
};

class ConfigManager {
 public:
  // constructor
  ConfigManager(Token_List&);
  ~ConfigManager();

  // getter
  auto get_mem_list() const { return _mem_config_list; }
  auto get_soc_list() const { return _soc_config_list; }

  // setter

  // function

 private:
  std::map<uint8_t, std::string> _id_refers_map;  // to store Reference as key
  std::vector<Config*> _mem_config_list;
  std::vector<Config*> _soc_config_list;
};

// Config

inline Config::Config(uint8_t id_ref, uint16_t amount, uint16_t width,
                      uint16_t height)
    : _id_refer(id_ref), _amount(amount), _width(width), _height(height) {}

inline void Config::set_value(RegexType type, uint16_t data) {
  switch (type) {
    case kTAG_REFERENCE_BEG:
      set_id_refer((uint8_t)data);
      break;
    case kTAG_AMOUNT_BEG:
      set_amount(data);
      break;
    case kTAG_WIDTH_BEG:
      set_width(data);
      break;
    case kTAG_HEIGHT_BEG:
      set_height(data);
      break;
    default:
      PANIC("Invalid typeid:%d", type);
      break;
  }
}

// ConfigManager

}  // namespace EDA_CHALLENGE_Q4

#endif