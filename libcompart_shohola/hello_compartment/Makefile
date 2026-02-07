CC?=gcc
# Adding -DLC_ALLOW_EXCHANGE_FD=X activates fd transfers, but this needs to also have been provided when compiling libcompart
#CFLAGS+=-std=gnu99 -Wall -Wextra -DPITCHFORK_DBGSTDOUT -DINCLUDE_PID -O0 -g -DLC_ALLOW_EXCHANGE_FD=5
CFLAGS+=-std=gnu99 -Wall -Wextra -DPITCHFORK_DBGSTDOUT -DINCLUDE_PID -O0 -g
ifeq ($(LC_LIB),dynamic)
	LC_LIB_CC_PARAM=-L../libcompart/ -lcompart
else
	LC_LIB_CC_PARAM=../libcompart/compart.o
endif

HELLO_EXE=../../target/bin/hello_compartment
OTHER_EXE=../../target/bin/other_compartment
THIRD_EXE=../../target/bin/third_compartment

default : $(HELLO_EXE)
split : $(HELLO_EXE) $(OTHER_EXE) $(THIRD_EXE)

clean :
	rm -f ${HELLO_EXE} *.o $(OTHER_EXE) $(THIRD_EXE)

hello_interface.o : hello_interface.c hello_interface.h
	${CC} ${CFLAGS} -c hello_interface.c -I../libcompart/

$(HELLO_EXE) : hello_compartment.c hello_interface.o
	${CC} hello_compartment.c hello_interface.o ${CFLAGS} -I../libcompart/ ${LC_LIB_CC_PARAM} -o $@

$(OTHER_EXE) : other_compartment.c hello_interface.o ../libcompart/compart.o hello_interface.h
	${CC} other_compartment.c hello_interface.o ${CFLAGS} -I../libcompart/ ${LC_LIB_CC_PARAM} -o $@

$(THIRD_EXE) : third_compartment.c hello_interface.o ../libcompart/compart.o hello_interface.h
	${CC} third_compartment.c hello_interface.o ${CFLAGS} -I../libcompart/ ${LC_LIB_CC_PARAM} -o $@
