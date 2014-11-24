#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <bfd.h>

// got it from http://www.jb.man.ac.uk/~slowe/cpp/itoa.html
// tweaked it a bit
char* itoa2(int val, int base) {
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

void getsyms(bfd *obj) {
  long storage_needed;
  asymbol **symbol_table;
  long number_of_symbols;
  int i;
  
  storage_needed = bfd_get_symtab_upper_bound(obj); // get the size needed to store all the pointers that point to the owner of each symbol used by obj.
 
  if (storage_needed <= 0) {
    write(1, "Size needed to store pointers <= 0\n", strlen("Size needed to store pointers <= 0\n")); // write the message to stdout (1)
    exit(-1);
  }
  
  symbol_table = (asymbol **) malloc(storage_needed); // asymbols are a pointer to a pointer to a symbol structure

  number_of_symbols = bfd_canonicalize_symtab(obj, symbol_table); // read all the symbols from obj and fill in the vector location symbol_table with pointers to the symbols and a trailing NULL. It returns the actual number of symbol pointers, not including the trailing NUlL.

  if (number_of_symbols < 0) {
    write(1, "Number of symbols < 0\n", strlen("Number of symbols < 0\n"));
    exit(-1);
  }
  
  for (i = 0; i < number_of_symbols; i++) {
    // print out the ame and value of each symbol
    write(1, "symbol name: ", strlen("symbol name: "));
    write(1, symbol_table[i]->name, strlen(symbol_table[i]->name));
    write(1, " and vma(hex): ", strlen(" and vma(hex): "));
    write(1, itoa2((symbol_table[i]->section->vma + symbol_table[i]->value), 16), strlen(itoa2((symbol_table[i]->section->vma + symbol_table[i]->value), 16)));
    //write(1, itoa(symbol_table[i]->value, 16), strlen(itoa(symbol_table[i]->value, 16)));
    write(1, " \n", strlen(" \n"));
  }
}
