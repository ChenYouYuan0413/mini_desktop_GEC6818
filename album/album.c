#include "album.h"
#include "../common/device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned int *g_pixels[ALBUM_MAX];
static int g_w[ALBUM_MAX], g_h[ALBUM_MAX];
static int g_idx = 0;
static int g_count = 0;

void album_init(const char *paths[], int count)
{
    int i;
    for (i = 0; i < count && i < ALBUM_MAX; i++)
    {
        int w, h;
        unsigned int *p = bmp_load(paths[i], &w, &h);
        if (p)
        {
            g_pixels[i] = p;
            g_w[i] = w;
            g_h[i] = h;
            printf("Album[%d]: %s (%dx%d)\n", i, paths[i], w, h);
        }
        else
        {
            g_pixels[i] = NULL;
            printf("Album[%d]: FAILED to load %s\n", i, paths[i]);
        }
    }
    g_count = count;
    g_idx = 0;
}

void album_show(int idx)
{
    if (idx < 0 || idx >= g_count) return;
    if (g_pixels[idx] == NULL) return;

    g_idx = idx;
    bmp_display(g_pixels[idx], g_w[idx], g_h[idx]);
}

void album_prev(void)
{
    if (g_count == 0) return;
    g_idx = (g_idx > 0) ? g_idx - 1 : g_count - 1;
    album_show(g_idx);
}

void album_next(void)
{
    if (g_count == 0) return;
    g_idx = (g_idx + 1 < g_count) ? g_idx + 1 : 0;
    album_show(g_idx);
}

int album_current(void) { return g_idx; }
int album_count(void)   { return g_count; }
