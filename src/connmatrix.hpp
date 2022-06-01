#pragma once

#include <set>
#include <vector>

#include "conn.hpp"

class ConnectionMatrix {
private:
    std::vector<std::set<Connection>> product;
    std::vector<std::set<Connection>::iterator> itrs;
    bool traversed_all = false;

public:
    size_t num_conns() const;
    void clear();
    void reset();
    void add(std::set<Connection> &&);
    std::vector<Connection> get_next_conns();
};
