#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    if(argc != 2) {
        fprintf(2, "Usage: sleep <seconds>\n");
        exit(1);
    }

    int ticks = atoi(argv[1]);

    // pause(): call the kernel pause function to sleep for a number of ticks
    if(ticks < 0 || pause(ticks) < 0) {
        fprintf(2, "sleep: pause failed\n");
        exit(1);
    }

    exit(0);
}