CCFLAGS=--std=gnu++11 -pthread -pedantic -Wall -Werror -ggdb3

start: start.o 
	g++ -o start $(CCFLAGS) start.o

%.o: %.cpp proxyserver.h cache.h response.h request.h
	g++ -c $(CCFLAGS) $<

clean:
	rm -f *.o  *~ start
