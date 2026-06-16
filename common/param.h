#ifndef PARAM_H
#define PARAM_H

/* --- Screen --- */
#define SCREEN_W  800
#define SCREEN_H  480

/* --- Touch calibration (GEC6818 gslX680) --- */
#define TS_RAW_W  1024
#define TS_RAW_H  600
#define TS_Y_OFF  95

/* --- Buzzer ioctl magic --- */
#define BEEP_IOCTL_CMD  2

/* --- Swipe threshold (pixels) --- */
#define SWIPE_MIN_DIST  30

/* --- Color helpers (GEC6818 32bpp: byte2=R, byte1=G, byte0=B) --- */
#define RGB(r, g, b)  (((unsigned int)(r) << 16) | ((unsigned int)(g) << 8) | (unsigned int)(b))

#endif
