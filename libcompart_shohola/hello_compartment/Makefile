CC?=gcc
EXEC_NAME=hello_compartment
# Adding -DLC_ALLOW_EXCHANGE_FD=X activates fd transfers, but this needs to also have been provided when compiling libcompart
#CFLAGS+=-std=gnu99 -Wall -Wextra -DPITCHFORK_DBGSTDOUT -DINCLUDE_PID -O0 -g -DLC_ALLOW_EXCHANGE_FD=5
CFLAGS+=-std=gnu99 -Wall -Wextra -DPITCHFORK_DBGSTDOUT -DINCLUDE_PID -O0 -g
ifeq ($(LC_LIB),dynamic)
	LC_LIB_CC_PARAM=-L../libcompart/ -lcompart
else
	LC_LIB_CC_PARAM=../libcompart/compart.o
endif

default : hello_compartment
split : hello_compartment other_compartment third_compartment

clean :
	rm -f ${EXEC_NAME} hello_interface.o other_compartment third_compartment

hello_interface.o : hello_interface.c hello_interface.h
	${CC} ${CFLAGS} -c hello_interface.c -I../libcompart/

hello_compartment : hello_compartment.c hello_interface.o
	${CC} hello_compartment.c hello_interface.o ${CFLAGS} -I../libcompart/ ${LC_LIB_CC_PARAM} -o hello_compartment

other_compartment : other_compartment.c hello_interface.o ../libcompart/compart.o hello_interface.h
	${CC} $@.c hello_interface.o ${CFLAGS} -I../libcompart/ ${LC_LIB_CC_PARAM} -o $@

third_compartment : third_compartment.c hello_interface.o ../libcompart/compart.o hello_interface.h
	${CC} $@.c hello_interface.o ${CFLAGS} -I../libcompart/ ${LC_LIB_CC_PARAM} -o $@
