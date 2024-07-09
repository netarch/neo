#include "invariant/invariant.hpp"

#include <cassert>
#include <csignal>
#include <unistd.h>

#include "logger.hpp"
#include "model-access.hpp"

Invariant::Invariant(bool correlated) {
    static int next_id = 1;
    if (correlated) {
        _id = 0;
    } else {
        _id = next_id++;
    }
}

size_t Invariant::num_conn_ecs() const {
    if (_correlated_invs.empty()) {
        return _conn_matrix.num_conns();
    } else {
        size_t num = 1;
        for (const auto &p : _correlated_invs) {
            num *= p->_conn_matrix.num_conns();
        }
        return num;
    }
}

void Invariant::compute_conn_matrix() {
    // update invariant-wide ECs with invariant-aware ranges
    if (_correlated_invs.empty()) {
        for (const ConnSpec &conn_spec : _conn_specs) {
            conn_spec.update_inv_ecs();
        }
    } else {
        for (const auto &p : _correlated_invs) {
            p->_conn_specs[0].update_inv_ecs();
        }
    }

    // gather _conn_specs' connections
    if (_correlated_invs.empty()) {
        _conn_matrix.clear();
        for (const ConnSpec &conn_spec : _conn_specs) {
            _conn_matrix.add(conn_spec.compute_connections());
        }
    } else {
        for (const auto &p : _correlated_invs) {
            p->_conn_matrix.clear();
            p->_conn_matrix.add(p->_conn_specs[0].compute_connections());
        }
    }
}

bool Invariant::set_conns() {
    if (_correlated_invs.empty()) {
        _conns = _conn_matrix.get_next_conns();
        if (!_conns.empty()) {
            return true;
        }
        return false;
    } else {
        for (const auto &p : _correlated_invs) {
            if (p->set_conns()) {
                assert(p->_conns.size() == 1);
                return true;
            } // return false for carrying the next "tick"
            p->_conn_matrix.reset();
        }
        return false;
    }
}

std::string Invariant::conns_str() const {
    std::string ret;
    for (const Connection &conn : _conns) {
        ret += conn.to_string() + "\n";
    }
    ret.pop_back();
    return ret;
}

void Invariant::report() const {
    if (model.get_violated()) {
        logger.info("*** Invariant violated! ***");
        kill(getppid(), SIGUSR1);
    } else {
        logger.info("*** Invariant holds! ***");
    }
}

void Invariant::init() {
    if (_correlated_invs.empty()) {
        // per-connection states
        for (size_t i = 0; i < _conns.size(); ++i) {
            _conns[i].init(i);
        }
        model.set_conn(0);
        model.set_num_conns(_conns.size());
        model.print_conn_states();
    } else {
        _correlated_invs[model.get_correlated_inv_idx()]->init();
    }
}

// reinit should only be overwritten by invariants with correlated
// sub-invariants
void Invariant::reinit() {
    logger.error("This shouldn't be called");
}
