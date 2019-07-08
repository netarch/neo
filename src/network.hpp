#pragma once

#include <map>
#include <string>
#include <memory>
#include <cpptoml/cpptoml.hpp>

#include "node.hpp"
#include "link.hpp"

class Network  // undirected graph
{
private:
    std::map<std::string, std::shared_ptr<Node> > nodes;
    std::set<std::shared_ptr<Link>, LinkCompare> links; // all links
    std::set<std::shared_ptr<Link> > failed_links;      // failed links

public:
    Network() = default;
    Network(const std::shared_ptr<cpptoml::table_array>& nodes_config,
            const std::shared_ptr<cpptoml::table_array>& links_config);

    const std::map<std::string, std::shared_ptr<Node> >& get_nodes() const;
    const std::set<std::shared_ptr<Link>, LinkCompare>& get_links() const;
};
