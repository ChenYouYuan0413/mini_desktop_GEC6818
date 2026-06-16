#include "../common/device.h"
#include "../ir_app/ir_app.h"
#include <stdio.h>

int main(void)
{
    device_init();
    ir_app_init();
    ir_app_run();
    device_deinit();
    return 0;
}
