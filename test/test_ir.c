#include "../common/device.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <errno.h>

int main(void)
{
    device_init();

    int fd = open("/dev/ttySAC1", O_RDWR | O_NOCTTY | O_NONBLOCK);
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

    printf("=== IR Raw Test ===\n");
    printf("Press IR remote. Ctrl+C to exit.\n");

    unsigned char buf[3];
    int idx = 0;

    while (1)
    {
        int n = read(fd, buf + idx, 3 - idx);
        if (n > 0)
        {
            printf("read %d bytes: ", n);
            int i;
            for (i = 0; i < n; i++) 
                printf("%02X ", buf[idx + i]);
            printf("\n");

            idx += n;
            if (idx == 3)
            {
                printf("  → GOT 3: %02X %02X %02X\n", buf[0], buf[1], buf[2]);
                beep_on(); usleep(100000); beep_off();
                idx = 0;
            }
        }
    }

    close(fd);
    device_deinit();
    return 0;
}
