# Nik Sultana, December 2025

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

ROOT     :=$(shell readlink -f .)
SRC      :=$(ROOT)/life
TGT      :=$(ROOT)/target
BIN      :=$(TGT)/bin

MAIN     :=$(SRC)/life.c
EXE      :=$(BIN)/life
GEXE     :=$(BIN)/gprof_life

PART_MAIN:=$(SRC)/partitioned_life.c
PART_EXE :=$(BIN)/partitioned_life
GPART_EXE:=$(BIN)/gprof_partitioned_life

default: $(EXE)


.PHONY: clean
clean:
	rm -rf $(TGT)

$(BIN):
	mkdir -p $(BIN)

.PHONY: prof
prof: $(GEXE)


$(EXE): $(MAIN) | $(BIN)
	$(CC) $(CFLAGS) $< -o $(EXE)


$(GEXE): $(MAIN) | $(BIN)
	$(CC) $(CFLAGS) -pg $< -o $(GEXE)

$(PART_EXE): $(PART_MAIN) | $(BIN)
	$(CC) $(CFLAGS) $< -o $(PART_EXE)


$(GPART_EXE): $(PART_MAIN) | $(BIN)
	$(CC) $(CFLAGS) -pg $< -o $(GPART_EXE)
