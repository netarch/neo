#include "middlebox/middlebox.hpp"

Middlebox::Middlebox(const std::shared_ptr<cpptoml::table>& node_config)
    : Node(node_config)
{
    // TODO read "driver", "config", ...
}
