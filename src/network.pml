/* We store some parts of the system state in hash tables, and put pointers to the hashtable entries
 * in the actual state object. If the size of a pointer isn't enough for storing possible states, the
 * addressing ability of the OS isn't enough for accessing all state-related
 * data.
 */
#define MAX_EC_COUNT 2 /* Model 2 simultaneous ECs by default. */

typedef network_state_type {
    /* Network state for one EC in Neo consists of the dataplane (fib),
    location of the packet, the history of updates made, and the next step to execute */

    /*
     * Pointer to a duplicate-eliminated FIB.
     * sizeof(void *) / sizeof(int) == 2 on 64-bit systems
     */
    int fib_ptr[SIZEOF_VOID_P / SIZEOF_INT];

    /* Where is the packet currently */
    int packet_location;

    /* Update history for the network. The history is duplicate eliminated. */
    int update_ptr[SIZEOF_VOID_P / SIZEOF_INT];

    /* What is the next step to be carried out in the execution of this equivalence class (enum) */
    byte next_step;
};

network_state_type network_state[MAX_EC_COUNT]; /* Need to store separate state for each EC that is modeled */

/* Range to choose non deterministically from. A number n in [0,n] is chosen. When choice_count is -1, the program quits*/
int choice_count;

/* Non-deterministically chosen value */
int choice;

/* Which of the MAX_EC_COUNT simultaneous ECs is executing right now */
byte itr_ec;

c_code {
    \#include "api.hpp"
};

init {
    c_code {
        initialize(&now);
        execute(&now);
    }

    do
        ::choice_count >= 0 ->
        select(choice: 0 .. choice_count);
    c_code {
        execute(&now);
    }
    :: else -> break
        od
    }
