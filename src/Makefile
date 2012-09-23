ARCH_PREFIX = mipsel-linux-

CC = $(ARCH_PREFIX)gcc
CFLAGS = -O2 -fno-builtin

LD = $(ARCH_PREFIX)ld
LDADD1 = -T /usr/local/share/umps2/elf32ltsmip.h.umpscore.x /usr/local/lib/umps2/crtso.o p1test_pkaya_v1.1.o asl.o pcb.o utils.o /usr/local/lib/umps2/libumps.o
LDADD2 = -T /usr/local/share/umps2/elf32ltsmip.h.umpscore.x /usr/local/lib/umps2/crtso.o /usr/local/lib/umps2/libumps.o init.o exception.o syscall.o scheduler.o interrupt.o pcb.o asl.o utils.o p2test2012_v1.0.o
LDADDLAB = -T /usr/share/umps2/elf32ltsmip.h.umpscore.x /usr/lib/umps2/crtso.o /usr/lib/umps2/libumps.o init.o exception.o syscall.o scheduler.o interrupt.o pcb.o asl.o utils.o p2test2012_v1.0.o
LDADDSCALABILITY = -T /usr/local/share/umps2/elf32ltsmip.h.umpscore.x /usr/local/lib/umps2/crtso.o /usr/local/lib/umps2/libumps.o init.o exception.o syscall.o scheduler.o interrupt.o pcb.o asl.o utils.o p2test2012_scalability_v1.0.o
LDADDSCALABILITYLAB = -T /usr/share/umps2/elf32ltsmip.h.umpscore.x /usr/lib/umps2/crtso.o /usr/lib/umps2/libumps.o init.o exception.o syscall.o scheduler.o interrupt.o pcb.o asl.o utils.o p2test2012_scalability_v1.0.o

INCLUDE = lib/const.h lib/uMPStypes.h lib/listx.h lib/types11.h lib/pcb.h lib/asl.h lib/utils.h lib/const.h lib/base.h lib/libumps.h
INCLUDE += scheduler.h init.h syscall.h interrupt.h exception.h

#phase2

all : kernel.core.umps

kernel.core.umps : kernelphase2
	umps2-elf2umps -k $<
	rm -f *.o kernelphase2

scalability : init.o exception.o syscall.o scheduler.o interrupt.o pcb.o asl.o utils.o p2test2012_scalability_v1.0.o
	$(LD) $(LDADDSCALABILITY) -o kernelphase2
	umps2-elf2umps -k kernelphase2
	rm -f *.o kernelphase2

kernelphase2 : p2test2012_v1.0.o init.o exception.o syscall.o scheduler.o interrupt.o pcb.o asl.o utils.o
	$(LD) $(LDADD2) -o kernelphase2

scalabilitylab : init.o exception.o syscall.o scheduler.o interrupt.o pcb.o asl.o utils.o p2test2012_scalability_v1.0.o
	$(LD) $(LDADDSCALABILITYLAB) -o kernelphase2
	umps2-elf2umps -k kernelphase2
	rm -f *.o kernelphase2

kernelphase2lab : p2test2012_v1.0.o init.o exception.o syscall.o scheduler.o interrupt.o pcb.o asl.o utils.o
	$(LD) $(LDADD2LAB) -o kernelphase2

p2test2012_v1.0.o : p2test2012_v1.0.c $(INCLUDE)
	$(CC) -O1 -c p2test2012_v1.0.c

p2test2012_scalability_v1.0.o : p2test2012_scalability_v1.0.c $(INCLUDE)
	$(CC) -O2 -c p2test2012_scalability_v1.0.c

init.o : init.c $(INCLUDE)
	$(CC) $(CFLAGS) -c init.c

exception.o : exception.c $(INCLUDE)
	$(CC) $(CFLAGS) -c exception.c

syscall.o : syscall.c $(INCLUDE)
	$(CC) $(CFLAGS) -c syscall.c

interrupt.o : interrupt.c $(INCLUDE)
	$(CC) $(CFLAGS) -c interrupt.c

scheduler.o : scheduler.c $(INCLUDE)
	$(CC) $(CFLAGS) -c scheduler.c

#phase1

kernelphase1 : p1test_pkaya_v1.1.o pcb.o asl.o utils.o
	$(LD) $(LDADD1) -o kernelphase1
	rm -f *.o

p1test_pkaya_v1.1.o : phase1/p1test_pkaya_v1.1.c lib/const.h lib/uMPStypes.h lib/listx.h lib/types11.h phase1/pcb.h phase1/asl.h lib/utils.h
	$(CC) $(CFLAGS) -c phase1/p1test_pkaya_v1.1.c
	
pcb.o : lib/pcb.c lib/const.h lib/listx.h lib/types11.h lib/utils.h
	$(CC) $(CFLAGS) -c lib/pcb.c
	
asl.o : lib/asl.c lib/const.h lib/listx.h lib/types11.h lib/utils.h
	$(CC) $(CFLAGS) -c lib/asl.c

#misc

utils.o :
	$(CC) $(CFLAGS) -c lib/utils.c
		
clean :
	rm -f kernel*.*.umps *.o kernelphase* phase1/*.o
