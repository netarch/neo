/*
 *
 *
 */

/*
 * State of the network.
 * 2 == sizeof(void *) / sizeof(int), on 64-bit systems
 *
 * If the size of a pointer isn't enough for storing possible states, the
 * addressing ability of the OS isn't enough for accessing all state-related
 * data.
 */
int state[2];

//c_code {
//    \#include <>
//};

init {
    c_code {
        //initialize(&now);
        printf("Network Promela Model...\n");
    }
}
