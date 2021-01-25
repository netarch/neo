#pragma once

#include <map>
#include <optional>

class EqClass;
class Node;
class FIB_IPNH;

class Choices
{
private:
    std::map<std::pair<EqClass *, Node *>, FIB_IPNH> tbl;

    friend class ChoicesHash;
    friend class ChoicesEq;

public:
    void add_choice(EqClass *, Node *, const FIB_IPNH&);
    std::optional<FIB_IPNH> get_choice(EqClass *, Node *) const;
};

class ChoicesHash
{
public:
    size_t operator()(const Choices *const&) const;
};

class ChoicesEq
{
public:
    bool operator()(const Choices *const&, const Choices *const&) const;
};
