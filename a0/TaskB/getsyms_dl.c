#include <stdlib.h>
#include <string.h>
#include <bfd.h>
#include <unistd.h>
#include <dlfcn.h>
#define rdtsc(x)      __asm__ __volatile__("rdtsc \n\t" : "=A" (*(x))) // do I need a semicolon here?

// tweaked version of itoa that can be found in
// either objsym.c or objsect.c
char* lltoa(long long int val, int base) {
  if (val == 0x0) {
   static char zero[] = "0";
   return &zero[0];
  }
  static char buf[64] = {0};
  int i = 60;  
  for(; val && i ; --i, val /= base)
    buf[i] = "0123456789abcdef"[val % base];
  return &buf[i+1];
}

int main(int argc, char *argv[]) {
  bfd *obj;
  void *handle;
  unsigned long long start, finish;

  bfd_init();

  obj = bfd_openr(argv[1], "elf64-x86-64");
  if (!obj) {
    bfd_perror("open failure\n");
    exit(-1);
  }

  rdtsc(&start);

  if (!strcmp(argv[2], "RTLD_LAZY"))
    handle = dlopen("./libobjdata.so", RTLD_LAZY);
  else
    handle = dlopen("./libobjdata.so", RTLD_NOW);

  rdtsc(&finish);


  if (!bfd_check_format(obj, bfd_object)) {
    write(1, "not an object file\n", strlen("not an object file\n"));
    exit(-1);
  }

  void (* func) (bfd *obj);
  *(void **) (&func) = dlsym(handle, "getsyms");

  func(obj);

  dlclose(handle);

  exit(0);
}
