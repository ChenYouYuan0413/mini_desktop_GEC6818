#ifndef TOOLS_H
#define TOOLS_H

/* Drawing utilities (depend on device.h for fb_put) */

void draw_circle(int cx, int cy, int r, unsigned int color);
void draw_circle_fill(int cx, int cy, int r, unsigned int color);
void draw_rect(int x, int y, int w, int h, unsigned int color);
void draw_rect_fill(int x, int y, int w, int h, unsigned int color);
void draw_cross(int cx, int cy, int sz, unsigned int color);
void draw_line(int x0, int y0, int x1, int y1, unsigned int color);

#endif
