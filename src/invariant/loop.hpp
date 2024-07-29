#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <tuple>

#include "eqclass.hpp"
#include "invariant/invariant.hpp"
#include "node.hpp"

/**
 * For the specified connections, there should be no forwarding loop.
 */
class Loop : public Invariant {
private:
    friend class ConfigParser;
    Loop(bool correlated = false) : Invariant(correlated) {}

public:
    std::string to_string() const override;
    void init() override;
    int check_violation() override;
};

class VisitedHops {
private:
    std::set<std::tuple<EqClass *, uint16_t, Node *>> _hops;
    std::tuple<EqClass *, uint16_t, Node *> _last_hop;

    friend class VisitedHopsHash;
    friend class VisitedHopsEq;

public:
    bool visited(const std::tuple<EqClass *, uint16_t, Node *> &) const;
    bool is_last_hop(const std::tuple<EqClass *, uint16_t, Node *> &) const;
    void add(std::tuple<EqClass *, uint16_t, Node *> &&);
};

class VisitedHopsHash {
public:
    size_t operator()(const VisitedHops *const &) const;
};

class VisitedHopsEq {
public:
    bool operator()(const VisitedHops *const &,
                    const VisitedHops *const &) const;
};
