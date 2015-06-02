#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

// open
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// perror
#include <errno.h>
#include <sys/ioctl.h>
#include "adc.h"

int main(int argc, const char *argv[])
{
    if (argc < 2){
        fprintf(stderr, "Usage : %s /dev/...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd; 
    /*double val;*/
    char val[32];
    int ret;
    fd = open(argv[1], O_RDWR);
    if (-1 == fd){
        perror("Fail to open");
        exit(EXIT_FAILURE);
    }

    ret = ioctl(fd, IOCTL_SET_RESOLUTION);
    if (-1 == ret){
        perror("Fail to open");
        exit(EXIT_FAILURE);
    }

    while (1){
    ret = read(fd, val, sizeof(val));
    if (ret < 0){
        perror("Fail to read");
    }
    printf("val = %s\n", val);

        sleep(1);
    }

    close(fd);


    return 0;
}
