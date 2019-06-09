#include <cpptoml/cpptoml.hpp>

#include "plankton.hpp"
#include "lib/fs.hpp"
#include "lib/logger.hpp"
#include "policy/reachability.hpp"
#include "policy/stateful-reachability.hpp"
#include "policy/waypoint.hpp"

Plankton::Plankton(bool verbose, bool rm_out_dir, int max_jobs,
                   const std::string& input_file, const std::string& output_dir)
    : max_jobs(max_jobs)
{
    if (rm_out_dir && fs::exists(output_dir)) {
        fs::remove(output_dir);
    }
    fs::mkdir(output_dir);
    in_file = fs::realpath(input_file);
    out_dir = fs::realpath(output_dir);
    Logger::get_instance().set_file(fs::append(out_dir, "verify.log"));
    Logger::get_instance().set_verbose(verbose);
}

void Plankton::load_config()
{
    Logger::get_instance().info("Loading network configurations...");

    auto config = cpptoml::parse_file(in_file);
    auto nodes_config = config->get_table_array("nodes");
    auto links_config = config->get_table_array("links");
    auto policies_config = config->get_table_array("policies");

    network.load_config(nodes_config, links_config);

    // Load policies configurations
    if (policies_config) {
        for (auto policy_config : *policies_config) {
            std::shared_ptr<Policy> policy;
            auto type = policy_config->get_as<std::string>("type");

            if (!type) {
                Logger::get_instance().err("Key error: type");
            }

            if (*type == "reachability") {
                policy = std::static_pointer_cast<Policy>
                         (std::make_shared<ReachabilityPolicy>());
            } else if (*type == "stateful-reachability") {
                policy = std::static_pointer_cast<Policy>
                         (std::make_shared<StatefulReachabilityPolicy>());
            } else if (*type == "waypoint") {
                policy = std::static_pointer_cast<Policy>
                         (std::make_shared<WaypointPolicy>());
            } else {
                Logger::get_instance().err("Unknown policy type: " + *type);
            }

            policy->load_config(policy_config);
            policies.push_back(policy);
        }
    }
    Logger::get_instance().info("Loaded " + std::to_string(policies.size()) +
                                " policies");
}

void Plankton::compute_ec()
{
}

void Plankton::run()
{
    load_config();
    compute_ec();

    // for each ec in ECs
    //  fork && spin_main

    //  static char param0[] = "neo";  // argv[0]
    //  static char param1[] = "-m100000";
    //  static char *spin_args[] = {param0, param1};
    //  spin_main(sizeof(spin_args) / sizeof(char *), spin_args);
}
