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
LIBS 		= -ltsqueue

.PHONY: all clean test2

#$(BINDIR)/supermarket : $(SRCDIR)/supermarket.c $(SRCDIR)/cashier.c $(SRCDIR)/client.c $(LIBDIR)/libtsqueue.a
#	$(CC) $(CFLAGS) $(THREADS) $(LDFLAGS) $(LIBS) $(INCLUDES) -g $^ -o $@

$(BINDIR)/supermarket : $(BUILDDIR)/supermarket.o $(BUILDDIR)/client.o $(BUILDDIR)/cashier.o $(LIBDIR)/libtsqueue.a
	$(CC) $(CFLAGS) $(THREADS) $(INCLUDES) $(LDFLAGS) $(LIBS) -g $^ -o $@

$(BUILDDIR)/supermarket.o : $(SRCDIR)/supermarket.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@  

$(BUILDDIR)/client.o : $(SRCDIR)/client.c $(INCDIR)/client.h $(BUILDDIR)/cashier.o
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILDDIR)/cashier.o : $(SRCDIR)/cashier.c $(INCDIR)/cashier.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(LIBDIR)/libtsqueue.a : $(BUILDDIR)/tsqueue.o
	ar rcs $@ $<

$(BUILDDIR)/tsqueue.o : $(SRCDIR)/tsqueue.c
	$(CC) $(CFLAGS) $(THREADS) $(INCLUDES) -c $< -o $@


clean:
	rm build/*
	rm lib/*
	rm bin/*

