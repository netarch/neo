/*
 *
 *
 */

/*
 * State of the network.
 * sizeof(void *) / sizeof(int) == 2 on 64-bit systems
 *
 * If the size of a pointer isn't enough for storing possible states, the
 * addressing ability of the OS isn't enough for accessing all state-related
 * data.
 */
int state[SIZEOF_VOID_P / SIZEOF_INT];

//c_code {
//    \#include <>
//};

init {
    c_code {
        //initialize(&now);
        printf("Network Promela Model...\n");
    }
}
