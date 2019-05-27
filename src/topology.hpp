#pragma once

#include <map>
#include <memory>
#include <cpptoml/cpptoml.hpp>

#include "lib/logger.hpp"
#include "node.hpp"

class Topology  // undirected graph
{
private:
    Logger& logger;

    std::map<std::string, std::shared_ptr<Node> > nodes;
    std::map<Link, std::shared_ptr<Link> > links;   // all links
    std::set<std::shared_ptr<Link> > failed_links;  // failed links

    void load_nodes(const std::shared_ptr<cpptoml::table_array>);
    void load_links(const std::shared_ptr<cpptoml::table_array>);

public:
    Topology();

    void load_config(const std::shared_ptr<cpptoml::table_array>,
                     const std::shared_ptr<cpptoml::table_array>);
};
