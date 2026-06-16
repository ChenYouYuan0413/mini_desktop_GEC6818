#include "ir_app.h"
#include "../common/device.h"
#include "../button/button.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>

#define IR_HEAD1 0xA1
#define IR_HEAD2 0xF1

/* ── Apple-style dark palette ── */
#define CLR_BG       RGB(28,  28,  30)   /* system background */
#define CLR_SURFACE  RGB(44,  44,  46)   /* card / surface   */
#define CLR_BLUE     RGB(0,   122, 255)  /* accent blue      */
#define CLR_GREEN    RGB(52,  199, 89)   /* system green     */
#define CLR_RED      RGB(255, 59,  48)   /* system red       */
#define CLR_GRAY     RGB(142, 142, 147)  /* secondary text   */
#define CLR_SEP      RGB(56,  56,  58)   /* separator        */
#define CLR_DOT      RGB(48,  209, 88)   /* learned dot      */
#define TOP_H        44                  /* nav bar height    */

/* ── Static data ── */
static int ir_fd = -1;
static button_t g_btns[5];               /* up/down/left/right/center */
static button_t g_menu[4];               /* Send Recv Learn Back */
static button_t g_mode_btn;              /* learn-page SEND↔RECV */

static unsigned char g_send_cmds[5][3];
static unsigned char g_recv_cmds[5][3];
static int g_send_learned[5];
static int g_recv_learned[5];

/* Pre-rendered Apple-style assets */
static unsigned int *g_bmp_page_bg = NULL;
static int g_bmp_page_bg_w, g_bmp_page_bg_h;
static unsigned int *g_bmp_bar = NULL;
static int g_bmp_bar_w, g_bmp_bar_h;
/* ── Serial ── */
static int ir_serial_open(void)
{
    int fd = open("/dev/ttySAC1", O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) { perror("ttySAC1"); return -1; }
    struct termios opt;
    tcgetattr(fd, &opt);
    cfsetispeed(&opt, B9600);
    cfsetospeed(&opt, B9600);
    opt.c_cflag |= (CLOCAL | CREAD);
    opt.c_cflag &= ~PARENB; opt.c_cflag &= ~CSTOPB; opt.c_cflag &= ~CSIZE;
    opt.c_cflag |= CS8;
    opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    opt.c_iflag &= ~(IXON | IXOFF | IXANY);
    opt.c_oflag &= ~OPOST;
    opt.c_cc[VMIN] = 0; opt.c_cc[VTIME] = 0;
    tcsetattr(fd, TCSANOW, &opt);
    return fd;
}

static void ir_save_cmd(int idx, const unsigned char *data, int is_recv)
{
    char name[32];
    snprintf(name, sizeof(name), is_recv ? "ir_recv_%d.txt" : "ir_send_%d.txt", idx);
    FILE *f = fopen(name, "w");
    if (f == NULL) return;
    fprintf(f, "%02X %02X %02X\n", data[0], data[1], data[2]);
    fclose(f);
    if (is_recv)
    {
        g_recv_cmds[idx][0] = data[0]; g_recv_cmds[idx][1] = data[1]; g_recv_cmds[idx][2] = data[2];
        g_recv_learned[idx] = 1;
    }
    else
    {
        g_send_cmds[idx][0] = data[0]; g_send_cmds[idx][1] = data[1]; g_send_cmds[idx][2] = data[2];
        g_send_learned[idx] = 1;
    }
}

static int ir_load_cmd(int idx, unsigned char *data, int is_recv)
{
    char name[32];
    snprintf(name, sizeof(name), is_recv ? "ir_recv_%d.txt" : "ir_send_%d.txt", idx);
    FILE *f = fopen(name, "r");
    if (f == NULL) return 0;
    unsigned int v[3];
    int n = fscanf(f, "%02X %02X %02X", &v[0], &v[1], &v[2]);
    fclose(f);
    if (n == 3) { data[0] = v[0]; data[1] = v[1]; data[2] = v[2]; return 1; }
    return 0;
}

static void ir_send(unsigned char d0, unsigned char d1, unsigned char d2)
{
    unsigned char pkt[5] = {IR_HEAD1, IR_HEAD2, d0, d1, d2};
    write(ir_fd, pkt, 5);
}

static unsigned char ir_buf[3];
static int ir_buf_idx = 0;

