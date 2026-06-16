#include "device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/fb.h>
#include <errno.h>
#include <linux/input.h>

/* ================================================================
 * Unified init / deinit
 * ================================================================ */
void device_init(void)
{
    if (fb_open() < 0) exit(1);
    if (ts_open() < 0) exit(1);
    beep_open();
    led_open();
}

void device_deinit(void)
{
    led_close();
    beep_close();
    ts_close();
    fb_close();
}

/* ================================================================
 * Globals
 * ================================================================ */
static int fb_fd   = -1;
static int fb_w    = 0;
static int fb_h    = 0;
static int fb_line = 0;
static long fb_size = 0;
static char *fb_map = NULL;

static int ts_fd = -1;

static int beep_fd = -1;

/* ================================================================
 * Framebuffer (/dev/fb0)
 * ================================================================ */
int fb_open(void)
{
    fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd < 0)
    {
        perror("open /dev/fb0");
        return -1;
    }

    struct fb_var_screeninfo vi;
    struct fb_fix_screeninfo fi;

    ioctl(fb_fd, FBIOGET_VSCREENINFO, &vi);
    ioctl(fb_fd, FBIOGET_FSCREENINFO, &fi);

    vi.yoffset = 0;
    ioctl(fb_fd, FBIOPAN_DISPLAY, &vi);

    fb_w    = vi.xres;
    fb_h    = vi.yres;
    fb_line = fi.line_length;
    fb_size = fi.smem_len;

    fb_map = (char *)mmap(0, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fb_map == MAP_FAILED)
    {
        perror("mmap");
        close(fb_fd);
        fb_fd = -1;
        return -1;
    }

    memset(fb_map, 0, fb_size);
    return 0;
}

void fb_close(void)
{
    if (fb_map)
    {
        memset(fb_map, 0, fb_size);
        munmap(fb_map, fb_size);
        fb_map = NULL;
    }
    if (fb_fd >= 0)
    {
        close(fb_fd);
        fb_fd = -1;
    }
}

void fb_reset(void)
{
    if (fb_fd < 0) return;
    struct fb_var_screeninfo vi;
    struct fb_fix_screeninfo fi;
    ioctl(fb_fd, FBIOGET_VSCREENINFO, &vi);
    ioctl(fb_fd, FBIOGET_FSCREENINFO, &fi);
    vi.yoffset = 0;
    ioctl(fb_fd, FBIOPAN_DISPLAY, &vi);
    fb_w    = vi.xres;
    fb_h    = vi.yres;
    fb_line = fi.line_length;
    fb_size = fi.smem_len;
    memset(fb_map, 0, fb_size);
}

void fb_put(int x, int y, unsigned int color)
{
    if (x < 0 || x >= fb_w || y < 0 || y >= fb_h)
        return;
    unsigned int off = y * fb_line + x * 4;
    *(unsigned int *)(fb_map + off) = color;
}

void fb_fill(int x, int y, int w, int h, unsigned int color)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > fb_w) w = fb_w - x;
    if (y + h > fb_h) h = fb_h - y;
    if (w <= 0 || h <= 0) return;

    int row, col;
    for (row = 0; row < h; row++)
    {
        unsigned int off = (y + row) * fb_line + x * 4;
        for (col = 0; col < w; col++)
            *(unsigned int *)(fb_map + off + col * 4) = color;
    }
}

void fb_fill_rounded(int x, int y, int w, int h, int r, unsigned int color)
{
    if (r <= 0) { fb_fill(x, y, w, h, color); return; }
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;

    /* Center + four edge rectangles */
    fb_fill(x + r, y, w - 2 * r, h, color);
    fb_fill(x, y + r, r, h - 2 * r, color);
    fb_fill(x + w - r, y + r, r, h - 2 * r, color);

    /* Four corner quarter-circles */
    int dx, dy, rr = r * r;
    for (dy = 0; dy < r; dy++)
    {
        for (dx = 0; dx < r; dx++)
        {
            if (dx * dx + dy * dy > rr) continue;
            int cx_tl = x + r - 1 - dx; int cy_tl = y + r - 1 - dy;
            fb_put(cx_tl, cy_tl, color);                           /* top-left */
            fb_put(x + w - r + dx, cy_tl, color);                  /* top-right */
            fb_put(cx_tl, y + h - r + dy, color);                  /* bottom-left */
            fb_put(x + w - r + dx, y + h - r + dy, color);         /* bottom-right */
        }
    }
}

