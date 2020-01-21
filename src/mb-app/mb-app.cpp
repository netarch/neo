#include "mb-app/mb-app.hpp"

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