static int ir_poll_3bytes(unsigned char *out)
{
    if (ir_fd < 0) return 0;
    int n = read(ir_fd, ir_buf + ir_buf_idx, 3 - ir_buf_idx);
    if (n < 0 && errno != EAGAIN)
        printf("  ir: read error %d\n", errno);
    else if (n > 0)
    {
        printf("  ir: got %d byte(s)\n", n);
        ir_buf_idx += n;
        if (ir_buf_idx == 3)
        {
            memcpy(out, ir_buf, 3);
            ir_buf_idx = 0;
            return 1;
        }
    }
    return 0;
}

static int recv_match(const unsigned char *code)
{
    int i;
    for (i = 0; i < 5; i++)
    {
        if (!g_recv_learned[i]) continue;
        if (code[0] == g_recv_cmds[i][0] &&
            code[1] == g_recv_cmds[i][1] &&
            code[2] == g_recv_cmds[i][2])
            return i;
    }
    return -1;
}

/* ── Drawing helpers ── */
static void draw_top_bar(const char *title, unsigned int color)
{
    if (g_bmp_bar)
        show_bmp(g_bmp_bar, g_bmp_bar_w, g_bmp_bar_h, 0, 0);
    else
        fb_fill(0, 0, 800, TOP_H, color);
    printf("=== %s ===\n", title);
}

static void draw_page_bg(void)
{
    if (g_bmp_page_bg)
        show_bmp(g_bmp_page_bg, g_bmp_page_bg_w, g_bmp_page_bg_h, 0, 0);
    else
        fb_fill(0, 0, 800, 480, CLR_BG);
}

static void draw_dir_buttons(int *learned)
{
    int i;
    for (i = 0; i < 5; i++)
        button_set_learned(&g_btns[i], learned[i]);
    for (i = 0; i < 5; i++)
        button_draw(&g_btns[i]);
}

/* ── Pages ── */
static void page_send(void)
{
    draw_page_bg();
    draw_top_bar("Send", CLR_BLUE);
    draw_dir_buttons(g_send_learned);

    int sx, sy, t, btn_ok = 0;
    while (1)
    {
        while (ts_read(&sx, &sy, &t) > 0)
        {
            if (t && sy < TOP_H) return;

            if (!t) { btn_ok = 0; continue; }
            if (btn_ok) continue;

            int i;
            for (i = 0; i < 5; i++)
            {
                if (button_hit(&g_btns[i], sx, sy) && g_send_learned[i])
                {
                    ir_send(g_send_cmds[i][0], g_send_cmds[i][1], g_send_cmds[i][2]);
                    printf("IR TX: %02X %02X %02X\n",
                           g_send_cmds[i][0], g_send_cmds[i][1], g_send_cmds[i][2]);
                    beep_on(); usleep(50000); beep_off();
                    btn_ok = 1;
                }
            }
        }
    }
}

static void page_learn(void)
{
    static int is_recv = 0;

    draw_top_bar(is_recv ? "Learn · Receive" : "Learn · Send", CLR_GREEN);
    draw_page_bg();
    button_draw(&g_mode_btn);

    int i;
    int *learned = is_recv ? g_recv_learned : g_send_learned;
    draw_dir_buttons(learned);

    int learn_idx = -1, btn_handled = 0;
    int sx, sy, t;
    ir_buf_idx = 0;

    int lfd = open("/dev/ttySAC1", O_RDWR | O_NOCTTY | O_NONBLOCK);
    { struct termios o; tcgetattr(lfd, &o);
      cfsetispeed(&o, B9600); cfsetospeed(&o, B9600);
      o.c_cflag |= (CLOCAL | CREAD);
      o.c_cflag &= ~PARENB; o.c_cflag &= ~CSTOPB; o.c_cflag &= ~CSIZE;
      o.c_cflag |= CS8;
      o.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
      o.c_iflag &= ~(IXON | IXOFF | IXANY);
      o.c_oflag &= ~OPOST;
      o.c_cc[VMIN] = 0; o.c_cc[VTIME] = 0;
      tcsetattr(lfd, TCSANOW, &o); }

    while (1)
    {
        while (ts_read(&sx, &sy, &t) > 0)
        {
            if (t && sy < TOP_H) { close(lfd); return; }

            if (!t) { btn_handled = 0; continue; }
            if (btn_handled) continue;

            if (button_hit(&g_mode_btn, sx, sy))
            {
                btn_handled = 1;
                is_recv = !is_recv;
                g_mode_btn.label = is_recv ? "RECV" : "SEND";
                draw_top_bar(is_recv ? "Learn · Receive" : "Learn · Send", CLR_GREEN);
                button_draw(&g_mode_btn);
                learned = is_recv ? g_recv_learned : g_send_learned;
                draw_dir_buttons(learned);
                printf("Learn mode: %s\n", is_recv ? "RECV" : "SEND");
                continue;
            }

            for (i = 0; i < 5; i++)
            {
                if (button_hit(&g_btns[i], sx, sy))
                {
                    learn_idx = i;
                    ir_buf_idx = 0;
                    btn_handled = 1;
                    printf("Learn(%s): waiting IR for btn %d...\n",
                           is_recv ? "RECV" : "SEND", i);
                    break;
                }
            }
        }

        if (learn_idx >= 0)
        {
            int n = read(lfd, ir_buf + ir_buf_idx, 3 - ir_buf_idx);
            if (n > 0)
            {
                printf("  ir: got %d byte(s)\n", n);
                ir_buf_idx += n;
                if (ir_buf_idx == 3)
                {
                    ir_save_cmd(learn_idx, ir_buf, is_recv);
                    printf("  Learned %s btn %d: %02X %02X %02X\n",
                           is_recv ? "RECV" : "SEND", learn_idx,
                           ir_buf[0], ir_buf[1], ir_buf[2]);
                    learned = is_recv ? g_recv_learned : g_send_learned;
                    button_set_learned(&g_btns[learn_idx], learned[learn_idx]);
                    button_draw(&g_btns[learn_idx]);
                    beep_on(); usleep(100000); beep_off();
                    learn_idx = -1;
                    ir_buf_idx = 0;
                }
            }
        }
    }
}

