#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void memdump(char *fmt, char *data, int len);

int 
main(int argc, char *argv[]) {
  if (argc == 1) {
    printf("Example 1:\n");
    int a[2] = {61810, 2026};
    memdump("ii", (char *)a, sizeof(a));

    printf("Example 2:\n");
    memdump("S", "a string", sizeof("a string"));

    printf("Example 3:\n");
    char *s = "another";
    memdump("s", (char *)&s, sizeof(s));

    struct sss {
      char *ptr;
      int num1;
      short num2;
      char byte;
      char bytes[8];
    } example;

    example.ptr = "hello";
    example.num1 = 1819438967;
    example.num2 = 100;
    example.byte = 'z';
    strcpy(example.bytes, "xyzzy");

    printf("Example 4:\n");
    memdump("pihcS", (char *)&example, sizeof(example));

    printf("Example 5:\n");
    memdump("sccccc", (char *)&example, sizeof(example));
  } else if (argc == 2) {
    // format in argv[1], up to 512 bytes of data from standard input.
    char data[512];
    int n = 0;
    memset(data, '\0', sizeof(data));
    while (n < sizeof(data)) {
      int nn = read(0, data + n, sizeof(data) - n);
      if (nn <= 0)
        break;
      n += nn;
    }
    memdump(argv[1], data, n);
  } else {
    printf("Usage: memdump [format]\n");
    exit(1);
  }
  exit(0);
}

// Decoded value : each format letter uses one member
union value {
  int i;
  short h;
  uint64 p;
  char c;
  char *s;
};

// One entry per format letter : how many bytes it consumes and how to print
struct spec {
  char fmt;
  int size;
  void (*print)(union value v);
};

static void print_i(union value v) {
  printf("%d\n", v.i);
}

static void print_h(union value v) {
  printf("%d\n", v.h);
}

static void print_p(union value v) {
  printf("%lx\n", v.p);
}

static void print_c(union value v) {
  printf("%c\n", v.c);
}

static void print_s(union value v) {
  printf("%s\n", v.s);
}

static struct spec specs[] = {
    {'i', sizeof(int), print_i},
    {'h', sizeof(short), print_h},
    {'p', sizeof(uint64), print_p},
    {'c', sizeof(char), print_c},
    {'s', sizeof(char *), print_s},
};

static struct spec* 
lookup(char key) {
  for (int k = 0; k < sizeof(specs) / sizeof(specs[0]); ++k) 
    if (specs[k].fmt == key)
      return &specs[k];
    
  return 0;
}

// Copy the next n bytes from *data into v
static int 
getBytes(char **data , char *end , void *v , int n , int key) {
  if (*data + n > end) {
    fprintf(2 , "memdump: not enough data for '%c'\n" , key);
    return 0;
  }
  memmove(v , *data , n); // Copy n bytes from *data to v
  *data += n; // Advance the pointer
  return 1;
}

// "S" print everything up tho the first null bytes
static char*
print_rest(char *data , char *end) {
  for (; data < end && *data; ++data) 
    printf("%c" , *data);
  printf("\n");
  return end;
}

void 
memdump(char *fmt, char *data, int len) {
  char *end = data + len; // first byte pass the valid data

  for (; *fmt; ++fmt) {
    if (*fmt == 'S') {
      data = print_rest(data, end);
      continue;
    }

    struct spec * sp = lookup(*fmt);
    if (!sp) {
      fprintf(2 , "memdump : unknown format '%c'\n" , *fmt);
      return;
    }
    union value v;
    if (!getBytes(&data, end, &v, sp->size, *fmt))
      return;
    sp->print(v);
  }
}