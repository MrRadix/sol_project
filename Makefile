CC 			= gcc
CFLAGS 		+= -Wall -pedantic

LIBDIR		= ./lib
INCDIR		= ./include
LIBDIR		= ./lib
SRCDIR		= ./src
BINDIR		= ./bin
BUILDDIR	= ./build

INCLUDES	= -I $(INCDIR)
LDFLAGS		= -L $(LIBDIR)

.PHONY: all clean test2

$(LIBDIR)/libtsqueue.a : $(BUILDDIR)/tsqueue.o
	ar rcs $@ $<

$(BUILDDIR)/tsqueue.o : $(SRCDIR)/tsqueue.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