void fb_clear(void)
{
    if (fb_map)
        memset(fb_map, 0, fb_size);
}

int fb_get_w(void) { return fb_w; }
int fb_get_h(void) { return fb_h; }

/* ================================================================
 * Touchscreen (/dev/input/event0)
 * ================================================================ */
int ts_open(void)
{
    ts_fd = open("/dev/input/event0", O_RDONLY | O_NONBLOCK);
    if (ts_fd < 0)
    {
        perror("open /dev/input/event0");
        return -1;
    }
    return 0;
}

void ts_close(void)
{
    if (ts_fd >= 0)
    {
        close(ts_fd);
        ts_fd = -1;
    }
}

void ts_raw_to_screen(int raw_x, int raw_y, int *sx, int *sy)
{
    *sx = raw_x * SCREEN_W / TS_RAW_W;
    *sy = (raw_y - TS_Y_OFF) * SCREEN_H / (TS_RAW_H - TS_Y_OFF);
    if (*sx < 0) *sx = 0;
    if (*sx >= SCREEN_W) *sx = SCREEN_W - 1;
    if (*sy < 0) *sy = 0;
    if (*sy >= SCREEN_H) *sy = SCREEN_H - 1;
}

int ts_read(int *sx, int *sy, int *touching)
{
    struct input_event ev;
    static int raw_x = 0;
    static int raw_y = 0;
    static int down  = 0;
    static int has_new = 0;

    has_new = 0;

    while (read(ts_fd, &ev, sizeof(ev)) == sizeof(ev))
    {
        if (ev.type == EV_ABS)
        {
            if (ev.code == ABS_X) raw_x = ev.value;
            if (ev.code == ABS_Y) raw_y = ev.value;
        }
        if (ev.type == EV_KEY && ev.code == BTN_TOUCH)
        {
            down = ev.value;
            ts_raw_to_screen(raw_x, raw_y, sx, sy);

            if (down)
            {
                *touching = 1;
                has_new = 1;
            }
            else
            {
                *touching = 0;
                has_new = 1;
            }
            if (has_new) return 1;
        }
        if (ev.type == EV_SYN && ev.code == SYN_REPORT && down)
        {
            ts_raw_to_screen(raw_x, raw_y, sx, sy);
            *touching = 1;
            has_new = 1;
            if (has_new) return 1;
        }
    }

    return has_new;
}

int get_touch_dir(void)
{
    static int start_x = -1, start_y = -1;
    static int cur_x = 0, cur_y = 0;

    struct input_event ev;
    static int raw_x, raw_y;

    while (ts_fd >= 0)
    {
        int n = read(ts_fd, &ev, sizeof(ev));
        if (n != sizeof(ev))
        {
            if (n < 0 && errno == EAGAIN) break;
            break;
        }

        if (ev.type == EV_ABS)
        {
            if (ev.code == ABS_X) raw_x = ev.value;
            if (ev.code == ABS_Y) raw_y = ev.value;
        }
        if (ev.type == EV_KEY && ev.code == BTN_TOUCH)
        {
            if (ev.value == 1)
            {
                ts_raw_to_screen(raw_x, raw_y, &start_x, &start_y);
                cur_x = start_x;
                cur_y = start_y;
            }
            else
            {
                ts_raw_to_screen(raw_x, raw_y, &cur_x, &cur_y);

                int dx = cur_x - start_x;
                int dy = cur_y - start_y;
                int ax = abs(dx);
                int ay = abs(dy);

                start_x = -1;

                if (ax < SWIPE_MIN_DIST && ay < SWIPE_MIN_DIST)
                    return 0;

                if (ax > ay * 2)
                    return dx > 0 ? 4 : 3;   /* RIGHT=4 : LEFT=3 */
                if (ay > ax * 2)
                    return dy > 0 ? 2 : 1;   /* DOWN=2 : UP=1 */

                return 0;
            }
        }
        if (ev.type == EV_SYN && ev.code == SYN_REPORT && start_x >= 0)
        {
            ts_raw_to_screen(raw_x, raw_y, &cur_x, &cur_y);
        }
    }

    return 0;
}

/* ================================================================
 * BMP loader
 * ================================================================ */
