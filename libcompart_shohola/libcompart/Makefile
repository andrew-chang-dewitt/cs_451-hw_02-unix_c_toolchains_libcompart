.PHONY: all clean compost_named compart_lib

CC?=gcc
# Adding -DLC_ALLOW_EXCHANGE_FD=X activates fd transfers
PF_FLAGS=-DPITCHFORK_DBGSTDOUT -DINCLUDE_PID
CFLAGS+=-std=c89 -D_BSD_SOURCE -Wall -Wextra
ifeq ($(EXPERIMENT_PERFORM),YES)
	CFLAGS+=-O2 -DPITCHFORK_NOLOG
else
	CFLAGS+=-O0 -g
endif

ifeq ($(LC_LIB),dynamic)
	LC_LIB_CC_PARAM=-fPIC
	LC_LIB_LD_PARAM=-shared -fPIC
	LC_LIB_FILE=libcompart.so
else
	LC_LIB_CC_PARAM=-fPIE
	LC_LIB_LD_PARAM=-relocatable
	LC_LIB_FILE=compart.o
endif

all: compost.o compart_lib

compost.o : compost.h compost.c
	${CC} ${CFLAGS} ${PF_FLAGS} ${LC_LIB_CC_PARAM} -c compost.c

# make compost_named && make compart_lib
compost_named : compost.h combin.h combin.c
	${CC} ${CFLAGS} ${PF_FLAGS} -c combin.c -o compost.o

compost_tcp : compost.h combin_tcp.h combin_tcp.c
	${CC} ${CFLAGS} ${PF_FLAGS} -c combin_tcp.c -o compost.o

compart_lib : compart_base.h compart_api.h compart.c compost.o
	@echo PF_FLAGS=${PF_FLAGS}
	${CC} ${CFLAGS} ${PF_FLAGS} ${LC_LIB_CC_PARAM} -c compart.c -o compart_general.o
	ld ${LC_LIB_LD_PARAM} compart_general.o compost.o -o ${LC_LIB_FILE}

clean :
	rm -f compart.o libcompart.so compost.o compart_general.o
