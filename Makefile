CC=gcc
OBJS=areastat.o
CFLAGS=-ggdb
CFLAGS+=-I/home/trooper/src/husky/usr/include
LFLAGS=-ggdb
LFLAGS+=-L/home/trooper/src/husky/usr/lib
LIBS=-lhusky -lfidoconfig -lsmapi

all: $(OBJS)
		$(CC) $(OBJS) $(LFLAGS) $(LIBS) -o areastat
