#include "connmatrix.hpp"

size_t ConnectionMatrix::num_conns() const {
    size_t num = 1;
    for (const auto &conn_set : product) {
        num *= conn_set.size();
    }
    return num;
}

void ConnectionMatrix::clear() {
    product.clear();
    itrs.clear();
    traversed_all = false;
}

void ConnectionMatrix::reset() {
    for (size_t i = 0; i < product.size(); ++i) {
        itrs[i] = product[i].begin();
    }
    traversed_all = false;
}

void ConnectionMatrix::add(std::set<Connection> &&conns) {
    product.push_back(std::move(conns));
    itrs.push_back(product.back().begin());
}

std::vector<Connection> ConnectionMatrix::get_next_conns() {
    std::vector<Connection> conns;

    if (traversed_all) {
        return conns;
    }

    for (const auto &set_itr : itrs) {
        conns.push_back(*set_itr);
    }

    // advance iterators
    size_t i;
    for (i = 0; i < product.size(); ++i) {
        if (++itrs[i] != product[i].end()) {
            break;
        }
        itrs[i] = product[i].begin();
    }
    if (i == product.size()) {
        traversed_all = true;
    }

    return conns;
}
