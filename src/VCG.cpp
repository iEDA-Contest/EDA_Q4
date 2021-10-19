#include "VCG.hpp"

namespace EDA_CHALLENGE_Q4 {

VCG::VCG(Token_List& tokens) {
    _adj_list.push_back(new VCGNode(kVCG_START));
    VCGNode* end = new VCGNode(kVCG_END);

    size_t column = 1;  // 
    size_t row = 0;     // character's row, like SM|SS, row of M equals 2
    VCGNode* p = nullptr;
    auto parent = _adj_list[0];

    for(auto token: tokens) {
        ASSERT(parent != nullptr, "VCG Node parent is nullptr");

        switch (token.second)
        {
        // skip
        case kSPACE:
            break;
        
        // next column
        case kColumn:
            row = 0;
            ++column;
            parent->insert_end(end);
            parent = _adj_list[0];
            break;

        // module
        case kMEM:
            ++row;
            p = new VCGNode(kVCG_MEM);
            _adj_list.push_back(p);
            parent->insert_next(p);
            parent = p;
            p = nullptr;
            break;
        case kSOC:
            ++row;
            p = new VCGNode(kVCG_SOC);
            _adj_list.push_back(p);
            parent->insert_next(p);
            parent = p;
            p = nullptr;
            break;

        // placeholder
        case kVERTICAL:
            ASSERT(row != 0, "'^' occurs at first row");
            ++row;
            parent->inc_v_placeholder();
            break;
        case kHORIZONTAL:
            ASSERT(column > 1, "'<' occurs at first column");
            ++row;
            p = _adj_list[0]->get_next(column - 1);
            for (size_t r = p->get_v_placeholder(); r < row;) {
            
                p = p->get_next();
                r += p->get_v_placeholder();
            }
            p->inc_h_placeholder();
            parent->insert_last_next(p);
            parent = p;
            break;

        // bugs
        default:
            PANIC("Unknown substring: %s", token.first.c_str());
            break;
        }
    }
    // last node to point to end
    parent->insert_end(end);
    _adj_list.push_back(end);
}

}  // namespace  EDA_CHALLENGE_Q4