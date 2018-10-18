#
# To build the thing 
# Benigno Gobbo 
# 20170727
#

CC     = g++
CFLAGS = -O -std=c++11

all: test loop ltof recon reset

test: test.cc libsnfsen.a
	$(CC) $(CFLAGS) -o test test.cc -L. -lsnfsen

loop: loop.cc libsnfsen.a
	$(CC) $(CFLAGS) -o loop loop.cc -L. -lsnfsen

recon: recon.cc libsnfsen.a
	$(CC) $(CFLAGS) -o recon recon.cc -L. -lsnfsen

reset: reset.cc libsnfsen.a
	$(CC) $(CFLAGS) -o reset reset.cc -L. -lsnfsen

ltof: ltof.cc libsnfsen.a
	$(CC) $(CFLAGS) -o ltof ltof.cc -L. -lsnfsen

libsnfsen.a: vaisa.o bronko.o adam.o init.o
	rm -f libsnfsen.a
	ar rv libsnfsen.a vaisa.o bronko.o adam.o init.o

adam.o: devdata.h adam.cc adam.h
	$(CC) $(CFLAGS) -c adam.cc

vaisa.o: devdata.h vaisa.cc vaisa.h
	$(CC) $(CFLAGS) -c vaisa.cc

bronko.o: devdata.h bronko.cc bronko.h
	$(CC) $(CFLAGS) -c bronko.cc

init.o: devdata.h init.cc init.h json.hpp
	$(CC) $(CFLAGS) -c init.cc

clean: 
	rm -f *.o *.a *~ libsnfsen.a test loop ltof recon reset
