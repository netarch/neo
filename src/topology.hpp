#pragma once

#include <map>
#include <memory>

#include "logger.hpp"
#include "node.hpp"
#include "lib/cpptoml.hpp"

class Topology
{
private:
    Logger& logger;

    std::map<std::string, std::shared_ptr<Node> > nodes;
    // (sorted list of shared_ptr<Link> by ) links;

    void add_node(const std::shared_ptr<Node>&);
    void load_nodes(const std::shared_ptr<cpptoml::table_array>);
    void load_links(const std::shared_ptr<cpptoml::table_array>);

public:
    Topology();

    void load_config(const std::shared_ptr<cpptoml::table_array>,
                     const std::shared_ptr<cpptoml::table_array>);
};
