#include "policy/waypoint.hpp"

WaypointPolicy::WaypointPolicy(
    const std::shared_ptr<cpptoml::table>& config,
    const Network& net __attribute__((unused)))
    : Policy(config)
{
}
