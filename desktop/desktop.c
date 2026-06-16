#include "desktop.h"
#include "../common/device.h"
#include "../button/button.h"
#include "../album/album.h"
#include "../ir_app/ir_app.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define CLR_BG       RGB(28,  28,  30)
#define CLR_SURFACE  RGB(44,  44,  46)
#define CLR_BLUE     RGB(0,   122, 255)
#define CLR_GREEN    RGB(52,  199, 89)
#define CLR_RED      RGB(255, 59,  48)
#define CLR_GRAY     RGB(142, 142, 147)
#define CLR_WHITE    RGB(255, 255, 255)
#define TOP_H        44

static unsigned int *g_bg = NULL;
static int g_bg_w, g_bg_h;

static unsigned int *g_icon_photo = NULL;
static int g_icon_photo_w, g_icon_photo_h;
static unsigned int *g_icon_game = NULL;
static int g_icon_game_w, g_icon_game_h;
static unsigned int *g_icon_ir = NULL;
static int g_icon_ir_w, g_icon_ir_h;

static button_t g_photo_btn;
static button_t g_game_btn;
static button_t g_ir_btn;
static int g_selected = -1;   /* -1=none, 0=photo, 1=game, 2=ir */

void desktop_init(void)
{
    g_bg = bmp_load("icon/background.bmp", &g_bg_w, &g_bg_h);

    g_icon_photo = bmp_load("icon/photo_icon.bmp", &g_icon_photo_w, &g_icon_photo_h);
    g_icon_game  = bmp_load("icon/game_icon.bmp", &g_icon_game_w, &g_icon_game_h);
    g_icon_ir    = bmp_load("icon/ir_icon.bmp", &g_icon_ir_w, &g_icon_ir_h);

    button_init_rounded(&g_photo_btn, 160, 260, 100, 100, CLR_BG, CLR_GRAY, 12);
    button_set_filled(&g_photo_btn, 0);
    button_init_rounded(&g_game_btn,  400, 260, 100, 100, CLR_BG, CLR_GRAY, 12);
    button_set_filled(&g_game_btn, 0);
    button_init_rounded(&g_ir_btn,    640, 260, 100, 100, CLR_BG, CLR_GRAY, 12);
    button_set_filled(&g_ir_btn, 0);

    printf("Desktop init OK\n");
}

static void draw_sel_box(button_t *btn)
{
    int x0 = btn->cx - btn->half_w - 4;
    int y0 = btn->cy - btn->half_h - 4;
    int w  = btn->half_w * 2 + 8;
    int h  = btn->half_h * 2 + 8;
    int t  = 3;

    fb_fill(x0, y0, w, t, CLR_WHITE);
    fb_fill(x0, y0 + h - t, w, t, CLR_WHITE);
    fb_fill(x0, y0, t, h, CLR_WHITE);
    fb_fill(x0 + w - t, y0, t, h, CLR_WHITE);
}

static void draw_desktop(void)
{
    if (g_bg)
        bmp_display(g_bg, g_bg_w, g_bg_h);
    else
        fb_fill(0, 0, 800, 480, CLR_BG);

    if (g_icon_photo)
    {
        int x = g_photo_btn.cx - g_icon_photo_w / 2;
        int y = g_photo_btn.cy - g_icon_photo_h / 2;
        show_bmp(g_icon_photo, g_icon_photo_w, g_icon_photo_h, x, y);
    }
    if (g_icon_game)
    {
        int x = g_game_btn.cx - g_icon_game_w / 2;
        int y = g_game_btn.cy - g_icon_game_h / 2;
        show_bmp(g_icon_game, g_icon_game_w, g_icon_game_h, x, y);
    }
    if (g_icon_ir)
    {
        int x = g_ir_btn.cx - g_icon_ir_w / 2;
        int y = g_ir_btn.cy - g_icon_ir_h / 2;
        show_bmp(g_icon_ir, g_icon_ir_w, g_icon_ir_h, x, y);
    }

    if (g_selected == 0) draw_sel_box(&g_photo_btn);
    if (g_selected == 1) draw_sel_box(&g_game_btn);
    if (g_selected == 2) draw_sel_box(&g_ir_btn);
}

/* Wait for finger to lift, then drain any stray IR */
static void wait_finger_up(void)
{
    int _sx, _sy, _t = 1;
    while (_t) { while (ts_read(&_sx, &_sy, &_t) > 0) {} if (_t) usleep(20000); }
}

/* Drain touch + IR after returning from a sub-app */
static void drain_both(void)
{
    int _sx, _sy, _t = 1;
    while (_t) { while (ts_read(&_sx, &_sy, &_t) > 0) {} if (_t) usleep(20000); }
    ir_flush();
}

/* Handle remote action in desktop context. Returns 1 if action consumed. */
static int desktop_remote(int action)
{
    switch (action)
    {
    case 2: /* LEFT */
        if (g_selected < 0) g_selected = 2;
        else if (g_selected > 0) g_selected--;
        draw_desktop();
        return 1;
    case 3: /* RIGHT */
        if (g_selected < 0) g_selected = 0;
        else if (g_selected < 2) g_selected++;
        draw_desktop();
        return 1;
    case 4: /* CENTER — no finger on remote, launch directly */
        if (g_selected < 0) return 1;
        if (g_selected == 0)      return 2;  /* album */
        else if (g_selected == 1) return 4;  /* game */
        else                      return 3;  /* ir */
    }
    return 1;
}

