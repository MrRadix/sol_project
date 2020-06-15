CC 			= gcc
CFLAGS 		+= -Wall -pedantic -std=c99
THREADS		= -pthread

LIBDIR		= ./lib
INCDIR		= ./include
SRCDIR		= ./src
TSTSRC		= ./tests
BINDIR		= ./bin
BUILDDIR	= ./build

DEFINES		= -D_POSIX_SOURCE=200809L -D_DEFAULT_SOURCE
INCLUDES	= -I$(INCDIR)
LDFLAGS		= -L$(LIBDIR)
LIBS 		= -ltsqueue -lpthread -llinkedlist

TARGET = $(BINDIR)/supermarket
TEST_LLIST = $(BINDIR)/linkedlist_test

CONFIG_FILE = config/config.txt
LOG_FN = test_out.log

.PHONY: all clean test

all: $(TARGET)

$(TARGET) : $(BUILDDIR)/supermarket.o $(BUILDDIR)/client.o $(BUILDDIR)/cashier.o $(BUILDDIR)/director.o $(LIBDIR)/libtsqueue.a $(LIBDIR)/liblinkedlist.a
	@mkdir $(BINDIR) 2>/dev/null || true
	$(CC) $(CFLAGS) $(THREADS) $(INCLUDES) $(LDFLAGS) $(LIBS) -g $^ -o $@

$(BUILDDIR)/supermarket.o : $(SRCDIR)/supermarket.c
	@mkdir $(BUILDDIR) 2>/dev/null || true
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -g -c $^ -o $@

$(BUILDDIR)/client.o : $(SRCDIR)/client.c $(INCDIR)/client.h
	@mkdir $(BUILDDIR) 2>/dev/null || true
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -g -c $< -o $@

$(BUILDDIR)/cashier.o : $(SRCDIR)/cashier.c $(INCDIR)/cashier.h
	@mkdir $(BUILDDIR) 2>/dev/null || true
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -g -c $< -o $@

$(BUILDDIR)/director.o : $(SRCDIR)/director.c $(INCDIR)/director.h
	@mkdir $(BUILDDIR) 2>/dev/null || true
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -g -c $< -o $@

$(BUILDDIR)/tsqueue.o : $(SRCDIR)/tsqueue.c $(INCDIR)/tsqueue.h
	@mkdir $(BUILDDIR) 2>/dev/null || true
	$(CC) $(CFLAGS) $(THREADS) $(INCLUDES) -c $< -o $@

$(BUILDDIR)/linkedlist.o : $(SRCDIR)/linkedlist.c $(INCDIR)/linkedlist.h
	@mkdir $(BUILDDIR) 2>/dev/null || true
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(LIBDIR)/libtsqueue.a : $(BUILDDIR)/tsqueue.o
	@mkdir $(LIBDIR) 2>/dev/null || true
	ar rcs $@ $<

$(LIBDIR)/liblinkedlist.a : $(BUILDDIR)/linkedlist.o
	@mkdir $(LIBDIR) 2>/dev/null || true
	ar rcs $@ $<

clean:
	@rm -r build/ 2>/dev/null || true
	@rm -r lib/ 2>/dev/null || true
	@rm -r bin/ 2>/dev/null || true
	@rm $(LOG_FN) 2>/dev/null || true

test:
	@echo [+] Generating config file in $(CONFIG_FILE)...
	@echo K 6 > $(CONFIG_FILE)
	@echo C 50 >> $(CONFIG_FILE)
	@echo E 3 >> $(CONFIG_FILE)
	@echo T 200 >> $(CONFIG_FILE)
	@echo P 100 >> $(CONFIG_FILE)
	@echo INITKN 2 >> $(CONFIG_FILE)
	@echo PRODTIME 20 >> $(CONFIG_FILE)
	@echo ANALYTICS_T 3000 >> $(CONFIG_FILE)
	@echo ANALYTICS_DIFF 1 >> $(CONFIG_FILE)
	@echo LOG_FN $(LOG_FN) >> $(CONFIG_FILE)
	@echo S1 2 >> $(CONFIG_FILE)
	@echo S2 10 >> $(CONFIG_FILE)

	@echo [+] Launching supermarket process...
	@$(BINDIR)/supermarket & echo $$! > sm.PID
	@echo [+] Supermarket process is running
	@sleep 25

	@if [ -e sm.PID ]; then \
		echo [+] sending SIGHUP to supermarket process...; \
		kill -1 $$(cat sm.PID) || true; \
		echo [+] waiting for supermarket process to close...; \
		while sleep 1; do ps -p $$(cat sm.PID) 1>/dev/null || break; done; \
	fi;

	@rm sm.PID
	@echo
	@echo
	@./analisi.sh $(LOG_FN)
	@echo
	@echo
