#pragma once

#include <map>
#include <vector>
#include <unordered_set>
#include <string>

#include "process/process.hpp"
#include "policy/policy.hpp"
#include "node.hpp"
#include "network.hpp"
class State;

/*
 * The state of how many updates have been installed.
 */
class OpenflowUpdateState
{
private:
    // index: node order in Openflow::updates
    // value: number of installed updates of that node
    std::vector<size_t> update_vector;

    friend bool operator==(const OpenflowUpdateState&, const OpenflowUpdateState&);
    friend struct OFUpdateStateHash;

public:
    OpenflowUpdateState(size_t num_nodes): update_vector(num_nodes, 0) {}
    OpenflowUpdateState(const OpenflowUpdateState&) = default;

    size_t num_of_installed_updates(int node_order) const;
    void install_update_at(int node_order);
};

bool operator==(const OpenflowUpdateState&, const OpenflowUpdateState&);

struct OFUpdateStateHash {
    size_t operator()(const OpenflowUpdateState *const&) const;
};

struct OFUpdateStateEq {
    bool operator()(const OpenflowUpdateState *const&,
                    const OpenflowUpdateState *const&) const;
};

/*
 * An openflow process handles the openflow updates within one verification
 * session.
 */
class OpenflowProcess : public Process
{
private:
    std::map<Node *, std::vector<Route>> updates;
    std::unordered_set<
        OpenflowUpdateState *,
        OFUpdateStateHash,
        OFUpdateStateEq> update_state_hist;

    void install_update(State *, Network&);

private:
    friend class Config;
    void add_update(Node *, Route&&);

public:
    OpenflowProcess() = default;
    ~OpenflowProcess();

    std::string to_string() const;
    const decltype(updates)& get_updates() const;
    bool has_updates(State *, Node *) const;

    void init(State *); // initialize the openflow process when system starts
    void reset(State *); // reset the openflow process
    void exec_step(State *, Network&) override;
};
