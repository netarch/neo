/*
 * The state of the whole system consists of:
 * - network state (per EC)
 *   - fib
 *   - update history
 * - fowarding process state (per EC)
 *   - forwarding mode
 *   - current packet location
 * - policy state (per EC)
 *   - violation status
 *
 * Some parts of the system state are stored in hash tables. Pointers to the
 * actual objects are saved in the respective state variables. If the size of a
 * pointer isn't enough for it to represent possible states, the addressing
 * ability of the OS isn't enough for accessing all state-related data.
 */

/* maximum number of ECs modelled simultaneously */
#define MAX_EC_COUNT 2

typedef network_state_t {
    /* network */
    int fib[SIZEOF_VOID_P / SIZEOF_INT];            /* (FIB *) */
    int update_hist[SIZEOF_VOID_P / SIZEOF_INT];    /* update history */

    /* forwarding process */
    byte fwd_mode;                                  /* execution mode */
    int pkt_location[SIZEOF_VOID_P / SIZEOF_INT];   /* (Node *) */
    /*int ingress_intf[SIZEOF_VOID_P / SIZEOF_INT];   /* (Interface *) */
    /*int l3_nhop[SIZEOF_VOID_P / SIZEOF_INT];        /* (Node *) */

    /* policy */
    bool violated;
};

network_state_t network_state[MAX_EC_COUNT];
byte itr_ec = 0;    /* index of the executing EC */
int choice;         /* non-determinisic selection result */
int choice_count;   /* non-determinisic selection range [0, choice_count) */
int candidates[SIZEOF_VOID_P / SIZEOF_INT]; /* (std::vector<Node *> *) */


c_code {
    \#include "api.hpp"
};

init {
    c_code {
        initialize(&now);
    }

    do
    :: choice_count > 0 ->
        select(choice: 0 .. choice_count - 1);
        c_code {
            exec_step(&now);
        }
    :: else -> break
    od

    c_code {
        report(&now);
    }
    assert(!network_state[itr_ec].violated);
}
