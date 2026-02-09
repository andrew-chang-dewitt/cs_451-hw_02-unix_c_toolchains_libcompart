/*
    Conway's Game of Life (and rough edges in the code for teaching purposes)
    Copyright (C) 2025 Nik Sultana

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.


gcc life.c -o life # -pg
valgrind -s --leak-check=full --show-leak-kinds=all ./life -s 20 -c 20 -i
010000000001000000000010000000010000000011100000000100000000 > life.out
./life -s 3 -c 3 -i 011001010
./life -s 5 -c 10 -i 011001010110101111
./life -s 20 -c 10 -i
011001010110101111011001010110101111010111101010101111100110111 Glider and
blinker:
./life -s 20 -c 20 -i
010000000001000000000010000000010000000011100000000100000000 Bee-hive and loaf:
./life -s 10 -c 100 -i
010000000001000000000010000000010000000011100000000100000000 > life.out
*/

#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "part_interface.h"
// imports compart, combin, extension_id, extension_data, NO_COMPARTS

void print_world(const unsigned long size, const unsigned long step_number);
void init_step(const unsigned long size, const char *init_world);
void step(const unsigned long size, const unsigned long step_number);
unsigned char living_neighbours(const unsigned long size,
                                const unsigned long step_number,
                                const unsigned long x, const unsigned long y);
char neighbour_state(const unsigned long size, const unsigned long step_number,
                     const unsigned long x, const unsigned long y,
                     const unsigned char neighbour_offset);
void set_value(const unsigned long size, const unsigned long step_number,
               const unsigned long x, const unsigned long y, const char value);
char get_value(const unsigned long size, const unsigned long step_number,
               const unsigned long x, const unsigned long y);

