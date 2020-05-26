//
// Created by Santhosh Prabhu on 5/25/20.
//
#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "forwarding.hpp"
#include "model.h"

class VectorPtrHasher
{
public:
    size_t operator()(const std::vector<size_t> *v) const
    {
        size_t value = 0;
        for (const auto& entry : *v) {
            ::hash::hash_combine(value, entry);
        }

        return value;
    }
};

class VectorPtrEqual
{
public:
    bool operator()(const std::vector<size_t> *a, const std::vector<size_t> *b) const
    {
        return *a == *b;
    }
};

class Openflow
{
    static std::unordered_set<std::vector<size_t> *, VectorPtrHasher, VectorPtrEqual> offsets_map;
    static std::map<std::string, std::vector<Route>>      updates;

    static std::vector<size_t> *get_offset_vector(State *state)
    {
        std::vector<size_t> *res;
        memccpy(&res, state->comm_state[state->comm].openflow_update_offsets, sizeof(std::vector<size_t> *), 1);
        return res;
    }

    static size_t get_update_size(const std::string& node, State *state)   //get how many updates are yet to be installed on node, in state
    {
        auto itr = updates.find(node);

        if (itr != updates.end()) {
            auto total_updates = itr->second.size();
            auto offset_vector_idx = std::distance(itr, updates.begin());
            const auto& offset_vector = *(get_offset_vector(state));
            auto installed_updates = offset_vector[offset_vector_idx];
            assert(installed_updates <= total_updates);
            return total_updates - installed_updates;
        } else {
            return 0;
        }
    }

    static Route get_next_update(const std::string& node, State *state)
    {
        auto itr = updates.find(node);
        assert(itr != updates.end() && !itr->second.empty());
        auto offset_vector_idx = std::distance(itr, updates.begin());
        const auto& offset_vector = *(get_offset_vector(state));
        auto installed_updates = offset_vector[offset_vector_idx];
        const Route& route = itr->second[installed_updates];

        std::vector<size_t> *new_offsets = new std::vector<size_t>(offset_vector);
        (*new_offsets)[offset_vector_idx]++;
        auto insert_result = offsets_map.insert(new_offsets);

        if (!insert_result.second) {
            delete new_offsets;
        }

        memccpy(state->comm_state[state->comm].openflow_update_offsets, &(*(insert_result.first)), sizeof(std::vector<size_t> *), 1);
        return route;
    }

public:
    static void parse_config(const std::shared_ptr<cpptoml::table_array>& of_config)
    {
        if (of_config) {
            for (const std::shared_ptr<cpptoml::table>& cfg : *of_config) {
                auto node_name = cfg->get_as<std::string>("node");
                auto update_cfg = cfg->get_table_array("updates");

                std::vector<Route> routes;

                for (const std::shared_ptr<cpptoml::table>& sr_cfg : *update_cfg) {
                    Route route (sr_cfg);
                    if (route.get_adm_dist() == 255) {  // user did not specify adm dist
                        route.set_adm_dist(1);
                    }

                    routes.push_back(std::move(route));
                }

                updates[*node_name] = std::move(routes);
            }
        }
    }

    static void install_updates(State *state, Network& network)
    {
        if (state->choice) {
            Node *current_node;
            memcpy(&current_node, state->comm_state[state->comm].pkt_location,
                   sizeof(Node *));
            auto route = Openflow::get_next_update(current_node->get_name(), state);
            current_node->get_rib().insert(std::move(route));
            network.update_fib(state);

            if (Openflow::has_updates(current_node->get_name(), state)) {
                state->comm_state[state->comm].fwd_mode = int(fwd_mode::CHECK_UPDATES);
                state->choice_count = 2;    // non-deterministic choice to decide whether to install an update or not
                return;
            }
        }

        state->comm_state[state->comm].fwd_mode = int(fwd_mode::COLLECT_NHOPS);
        state->choice_count = 1;    // deterministic choice
    }

    static bool has_updates(const std::string& node, State *state)
    {
        return Openflow::get_update_size(node, state) > 0;
    }
};
