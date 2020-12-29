# system configuration

CC = gcc
CFLAGS = -O0
#CFLAGS += -Wall
#CFLAGS += -ggdb
LDFLAGS = -s
INCLUDE = ./
LIBS = -lcrypto -lpthread
#LIBS += -lz
SERVER_EXEC = cocito
SERVER_EXEC_TEST = cocito_testconf
SERVER_EXEC_QUIT = cocito_quit
SERVER_EXEC_HUP = cocito_hup
CLIENT_EXEC = dite
CLIENT_EXEC_TEST = dite_testconf
CLIENT_EXEC_QUIT = dite_quit
CLIENT_EXEC_UPDATE = dite_update
DEFAULT_CLIENT_CONF_NAME = /etc/dite.conf
DEFAULT_CLIENT_PID_FILENAME = /var/run/dite.pid
DEFAULT_CLIENT_LOGFILE = /var/log/dite.log
DEFAULT_SERVER_CONF_FILE_NAME = /etc/cocito.conf
DEFAULT_SERVER_PID_FILENAME = /var/run/cocito.pid
DEFAULT_SERVER_PREFIX = /var/log/cocito
DEFAULT_SERVER_CONF_PREFIX = /var/log/cocito
DEFAULT_SERVER_LOGFILE = /var/log/cocito.log
prefix = /usr/local

# end of system configuration

MAJOR = 0
MINOR = 1c

VERSION = $(MAJOR).$(MINOR)

CFLAGS += -DDEFAULT_CLIENT_CONF_NAME=\"$(DEFAULT_CLIENT_CONF_NAME)\"
CFLAGS += -DDEFAULT_CLIENT_PID_FILENAME=\"$(DEFAULT_CLIENT_PID_FILENAME)\"
CFLAGS += -DDEFAULT_SERVER_CONF_FILE_NAME=\"$(DEFAULT_SERVER_CONF_FILE_NAME)\"
CFLAGS += -DDEFAULT_SERVER_PID_FILENAME=\"$(DEFAULT_SERVER_PID_FILENAME)\"
CFLAGS += -DDEFAULT_SERVER_PREFIX=\"$(DEFAULT_SERVER_PREFIX)\"
CFLAGS += -DDEFAULT_SERVER_CONF_PREFIX=\"$(DEFAULT_SERVER_CONF_PREFIX)\"
CFLAGS += -DDEFAULT_CLIENT_LOGFILE=\"$(DEFAULT_CLIENT_LOGFILE)\"
CFLAGS += -DDEFAULT_SERVER_LOGFILE=\"$(DEFAULT_SERVER_LOGFILE)\"

# source files for client program
SRC_CLIENT = client.c
OBJ_CLIENT = client.o

# source files for server program
SRC_SERVER = server.c
OBJ_SERVER = server.o

# common source files
SRCS = algo.c rsa.c tcp.c udp.c ssl.c net.c files.c kernel.c \
	common.c parse.c time_queue.c
OBJS = algo.o rsa.o tcp.o udp.o ssl.o net.o files.o kernel.o \
	common.o parse.o time_queue.o

all: $(SERVER_EXEC) $(CLIENT_EXEC)

$(SERVER_EXEC): $(OBJS) $(OBJ_SERVER)
	$(CC) $(LDFLAGS) -o $@ -I$(INCLUDE) $(OBJS) $(OBJ_SERVER) $(LIBS)

$(CLIENT_EXEC): $(OBJS) $(OBJ_CLIENT)
	$(CC) $(LDFLAGS) -o $@ -I$(INCLUDE) $(OBJS) $(OBJ_CLIENT) $(LIBS)

tgz: distclean
	tar zc -C ../ dite-$(VERSION) -f ../dite-$(VERSION).tgz

install:
	install -o root -g root -m 755 $(SERVER_EXEC) $(prefix)/bin
	install -o root -g root -m 755 $(CLIENT_EXEC) $(prefix)/bin
	ln -fs $(prefix)/bin/$(SERVER_EXEC) $(prefix)/bin/$(SERVER_EXEC_TEST)
	ln -fs $(prefix)/bin/$(SERVER_EXEC) $(prefix)/bin/$(SERVER_EXEC_HUP)
	ln -fs $(prefix)/bin/$(CLIENT_EXEC) $(prefix)/bin/$(CLIENT_EXEC_TEST)
	ln -fs $(prefix)/bin/$(CLIENT_EXEC) $(prefix)/bin/$(CLIENT_EXEC_QUIT)
	ln -fs $(prefix)/bin/$(CLIENT_EXEC) $(prefix)/bin/$(CLIENT_EXEC_UPDATE)
	ln -fs $(prefix)/bin/$(SERVER_EXEC) $(prefix)/bin/$(SERVER_EXEC_QUIT)
	install -o root -g root -m 644 man/$(SERVER_EXEC).8 $(prefix)/man/man8
	install -o root -g root -m 644 man/$(CLIENT_EXEC).8 $(prefix)/man/man8

uninstall:
	rm -f $(prefix)/bin/$(SERVER_EXEC)
	rm -f $(prefix)/bin/$(CLIENT_EXEC)
	rm -f $(prefix)/bin/$(SERVER_EXEC_TEST)
	rm -f $(prefix)/bin/$(SERVER_EXEC_HUP)
	rm -f $(prefix)/bin/$(SERVER_EXEC_QUIT)
	rm -f $(prefix)/bin/$(CLIENT_EXEC_TEST)
	rm -f $(prefix)/bin/$(CLIENT_EXEC_QUIT)
	rm -f $(prefix)/bin/$(CLIENT_EXEC_UPDATE)
	rm -f $(prefix)/man/man8/$(SERVER_EXEC).8
	rm -f $(prefix)/man/man8/$(CLIENT_EXEC).8
	
clean:
	rm -f *.o *~ core

distclean: clean
	rm -f $(SERVER_EXEC) $(CLIENT_EXEC)
