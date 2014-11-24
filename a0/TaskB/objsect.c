#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <bfd.h>

//got it from http://www.jb.man.ac.uk/~slowe/cpp/itoa.html
//tweaked it a bit
char* itoa(int val, int base) {
  if (val == 0x0) {
   static char zero[] = "0";
   return &zero[0];
  }
  static char buf[32] = {0};
  int i = 30;
  for(; val && i ; --i, val /= base)
    buf[i] = "0123456789abcdef"[val % base];
  return &buf[i+1];
}

void findsections(bfd *obj, asection *sect, void* ptrobject) {
  write(1, "section:", strlen("section:"));
  write(1, sect->name, strlen(sect->name)); // the section name
  write(1, " vma (hex):", strlen(" vma (hex):"));
  write(1, itoa(sect->vma, 16), strlen(itoa(sect->vma, 16)));
  write(1, " raw (hex):", strlen(" raw (hex):"));
  write(1, itoa(sect->rawsize, 16), strlen(itoa(sect->rawsize, 10)));
  write(1, " cooked (hex):", strlen(" cooked (hex):"));
  write(1, itoa(sect->size, 16), strlen(itoa(sect->size, 10)));
  write(1, " and pos (hex):", strlen(" and pos (hex):"));
  write(1, itoa(sect->filepos, 16), strlen(itoa(sect->filepos, 16)));
  write(1, " \n", strlen(" \n"));
}