static void page_recv(void)
{
    draw_page_bg();
    draw_top_bar("Receive", CLR_RED);

    /* Draw 4 rounded indicator boxes on the left */
    int i, y0;
    for (i = 0; i < 5; i++)
    {
        y0 = 70 + i * 90;
        unsigned int c = g_recv_learned[i] ? CLR_GREEN : CLR_SEP;
        fb_fill_rounded(10, y0, 80, 60, 8, c);
        /* Subtle border */
        fb_fill(10, y0, 80, 1, CLR_SURFACE);
        fb_fill(10, y0 + 59, 80, 1, CLR_SURFACE);
    }

    int sx, sy, t;
    while (read(ir_fd, ir_buf, 3) > 0) {}
    ir_buf_idx = 0;

    while (1)
    {
        while (ts_read(&sx, &sy, &t) > 0)
        {
            if (t && sy < TOP_H) return;
        }

        unsigned char buf[3];
        if (ir_poll_3bytes(buf))
        {
            printf("IR RX: %02X %02X %02X\n", buf[0], buf[1], buf[2]);
            beep_on(); usleep(80000); beep_off(); usleep(40000);
            beep_on(); usleep(80000); beep_off();

            int m = recv_match(buf);
            if (m >= 0)
            {
                printf("  → matched recv[%d]\n", m);
                y0 = 70 + m * 90;
                fb_fill_rounded(10, y0, 80, 60, 8, RGB(0, 255, 0));
                beep_on(); usleep(200000); beep_off();
                fb_fill_rounded(10, y0, 80, 60, 8, CLR_GREEN);

                switch (m)
                {
                case 0: led_on(1); break;
                case 1: led_on(2); break;
                case 2: led_on(0); break;
                case 3: led_off(0); break;
                }
            }
        }
    }
}

static void show_menu(void)
{
    draw_page_bg();
    draw_top_bar("Infrared", CLR_BG);

    int i;
    for (i = 0; i < 4; i++)
        button_draw(&g_menu[i]);
    printf("[Send] [Recv] [Learn] [Back]\n");
}

/* ── Public ── */
int ir_poll(int *action)
{
    unsigned char buf[3];
    if (!ir_poll_3bytes(buf)) return 0;
    int m = recv_match(buf);
    if (m < 0) return 0;
    if (action) *action = m;
    printf("IR remote: btn[%d] %02X%02X%02X\n", m, buf[0], buf[1], buf[2]);
    return 1;
}

void ir_flush(void)
{
    unsigned char drain[64];
    while (read(ir_fd, drain, sizeof(drain)) > 0) {}
    ir_buf_idx = 0;
}

