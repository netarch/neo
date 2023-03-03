#ifndef NEO_SRC_MB_ENV_DOCKER_NETNS_H_
#define NEO_SRC_MB_ENV_DOCKER_NETNS_H_

#include "mb-env.hpp"
#include "netns.hpp"

class Docker_NetNS : public NetNS {
    void init(const Middlebox &) override;

    ~Docker_NetNS() override;
};

#endif // NEO_SRC_MB_ENV_DOCKER_NETNS_H_
