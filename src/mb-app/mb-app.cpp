#include "mb-app/mb-app.hpp"

#include "lib/logger.hpp"

MB_App::MB_App(const std::shared_ptr<cpptoml::table>& config)
{
    auto t = config->get_as<long>("timeout");

    if (!t) {
        Logger::get().err("Missing timeout");
    }

    if (*t < 0) {
        Logger::get().err("Invalid timeout value: " + std::to_string(*t));
    }
    timeout = std::chrono::microseconds(*t);
}

std::chrono::microseconds MB_App::get_timeout() const
{
    return timeout;
}

void mb_app_init(MB_App *app)
{
    app->init();
}

void mb_app_reset(MB_App *app)
{
    app->reset();
}
