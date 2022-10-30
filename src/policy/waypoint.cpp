#include "policy/waypoint.hpp"

#include "model-access.hpp"
#include "node.hpp"
#include "process/forwarding.hpp"
#include "protocols.hpp"

std::string WaypointPolicy::to_string() const {
    std::string ret = "Waypoint (";
    ret += pass_through ? "O" : "X";
    ret += "): [";
    for (Node *node : target_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ]";
    return ret;
}

void WaypointPolicy::init() {
    Policy::init();
    model.set_violated(false);
}

int WaypointPolicy::check_violation() {
    int mode = model.get_fwd_mode();

    if (mode == fwd_mode::COLLECT_NHOPS) {
        if (target_nodes.count(model.get_pkt_location()) > 0) {
            // encountering waypoint
            model.set_violated(!pass_through);
            model.set_choice_count(0);
        }
    }

    return POL_NULL;
}
