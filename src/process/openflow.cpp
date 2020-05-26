//
// Created by Santhosh Prabhu on 5/25/20.
//

#include "openflow.hpp"

std::unordered_set<std::vector<size_t> *, VectorPtrHasher, VectorPtrEqual> Openflow::offsets_map;
std::map<std::string, std::vector<Route>> Openflow::updates;