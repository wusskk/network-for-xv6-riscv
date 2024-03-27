#include "kernel/types.h"
#include "user/user.h"

int main(void) {
    int fd;
    char *message = "a message from xv6";

    // Use the connect system call to connect to the server
    fd = connect(0xAC1C2BD8, 3000, 38500, 0); // 0xAC1C2BD8 is 172.28.43.216 in hexadecimal
    if (fd < 0) {
        printf("connect failed\n");
        exit(1);
    }

    // Use the write system call to send the message
    if (write(fd, message, strlen(message)) < 0) {
        printf("write failed\n");
        exit(1);
    }
    printf("send: a message from xv6\n");


    char buf[1514] = {0};
    if (read(fd, buf, 1514) < 0) {
        printf("read failed\n");
        exit(1);
    }
    
    printf("receive: %s\n", buf);

    if (close(fd) < 0) {
        printf("close failed\n");
        exit(1);
    }
    exit(0);
}