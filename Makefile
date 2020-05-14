CC 			= gcc
CFLAGS 		+= -Wall -pedantic
THREADS		= -pthread

LIBDIR		= ./lib
INCDIR		= ./include
SRCDIR		= ./src
BINDIR		= ./bin
BUILDDIR	= ./build

INCLUDES	= -I $(INCDIR)
LDFLAGS		= -L $(LIBDIR)
LIBS 		= -ltsqueue -lpthread

TARGET = $(BINDIR)/supermarket

.PHONY: all clean test2

all: $(TARGET)

$(TARGET) : $(BUILDDIR)/supermarket.o $(BUILDDIR)/client.o $(BUILDDIR)/cashier.o $(BUILDDIR)/director.o $(LIBDIR)/libtsqueue.so
	$(CC) $(CFLAGS) $(THREADS) $(INCLUDES) $(LDFLAGS) $(LIBS) -g $^ -o $@

$(BUILDDIR)/supermarket.o : $(SRCDIR)/supermarket.c
	$(CC) $(CFLAGS) $(INCLUDES) -g -c $^ -o $@  

$(BUILDDIR)/client.o : $(SRCDIR)/client.c $(INCDIR)/client.h
	$(CC) $(CFLAGS) $(INCLUDES) -g -c $< -o $@

$(BUILDDIR)/cashier.o : $(SRCDIR)/cashier.c $(INCDIR)/cashier.h
	$(CC) $(CFLAGS) $(INCLUDES) -g -c $< -o $@

$(BUILDDIR)/director.o : $(SRCDIR)/director.c $(INCDIR)/director.h
	$(CC) $(CFLAGS) $(INCLUDES) -g -c $< -o $@

$(LIBDIR)/libtsqueue.so : $(BUILDDIR)/tsqueue.o
	gcc -shared $< -o $@
	#ar rcs $@ $<

$(BUILDDIR)/tsqueue.o : $(SRCDIR)/tsqueue.c
	$(CC) $(CFLAGS) $(THREADS) $(INCLUDES) -fPIC -c $< -o $@

clean:
	rm build/*
	rm lib/*
	rm bin/*

