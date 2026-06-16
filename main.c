#include "common/device.h"
#include "desktop/desktop.h"
#include "ir_app/ir_app.h"

int main(void)
{
    device_init();
    ir_app_init();
    desktop_init();
    desktop_run();
    device_deinit();
    return 0;
}
