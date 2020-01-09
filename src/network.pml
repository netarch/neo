/*
 * The state of the whole system consists of:
 * - network state
 *      - fib
 *      - update history
 * - fowarding process state
 *      - packet state
 *      - forwarding mode
 *      - packet EC (destination IP range)
 *      - source IP of the current EC
 *      - source node (entry node) of the current EC
 *      - packet history (of the whole network)
 *      - current packet location
 *      - ingress interface (entry interface) of the current EC
 * - policy state
 *      - violation status
 *
 * Some parts of the system state are stored in hash tables. Pointers to the
 * actual objects are saved in the respective state variables. If the size of a
 * pointer isn't enough for it to represent possible states, the addressing
 * ability of the OS isn't enough for accessing all state-related data.
 */

/* maximum number of communications modelled simultaneously */
#define MAX_COMM_COUNT 2

typedef comm_state_t {
    /* network */
    int fib[SIZEOF_VOID_P / SIZEOF_INT];            /* (FIB *) */

    /* forwarding process */
    unsigned pkt_state : 4;                         /* packet state */
    unsigned fwd_mode : 3;                          /* execution mode */
    int ec[SIZEOF_VOID_P / SIZEOF_INT];             /* (EqClass *), current EC (destination IP range) */
    int seq[4 / SIZEOF_INT];                        /* (uint32_t) */
    int ack[4 / SIZEOF_INT];                        /* (uint32_t) */
    int src_ip[4 / SIZEOF_INT];                     /* (uint32_t) */
    int src_node[SIZEOF_VOID_P / SIZEOF_INT];       /* (Node *) */
    int pkt_hist[SIZEOF_VOID_P / SIZEOF_INT];       /* (PacketHistory *) */
    int pkt_location[SIZEOF_VOID_P / SIZEOF_INT];   /* (Node *) */
    int ingress_intf[SIZEOF_VOID_P / SIZEOF_INT];   /* (Interface *) */

    /* policy */
    bool violated;
};

comm_state_t comm_state[MAX_COMM_COUNT];
byte comm;          /* index of the executing communication */
int choice;         /* non-determinisic selection result */
int choice_count;   /* non-determinisic selection range [0, choice_count) */
int candidates[SIZEOF_VOID_P / SIZEOF_INT]; /* (std::vector<FIB_IPNH> *) */


c_code {
    \#include "api.hpp"
};

init {
    c_code {
        now.comm = 0;
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
    assert(!comm_state[comm].violated);
}