int main(int argc, char *const *argv) {
  compart_check();
  compart_init(NO_COMPARTS, comparts, default_config);
  // register pointer to function extension calculate step on step compartment
  step_ext = compart_register_fn("step compartment", &ext_step);
  compart_start("main compartment");

  // get a handle on stdout
#ifndef LC_ALLOW_EXCHANGE_FD
  int fd = STDOUT_FILENO;
#else
  int fd = open("stdout", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
  printf("(%d) open fd=%d\n", getpid(), fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD

  unsigned long size = 3;
  char *init_world = NULL;
  unsigned long cycles = 3;

  int option;
  while ((option = getopt(argc, argv, "s:i:c:")) != -1) {
    switch (option) {
    case 'i':
      init_world = malloc(strlen(optarg) + 1);
      strcpy(init_world, optarg);
      break;
    case 's':
      size = strtoul(optarg, NULL, 10);
      break;
    case 'c':
      cycles = strtoul(optarg, NULL, 10);
      break;
    default:
      // FIXME: print error message.
      exit(EXIT_FAILURE);
      break;
    }
  }

  assert(size > 1);
  assert(cycles > 1);

  world_history = malloc(size * size * cycles);

  for (unsigned long i = 0; i < size * size * cycles; i++) {
    world_history[i] = 0;
  }

  init_step(size, init_world);
  print_world(size, 0);
  printf("\n");

  unsigned long i = 1;
  dprintf(fd, "before for loop from %li to %li\n", i, cycles);
  for (; i < cycles; i++) {
    dprintf(fd, "starting step %li\n", i);
    // step(size, i);
    ext_history_from_arg( // update global state from step compartment
        compart_call_fn(step_ext, ext_step_to_arg(i, size, world_history)),
        sizeof(2 * i), world_history);
    print_world(size, i);
    printf("\n");
  }
  dprintf(fd, "after for loop, completed %li iterations before reaching %li\n",
          i - 1, cycles);

  free(world_history);
  free(init_world);

  close(fd);
  return EXIT_SUCCESS;
}

char get_value(const unsigned long size, const unsigned long step_number,
               const unsigned long x, const unsigned long y) {
  // FIXME assert x >= 0
  // FIXME assert y >= 0
  // FIXME assert x < size
  // FIXME assert y < size
  return world_history[size * size * step_number + y * size + x];
}

void set_value(const unsigned long size, const unsigned long step_number,
               const unsigned long x, const unsigned long y, const char value) {
  // FIXME assert x >= 0
  // FIXME assert y >= 0
  // FIXME assert x < size
  // FIXME assert y < size
  world_history[size * size * step_number + y * size + x] = value;
}

char neighbour_state(const unsigned long size, const unsigned long step_number,
                     const unsigned long x, const unsigned long y,
                     const unsigned char neighbour_offset) {
  /* Numberings of the neighbours of the cell labeled "C":
        _ _ _
       |0|1|2|
       |-|-|-|
       |3|C|4|
       |-|-|-|
       |5|6|7|
        - - -
  */
  char result = -2;

  switch (neighbour_offset) {
  case 0:
    if (0 == x || 0 == y) {
      result = -1;
    } else {
      result = get_value(size, step_number, x - 1, y - 1);
    }
    break;
  case 1:
    if (0 == y) {
      result = -1;
    } else {
      result = get_value(size, step_number, x, y - 1);
    }
    break;
  case 2:
    if (size - 1 == x || 0 == y) {
      result = -1;
    } else {
      result = get_value(size, step_number, x + 1, y - 1);
    }
    break;
  case 3:
    if (0 == x) {
      result = -1;
    } else {
      result = get_value(size, step_number, x - 1, y);
    }
    break;
  case 4:
    if (size - 1 == x) {
      result = -1;
    } else {
      result = get_value(size, step_number, x + 1, y);
    }
    break;
  case 5:
    if (0 == x || size - 1 == y) {
      result = -1;
    } else {
      result = get_value(size, step_number, x - 1, y + 1);
    }
    break;
  case 6:
    if (size - 1 == y) {
      result = -1;
    } else {
      result = get_value(size, step_number, x, y + 1);
    }
    break;
  case 7:
    if (size - 1 == x || size - 1 == y) {
      result = -1;
    } else {
      result = get_value(size, step_number, x + 1, y + 1);
    }
    break;
  default:
    // FIXME terminate with error message.
    break;
  }

  return result;
}

unsigned char living_neighbours(const unsigned long size,
                                const unsigned long step_number,
                                const unsigned long x, const unsigned long y) {

  // FIXME assert y >= 0
  // FIXME assert x < size
  // FIXME assert y < size
  unsigned char result = 0;

  for (unsigned char i = 0; i < 8; i++) {
    if (1 == neighbour_state(size, step_number, x, y, i)) {
      result += 1;
    }
  }

  return result;
}

void step(const unsigned long size, const unsigned long step_number) {
  for (unsigned long y = 0; y < size; y++) {
    for (unsigned long x = 0; x < size; x++) {
      const unsigned char ln = living_neighbours(size, step_number - 1, x, y);
      char state = get_value(size, step_number - 1, x, y);
      if (1 == state) {
        if (ln < 2) {
          state = 0;
        } else if (2 == ln || 3 == ln) {
          state = 1;
        } else if (ln > 3) {
          state = 0;
        }
      } else if (0 == state) {
        if (3 == ln) {
          state = 1;
        }
      }

      set_value(size, step_number, x, y, state);
    }
  }
}

void init_step(const unsigned long size, const char *init_world) {
  bool completely_uninitialized = false;
  unsigned long init_size = 0;
  if (NULL != init_world)
    init_size = strlen(init_world);
  else
    completely_uninitialized = true;

  for (unsigned long i = 0; i < size * size; i++) {
    if (completely_uninitialized || i >= init_size) {
      world_history[i] = 0;
      continue;
    }

    if (0 == strncmp("0", init_world + i, 1)) {
      world_history[i] = 0;
    } else if (0 == strncmp("1", init_world + i, 1)) {
      world_history[i] = 1;
    } else if (0 == strncmp("\0", init_world + i, 1)) {
      return;
    } else {
      fprintf(stderr,
              "Invalid initial state"); // FIXME: show the offending character.
      exit(EXIT_FAILURE);
    }
  }
}

void print_world(const unsigned long size, const unsigned long step_number) {
  /*
    for (int y = 0; y < size; y++) {
      for (int x = 0; x < size; x++) {
        int v = get_value(size, step_number, x, y);
        printf("%d", v);
      }
      printf("\n");
    }
  */
  for (unsigned long y = 0; y < size; y++) {
    for (unsigned long x = 0; x < size; x++) {
      char v = get_value(size, step_number, x, y);
      char terminator[1] = "\n";
      if (x < size - 1)
        terminator[0] = ',';
      printf("%d%s", v, terminator);
    }
  }
}
