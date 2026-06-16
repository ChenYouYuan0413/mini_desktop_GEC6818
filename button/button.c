#include "button.h"
#include "../common/device.h"
#include "../fonts/font_16x16.h"
#include <stddef.h>

void button_init(button_t *b, int cx, int cy, int w, int h,
                 unsigned int bg, unsigned int border)
{
    b->cx = cx;
    b->cy = cy;
    b->half_w = w / 2;
    b->half_h = h / 2;
    b->bg_color = bg;
    b->border_color = border;
    b->visible = 1;
    b->learned = 0;
    b->filled = 1;
    b->corner_radius = 0;
    b->label = NULL;
    b->label_color = 0;
    b->bg_bmp = NULL;
    b->bg_bmp_w = 0;
    b->bg_bmp_h = 0;
}

void button_init_rounded(button_t *b, int cx, int cy, int w, int h,
                         unsigned int bg, unsigned int border, int cr)
{
    button_init(b, cx, cy, w, h, bg, border);
    b->corner_radius = cr;
}

void button_draw(button_t *b)
{
    if (!b->visible) return;

    int x0 = b->cx - b->half_w;
    int y0 = b->cy - b->half_h;
    int w  = b->half_w * 2;
    int h  = b->half_h * 2;
    int cr = b->corner_radius;

    /* BMP image background (pre-rendered Apple-style button) */
    if (b->bg_bmp)
    {
        int bx = b->cx - b->bg_bmp_w / 2;
        int by = b->cy - b->bg_bmp_h / 2;
        show_bmp(b->bg_bmp, b->bg_bmp_w, b->bg_bmp_h, bx, by);
        /* skip regular fill/border — fall through to label + learned dot */
    }
    else if (cr > 0)
    {
        /* Rounded rect: border (full size) then fill (inset by 2px) */
        if (b->filled)
        {
            if (b->border_color != b->bg_color)
            {
                fb_fill_rounded(x0, y0, w, h, cr, b->border_color);
                fb_fill_rounded(x0 + 2, y0 + 2, w - 4, h - 4,
                                cr > 2 ? cr - 2 : 0, b->bg_color);
            }
            else
            {
                fb_fill_rounded(x0, y0, w, h, cr, b->bg_color);
            }
        }
    }
    else
    {
        /* Original sharp-rectangle path */
        if (b->filled)
            fb_fill(x0, y0, w, h, b->bg_color);

        int i;
        for (i = 0; i < 2; i++)
        {
            fb_fill(x0 + i, y0, w - i * 2, 1, b->border_color);
            fb_fill(x0 + i, y0 + h - 1, w - i * 2, 1, b->border_color);
        }
        for (i = 0; i < 2; i++)
        {
            fb_fill(x0, y0 + i, 1, h - i * 2, b->border_color);
            fb_fill(x0 + w - 1, y0 + i, 1, h - i * 2, b->border_color);
        }
    }

    /* Label */
    if (b->label && b->label[0])
    {
        int len = 0;
        const char *p;
        for (p = b->label; *p; p++) len++;

        int avail_w = w - 8;
        int avail_h = h - 6;

        int target_dh = avail_h;
        if (target_dh > 22) target_dh = 22;
        int target_dw = target_dh;

        int total_w = target_dw * len;
        if (total_w > avail_w)
        {
            target_dw = avail_w / len;
            target_dh = target_dw;
        }

        int start_x = b->cx - (target_dw * len) / 2;
        int start_y = b->cy - target_dh / 2;

        show_string(font_16x16, FONT_FW, FONT_FH,
                    b->label, start_x, start_y, target_dw, target_dh,
                    b->label_color);
    }

    /* Learned dot */
    if (b->learned)
    {
        int dx = x0 + w - 10;
        int dy = y0 + 8;
        int r = 4;
        int x, y;
        for (y = dy - r; y <= dy + r; y++)
        {
            for (x = dx - r; x <= dx + r; x++)
            {
                if ((x - dx) * (x - dx) + (y - dy) * (y - dy) <= r * r)
                {
                    fb_put(x, y, RGB(0, 255, 0));
                }
            }
        }
    }
}

int button_hit(button_t *b, int sx, int sy)
{
    if (!b->visible) return 0;
    return (sx >= b->cx - b->half_w && sx <= b->cx + b->half_w &&
            sy >= b->cy - b->half_h && sy <= b->cy + b->half_h);
}

void button_set_learned(button_t *b, int on)
{
    b->learned = on;
}

void button_set_filled(button_t *b, int filled)
{
    b->filled = filled;
}
