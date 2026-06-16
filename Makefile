CC      = arm-linux-gcc
CFLAGS  = -Wall -O2 -I .

SRCS = main.c \
       desktop/desktop.c album/album.c ir_app/ir_app.c button/button.c \
       common/device.c common/tools.c

TARGET = mini_desktop

.PHONY: all static clean

static: $(TARGET)
	@file $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -static -o $@ $(SRCS)

clean:
	rm -f $(TARGET) test/*_static
