#pragma once

#include <vector>
#include <string>

#include "connspec.hpp"
#include "connmatrix.hpp"
#include "conn.hpp"
class Network;
struct State;

#define POL_NULL        0
#define POL_INIT_FWD    1

class Policy
{
protected:
    int id;
    std::vector<ConnSpec> conn_specs;   // specs of concurrent connections
    ConnectionMatrix conn_matrix;       // conn matrix computed from spec
    std::vector<Connection> conns;      // initial conns for verification
    /*
     * If there is any correlated policy, conn_specs should be empty. There
     * should be only one conn_spec within each correlated policy.
     */
    std::vector<Policy *> correlated_policies;
    const Network *network;

protected:
    friend class Config;
    Policy(bool correlated = false);

public:
    virtual ~Policy();

    int get_id() const;
    size_t num_conn_ecs() const;
    void compute_conn_matrix();
    bool set_conns();
    const std::vector<Connection>& get_conns() const;
    void report(State *) const;

    virtual std::string to_string() const = 0;
    virtual void init(State *, const Network *) const;
    virtual int check_violation(State *) = 0;
};

class Policies
{
private:
    std::vector<Policy *> policies;

private:
    friend class Config;

public:
    typedef std::vector<Policy *>::size_type size_type;
    typedef std::vector<Policy *>::iterator iterator;
    typedef std::vector<Policy *>::const_iterator const_iterator;
    typedef std::vector<Policy *>::reverse_iterator reverse_iterator;
    typedef std::vector<Policy *>::const_reverse_iterator const_reverse_iterator;

    Policies() = default;
    Policies(const Policies&) = delete;
    Policies(Policies&&) = delete;
    Policies& operator=(const Policies&) = delete;
    Policies& operator=(Policies&&) = default;

    ~Policies();

    iterator               begin();
    const_iterator         begin() const;
    iterator               end();
    const_iterator         end() const;
    reverse_iterator       rbegin();
    const_reverse_iterator rbegin() const;
    reverse_iterator       rend();
    const_reverse_iterator rend() const;
};