void desktop_run(void)
{
    draw_desktop();

    printf("Desktop: touch Photo(left) or IR(right) icon\n");

    const char *photos[] = {
        "images/photo_1.bmp",
        "images/photo_2.bmp",
        "images/photo_3.bmp",
    };

    int sx, sy, touching;
    int swipe_start_x = -1, swipe_start_y = -1;
    int swipe_tracking = 0;

    while (1)
    {
        while (ts_read(&sx, &sy, &touching) > 0)
        {
            if (touching)
            {
                if (!swipe_tracking)
                {
                    swipe_start_x = sx;
                    swipe_start_y = sy;
                    swipe_tracking = 1;
                }

                if (button_hit(&g_photo_btn, sx, sy))
                {
                    g_selected = 0;
                    printf("→ Album\n");
                    wait_finger_up();
                    album_init(photos, 3);
                    album_show(0);

                    int in_album = 1;
                    int a_start_x = -1, a_start_y = -1, a_tracking = 0;
                    while (in_album)
                    {
                        /* Poll IR for album remote control */
                        int ract;
                        while (ir_poll(&ract))
                        {
                            if (ract == 2)       album_next();
                            else if (ract == 3)  album_prev();
                            else if (ract == 0)  { in_album = 0; break; }
                            usleep(50000);
                        }

                        int asx, asy, at;
                        while (ts_read(&asx, &asy, &at) > 0)
                        {
                            if (at)
                            {
                                if (!a_tracking)
                                {
                                    a_start_x = asx; a_start_y = asy;
                                    a_tracking = 1;
                                }
                                if (asy < TOP_H)
                                {
                                    in_album = 0;
                                    break;
                                }
                            }
                            else
                            {
                                if (a_tracking && a_start_x >= 0)
                                {
                                    int dx = asx - a_start_x;
                                    int dy = asy - a_start_y;
                                    if (abs(dx) > abs(dy) && abs(dx) > 30)
                                    {
                                        if (dx < 0) album_next();
                                        else album_prev();
                                    }
                                }
                                a_tracking = 0;
                                a_start_x = -1;
                            }
                        }
                        /* Check IR again after touch drain */
                        if (ir_poll(&ract))
                        {
                            if (ract == 2)       album_next();
                            else if (ract == 3)  album_prev();
                            else if (ract == 0)  { in_album = 0; break; }
                        }
                    }
                    drain_both();
                    draw_desktop();
                    swipe_tracking = 0;
                }

                if (button_hit(&g_game_btn, sx, sy))
                {
                    g_selected = 1;
                    printf("→ Game\n");
                    wait_finger_up();
                    system("./raycast_game_static");
                    fb_reset();
                    drain_both();
                    draw_desktop();
                }

                if (button_hit(&g_ir_btn, sx, sy))
                {
                    g_selected = 2;
                    printf("→ IR\n");
                    wait_finger_up();
                    ir_app_run();
                    drain_both();
                    draw_desktop();
                }
            }
            else
            {
                if (swipe_tracking && swipe_start_x >= 0)
                {
                    int dx = sx - swipe_start_x;
                    int dy = sy - swipe_start_y;
                    if (abs(dx) > abs(dy) && abs(dx) > 30)
                        printf("Desktop swipe: %s\n", dx > 0 ? "RIGHT" : "LEFT");
                }
                swipe_tracking = 0;
                swipe_start_x = -1;
            }
        }

        /* Background IR polling on desktop */
        int ract;
        while (ir_poll(&ract))
        {
            int ret = desktop_remote(ract);
            if (ret == 2)
            {
                /* Launch album via remote */
                album_init(photos, 3);
                album_show(0);
                int in_album = 1;
                while (in_album)
                {
                    int ract2;
                    while (ir_poll(&ract2))
                    {
                        if (ract2 == 2)       album_next();
                        else if (ract2 == 3)  album_prev();
                        else if (ract2 == 0)  { in_album = 0; break; }
                        usleep(50000);
                    }
                    int asx, asy, at;
                    while (ts_read(&asx, &asy, &at) > 0)
                    {
                        if (at && asy < TOP_H) { in_album = 0; break; }
                        if (!at) continue;
                    }
                    if (ir_poll(&ract2))
                    {
                        if (ract2 == 2)       album_next();
                        else if (ract2 == 3)  album_prev();
                        else if (ract2 == 0)  { in_album = 0; break; }
                    }
                }
                drain_both();
                draw_desktop();
                swipe_tracking = 0;
            }
            else if (ret == 3)
            {
                /* Launch IR app via remote */
                ir_app_run();
                drain_both();
                draw_desktop();
                swipe_tracking = 0;
            }
            else if (ret == 4)
            {
                /* Launch game via remote */
                printf("→ Game (remote)\n");
                system("./raycast_game_static");
                fb_reset();
                drain_both();
                draw_desktop();
                swipe_tracking = 0;
            }
        }
    }
}
