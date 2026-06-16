CC      = arm-linux-gcc
CFLAGS  = -Wall -O2 -I ..

COMMON  = ../common/device.c ../common/tools.c

.PHONY: all clean

all: test_swipe_static test_image_static test_album_static

test_swipe_static: test_swipe.c ../button/button.c $(COMMON)
	$(CC) $(CFLAGS) -static -o $@ test_swipe.c ../button/button.c $(COMMON)

test_image_static: test_image.c ../album/album.c $(COMMON)
	$(CC) $(CFLAGS) -static -o $@ test_image.c ../album/album.c $(COMMON)

test_album_static: test_album.c ../desktop/desktop.c ../album/album.c ../button/button.c $(COMMON)
	$(CC) $(CFLAGS) -static -o $@ test_album.c ../desktop/desktop.c ../album/album.c ../button/button.c $(COMMON)

clean:
	rm -f *_static
