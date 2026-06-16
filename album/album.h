#ifndef ALBUM_H
#define ALBUM_H

#define ALBUM_MAX 10

void album_init(const char *paths[], int count);
void album_show(int idx);
void album_prev(void);
void album_next(void);
int  album_current(void);
int  album_count(void);

#endif
