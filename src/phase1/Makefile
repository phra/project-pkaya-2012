ARCH_PREFIX = mipsel-linux-
CC = $(ARCH_PREFIX)gcc
LD = $(ARCH_PREFIX)ld

CFLAGS = -Wall

all : kernel.core.umps

kernel.core.umps : kernel
	umps2-elf2umps -k $<
	#dopo aver trasformato l'eseguibile lo eliminiamo insieme ai file oggetto
	rm -f *.o kernel

kernel : p1test_pkaya_v1.1.o pcb.o asl.o
	$(LD) -T /usr/local/share/umps2/umpscore.ldscript /usr/local/lib/umps2/crtso.o p1test_pkaya_v1.1.o asl.o pcb.o /usr/local/lib/umps2/libumps.o -o kernel

p1test_pkaya_v1.1.o : p1test_pkaya_v1.1.c const.h uMPStypes.h listx.h types11.h pcb.e asl.e
	$(CC) $(CFLAGS) -c p1test_pkaya_v1.1.c
	
pcb.o : pcb.c const.h listx.h types11.h
	$(CC) $(CFLAGS) -c pcb.c
	
asl.o : asl.c const.h listx.h types11.h
	$(CC) $(CFLAGS) -c asl.c
		
clean :
	rm -f kernel.*.umps

doc:
	w3m ../doc/doc.html || lynx ../doc/doc.html
