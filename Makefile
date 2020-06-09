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

CONFIG_FILE = config/config.txt
LOG_FN = test_out.log

.PHONY: all clean test

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

test:
	@echo [+] Generating config file in $(CONFIG_FILE)...
	@echo K 6 > $(CONFIG_FILE)
	@echo C 50 >> $(CONFIG_FILE)
	@echo E 3 >> $(CONFIG_FILE)
	@echo T 200 >> $(CONFIG_FILE)
	@echo P 100 >> $(CONFIG_FILE)
	@echo INITKN 3 >> $(CONFIG_FILE)
	@echo PRODTIME 20 >> $(CONFIG_FILE)
	@echo ANALYTICS_T 2000 >> $(CONFIG_FILE)
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


