/*
 * Some parts of the system state is stored in hash tables. Pointers to the
 * hash table entries are put in the actual state object. If the size of a
 * pointer isn't enough for storing possible states, the addressing ability of
 * the OS isn't enough for accessing all state-related data.
 */

/* maximum number of ECs modelled simultaneously */
#define MAX_EC_COUNT 2

/*
 * Network state for one EC in Neo consists of the dataplane (fib), location
 * of the packet, the history of updates made, and the execution mode
 */
typedef network_state_t {
    int fib[SIZEOF_VOID_P / SIZEOF_INT];            /* FIB/dataplane */
    int packet_location;                            /* current pkt location */
    int update_hist[SIZEOF_VOID_P / SIZEOF_INT];    /* update history */
    byte exec_mode;                                 /* execution mode
                                                       (process/process.hpp) */
};

network_state_t network_state[MAX_EC_COUNT];
byte itr_ec;        /* index of the executing EC */
int choice_count;   /* non-determinisic selection range [0, choice_count) */
int choice;         /* non-determinisic selection result */


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
            execute(&now);
        }
    :: else -> break
    od
}
