HELLO_DIR =$(shell readlink -f .)
LIB_DIR   =$(HELLO_DIR)/../libcompart
BIN_DIR   =$(HELLO_DIR)/../../target/bin

HELLO_EXE =$(BIN_DIR)/hello_compartment
OTHER_EXE =$(BIN_DIR)/other_compartment
THIRD_EXE =$(BIN_DIR)/third_compartment

CC?=gcc
# Adding -DLC_ALLOW_EXCHANGE_FD=X activates fd transfers, but this needs to also have been provided when compiling libcompart
#CFLAGS+=-std=gnu99 -Wall -Wextra -DPITCHFORK_DBGSTDOUT -DINCLUDE_PID -O0 -g -DLC_ALLOW_EXCHANGE_FD=5
CFLAGS+=-std=gnu99 -Wall -Wextra -DPITCHFORK_DBGSTDOUT -DINCLUDE_PID -O0 -g
ifeq ($(LC_LIB),dynamic)
	LC_LIB_CC_PARAM=-L$(LIB_DIR)/ -lcompart
else
	LC_LIB_CC_PARAM=$(LIB_DIR)/compart.o
endif

default : $(HELLO_EXE)
split : $(HELLO_EXE) $(OTHER_EXE) $(THIRD_EXE)

clean :
	rm -f ${HELLO_EXE} *.o $(OTHER_EXE) $(THIRD_EXE)

hello_interface.o : hello_interface.c hello_interface.h
	${CC} ${CFLAGS} -c hello_interface.c -I$(LIB_DIR)/

$(HELLO_EXE) : hello_compartment.c hello_interface.o | $(BIN_DIR)
	${CC} hello_compartment.c hello_interface.o ${CFLAGS} -I$(LIB_DIR)/ ${LC_LIB_CC_PARAM} -o $@

$(OTHER_EXE) : other_compartment.c hello_interface.o $(LIB_DIR)/compart.o hello_interface.h | $(BIN_DIR)
	${CC} other_compartment.c hello_interface.o ${CFLAGS} -I$(LIB_DIR)/ ${LC_LIB_CC_PARAM} -o $@

$(THIRD_EXE) : third_compartment.c hello_interface.o $(LIB_DIR)/compart.o hello_interface.h | $(BIN_DIR)
	${CC} third_compartment.c hello_interface.o ${CFLAGS} -I$(LIB_DIR)/ ${LC_LIB_CC_PARAM} -o $@

$(BIN_DIR):
	mkdir -p $(BIN_DIR)
