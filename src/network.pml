/*
 * Some parts of the system state are stored in hash tables. Pointers to the
 * actual objects are saved in the respective state variables. If the size of a
 * pointer isn't enough for it to represent possible states, the addressing
 * ability of the OS isn't enough for accessing all state-related data.
 */

typedef conn_state_t {
    /* execution logic */
    bool is_executable;
    unsigned process_id : 2;                        /* executing process */

    /* flow information */
    unsigned proto_state : 4;                       /* protocol state */
    int src_ip[4 / SIZEOF_INT];                     /* (uint32_t) */
    int dst_ip_ec[SIZEOF_VOID_P / SIZEOF_INT];      /* (EqClass *) */
    unsigned src_port : 16;                         /* (uint16_t) */
    unsigned dst_port : 16;                         /* (uint16_t) */
    int seq[4 / SIZEOF_INT];                        /* (uint32_t) */
    int ack[4 / SIZEOF_INT];                        /* (uint32_t) */
    int src_node[SIZEOF_VOID_P / SIZEOF_INT];       /* (Node *) */
    int tx_node[SIZEOF_VOID_P / SIZEOF_INT];        /* (Node *) */
    int rx_node[SIZEOF_VOID_P / SIZEOF_INT];        /* (Node *) */

    /* forwarding information */
    unsigned fwd_mode : 3;                          /* forwarding mode */
    int pkt_location[SIZEOF_VOID_P / SIZEOF_INT];   /* (Node *) */
    int ingress_intf[SIZEOF_VOID_P / SIZEOF_INT];   /* (Interface *) */
    int candidates[SIZEOF_VOID_P / SIZEOF_INT];     /* (Candidates *) */

    /* per-flow data plane state */
    int fib[SIZEOF_VOID_P / SIZEOF_INT];            /* (FIB *) */
    int path_choices[SIZEOF_VOID_P / SIZEOF_INT];   /* (Choices *), multipath choices for stateful connections */
};

/* connection state */
conn_state_t conn_state[MAX_CONNS];

/* execution logic */
int choice;                 /* non-determinisic selection result */
int choice_count;           /* non-determinisic selection range [0, choice_count) */
int conn;                   /* index of the currently active connection */
int num_conns;              /* total number of (concurrent) connections */

/* data plane state */
int pkt_hist[SIZEOF_VOID_P / SIZEOF_INT];       /* (PacketHistory *), emulation state */
int openflow_update_state[SIZEOF_VOID_P / SIZEOF_INT];  /* (OpenflowUpdateState *) */

/* policy */
bool violated;              /* whether the policy has been violated */
int correlated_policy_idx;  /* index of the current correlated policy */
/* loadbalance policy */
int reach_counts[SIZEOF_VOID_P / SIZEOF_INT];   /* (ReachCounts *) */


c_code {
    \#include "api.hpp"
};

init {
    c_code {
        initialize(&now);
    };

    do
    :: choice_count > 0 ->
        select(choice: 0 .. choice_count - 1);
        c_code {
            exec_step(&now);
        };
    :: else -> break;
    od

    c_code {
        report(&now);
    };
    assert(!violated);
}
