#include "device.h"
#include <stdlib.h>

/* --- Circle outline (midpoint algorithm) --- */
void draw_circle(int cx, int cy, int r, unsigned int color)
{
    int x = 0;
    int y = r;
    int d = 1 - r;

    while (x <= y)
    {
        fb_put(cx + x, cy + y, color);
        fb_put(cx + y, cy + x, color);
        fb_put(cx - x, cy + y, color);
        fb_put(cx - y, cy + x, color);
        fb_put(cx + x, cy - y, color);
        fb_put(cx + y, cy - x, color);
        fb_put(cx - x, cy - y, color);
        fb_put(cx - y, cy - x, color);

        if (d < 0)
            d += 2 * x + 3;
        else
        {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

/* --- Filled circle --- */
void draw_circle_fill(int cx, int cy, int r, unsigned int color)
{
    int x = 0;
    int y = r;
    int d = 1 - r;

    while (x <= y)
    {
        int i;
        for (i = cx - x; i <= cx + x; i++) fb_put(i, cy - y, color);
        for (i = cx - y; i <= cx + y; i++) fb_put(i, cy - x, color);
        for (i = cx - x; i <= cx + x; i++) fb_put(i, cy + y, color);
        for (i = cx - y; i <= cx + y; i++) fb_put(i, cy + x, color);

        if (d < 0)
            d += 2 * x + 3;
        else
        {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

/* --- Rectangle outline --- */
void draw_rect(int x, int y, int w, int h, unsigned int color)
{
    int i;
    for (i = x; i < x + w; i++) { fb_put(i, y, color); fb_put(i, y + h - 1, color); }
    for (i = y; i < y + h; i++) { fb_put(x, i, color); fb_put(x + w - 1, i, color); }
}

/* --- Filled rectangle --- */
void draw_rect_fill(int x, int y, int w, int h, unsigned int color)
{
    fb_fill(x, y, w, h, color);
}

/* --- Crosshair --- */
void draw_cross(int cx, int cy, int sz, unsigned int color)
{
    int i;
    for (i = cx - sz; i <= cx + sz; i++) fb_put(i, cy, color);
    for (i = cy - sz; i <= cy + sz; i++) fb_put(cx, i, color);
}

/* --- Line (Bresenham) --- */
void draw_line(int x0, int y0, int x1, int y1, unsigned int color)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    int x = x0, y = y0;

    while (1)
    {
        fb_put(x, y, color);
        if (x == x1 && y == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }
}
