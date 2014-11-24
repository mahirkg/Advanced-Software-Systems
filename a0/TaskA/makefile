all: getsections getsyms

getsyms: libobjdata.a getsyms.c
	gcc getsyms.c -L. -lobjdata -lbfd -o getsyms

getsections: libobjdata.a getsections.c
	gcc getsections.c -L. -lobjdata -lbfd -o getsections

libobjdata.a: objsym.o objsect.o
	ar cvqs libobjdata.a objsym.o objsect.o

objsym.o: objsym.c
	gcc -c objsym.c

objsect.o: objsect.c
	gcc -c objsect.c
clean:
	rm -f getsections getsyms objsym.o objsect.o  libobjdata.a
