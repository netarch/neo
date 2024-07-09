#pragma once

#include <string>
#include <vector>

#include "conn.hpp"
#include "connmatrix.hpp"
#include "connspec.hpp"

#define POL_NULL      0
#define POL_REINIT_DP 1

class Invariant {
protected:
    int _id;
    std::vector<ConnSpec> _conn_specs; // specs of concurrent connections
    ConnectionMatrix _conn_matrix;     // conn matrix computed from spec
    std::vector<Connection> _conns;    // initial conns for verification
    /**
     * If there is any correlated invariant, _conn_specs should be empty. There
     * should be only one conn_spec within each correlated invariant.
     */
    std::vector<std::shared_ptr<Invariant>> _correlated_invs;

protected:
    friend class ConfigParser;
    Invariant(bool correlated = false);

public:
    virtual ~Invariant() = default;

    int id() const { return _id; }
    const decltype(_conns) &conns() const { return _conns; }

    size_t num_conn_ecs() const;
    void compute_conn_matrix();
    bool set_conns();
    std::string conns_str() const;
    void report() const;

    virtual std::string to_string() const = 0;
    virtual void init();
    virtual void reinit();
    virtual int check_violation() = 0;
};

class Invariants {
private:
    std::vector<Invariant *> _invs;

private:
    friend class ConfigParser;

public:
    typedef std::vector<Invariant *>::size_type size_type;
    typedef std::vector<Invariant *>::iterator iterator;
    typedef std::vector<Invariant *>::const_iterator const_iterator;
    typedef std::vector<Invariant *>::reverse_iterator reverse_iterator;
    typedef std::vector<Invariant *>::const_reverse_iterator
        const_reverse_iterator;

    Invariants()                              = default;
    Invariants(const Invariants &)            = delete;
    Invariants(Invariants &&)                 = delete;
    Invariants &operator=(const Invariants &) = delete;
    Invariants &operator=(Invariants &&)      = default;

    ~Invariants() {
        for (Invariant *inv : _invs) {
            delete inv;
        }
    }

    iterator begin() { return _invs.begin(); }
    const_iterator begin() const { return _invs.begin(); }
    iterator end() { return _invs.end(); }
    const_iterator end() const { return _invs.end(); }
    reverse_iterator rbegin() { return _invs.rbegin(); }
    const_reverse_iterator rbegin() const { return _invs.rbegin(); }
    reverse_iterator rend() { return _invs.rend(); }
    const_reverse_iterator rend() const { return _invs.rend(); }
};
