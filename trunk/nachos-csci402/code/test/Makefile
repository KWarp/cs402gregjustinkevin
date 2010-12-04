# use normal make for this Makefile
#
# Makefile for building user programs to run on top of Nachos
#
# Several things to be aware of:
#
#    Nachos assumes that the location of the program startup routine (the
# 	location the kernel jumps to when the program initially starts up)
#       is at location 0.  This means: start.o must be the first .o passed 
# 	to ld, in order for the routine "Start" to be loaded at location 0
#

# If you are running on SunOS 5.x machines, uncomment the next line
GCCDIR = /auto/home-scf-22/csci402/testgnu/decstation-ultrix/bin/
LDFLAGS = -T newscript -N
ASFLAGS = -mips1
CPPFLAGS = $(INCDIR)

# if you aren't cross-compiling:
# GCCDIR =
# LDFLAGS = -N -T 0
# ASFLAGS =
# CPPFLAGS = -P $(INCDIR)


CC = $(GCCDIR)gcc
AS = $(GCCDIR)as
LD = $(GCCDIR)ld
CPP = $(GCCDIR)cpp

INCDIR =-I../userprog -I../threads
CFLAGS = -G 0 -c $(INCDIR)

all: halt matmult sort testfiles testfork testexec exectestprogram1 exectestprogram2 testnetsyscalls \
      syscalltest testsynchmanager testtwomatmult testtwosort testmultisim \
      teststartuserprogram teststartuserprogramsub testp4syscalls testp4syscallssub \
      customer ordertaker manager cook waiter launchcustomer launchordertaker launchmanager \
	  networksim

start.o: start.s ../userprog/syscall.h
	$(CPP) $(CPPFLAGS) start.s > strt.s
	$(AS) $(ASFLAGS) -o start.o strt.s
	rm strt.s

halt.o: halt.c
	$(CC) $(CFLAGS) -c halt.c
halt: halt.o start.o
	$(LD) $(LDFLAGS) start.o halt.o -o halt.coff
	../bin/coff2noff halt.coff halt

sort.o: sort.c
	$(CC) $(CFLAGS) -c sort.c
sort: sort.o start.o
	$(LD) $(LDFLAGS) start.o sort.o -o sort.coff
	../bin/coff2noff sort.coff sort

matmult.o: matmult.c
	$(CC) $(CFLAGS) -c matmult.c
matmult: matmult.o start.o
	$(LD) $(LDFLAGS) start.o matmult.o -o matmult.coff
	../bin/coff2noff matmult.coff matmult

testfiles.o: testfiles.c
	$(CC) $(CFLAGS) -c testfiles.c
testfiles: testfiles.o start.o
	$(LD) $(LDFLAGS) start.o testfiles.o -o testfiles.coff
	../bin/coff2noff testfiles.coff testfiles

syscalltest.o: syscalltest.c
	$(CC) $(CFLAGS) -c syscalltest.c
syscalltest: syscalltest.o start.o
	$(LD) $(LDFLAGS) start.o syscalltest.o -o syscalltest.coff
	../bin/coff2noff syscalltest.coff syscalltest

testnetsyscalls.o: testnetsyscalls.c
	$(CC) $(CFLAGS) -c testnetsyscalls.c
testnetsyscalls: testnetsyscalls.o start.o
	$(LD) $(LDFLAGS) start.o testnetsyscalls.o -o testnetsyscalls.coff
	../bin/coff2noff testnetsyscalls.coff testnetsyscalls
  
testsynchmanager.o: testsynchmanager.c
	$(CC) $(CFLAGS) -c testsynchmanager.c
testsynchmanager: testsynchmanager.o start.o
	$(LD) $(LDFLAGS) start.o testsynchmanager.o -o testsynchmanager.coff
	../bin/coff2noff testsynchmanager.coff testsynchmanager	
  
testtwomatmult.o: testtwomatmult.c
	$(CC) $(CFLAGS) -c testtwomatmult.c
testtwomatmult: testtwomatmult.o start.o
	$(LD) $(LDFLAGS) start.o testtwomatmult.o -o testtwomatmult.coff
	../bin/coff2noff testtwomatmult.coff testtwomatmult	  
  
testtwosort.o: testtwosort.c
	$(CC) $(CFLAGS) -c testtwosort.c
testtwosort: testtwosort.o start.o
	$(LD) $(LDFLAGS) start.o testtwosort.o -o testtwosort.coff
	../bin/coff2noff testtwosort.coff testtwosort	    
	
testfork.o: testfork.c
	$(CC) $(CFLAGS) -c testfork.c
testfork: testfork.o start.o
	$(LD) $(LDFLAGS) start.o testfork.o -o testfork.coff
	../bin/coff2noff testfork.coff testfork
	
testmultisim.o: testmultisim.c
	$(CC) $(CFLAGS) -c testmultisim.c
testmultisim: testmultisim.o start.o
	$(LD) $(LDFLAGS) start.o testmultisim.o -o testmultisim.coff
	../bin/coff2noff testmultisim.coff testmultisim	

exectestprogram1.o: exectestprogram1.c
	$(CC) $(CFLAGS) -c exectestprogram1.c
