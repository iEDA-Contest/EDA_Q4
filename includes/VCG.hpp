#ifndef __VCG_HPP_
#define __VCG_HPP_

#include "Debug.h"
#include "Regex.hpp"

namespace EDA_CHALLENGE_Q4 {

enum VCGNodeType { kVCG_START, kVCG_MEM, kVCG_SOC, kVCG_END };

class VCGNode {
 public:
  // constructor
  VCGNode(VCGNodeType);
  VCGNode(VCGNodeType, uint8_t);
  ~VCGNode();

  // getter
  auto get_id() const { return _id; }
  auto get_type() const { return _type; }
  auto get_v_placeholder() const { return _v_placeholder; }
  auto get_h_placeholder() const { return _h_placeholder; }
  std::vector<VCGNode*>& get_froms() { return _froms; }
  VCGNode* get_from(size_t n = 1);

  // setter
  void set_id(uint8_t id) { _id = id; }

  // function
  void inc_v_placeholder() { ++_v_placeholder; }
  void inc_h_placeholder() { ++_h_placeholder; }
  void insert_from(VCGNode*);
  void insert_last_from(VCGNode*);
  void insert_start(VCGNode*);
  void insert_to(VCGNode*);
  void release_froms();
  void release_tos();
  void show_froms();
  void show_tos();

 private:
  // members
  uint8_t _id;
  VCGNodeType _type;
  uint8_t _v_placeholder;        // if "^" occur
  uint8_t _h_placeholder;        // if "<" occur
  std::vector<VCGNode*> _froms;  // in column order, top-down
  std::vector<VCGNode*> _tos;    // order in shortest path to END, bottom-up
  
};

class VCG {
 public:
  // constructor
  VCG(Token_List&);
  ~VCG();

  // getter
  auto get_vertex_num() const { return _adj_list.size(); }

  // setter

  // function
  void show_topology();

 private:
  // getter

  // setter
  void set_id_grid(size_t, size_t, uint8_t);

  // function
  void build_tos();

  // members
  std::vector<VCGNode*> _adj_list;  // Node0 is end, final Node is start
  std::vector<std::vector<uint8_t>> _id_grid; // [column][row]
};

// VCGNode
inline VCGNode::VCGNode(VCGNodeType type)
    : _id(0), _v_placeholder(1), _h_placeholder(1) {
  _type = type;
}

inline VCGNode::VCGNode(VCGNodeType type, uint8_t id)
    : _id(id), _v_placeholder(1), _h_placeholder(1) {
  _type = type;
}

// n is a natural
inline VCGNode* VCGNode::get_from(size_t n) {
  return 0 < n && n <= _froms.size() ? _froms[n - 1] : nullptr;
}

inline VCGNode::~VCGNode() { TODO(); }

inline void VCGNode::insert_from(VCGNode* from) {
  if (from) {
    _froms.push_back(from);
  }
}

/**
 * @brief insert start point of VCG
 *
 *
 *
 * @param start
 */
inline void VCGNode::insert_start(VCGNode* start) {
  if (start && _froms.size() == 0) {
    _froms.push_back(start);
  }
}

/**
 * @brief insert node into the end of _froms
 *
 *
 *
 * @param from
 */
inline void VCGNode::insert_last_from(VCGNode* from) {
  if (from) {
    if (_froms.size() == 0) {
      _froms.push_back(from);
    } else if (_froms[_froms.size() - 1] != from) {
      _froms.push_back(from);
    }
  }
}

// Attention: we release memory in VCG Not in VCGNode.
inline void VCGNode::release_froms() { _froms.resize(0); }

inline void VCGNode::release_tos() { _tos.resize(0); }

inline void VCGNode::insert_to(VCGNode* to) {
  if (to) {
    _tos.push_back(to);
  }
}

inline void VCGNode::show_froms() {
  printf("froms\n[%d]:", get_id());
  for (auto from: _froms) {
    printf("%2d, ", from->get_id());
  }
  printf("\n");
}

inline void VCGNode::show_tos() {
  printf("tos\n[%d]:", get_id());
  for (auto to: _tos) {
    printf("%2d, ", to->get_id());
  }
  printf("\n");
}

// VCG
inline VCG::~VCG() {
  for (auto p : _adj_list) {
    p->release_froms();
    p->release_tos();
    free(p);
    p = nullptr;
  }
}

inline void VCG::set_id_grid(size_t column, size_t row, uint8_t id) {
  if (column == _id_grid.size() + 1 && row == 1) {
    _id_grid.resize(_id_grid.size() + 1);
    _id_grid[column - 1].push_back(id);
  } else if (column == _id_grid.size() && row == _id_grid[column - 1].size() + 1) {
    _id_grid[column - 1].push_back(id);
  }
}


}  // namespace EDA_CHALLENGE_Q4

#endif