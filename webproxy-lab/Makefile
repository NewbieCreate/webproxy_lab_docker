# # Makefile for Proxy Lab 
# #
# # You may modify this file any way you like (except for the handin
# # rule). You instructor will type "make" on your specific Makefile to
# # build your proxy from sources.

# CC = gcc
# CFLAGS = -g -Wall
# LDFLAGS = -lpthread

# all: proxy

# csapp.o: csapp.c csapp.h
# 	$(CC) $(CFLAGS) -c csapp.c

# proxy.o: proxy.c csapp.h
# 	$(CC) $(CFLAGS) -c proxy.c

# proxy: proxy.o csapp.o
# 	$(CC) $(CFLAGS) proxy.o csapp.o -o proxy $(LDFLAGS)

# # Creates a tarball in ../proxylab-handin.tar that you can then
# # hand in. DO NOT MODIFY THIS!
# handin:
# 	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

# clean:
# 	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: proxy echoserver echoclient

# 기존 대상
csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c

proxy: proxy.o csapp.o
	$(CC) $(CFLAGS) proxy.o csapp.o -o proxy $(LDFLAGS)

# echo 함수가 있는 echo.o
echo.o: echo.c csapp.h
	$(CC) $(CFLAGS) -c echo.c

# echoserver - echo.o 포함!
echoserver.o: echoserver.c csapp.h
	$(CC) $(CFLAGS) -c echoserver.c

echoserver: echoserver.o echo.o csapp.o
	$(CC) $(CFLAGS) -o echoserver echoserver.o echo.o csapp.o $(LDFLAGS)

# echoclient
echoclient.o: echoclient.c csapp.h
	$(CC) $(CFLAGS) -c echoclient.c

echoclient: echoclient.o csapp.o
	$(CC) $(CFLAGS) -o echoclient echoclient.o csapp.o $(LDFLAGS)


clean:
	rm -f *~ *.o proxy echoserver echoclient core *.tar *.zip *.gzip *.bzip *.gz