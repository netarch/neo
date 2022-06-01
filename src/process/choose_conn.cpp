#include "process/choose_conn.hpp"

#include <cassert>

#include "candidates.hpp"
#include "lib/logger.hpp"
#include "model-access.hpp"
#include "process/forwarding.hpp"
#include "protocols.hpp"

#include "model.h"

static inline int get_conn_executable(State *state, int conn) {
    return state->conn_state[conn].executable;
}

static inline int get_conn_fwd_mode(State *state, int conn) {
    return state->conn_state[conn].fwd_mode;
}

static inline std::vector<std::vector<int>> get_conn_map(State *state) {
    std::vector<std::vector<int>> conn_map(3); // executable -> [ conns ]
    for (int conn = 0; conn < get_num_conns(state); ++conn) {
        int exec = get_conn_executable(state, conn);
        if (exec == 0 && get_conn_fwd_mode(state, conn) == fwd_mode::DROPPED) {
            continue;
        }
        conn_map[exec].push_back(conn);
    }
    return conn_map;
}

void ChooseConnProcess::update_choice_count(State *state) const {
    if (!enabled) {
        return;
    }

    std::vector<std::vector<int>> conn_map = get_conn_map(state);
    /* 2: executable, not entering a middlebox
     * 1: executable, about to enter a middlebox
     * 0: not executable (missing packet) */

    if (!conn_map[2].empty()) {
        set_choice_count(state, 1);
    } else if (!conn_map[1].empty()) {
        set_choice_count(state, conn_map[1].size());
    } else if (!conn_map[0].empty()) {
        set_choice_count(state, 1);
    } else {
        // every connection is dropped
        set_choice_count(state, 0);
    }
}

void ChooseConnProcess::exec_step(State *state,
                                  const Network &network
                                  __attribute__((unused))) {
    if (!enabled) {
        return;
    }

    int choice = get_choice(state);
    std::vector<std::vector<int>> conn_map = get_conn_map(state);
    /* 2: executable, not entering a middlebox
     * 1: executable, about to enter a middlebox
     * 0: not executable (missing packet) */

    if (!conn_map[2].empty()) {
        assert(choice == 0);
        set_conn(state, conn_map[2][0]);
        print_conn_states(state);
    } else if (!conn_map[1].empty()) {
        set_conn(state, conn_map[1][choice]);
        set_executable(state, 2);
        print_conn_states(state);
    } else if (!conn_map[0].empty()) {
        // pick the first connection that's not dropped and drop it
        assert(choice == 0);
        set_conn(state, conn_map[0][0]);
        Logger::info("Connection " + std::to_string(conn_map[0][0]) +
                     " dropped by " + get_pkt_location(state)->to_string());
        set_fwd_mode(state, fwd_mode::DROPPED);
    } else {
        Logger::error("No executable connection");
    }

    // update choice_count for the connection chosen
    if (get_fwd_mode(state) == fwd_mode::FORWARD_PACKET ||
        get_fwd_mode(state) == fwd_mode::FIRST_FORWARD) {
        set_choice_count(state, get_candidates(state)->size());
    } else {
        set_choice_count(state, 1);
    }
}
