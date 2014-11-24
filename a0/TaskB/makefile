all: getsections_dl getsyms_dl

getsyms_dl: libobjdata.so getsyms_dl.c
	gcc getsyms_dl.c -L. -ldl -lbfd -o getsyms_dl 

getsections_dl: libobjdata.so getsections_dl.c
	gcc getsections_dl.c -L. -ldl -lbfd -o getsections_dl

libobjdata.so: objsym.o objsect.o
	gcc -shared -o libobjdata.so objsym.o objsect.o

objsym.o: objsym.c
	gcc -Wall -fPIC -c objsym.c

objsect.o: objsect.c
	gcc -Wall -fPIC -c objsect.c
clean:
	rm -f getsections_dl getsyms_dl  objsym.o objsect.o  libobjdata.so