exectestprogram1: exectestprogram1.o start.o
	$(LD) $(LDFLAGS) start.o exectestprogram1.o -o exectestprogram1.coff
	../bin/coff2noff exectestprogram1.coff exectestprogram1  
  
exectestprogram2.o: exectestprogram2.c
	$(CC) $(CFLAGS) -c exectestprogram2.c
exectestprogram2: exectestprogram2.o start.o
	$(LD) $(LDFLAGS) start.o exectestprogram2.o -o exectestprogram2.coff
	../bin/coff2noff exectestprogram2.coff exectestprogram2 
  
testexec.o: testexec.c
	$(CC) $(CFLAGS) -c testexec.c
testexec: testexec.o exectestprogram1.o exectestprogram2.o start.o
	$(LD) $(LDFLAGS) start.o testexec.o -o testexec.coff
	../bin/coff2noff testexec.coff testexec
	
#simulation.o: simulation.c simulation.h
#	$(CC) $(CFLAGS) -c simulation.c
#simulation: simulation.o start.o
#	$(LD) $(LDFLAGS) start.o simulation.o -o simulation.coff
#	../bin/coff2noff simulation.coff simulation
  
teststartuserprogram.o: teststartuserprogram.c
	$(CC) $(CFLAGS) -c teststartuserprogram.c
teststartuserprogram: teststartuserprogram.o start.o
	$(LD) $(LDFLAGS) start.o teststartuserprogram.o -o teststartuserprogram.coff
	../bin/coff2noff teststartuserprogram.coff teststartuserprogram  

teststartuserprogramsub.o: teststartuserprogramsub.c
	$(CC) $(CFLAGS) -c teststartuserprogramsub.c
teststartuserprogramsub: teststartuserprogramsub.o start.o
	$(LD) $(LDFLAGS) start.o teststartuserprogramsub.o -o teststartuserprogramsub.coff
	../bin/coff2noff teststartuserprogramsub.coff teststartuserprogramsub    

testp4syscalls.o: testp4syscalls.c
	$(CC) $(CFLAGS) -c testp4syscalls.c
testp4syscalls: testp4syscalls.o start.o
	$(LD) $(LDFLAGS) start.o testp4syscalls.o -o testp4syscalls.coff
	../bin/coff2noff testp4syscalls.coff testp4syscalls  
  
testp4syscallssub.o: testp4syscallssub.c
	$(CC) $(CFLAGS) -c testp4syscallssub.c
testp4syscallssub: testp4syscallssub.o start.o
	$(LD) $(LDFLAGS) start.o testp4syscallssub.o -o testp4syscallssub.coff
	../bin/coff2noff testp4syscallssub.coff testp4syscallssub    
  
customer.o: customer.c simulation.c 
	$(CC) $(CFLAGS) -c customer.c simulation.c
customer: customer.o start.o 
	$(LD) $(LDFLAGS) start.o  customer.o -o customer.coff
	../bin/coff2noff customer.coff customer  
  
ordertaker.o: ordertaker.c simulation.c 
	$(CC) $(CFLAGS) -c ordertaker.c
ordertaker: ordertaker.o start.o 
	$(LD) $(LDFLAGS) start.o  ordertaker.o -o ordertaker.coff
	../bin/coff2noff ordertaker.coff ordertaker  
  
manager.o: manager.c simulation.c 
	$(CC) $(CFLAGS) -c manager.c
manager: manager.o start.o 
	$(LD) $(LDFLAGS) start.o  manager.o -o manager.coff
	../bin/coff2noff manager.coff manager  
  
cook.o: cook.c simulation.c 
	$(CC) $(CFLAGS) -c cook.c
cook: cook.o start.o 
	$(LD) $(LDFLAGS) start.o  cook.o -o cook.coff
	../bin/coff2noff cook.coff cook  
	
waiter.o: waiter.c simulation.c 
	$(CC) $(CFLAGS) -c waiter.c
waiter: waiter.o start.o 
	$(LD) $(LDFLAGS) start.o  waiter.o -o waiter.coff
	../bin/coff2noff waiter.coff waiter  	

networksim.o: networksim.c simulation.c 
	$(CC) $(CFLAGS) -c networksim.c
networksim: networksim.o start.o 
	$(LD) $(LDFLAGS) start.o  networksim.o -o networksim.coff
	../bin/coff2noff networksim.coff networksim  	
  
launchcustomer.o: launchcustomer.c
	$(CC) $(CFLAGS) -c launchcustomer.c
launchcustomer: launchcustomer.o start.o
	$(LD) $(LDFLAGS) start.o launchcustomer.o -o launchcustomer.coff
	../bin/coff2noff launchcustomer.coff launchcustomer  
  
launchordertaker.o: launchordertaker.c
	$(CC) $(CFLAGS) -c launchordertaker.c
launchordertaker: launchordertaker.o start.o
	$(LD) $(LDFLAGS) start.o launchordertaker.o -o launchordertaker.coff
	../bin/coff2noff launchordertaker.coff launchordertaker  
  
launchmanager.o: launchmanager.c
	$(CC) $(CFLAGS) -c launchmanager.c
launchmanager: launchmanager.o start.o
	$(LD) $(LDFLAGS) start.o launchmanager.o -o launchmanager.coff
	../bin/coff2noff launchmanager.coff launchmanager  
	
	
  
clean:
	rm -f *.o *.coff