void ir_app_init(void)
{
    ir_fd = ir_serial_open();

    int i;
    for (i = 0; i < 5; i++)
    {
        unsigned char cmd[3];
        if (ir_load_cmd(i, cmd, 0))
        {
            g_send_cmds[i][0] = cmd[0]; g_send_cmds[i][1] = cmd[1]; g_send_cmds[i][2] = cmd[2];
            g_send_learned[i] = 1;
        }
        else g_send_learned[i] = 0;

        if (ir_load_cmd(i, cmd, 1))
        {
            g_recv_cmds[i][0] = cmd[0]; g_recv_cmds[i][1] = cmd[1]; g_recv_cmds[i][2] = cmd[2];
            g_recv_learned[i] = 1;
        }
        else g_recv_learned[i] = 0;
    }

    /* Load Apple-style BMP assets */
    g_bmp_page_bg = bmp_load("icon/bg_page.bmp", &g_bmp_page_bg_w, &g_bmp_page_bg_h);
    g_bmp_bar     = bmp_load("icon/bar_top.bmp", &g_bmp_bar_w, &g_bmp_bar_h);
    /* Direction buttons — cross layout, rounded */
    button_init_rounded(&g_btns[0], 400, 150, 90, 70, CLR_SURFACE, CLR_SEP, 12);
    g_btns[0].label = "UP";  g_btns[0].label_color = RGB(255, 255, 255);
    button_init_rounded(&g_btns[1], 400, 330, 90, 70, CLR_SURFACE, CLR_SEP, 12);
    g_btns[1].label = "DN";  g_btns[1].label_color = RGB(255, 255, 255);
    button_init_rounded(&g_btns[2], 300, 240, 90, 70, CLR_SURFACE, CLR_SEP, 12);
    g_btns[2].label = "LT";  g_btns[2].label_color = RGB(255, 255, 255);
    button_init_rounded(&g_btns[3], 500, 240, 90, 70, CLR_SURFACE, CLR_SEP, 12);
    g_btns[3].label = "RT";  g_btns[3].label_color = RGB(255, 255, 255);

    /* Center button — circle */
    button_init_rounded(&g_btns[4], 400, 240, 60, 60, CLR_SURFACE, CLR_SEP, 30);
    g_btns[4].label = "OK";  g_btns[4].label_color = RGB(255, 255, 255);

    /* Mode toggle — pill shape at bottom */
    button_init_rounded(&g_mode_btn, 400, 430, 160, 38, CLR_SEP, CLR_GRAY, 19);
    g_mode_btn.label = "SEND";  g_mode_btn.label_color = RGB(255, 255, 255);

    /* Menu buttons — rounded, accent colors, evenly spaced */
    button_init_rounded(&g_menu[0], 160, 260, 130, 90, CLR_BLUE,  CLR_BLUE,  14);
    g_menu[0].label = "SEND";   g_menu[0].label_color = RGB(255, 255, 255);
    button_init_rounded(&g_menu[1], 320, 260, 130, 90, CLR_RED,   CLR_RED,   14);
    g_menu[1].label = "RECV";   g_menu[1].label_color = RGB(255, 255, 255);
    button_init_rounded(&g_menu[2], 480, 260, 130, 90, CLR_GREEN, CLR_GREEN, 14);
    g_menu[2].label = "LEARN";  g_menu[2].label_color = RGB(255, 255, 255);
    button_init_rounded(&g_menu[3], 640, 260, 130, 90, CLR_GRAY,  CLR_GRAY,  14);
    g_menu[3].label = "BACK";   g_menu[3].label_color = RGB(255, 255, 255);

    printf("IR app init OK\n");
}

void ir_app_run(void)
{
    int sx, sy, t;

    /* Flush any residual touch before showing menu */
    {
        int _t = 1;
        while (_t)
        {
            while (ts_read(&sx, &sy, &_t) > 0) {}
            if (_t) usleep(20000);
        }
        usleep(50000);
        while (ts_read(&sx, &sy, &_t) > 0) {}
    }

    while (1)
    {
        show_menu();

        while (1)
        {
            while (ts_read(&sx, &sy, &t) > 0)
            {
                if (!t) continue;
                printf("  touch at (%d,%d)\n", sx, sy);

                if (button_hit(&g_menu[0], sx, sy))
                    { page_send(); show_menu(); break; }
                if (button_hit(&g_menu[1], sx, sy))
                    { page_recv(); show_menu(); break; }
                if (button_hit(&g_menu[2], sx, sy))
                    { page_learn(); show_menu(); break; }
                if (button_hit(&g_menu[3], sx, sy))
                    { printf("IR exit\n"); return; }
            }
        }
    }
}
