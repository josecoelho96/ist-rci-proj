CC=gcc
CFLAGS=-std=gnu11 -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -g -O0
LIBFILESBASE=lib/misc.c lib/udp.c lib/tcp.c
LIBFILESSERVICE= $(LIBFILESBASE) lib/com_service.c
LIBFILESREQSERVER= $(LIBFILESBASE) lib/com_reqserver.c
CFILESSERVICE=src/service.c
CFILESREQSERVER=src/reqserver.c

default: all

all: service reqserver

service: src/service.c
	$(CC) -o bin/service $(CFILESSERVICE) $(LIBFILESSERVICE) $(CFLAGS)

reqserver: src/reqserver.c
	$(CC) -o bin/reqserver $(CFILESREQSERVER) $(LIBFILESREQSERVER) $(CFLAGS)

clean:
	rm -f *.o *~ bin/service bin/reqserver sors_start.txt sors_ds.txt

sor-clean:
	rm sors_start.txt sors_ds.txt

run-s1:
	bin/service -n 1 -j 127.0.0.1 -u 57001 -t 58001 -i 127.0.0.1 -p 59000

run-s2:
	bin/service -n 2 -j 127.0.0.1 -u 57002 -t 58002 -i 127.0.0.1 -p 59000

run-s3:
	bin/service -n 3 -j 127.0.0.1 -u 57003 -t 58003 -i 127.0.0.1 -p 59000

run-s4:
	bin/service -n 4 -j 127.0.0.1 -u 57004 -t 58004 -i 127.0.0.1 -p 59000

run-s5:
	bin/service -n 5 -j 127.0.0.1 -u 57005 -t 58005 -i 127.0.0.1 -p 59000

run-s1-default:
	bin/service -n 1 -j 127.0.0.1 -u 57001 -t 58001

run-s2-default:
	bin/service -n 2 -j 127.0.0.1 -u 57002 -t 58002

run-service-default:
	bin/service -n 1 -j 127.0.0.1 -u 57000 -t 58000

run-sor:
	bin/sor -debug

run-reqserver:
	bin/reqserver -i 127.0.0.1 -p 59000

run-reqserver-default:
	bin/reqserver

run-valgrind-service:
	valgrind --leak-check=full bin/service -n 1 -j 127.0.0.1 -u 57000 -t 58000 -i 127.0.0.1 -p 59000

run-valgrind-reqserver:
	valgrind --leak-check=full bin/reqserver -i 127.0.0.1 -p 59000