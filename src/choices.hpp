//
// Created by Santhosh Prabhu on 1/29/20.
//

#pragma once
#include <map>
#include <tuple>
#include <unordered_set>
#include "eqclass.hpp"
#include "fib.hpp"
#include "pan.h"
#include "node.hpp"


class Choices
{
    std::map<std::pair<EqClass *, Node *>, FIB_IPNH> _choicesMap;
    Choices *createNewObjectWithNewChoice(EqClass *ec, Node *node, FIB_IPNH choice) const;
public:
    size_t hash() const;
    bool equal(const Choices *other) const;
    std::optional<FIB_IPNH> getChoiceIfPresent(EqClass *ec, Node *node) const;
    Choices *setChoice(EqClass *ec, Node *node, FIB_IPNH choice) const;
    ~Choices()
    {

    }
    Choices() = default;
    Choices(const Choices& other) = default;
};

struct ChoicesHasher {
    size_t operator()(const Choices *c) const
    {
        return c->hash();
    }
};

struct ChoicesEqual {
    bool operator()(const Choices *c1, const Choices *c2) const
    {
        return c1->equal(c2);
    }
};

static std::unordered_set<Choices *, ChoicesHasher, ChoicesEqual> choicesSet;
std::optional<FIB_IPNH> getChoiceIfPresent(State *state);
void setChoice(State *state, FIB_IPNH choice);


