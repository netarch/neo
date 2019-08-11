#include "process/forwarding.hpp"
#include "plankton.hpp"
#include "util.hpp"

std::unordered_set<std::vector<Node *>> ForwardingProcess::selected_nodes_set;
static Plankton& plankton = Plankton::get_instance();

ForwardingProcess::ForwardingProcess(): current_node(nullptr),
    ingress_intf(nullptr), l3_nhop(nullptr)
{
}

void ForwardingProcess::init()
{
}

const std::vector<Node *> *ForwardingProcess::get_or_create_selection_list(std::vector<Node *>&& input)
{
    decltype(ForwardingProcess::selected_nodes_set)::iterator itr;
    std::tie(itr, std::ignore) = ForwardingProcess::selected_nodes_set.insert(std::move(input));
    return (&(*itr));
}

static void update_node_selection_vector_and_size(State *state, std::vector<Node *>&& selection_vector)
{
    int& choice_count = state->choice_count;
    choice_count = selection_vector.size();
    auto selected_nodes_ptr = ForwardingProcess::get_or_create_selection_list(std::move(selection_vector));
    plankton_util::copy_ptr_to_int_arr(state->selected_nodes, selected_nodes_ptr);
}

static void populate_neighbors_in_selection_vector(State *state)
{
    auto& packet_location = state->network_state[state->itr_ec].packet_location;
    auto fib = plankton.get_network().get_fib();
    Node *packet_location_node;
    plankton_util::copy_int_arr_to_ptr(packet_location_node, packet_location);
    Logger::get_instance().info("Current packet location: " + packet_location_node->get_name());
    auto selected_nodes_tmp = fib->find_ip_nexthops(packet_location_node);
    if (selected_nodes_tmp.empty()) {
        //Packet dropped
        Logger::get_instance().info("Nowhere to go from: " + packet_location_node->get_name());
        return;
    }
    if (selected_nodes_tmp.size() == 1 && selected_nodes_tmp[0] == packet_location_node) {
        Logger::get_instance().info("Delivered: " + packet_location_node->get_name());
        return;
    }
    update_node_selection_vector_and_size(state, std::move(selected_nodes_tmp));
}

void ForwardingProcess::exec_step(State *state) const
{
    auto& exec_mode = state->network_state[state->itr_ec].exec_mode;
    auto& packet_location = state->network_state[state->itr_ec].packet_location;
    int& choice_count = state->choice_count;
    state->choice_count = 0; /* No non-determinsitic choices to be made as of now */

    switch (exec_mode) {
        case exec_type::INIT       : {
            auto selected_nodes_tmp = plankton.get_policy()->get_packet_entry_points(state);
            assert(!selected_nodes_tmp.empty());
            update_node_selection_vector_and_size(state, std::move(selected_nodes_tmp));
            exec_mode = int(exec_type::INJECT_PACKET);
            break;
        }
        case exec_type::INJECT_PACKET      : {
            std::vector<Node *> *selected_nodes_ptr;
            plankton_util::copy_int_arr_to_ptr(selected_nodes_ptr, state->selected_nodes);
            Node *injection_point = (*selected_nodes_ptr)[state->choice];
            Logger::get_instance().info("Chosen injection point is: " + injection_point->get_name());
            plankton_util::copy_ptr_to_int_arr(packet_location, injection_point);
            exec_mode = int(exec_type::PICK_NEIGHBOR);
            choice_count = 1; // No non-determinisic choice to be made, but don't quit.
            break;
        }
        case exec_type::PICK_NEIGHBOR      : {
            populate_neighbors_in_selection_vector(state);
            exec_mode = int(exec_type::FORWARD_PACKET);
            break;
        }
        case exec_type::FORWARD_PACKET      : {
            std::vector<Node *> *selected_nodes_ptr;
            plankton_util::copy_int_arr_to_ptr(selected_nodes_ptr, state->selected_nodes);
            Node *chosen_neighbor = (*selected_nodes_ptr)[state->choice];
            Logger::get_instance().info("Chosen neighbor is: " + chosen_neighbor->get_name());
            plankton_util::copy_ptr_to_int_arr(packet_location, chosen_neighbor);
            exec_mode = int(exec_type::PICK_NEIGHBOR);
            choice_count = 1; // No non-determinisic choice to be made, but don't quit.
            break;
        }
        default                    :
            throw std::logic_error("Unknown step");
    }

    return;
}
