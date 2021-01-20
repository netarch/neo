#include "choices.hpp"

#include "lib/logger.hpp"

void Choices::add_choice(EqClass *ec, Node *node, const FIB_IPNH& choice)
{
    auto res = tbl.emplace(std::make_pair(ec, node), choice);
    if (!res.second) {
        Logger::error("Duplicate choice key: (" + ec->to_string() + ", "
                      + node->to_string() + ")");
    }
}

std::optional<FIB_IPNH> Choices::get_choice(EqClass *ec, Node *node) const
{
    auto it = tbl.find(std::make_pair(ec, node));
    if (it == tbl.end()) {
        return std::optional<FIB_IPNH>();
    }
    return std::optional<FIB_IPNH>(it->second);
}

bool operator==(const Choices& a, const Choices& b)
{
    return a.tbl == b.tbl;
}
