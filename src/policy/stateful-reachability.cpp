#include "policy/stateful-reachability.hpp"

StatefulReachabilityPolicy::StatefulReachabilityPolicy(
    const std::shared_ptr<cpptoml::table>& config,
    const Network& net __attribute__((unused)))
    : Policy(config)
{
}
