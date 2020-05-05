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

$(BINDIR)/supermarket : $(SRCDIR)/supermarket.c $(SRCDIR)/cashier.c $(SRCDIR)/client.c $(LIBDIR)/libtsqueue.a
	$(CC) $(CFLAGS) $(THREADS) $(LDFLAGS) $(LIBS) $(INCLUDES) -g $^ -o $@

#$(BUILDDIR)/client_cashier_test.o : $(SRCDIR)/client_cashier_test.c
#	$(CC) $(CFLAGS) $(THREADS) $(INCLUDES) -c $^ -o $@  

#$(BUILDDIR)/client.o : $(SRCDIR)/client.c $(INCDIR)/client.h $(BUILDDIR)/cashier.o $(INCDIR)/cashier.h $(LIBDIR)/libtsqueue.a
#	$(CC) $(CFLAGS) $(THREADS) $(LDFLAGS) $(LIBS) $(INCLUDES) -c $< -o $@

#$(BUILDDIR)/cashier.o : $(SRCDIR)/cashier.c $(INCDIR)/cashier.h $(LIBDIR)/libtsqueue.a
#	$(CC) $(CFLAGS) $(THREADS) $(LDFLAGS) $(LIBS) $(INCLUDES) -c $< -o $@

$(LIBDIR)/libtsqueue.a : $(BUILDDIR)/tsqueue.o
	ar rcs $@ $<

$(BUILDDIR)/tsqueue.o : $(SRCDIR)/tsqueue.c
	$(CC) $(CFLAGS) $(THREADS) $(INCLUDES) -c $< -o $@