unsigned int *bmp_load(const char *filename, int *w, int *h)
{
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        printf("bmp_load: cannot open %s\n", filename);
        return NULL;
    }

    struct stat st;
    fstat(fd, &st);
    char *raw = malloc(st.st_size);
    read(fd, raw, st.st_size);
    close(fd);

    int data_off = *(int *)(raw + 10);
    int bmp_w    = *(int *)(raw + 18);
    int bmp_h    = abs(*(int *)(raw + 22));
    int bpp      = *(short *)(raw + 28);
    int row_size = ((bmp_w * bpp + 31) / 32) * 4;

    printf("bmp_load: %dx%d @ %dbpp\n", bmp_w, bmp_h, bpp);

    unsigned int *pixels = malloc(bmp_w * bmp_h * 4);

    int y;
    for (y = 0; y < bmp_h; y++)
    {
        int bmp_y = bmp_h - 1 - y;
        unsigned char *row = (unsigned char *)(raw + data_off + bmp_y * row_size);

        int x;
        for (x = 0; x < bmp_w; x++)
        {
            int stride = (bpp == 32) ? 4 : 3;
            unsigned char b = row[x * stride + 0];
            unsigned char g = row[x * stride + 1];
            unsigned char r = row[x * stride + 2];
            unsigned char a = (bpp == 32) ? row[x * stride + 3] : 255;
            pixels[y * bmp_w + x] = ((unsigned int)a << 24) | RGB(r, g, b);
        }
    }

    free(raw);
    *w = bmp_w;
    *h = bmp_h;
    return pixels;
}

void show_bmp(unsigned int *pixels, int w, int h, int x0, int y0)
{
    /* Clip X */
    int x_start = 0, x_end = w;
    if (x0 < 0) x_start = -x0;
    if (x0 + w > fb_w) x_end = fb_w - x0;
    if (x_start >= x_end) return;
    int copy_bytes = (x_end - x_start) * 4;

    /* Fast path: no clipping + matching stride → single memcpy */
    if (x_start == 0 && x_end == w && y0 >= 0 && y0 + h <= fb_h && w * 4 == fb_line)
    {
        memcpy(fb_map + y0 * fb_line + x0 * 4, pixels, w * h * 4);
        return;
    }

    /* Row-by-row fallback */
    int y;
    for (y = 0; y < h; y++)
    {
        int sy = y0 + y;
        if (sy < 0 || sy >= fb_h) continue;
        memcpy(fb_map + sy * fb_line + (x0 + x_start) * 4,
               pixels + y * w + x_start, copy_bytes);
    }
}

void bmp_display(unsigned int *pixels, int w, int h)
{
    int x0 = (fb_w - w) / 2;
    int y0 = (fb_h - h) / 2;

    /* Fast path: full-screen BMP with matching stride → single memcpy */
    if (x0 == 0 && y0 == 0 && w * 4 == fb_line)
    {
        memcpy(fb_map, pixels, w * h * 4);
        return;
    }

    /* Row-by-row fallback (stride mismatch or partial placement) */
    int y;
    for (y = 0; y < h; y++)
    {
        int sy = y0 + y;
        if (sy < 0 || sy >= fb_h) continue;
        memcpy(fb_map + sy * fb_line + x0 * 4,
               pixels + y * w, w * 4);
    }
}

unsigned int *bmp_reshape(unsigned int *src, int sw, int sh, int dw, int dh)
{
    unsigned int *dst = malloc(dw * dh * 4);
    int y, x;

    for (y = 0; y < dh; y++)
    {
        int sy = y * sh / dh;
        for (x = 0; x < dw; x++)
        {
            int sx = x * sw / dw;
            dst[y * dw + x] = src[sy * sw + sx];
        }
    }

    return dst;
}

/* ================================================================
 * Font rendering (monochrome bitmap, 8 pixels per byte)
 * ================================================================ */
void show_char(const unsigned char *font, int idx, int fw, int fh,
               int x0, int y0, unsigned int color)
{
    int bytes_per_col = (fh + 7) / 8;
    int char_bytes = bytes_per_col * fw;
    const unsigned char *data = font + idx * char_bytes;
    int row, col;

    for (row = 0; row < fh; row++)
    {
        for (col = 0; col < fw; col++)
        {
            int byte_idx = col * bytes_per_col + row / 8;
            int bit_idx  = row % 8;

            if (data[byte_idx] & (1 << bit_idx))
            {
                fb_put(x0 + col, y0 + row, color);
            }
        }
    }
}

