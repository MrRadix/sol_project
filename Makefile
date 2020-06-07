CC 			= gcc
CFLAGS 		+= -Wall -pedantic
THREADS		= -pthread

LIBDIR		= ./lib
INCDIR		= ./include
SRCDIR		= ./src
TSTSRC		= ./tests
BINDIR		= ./bin
BUILDDIR	= ./build

INCLUDES	= -I $(INCDIR)
LDFLAGS		= -L $(LIBDIR)
LIBS 		= -ltsqueue -lpthread -lllist

TARGET = $(BINDIR)/supermarket
TEST_LLIST = $(BINDIR)/linkedlist_test

.PHONY: all clean test2 tests

all: $(TARGET)

tests: $(TEST_LLIST)

$(TEST_LLIST) : $(TSTSRC)/linkedlist_test.c $(LIBDIR)/libllist.a
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) $(LIBS) -g $^ -o $@

$(TARGET) : $(BUILDDIR)/supermarket.o $(BUILDDIR)/client.o $(BUILDDIR)/cashier.o $(BUILDDIR)/director.o $(LIBDIR)/libtsqueue.a $(LIBDIR)/libllist.a
	$(CC) $(CFLAGS) $(THREADS) $(INCLUDES) $(LDFLAGS) $(LIBS) -g $^ -o $@

$(BUILDDIR)/supermarket.o : $(SRCDIR)/supermarket.c
	$(CC) $(CFLAGS) $(INCLUDES) -g -c $^ -o $@

$(BUILDDIR)/client.o : $(SRCDIR)/client.c $(INCDIR)/client.h
	$(CC) $(CFLAGS) $(INCLUDES) -g -c $< -o $@

$(BUILDDIR)/cashier.o : $(SRCDIR)/cashier.c $(INCDIR)/cashier.h
	$(CC) $(CFLAGS) $(INCLUDES) -g -c $< -o $@

$(BUILDDIR)/director.o : $(SRCDIR)/director.c $(INCDIR)/director.h
	$(CC) $(CFLAGS) $(INCLUDES) -g -c $< -o $@

$(LIBDIR)/libtsqueue.a : $(BUILDDIR)/tsqueue.o
	ar rcs $@ $<

$(BUILDDIR)/tsqueue.o : $(SRCDIR)/tsqueue.c $(INCDIR)/tsqueue.h
	$(CC) $(CFLAGS) $(THREADS) $(INCLUDES) -c $< -o $@

$(LIBDIR)/libllist.a : $(BUILDDIR)/linkedlist.o
	ar rcs $@ $<

$(BUILDDIR)/linkedlist.o : $(SRCDIR)/linkedlist.c $(INCDIR)/linkedlist.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm build/*
	rm lib/*
	rm bin/*

