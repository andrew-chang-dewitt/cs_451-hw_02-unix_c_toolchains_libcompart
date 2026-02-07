ROOT     :=$(shell readlink -f .)
SRC      :=$(ROOT)/life
TGT      :=$(ROOT)/target
BIN      :=$(TGT)/bin

MAIN     :=$(SRC)/life.c
EXE      :=$(BIN)/life
GEXE     :=$(BIN)/gprof_life

CMPT_DIR :=$(ROOT)/libcompart_shohola/libcompart

PART_MAIN:=$(SRC)/partitioned_life.c
PART_EXE :=$(BIN)/partitioned_life
GPART_EXE:=$(BIN)/gprof_partitioned_life


ifdef DEBUGGING
DEBUGGING=-O0 -g -include stdbool.h
else
DEBUGGING=-O3
endif

# from https://stackoverflow.com/questions/154630/recommended-gcc-warning-options-for-c
CFLAGS   +=-Wall -Wextra -Wformat=2 -Wswitch-default -Wcast-align -Wpointer-arith \
    -Wbad-function-cast -Wstrict-prototypes -Winline -Wundef -Wnested-externs \
    -Wcast-qual -Wshadow -Wwrite-strings -Wconversion -Wunreachable-code \
    -Wstrict-aliasing=2 -fno-common -fstrict-aliasing \
    -std=c99 -pedantic \
    $(DEBUGGING)

ifeq ($(LC_LIB),dynamic)
	LC_LIB_CC_PARAM=-L$(CMPT_DIR)/ -lcompart
else
	LC_LIB_CC_PARAM=$(CMPT_DIR)/compart.o
endif

COMPART_FLAGS = -DPITCHFORK_DBGSTDOUT -DINCLUDE_PID -I$(CMPT_DIR)/ ${LC_LIB_CC_PARAM}

default: $(EXE)

.PHONY: prof
prof: $(GEXE)

.PHONY: part
part: $(PART_EXE)

.PHONY: gpart
gpart: $(GPART_EXE)


.PHONY: clean
clean:
	rm -rf $(TGT)

$(BIN):
	mkdir -p $(BIN)


$(EXE): $(MAIN) | $(BIN)
	$(CC) $(CFLAGS) $< -o $(EXE)


$(GEXE): $(MAIN) | $(BIN)
	$(CC) $(CFLAGS) -pg $< -o $(GEXE)

$(PART_EXE): $(PART_MAIN) | $(BIN)
	$(CC) $(CFLAGS) $(COMPART_FLAGS) $< -o $(PART_EXE)


$(GPART_EXE): $(PART_MAIN) | $(BIN)
	$(CC) $(CFLAGS) $(COMPART_FLAGS) -pg $< -o $(GPART_EXE)
