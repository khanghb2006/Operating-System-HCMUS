#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc , char *argv[]) {
    // "sleep 100" gives 2 argcs
    if (argc < 2) {
        fprintf(2 , "usage : sleep <sticks>\n");
        exit(1);
    }
    int ticks = atoi(argv[1]);
    pause(ticks);
    exit(0);
}