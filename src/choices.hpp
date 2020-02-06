#pragma once

#include <map>
#include <optional>
#include <functional>

#include "eqclass.hpp"
#include "middlebox.hpp"
#include "node.hpp"
#include "fib.hpp"
#include "lib/hash.hpp"

class Choices
{
private:
    std::map<std::pair<EqClass *, Node *>, inject_result_t> tbl;

    friend class std::hash<Choices>;
    friend bool operator==(const Choices&, const Choices&);

public:
    //Choices() = default;
    //Choices(Choices&&) = default;

    void add_choice(EqClass *, Node *, const inject_result_t&);
    std::optional<inject_result_t> get_choice(EqClass *, Node *) const;
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
            ::hash::hash_combine(value, ipnh_hf(entry.second.first)); //ignore the next hop ip, since it will always be 0
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
