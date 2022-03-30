TEST_ENCODE = encode
TEST_DECODE = decode
LIBFILE = libsms_pdu.so
DEMO_BUILD_DIR ?= ./

OBJ_DIR = ./

CC = gcc
STRIP = strip

CFLAGS += -g -Wall -DLOG_NEED_GET_ALL_LOGCINFIG -O0 -g3
LDFLAGS += $(TARGET_LDFLAGS) -L. -L $(OBJ_DIR)/lib/ -lsms_pdu

DEMO_CODE = test.c

SRCS_ENCODE := test_encode.c
OBJS_ENCODE = $(SRCS_ENCODE:%.c=%.o)

SRCS_DECODE := test_decode.c
OBJS_DECODE = $(SRCS_DECODE:%.c=%.o)



all:prep $(OBJ_DIR)/bin/$(TEST_ENCODE) $(OBJ_DIR)/bin/$(TEST_DECODE) $(OBJ_DIR)/lib/$(LIBFILE)

prep:
	rm -fr $(OBJ_DIR)/bin/$(OBJS_ENCODE) $(OBJ_DIR)/bin/$(OBJS_DECODE)
	mkdir -p $(OBJ_DIR)/bin
	mkdir -p $(OBJ_DIR)/lib

$(OBJ_DIR)/bin/$(TEST_ENCODE): $(OBJS_ENCODE) $(OBJ_DIR)/lib/$(LIBFILE)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	$(STRIP) $@

$(OBJ_DIR)/bin/$(TEST_DECODE): $(OBJS_DECODE) $(OBJ_DIR)/lib/$(LIBFILE)
	$(CC) $(CFLAGS) $^ -o $@  $(LDFLAGS)
	$(STRIP) $@

$(OBJ_DIR)/lib/$(LIBFILE): Pdu.c SDL_iconv.c sms_pdu.c
	$(CC) $(CFLAGS) -g -DUSE_HOSTCC -fpic -shared -o $@ $^ 
	$(STRIP) $@

clean:
	rm -fr *.o *.so $(OBJ_DIR)/bin/ $(OBJ_DIR)/lib/

# run with:
# export LD_LIBRARY_PATH=./lib/
# ./encode --help
# ./decode --help
