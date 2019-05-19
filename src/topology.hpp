#ifndef TOPOLOGY_HPP
#define TOPOLOGY_HPP

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

public:
    Topology();

    void add_node(const std::shared_ptr<Node>&);
    void load_config(const std::shared_ptr<cpptoml::table_array>,
                     const std::shared_ptr<cpptoml::table_array>);
};

#endif
