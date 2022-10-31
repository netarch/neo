#include "process/choose_conn.hpp"

#include <cassert>

#include "candidates.hpp"
#include "lib/logger.hpp"
#include "model-access.hpp"
#include "process/forwarding.hpp"
#include "protocols.hpp"

static inline std::vector<std::vector<int>> get_conn_map() {
    std::vector<std::vector<int>> conn_map(3); // executable -> [ conns ]

    for (int conn = 0; conn < model.get_num_conns(); ++conn) {
        int exec = model.get_executable_for_conn(conn);
        int mode = model.get_fwd_mode_for_conn(conn);
        int proto_state = model.get_proto_state_for_conn(conn);

        if ((exec == 0 && mode == fwd_mode::DROPPED) ||
            (exec == 0 && mode == fwd_mode::ACCEPTED &&
             PS_IS_LAST(proto_state))) {
            continue;
        }

        conn_map[exec].push_back(conn);
    }

    return conn_map;
}

void ChooseConnProcess::update_choice_count() const {
    if (!enabled) {
        return;
    }

    std::vector<std::vector<int>> conn_map = get_conn_map();
    /* 2: executable, not entering a middlebox
     * 1: executable, about to enter a middlebox
     * 0: not executable (missing packet or terminated) */

    if (!conn_map[2].empty()) {
        model.set_choice_count(1);
    } else if (!conn_map[1].empty()) {
        model.set_choice_count(conn_map[1].size());
    } else if (!conn_map[0].empty()) {
        model.set_choice_count(1);
    } else {
        // every connection is dropped
        model.set_choice_count(0);
        model.print_conn_states();
    }
}

void ChooseConnProcess::exec_step() {
    if (!enabled) {
        return;
    }

    int choice = model.get_choice();
    std::vector<std::vector<int>> conn_map = get_conn_map();
    /* 2: executable, not entering a middlebox
     * 1: executable, about to enter a middlebox
     * 0: not executable (missing packet or terminated) */

    if (!conn_map[2].empty()) {
        assert(choice == 0);
        model.set_conn(conn_map[2][0]);
        model.print_conn_states();
    } else if (!conn_map[1].empty()) {
        model.set_conn(conn_map[1][choice]);
        model.set_executable(2);
        model.print_conn_states();
    } else if (!conn_map[0].empty()) {
        // pick the first connection that hasn't been dropped or terminated and
        // drop it
        assert(choice == 0);
        model.set_conn(conn_map[0][0]);
        int mode = model.get_fwd_mode();
        int proto_state = model.get_proto_state();
        if (!(mode == fwd_mode::DROPPED) &&
            !(mode == fwd_mode::ACCEPTED && PS_IS_LAST(proto_state))) {
            Logger::info("Connection " + std::to_string(conn_map[0][0]) +
                         " dropped by " +
                         model.get_pkt_location()->to_string());
            model.set_fwd_mode(fwd_mode::DROPPED);
        }
        model.print_conn_states();
    } else {
        Logger::error("No executable connection");
    }

    // update choice_count for the connection chosen
    if (model.get_fwd_mode() == fwd_mode::FORWARD_PACKET ||
        model.get_fwd_mode() == fwd_mode::FIRST_FORWARD) {
        model.set_choice_count(model.get_candidates()->size());
    } else {
        model.set_choice_count(1);
    }
}
