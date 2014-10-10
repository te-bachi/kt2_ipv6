
PROGRAMS                    = echo_client echo_server web_client

CC                          = gcc
GLOBAL_CFLAGS               = -O0 -pipe -Wall -ggdb -std=gnu99 -fms-extensions \
                              -Iinclude \
                              -Wmissing-prototypes -Wno-uninitialized -Wstrict-prototypes \
                              -DENABLE_LOG_DEBUG
GLOBAL_LDFLAGS              = -pthread

### KT2 ################################################################

echo_client_CFLAGS          = -DWITH_ECHO_CLIENT
echo_client_LDFLAGS         = 
echo_client_SOURCE          = Main.c \
                              Process.c \
                              Log.c \
                              EchoClient.c

echo_server_CFLAGS          = -DWITH_ECHO_SERVER
echo_server_LDFLAGS         = 
echo_server_SOURCE          = Main.c \
                              Process.c \
                              Log.c \
                              EchoServer.c

web_client_CFLAGS           = -DWITH_WEB_CLIENT
web_client_LDFLAGS          = 
web_client_SOURCE           = Main.c \
                              Process.c \
                              Log.c \
                              WebClient.c

include Makefile.inc

