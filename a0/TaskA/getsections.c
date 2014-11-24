#include <stdlib.h>
#include <string.h>
#include <bfd.h>
#include <unistd.h>

extern void findsections(bfd *obj, asection *sect, void* ptrobject);

int main(int argc, char *argv[]) {
  bfd *obj;
  void (* func) (bfd *obj);
  
  bfd_init();
  
  obj = bfd_openr(argv[1], "elf64-x86-64");
  if (!obj) { 
    bfd_perror("open failure\n");
    exit(-1);
  }
  
  if (!bfd_check_format(obj, bfd_object)) {
    write(1, "not an object file\n", strlen("not an object file\n"));
    exit(-1);
  }
  
  bfd_map_over_sections(obj, findsections, NULL);
  exit(0);
}
