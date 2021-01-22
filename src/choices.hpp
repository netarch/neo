#pragma once

#include <map>
#include <optional>
#include <functional>

#include "eqclass.hpp"
#include "node.hpp"
#include "fib.hpp"
#include "lib/hash.hpp"

class Choices
{
private:
    std::map<std::pair<EqClass *, Node *>, FIB_IPNH> tbl;

    friend struct std::hash<Choices>;
    friend bool operator==(const Choices&, const Choices&);

public:
    void add_choice(EqClass *, Node *, const FIB_IPNH&);
    std::optional<FIB_IPNH> get_choice(EqClass *, Node *) const;
};

bool operator==(const Choices&, const Choices&);

namespace std
{

template<>
struct hash<Choices> {
    size_t operator()(const Choices& c) const
    {
        size_t value = 0;
        hash<EqClass *> ec_hf;
        hash<Node *> node_hf;
        hash<FIB_IPNH> ipnh_hf;
        for (const auto& entry : c.tbl) {
            ::hash::hash_combine(value, ec_hf(entry.first.first));
            ::hash::hash_combine(value, node_hf(entry.first.second));
            ::hash::hash_combine(value, ipnh_hf(entry.second));
        }
        return value;
    }
};

template <>
struct hash<Choices *> {
    size_t operator()(Choices *const& c) const
    {
        return hash<Choices>()(*c);
    }
};

template <>
struct equal_to<Choices *> {
    bool operator()(Choices *const& a, Choices *const& b) const
    {
        return *a == *b;
    }
};

} // namespace std
