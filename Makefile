ROOT      := $(shell readlink -f .)
SRC       := $(ROOT)/life
TGT       := $(ROOT)/target
BIN       := $(TGT)/bin

MAIN      := $(SRC)/life.c
EXE       := $(BIN)/life
GEXE      := $(BIN)/gprof_life

CMPT_DIR  := $(ROOT)/libcompart_shohola/libcompart

PART_MAIN := $(SRC)/partitioned_life.c
PART_INTO := $(SRC)/part_interface.o
PART_INTH := $(SRC)/part_interface.h
PART_INTC := $(SRC)/part_interface.c
PART_DEPS := $(PART_MAIN) $(PART_INTO)
PART_EXE  := $(BIN)/partitioned_life
GPART_EXE := $(BIN)/gprof_partitioned_life
PART_DBG  := $(PART_EXE)_debug



# from https://stackoverflow.com/questions/154630/recommended-gcc-warning-options-for-c
ALL_FLAGS := -Wall -Wextra -Wformat=2 -Wswitch-default -Wcast-align -Wpointer-arith \
             -Wbad-function-cast -Wstrict-prototypes -Winline -Wundef \
             -Wnested-externs -Wcast-qual -Wshadow -Wwrite-strings \
             -Wconversion -Wunreachable-code -Wstrict-aliasing=2 \
             -fno-common -fstrict-aliasing -std=c99 -pedantic

CFLAGS    += $(ALL_FLAGS) -O3
DBG_FLAGS := $(ALL_FLAGS) -O0 -g -include stdbool.h

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

part_dbg: $(PART_DBG)


.PHONY: clean
clean:
	rm -rf $(TGT)
	rm ./**/*.o

$(BIN):
	mkdir -p $@


$(EXE): $(MAIN) | $(BIN)
	$(CC) $(CFLAGS) $< -o $@


$(GEXE): $(MAIN) | $(BIN)
	$(CC) $(CFLAGS) -pg $< -o $%


$(PART_EXE): $(PART_DEPS) | $(BIN)
	$(CC) $(CFLAGS) $(COMPART_FLAGS) $< -o $@


$(GPART_EXE): $(PART_DEPS) | $(BIN)
	$(CC) $(CFLAGS) $(COMPART_FLAGS) -pg $< -o $@

$(PART_DBG): $(PART_DEPS) | $(BIN)
	$(CC) $(DBG_FLAGS) $(COMPART_FLAGS) $< -o $@

.PHONY: obj
obj: $(PART_INTO)


$(PART_INTO): $(PART_INTH) $(PART_INTC)
	$(CC) $(CFLAGS) -c $(PART_INTC) -I$(CMPT_DIR)/ -o $@