void show_char_scale(const unsigned char *font, int idx, int fw, int fh,
                     int x0, int y0, int dw, int dh, unsigned int color)
{
    int bytes_per_col = (fh + 7) / 8;
    int char_bytes = bytes_per_col * fw;
    const unsigned char *data = font + idx * char_bytes;
    int row, col;

    for (row = 0; row < dh; row++)
    {
        int src_row = row * fh / dh;

        for (col = 0; col < dw; col++)
        {
            int src_col = col * fw / dw;
            int byte_idx = src_col * bytes_per_col + src_row / 8;
            int bit_idx  = src_row % 8;

            if (data[byte_idx] & (1 << bit_idx))
            {
                fb_put(x0 + col, y0 + row, color);
            }
        }
    }
}

void show_number(const unsigned char *num_font, int stride, int num,
                 int fw, int fh, int x0, int y0, int dw, int dh,
                 unsigned int color)
{
    char buf[16];
    sprintf(buf, "%d", num);
    int i;
    for (i = 0; buf[i]; i++)
    {
        int d = buf[i] - '0';
        show_char_scale(num_font + d * stride, 0, fw, fh,
                        x0 + i * dw, y0, dw, dh, color);
    }
}

static int ascii_to_glyph(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= '0' && c <= '9') return c - '0' + 26;
    return -1;
}

void show_string(const unsigned char *font, int fw, int fh,
                 const char *str, int x0, int y0, int dw, int dh,
                 unsigned int color)
{
    if (!str || !font) return;
    int i;
    for (i = 0; str[i]; i++)
    {
        int idx = ascii_to_glyph(str[i]);
        if (idx >= 0)
            show_char_scale(font, idx, fw, fh, x0 + i * dw, y0, dw, dh, color);
    }
}

/* ================================================================
 * Buzzer (sysfs: /sys/kernel/gec_ctrl/beep, expects 4-byte write)
 * ================================================================ */
int beep_open(void)
{
    beep_fd = open("/sys/kernel/gec_ctrl/beep", O_WRONLY);
    if (beep_fd < 0)
    {
        perror("open /sys/kernel/gec_ctrl/beep");
        return -1;
    }
    return 0;
}

void beep_close(void)
{
    if (beep_fd >= 0)
    {
        beep_off();
        close(beep_fd);
        beep_fd = -1;
    }
}

void beep_on(void)
{
    if (beep_fd < 0) return;
    int v = 1;
    lseek(beep_fd, 0, SEEK_SET);
    write(beep_fd, &v, sizeof(v));
}

void beep_off(void)
{
    if (beep_fd < 0) return;
    int v = 0;
    lseek(beep_fd, 0, SEEK_SET);
    write(beep_fd, &v, sizeof(v));
}

void beep_freq(int hz)
{
    (void)hz;
    beep_on();
}

void beep_note(int hz, int ms)
{
    (void)hz;
    beep_on();
    usleep(ms * 1000);
    beep_off();
    usleep(20 * 1000);
}

/* ================================================================
 * LED (standard Linux LED class: /sys/class/leds/ledN/brightness)
 * ================================================================ */
static int led_fds[6];  /* 0=all, 1-5 = led1..led5 */

static const char *led_path(int n)
{
    static char buf[64];
    if (n <= 0) snprintf(buf, sizeof(buf), "/sys/class/leds/led1/brightness");
    else        snprintf(buf, sizeof(buf), "/sys/class/leds/led%d/brightness", n);
    return buf;
}

int led_open(void)
{
    int i;
    for (i = 1; i <= 5; i++)
    {
        led_fds[i] = open(led_path(i), O_WRONLY);
        if (led_fds[i] < 0) perror(led_path(i));
    }
    return led_fds[1] >= 0 ? 0 : -1;
}

void led_close(void)
{
    int i;
    for (i = 1; i <= 5; i++)
    {
        if (led_fds[i] >= 0) { close(led_fds[i]); led_fds[i] = -1; }
    }
}

void led_on(int n)
{
    int i;
    if (n <= 0 || n > 5)
    {
        for (i = 1; i <= 5; i++) led_on(i);
        return;
    }
    if (led_fds[n] < 0) return;
    lseek(led_fds[n], 0, SEEK_SET);
    write(led_fds[n], "1", 1);
}

void led_off(int n)
{
    int i;
    if (n <= 0 || n > 5)
    {
        for (i = 1; i <= 5; i++) led_off(i);
        return;
    }
    if (led_fds[n] < 0) return;
    lseek(led_fds[n], 0, SEEK_SET);
    write(led_fds[n], "0", 1);
}
