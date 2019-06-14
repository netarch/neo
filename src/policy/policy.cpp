#include "policy/policy.hpp"

bool Policy::check_violation(
    const Network& net __attribute__((unused)),
    const ForwardingProcess& fwd __attribute__((unused)))
{
    return false;
}
