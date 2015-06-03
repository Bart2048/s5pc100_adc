#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

// open
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// perror
#include <errno.h>
#include <sys/ioctl.h>
#include "adc.h"

// select
#include <sys/select.h>
#include <sys/time.h>

int main(int argc, const char *argv[])
{
    if (argc < 2){
        fprintf(stderr, "Usage : %s /dev/...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd; 
    int vol;
    int ret;
    fd_set readfds;

    fd = open(argv[1], O_RDWR);
    if (-1 == fd){
        perror("Fail to open");
        exit(EXIT_FAILURE);
    }

    ret = ioctl(fd, IOCTL_SET_RESOLUTION, 12);
    if (-1 == ret){
        perror("Fail to open");
        exit(EXIT_FAILURE);
    }

    while (1){
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        ret = select(fd + 1, &readfds, NULL, NULL, NULL);
        if (ret < 0)
        {
            perror("Not ready to read");
            break;
        }

        ret = read(fd, &vol, sizeof(vol));
        if (ret < 0){
            perror("Fail to read");
        }
        printf("vol = %.2f\n", vol * 3.3 / 4096);

        sleep(1);
    }

    close(fd);

    return 0;
}
