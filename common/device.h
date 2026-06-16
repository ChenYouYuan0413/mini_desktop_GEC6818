#ifndef DEVICE_H
#define DEVICE_H

#include "param.h"

/* ================================================================
 * Unified init / deinit
 * ================================================================ */
void device_init(void);
void device_deinit(void);

/* ================================================================
 * Framebuffer (/dev/fb0)
 * ================================================================ */
int  fb_open(void);
void fb_close(void);
void fb_reset(void);
void fb_put(int x, int y, unsigned int color);
void fb_fill(int x, int y, int w, int h, unsigned int color);
void fb_fill_rounded(int x, int y, int w, int h, int r, unsigned int color);
void fb_clear(void);
int  fb_get_w(void);
int  fb_get_h(void);

/* ================================================================
 * Touchscreen (/dev/input/event0)
 * ================================================================ */
int  ts_open(void);
void ts_close(void);
void ts_raw_to_screen(int raw_x, int raw_y, int *sx, int *sy);
int  ts_read(int *sx, int *sy, int *touching);
int  get_touch_dir(void);

/* ================================================================
 * BMP loader
 * ================================================================ */
unsigned int *bmp_load(const char *filename, int *w, int *h);
void bmp_display(unsigned int *pixels, int w, int h);
void show_bmp(unsigned int *pixels, int w, int h, int x0, int y0);
unsigned int *bmp_reshape(unsigned int *src, int sw, int sh, int dw, int dh);
void show_char(const unsigned char *font, int idx, int fw, int fh,
               int x0, int y0, unsigned int color);
void show_char_scale(const unsigned char *font, int idx, int fw, int fh,
                     int x0, int y0, int dw, int dh, unsigned int color);
void show_number(const unsigned char *num_font, int stride, int num,
                 int fw, int fh, int x0, int y0, int dw, int dh,
                 unsigned int color);
void show_string(const unsigned char *font, int fw, int fh,
                 const char *str, int x0, int y0, int dw, int dh,
                 unsigned int color);

/* ================================================================
 * Buzzer (/dev/beep)
 * ================================================================ */
int  beep_open(void);
void beep_close(void);
void beep_on(void);
void beep_off(void);
void beep_freq(int hz);
void beep_note(int hz, int ms);

/* ================================================================
 * LED (sysfs: /sys/kernel/gec_ctrl/led_*)
 * ================================================================ */
int  led_open(void);
void led_close(void);
void led_on(int n);
void led_off(int n);
/* n: 1-5 individual, <=0 all */

#endif
