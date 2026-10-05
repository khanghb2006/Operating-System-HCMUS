#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

void sixfive(int fd) {
    char c;
    int n;

    int has_token = 0;
    int valid = 1;
    int value = 0;

    while((n = read(fd, &c, 1)) > 0) {
        if(c != '\0' && strchr(" -\r\t\n./,", c) != 0) {
            // end of token
            if(has_token && valid && (value % 5 == 0 || value % 6 == 0)) {
                printf("%d\n", value);
            }
            // reset for next token 
            has_token = 0;
            valid = 1;
            value = 0;
        } else {
            has_token = 1;

            if(c >= '0' && c <= '9') {
                int digit = c - '0';

                if(value > (2147483647 - digit) / 10) {
                    valid = 0; // overflow
                } else {
                    value = value * 10 + digit;
                }
            } else {
                valid = 0; // invalid character
            }
        }
    }

    if(n < 0) {
        fprintf(2, "sixfive: read error on fd %d\n", fd);
        exit(1);
    }

    // EOF
    if(has_token && valid && (value % 5 == 0 || value % 6 == 0)) {
        printf("%d\n", value);
    }
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        sixfive(0);
        exit(0);
    }

    int failed = 0;

    for(int i = 1; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);

        if(fd < 0) {
            fprintf(2, "sixfive: cannot open %s\n", argv[i]);
            failed = 1;
            continue;
        }

        sixfive(fd);
        close(fd);
    }

    exit(failed);
}