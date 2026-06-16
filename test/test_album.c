#include "../common/device.h"
#include "../desktop/desktop.h"
#include <stdio.h>

int main(void)
{
    device_init();
    desktop_init();
    desktop_run();
    device_deinit();
    return 0;
}
