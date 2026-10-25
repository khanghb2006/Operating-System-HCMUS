#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

#define NPAGES 64
#define MSG "Here it is: "

int 
main(int argc, char *argv[])
{
  int len = NPAGES * PGSIZE;
  int msglen = strlen(MSG);

  char *buffer = sbrk(len);
  if (buffer == (char*) -1) 
    exit(1);

  for (int i = 0; i + msglen < len; i++) {
    if (memcmp(buffer + i , MSG , msglen) != 0)
      continue;

    char *secret = buffer + i + msglen;
    int n = 0;
    while(i + msglen + n < len - 1 && 
      ((secret[n] >= '0' && secret[n] <= '9') ||
      (secret[n] >= 'a' && secret[n] <= 'z') ||
      (secret[n] >= 'A' && secret[n] <= 'Z'))) 
        n++;

    secret[n] = 0;
    printf("%s\n", secret);
  }

  exit(1);
}
