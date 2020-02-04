#include "choices.hpp"

#include "lib/logger.hpp"

void Choices::add_choice(EqClass *ec, Node *node, const inject_result_t& choice)
{
    auto res = tbl.emplace(std::make_pair(ec, node), choice);
    if (!res.second) {
        Logger::get().err("Duplicate choice key: (" + ec->to_string() + ", "
                          + node->to_string() + ")");
    }
}

std::optional<inject_result_t> Choices::get_choice(EqClass *ec, Node *node) const
{
    auto it = tbl.find(std::make_pair(ec, node));
    if (it == tbl.end()) {
        return std::optional<inject_result_t>();
    }
    return std::optional<inject_result_t>(it->second);
}

bool operator==(const Choices& a, const Choices& b)
{
    return a.tbl == b.tbl;
}
