#include "../common/device.h"
#include "../album/album.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    device_init();
    fb_clear();

    const char *photos[] = {
        "images/photo_1.bmp",
        "images/photo_2.bmp",
        "images/photo_3.bmp",
    };
    int photo_count = 3;

    album_init(photos, photo_count);
    album_show(0);

    printf("=== Photo Album Test ===\n");
    printf("Swipe LEFT = next, RIGHT = prev\n");
    printf("Q to quit.\n\n");

    int running = 1;
    while (running)
    {
        int dir = get_touch_dir();
        switch (dir)
        {
        case 3:
            printf("LEFT → next photo\n");
            album_next();
            break;
        case 4:
            printf("RIGHT → prev photo\n");
            album_prev();
            break;
        }
    }

    device_deinit();
    return 0;
}
