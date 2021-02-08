#include "mb-app/mb-app.hpp"

void mb_app_init(MB_App *app)
{
    app->init();
}

void mb_app_reset(MB_App *app)
{
    app->reset();
}
