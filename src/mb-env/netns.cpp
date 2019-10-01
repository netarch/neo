#include "mb-env/netns.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <unistd.h>

#include "lib/logger.hpp"

NetNS::NetNS(const Node *node __attribute__((unused)))
{
    const char *netns_path = "/proc/self/ns/net";

    // save the original netns fd
    if ((old_net = open(netns_path, O_RDONLY)) < 0) {
        Logger::get_instance().err(netns_path, errno);
    }
    // create and enter a new netns
    if (unshare(CLONE_NEWNET) < 0) {
        Logger::get_instance().err("Failed to create a new netns", errno);
    }
    // save the new netns fd
    if ((new_net = open(netns_path, O_RDONLY)) < 0) {
        Logger::get_instance().err(netns_path, errno);
    }
    // create virtual interfaces and bridges
    // TODO
    // update routing table according to node->rib
    // TODO
    // return to the original netns
    if (setns(old_net, CLONE_NEWNET) < 0) {
        Logger::get_instance().err("Failed to setns", errno);
    }
}

NetNS::~NetNS()
{
    // delete the created virtual interfaces and bridges
    // TODO

    close(new_net);
}

void NetNS::run(void (*app_action)(MB_App *), MB_App *app)
{
    // enter the isolated netns
    if (setns(new_net, CLONE_NEWNET) < 0) {
        Logger::get_instance().err("Failed to setns", errno);
    }

    app_action(app);

    // return to the original netns
    if (setns(old_net, CLONE_NEWNET) < 0) {
        Logger::get_instance().err("Failed to setns", errno);
    }
}
