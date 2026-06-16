#include "../common/device.h"
#include "../button/button.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    device_init();
    fb_clear();

    /* Create 2 test buttons */
    button_t btn1, btn2;
    button_init(&btn1, 200, 300, 160, 80, RGB(255, 255, 255), RGB(0, 0, 0));
    button_init(&btn2, 600, 300, 160, 80, RGB(255, 255, 255), RGB(0, 0, 0));

    printf("=== Swipe + Button Test ===\n");
    printf("Left button = learn toggle, Right button = quit\n");
    printf("Swipe left/right to change button color.\n\n");

    int running = 1;
    while (running)
    {
        fb_clear();

        /* Check swipe */
        int dir = get_touch_dir();
        switch (dir)
        {
        case 3:
            printf("LEFT swipe\n");
            btn1.bg_color = RGB(255, 200, 200);
            break;
        case 4:
            printf("RIGHT swipe\n");
            btn1.bg_color = RGB(200, 200, 255);
            break;
        case 0:
            break;
        default:
            btn1.bg_color = RGB(255, 255, 255);
        }

        /* Check touch for buttons via ts_read */
        int sx, sy, touching;
        while (ts_read(&sx, &sy, &touching) > 0)
        {
            if (touching && button_hit(&btn1, sx, sy))
            {
                printf("BTN1 pressed\n");
                button_set_learned(&btn1, !btn1.learned);
            }
            if (touching && button_hit(&btn2, sx, sy))
            {
                printf("BTN2 pressed → quit\n");
                running = 0;
            }
        }

        btn1.bg_color = btn1.learned ? RGB(200, 255, 200) : btn1.bg_color;
        button_draw(&btn1);
        button_draw(&btn2);
    }

    device_deinit();
    return 0;
}
