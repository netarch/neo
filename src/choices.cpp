#include "choices.hpp"

#include "eqclass.hpp"
#include "fib.hpp"
#include "lib/hash.hpp"
#include "logger.hpp"
#include "node.hpp"

void Choices::add_choice(EqClass *ec, Node *node, const FIB_IPNH &choice) {
    auto res = tbl.emplace(std::make_pair(ec, node), choice);
    if (!res.second) {
        logger.error("Duplicate choice key: (" + ec->to_string() + ", " +
                     node->to_string() + ")");
    }
}

std::optional<FIB_IPNH> Choices::get_choice(EqClass *ec, Node *node) const {
    auto it = tbl.find(std::make_pair(ec, node));
    if (it == tbl.end()) {
        return std::optional<FIB_IPNH>();
    }
    return std::optional<FIB_IPNH>(it->second);
}

size_t ChoicesHash::operator()(const Choices *const &choices) const {
    size_t value = 0;
    std::hash<EqClass *> ec_hf;
    std::hash<Node *> node_hf;
    FIB_IPNH_Hash ipnh_hf;
    for (const auto &entry : choices->tbl) {
        hash::hash_combine(value, ec_hf(entry.first.first));
        hash::hash_combine(value, node_hf(entry.first.second));
        hash::hash_combine(value, ipnh_hf(entry.second));
    }
    return value;
}

bool ChoicesEq::operator()(const Choices *const &a,
                           const Choices *const &b) const {
    return a->tbl == b->tbl;
}
