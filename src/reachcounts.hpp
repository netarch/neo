#pragma once

#include <unordered_map>
#include <string>
class Node;

class ReachCounts
{
private:
    std::unordered_map<Node *, int> counts;

    friend class ReachCountsHash;
    friend class ReachCountsEq;

public:
    std::string to_string() const;
    int operator[](Node *const&) const;
    int total_counts() const;
    void increase(Node *const&);
};

class ReachCountsHash
{
public:
    size_t operator()(const ReachCounts *const&) const;
};

class ReachCountsEq
{
public:
    bool operator()(const ReachCounts *const&, const ReachCounts *const&) const;
};
