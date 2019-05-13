#ifndef TOPOLOGY_HPP
#define TOPOLOGY_HPP

#include <map>
#include <memory>

#include "node.hpp"
#include "lib/logger.hpp"
#include "lib/cpptoml.hpp"

class Topology
{
private:
    Logger& logger;

    std::map<std::string, std::shared_ptr<Node> > nodes;
    //links;

public:
    Topology();

    void load_config(const std::shared_ptr<cpptoml::table_array>,
                     const std::shared_ptr<cpptoml::table_array>);
};

#endif
