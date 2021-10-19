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
    ~VCGNode();

    // getter
    auto get_type() const { return _type; }
    std::vector<VCGNode*>& get_nexts() {return _nexts;}
    VCGNode* get_next(size_t n = 1);
    auto get_v_placeholder() const { return _v_placeholder; }
    auto get_h_placeholder() const { return _h_placeholder; }
    
    // setter

    // function
    void insert_next(VCGNode* );
    void inc_v_placeholder() { ++_v_placeholder; }
    void inc_h_placeholder() { ++_h_placeholder; }
    void insert_end(VCGNode* );
    void insert_last_next(VCGNode* );
    void release_nexts();

  private:
    // members
    VCGNodeType _type;
    std::vector<VCGNode*> _nexts;
    uint8_t _v_placeholder; // if "^" occur
    uint8_t _h_placeholder; // if "<" occur 
};

class VCG {
 public:
  // constructor
  VCG(Token_List&);
  ~VCG();

  // getter

  // setter

  // function

 private:
  // members
  std::vector<VCGNode*> _adj_list; // Node0 is start, final Node is end
};



// VCGNode
inline VCGNode::VCGNode(VCGNodeType type)
  :_v_placeholder(1), _h_placeholder(1) 
  { _type = type; }

// n is a natural
inline VCGNode* VCGNode::get_next(size_t n) {
  return 0 < n && n <= _nexts.size() ? _nexts[n -1] : nullptr ;
}

inline VCGNode::~VCGNode() { TODO(); }

inline void VCGNode::insert_next(VCGNode* next) {
  if (next) {
    _nexts.push_back(next);
  }
}

inline void VCGNode::insert_end(VCGNode* end) {
  if (end && _nexts.size() == 0) {
    _nexts.push_back(end);
 }
}

inline void VCGNode::insert_last_next(VCGNode* next) {
  if (next) {
    if (_nexts.size() == 0) {
      _nexts.push_back(next);
    } else if ( _nexts[_nexts.size() - 1] != next) {
      _nexts.push_back(next);
    }
  }
}

// Attention: we release memory in VCG Not in VCGNode.
inline void VCGNode::release_nexts() {
  _nexts.resize(0);
}

// VCG
inline VCG::~VCG() { 
  for (auto p : _adj_list) {
    p->release_nexts();
    free(p);
  }
 }


}  // namespace EDA_CHALLENGE_Q4

#endif