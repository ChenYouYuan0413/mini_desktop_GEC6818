#ifndef BUTTON_H
#define BUTTON_H

typedef struct
{
    int cx, cy;
    int half_w, half_h;
    unsigned int bg_color;
    unsigned int border_color;
    int visible;
    int learned;
    int filled;
    int corner_radius;
    const char *label;
    unsigned int label_color;
    unsigned int *bg_bmp;
    int bg_bmp_w, bg_bmp_h;
} button_t;

void button_init(button_t *b, int cx, int cy, int w, int h,
                 unsigned int bg, unsigned int border);
void button_init_rounded(button_t *b, int cx, int cy, int w, int h,
                         unsigned int bg, unsigned int border, int cr);
void button_draw(button_t *b);
int  button_hit(button_t *b, int sx, int sy);
void button_set_learned(button_t *b, int on);
void button_set_filled(button_t *b, int filled);

#endif
