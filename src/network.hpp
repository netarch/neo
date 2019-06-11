#pragma once

#include <map>
#include <memory>
#include <cpptoml/cpptoml.hpp>

#include "node.hpp"

class Network  // undirected graph
{
private:
    std::map<std::string, std::shared_ptr<Node> > nodes;
    std::map<Link, std::shared_ptr<Link> > links;   // all links
    std::set<std::shared_ptr<Link> > failed_links;  // failed links

    void load_nodes(const std::shared_ptr<cpptoml::table_array>&);
    void load_links(const std::shared_ptr<cpptoml::table_array>&);

public:
    Network() = default;

    void load_config(const std::shared_ptr<cpptoml::table_array>&,
                     const std::shared_ptr<cpptoml::table_array>&);

    const std::map<std::string, std::shared_ptr<Node> >& get_nodes() const;
    const std::map<Link, std::shared_ptr<Link> >& get_links() const;
};
