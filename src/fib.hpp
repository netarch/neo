#pragma once

#include <map>
#include <set>
#include <memory>
#include <string>

#include "node.hpp"

/*
 * An FIB holds the dataplane information for the current EC.
 */
class FIB
{
private:
    std::map<std::shared_ptr<Node>, std::set<std::shared_ptr<Node> > > tbl;
    friend class FIBHash;

public:
    typedef std::map<std::shared_ptr<Node>,
            std::set<std::shared_ptr<Node> > >::size_type size_type;
    typedef std::map<std::shared_ptr<Node>,
            std::set<std::shared_ptr<Node> > >::iterator iterator;
    typedef std::map<std::shared_ptr<Node>,
            std::set<std::shared_ptr<Node> > >::const_iterator const_iterator;
    typedef std::map<std::shared_ptr<Node>,
            std::set<std::shared_ptr<Node> > >::reverse_iterator
            reverse_iterator;
    typedef std::map<std::shared_ptr<Node>,
            std::set<std::shared_ptr<Node> > >::const_reverse_iterator
            const_reverse_iterator;

    void set_next_hops(const std::shared_ptr<Node>& node,
                       std::set<std::shared_ptr<Node> >&& next_hops);
    void set_next_hops(std::shared_ptr<Node>&& node,
                       std::set<std::shared_ptr<Node> >&& next_hops);
    const std::set<std::shared_ptr<Node> >& get_next_hops(
        const std::shared_ptr<Node>&) const;
    const std::set<std::shared_ptr<Node> >& get_next_hops(
        std::shared_ptr<Node>&&) const;

    std::string to_string() const;
    bool empty() const;
    size_type size() const;

    bool operator==(const FIB&) const;

    iterator               begin();
    const_iterator         begin() const;
    iterator               end();
    const_iterator         end() const;
    reverse_iterator       rbegin();
    const_reverse_iterator rbegin() const;
    reverse_iterator       rend();
    const_reverse_iterator rend() const;
};

class FIBHash
{
public:
    size_t operator()(const std::shared_ptr<FIB>&) const;
};

class FIBEqual
{
public:
    bool operator()(const std::shared_ptr<FIB>&,
                    const std::shared_ptr<FIB>&) const;
};
