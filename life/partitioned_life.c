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

#include <combin.h>
#include <compart_base.h>

#ifndef LIBCOMPART_VERSION_SHOHOLA
#error Using incompatible version of libcompart
#endif // LIBCOMPART_VERSION_SHOHOLA

#define NO_COMPARTS 2
extern struct compart comparts[NO_COMPARTS] = {{.name = "main compartment",
                                                .uid = 65534,
                                                .gid = 65534,
                                                .path = "/tmp",
                                                .comms = NULL},
                                               {.name = "step compartment",
                                                .uid = 65534,
                                                .gid = 65534,
                                                .path = "/tmp",
                                                .comms = NULL}};
// not important to deeply understand for our purposes for now, but this part
// defines how to create the separate binaries needed to run this program on
// different nodes
// FIXME: no idea what this does or if it's right at all...
struct combin combins[NO_COMPARTS] = {
    {.path = "/home/andrew/chopchop/compartmenting/partitioned_life/step"}};

// init public extension identifer pointers as NULL
// they must be redefined by user of this code
struct extension_id *step_ext = NULL;

char *world_history = NULL;

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
  // FIXME assert x >= 0
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

// pack (serialize to bytes) a given step number (integer)
// & world history (size int & string of states)
#ifndef LC_ALLOW_EXCHANGE_FD
struct extension_data ext_step_to_arg(unsigned long step_num,
                                      unsigned long size, char *history)
#else
struct extension_data ext_step_to_arg(unsigned long step_num,
                                      unsigned long size, char *history, int fd)
#endif // ndef LC_ALLOW_EXCHANGE_FD
{
  // create empty result value
  struct extension_data result;
  // declare size of buffer to be memory size of int + int + step number * size
  // * size
  int size_step_num = sizeof(step_num);
  int size_size = sizeof(size);
  int size_history = step_num * size * size * sizeof(*history);
  result.bufc = size_step_num + size_size + size_history;
  // copy 4-byte repr of step number to buffer on result
  memcpy(result.buf, &step_num, size_step_num);
  // append 4-byte repr of size to that
  int offset = size_step_num;
  memcpy(result.buf[offset], &size, size_size);
  // then append bytestr of states
  offset += size_size;
  memcpy(result.buf[offset], history, size_history);
#ifdef LC_ALLOW_EXCHANGE_FD
  result.fdc = 1;
  printf("(%d) ext_step_to_arg fd=%d\n", getpid(), fd);
  result.fd[0] = fd;
#endif // LC_ALLOW_EXCHANGE_FD
  // return result object
  return result;
}

// unpack a step number (integer), size (integer), & world history (string of
// states) from a given argument & store each in given pointers
#ifndef LC_ALLOW_EXCHANGE_FD
void ext_step_from_arg(struct extension_data data, unsigned long *step_num_ptr,
                       unsigned long *size_ptr, char *history_ptr)
#else
void ext_step_from_arg(struct extension_data data, unsigned long *step_num_ptr,
                       unsigned long *size_ptr, char *history_ptr, int *fd)
#endif // ndef LC_ALLOW_EXCHANGE_FD
{
  int size_step_num = sizeof(*step_num_ptr);
  int size_size = sizeof(*size_ptr);
  // use memcpy to read new step num from front of buffer
  memcpy(step_num_ptr, data.buf, size_step_num);
  // then get size
  int offset = size_step_num;
  memcpy(size_ptr, data.buf[offset], size_size);
  // and history states
  int size_history = data.bufc - size_step_num - size_size;
  offset += size_size;
  memcpy(history_ptr, data.buf[offset], size_history);
  // read history states from buffer
#ifdef LC_ALLOW_EXCHANGE_FD
  // FIXME assert result.fdc == 1;
  *fd = data.fd[0];
#endif // LC_ALLOW_EXCHANGE_FD
}

// execute a step on the given history of states
// shows pattern we'll see across other ext_* functions:
// 1. declare a file descriptor for logging
// 2. deserialize argument data
// 3. perform computation w/ output of (2)
// 4. serialize value to return
// 5. return output of (4)
struct extension_data ext_step(struct extension_data data) {
  // 1. declare a file descriptor for logging
  int fd = -1;
#ifndef LC_ALLOW_EXCHANGE_FD
  fd = STDOUT_FILENO;
  // unpack the integer given in `data`
  // 2. deserialize argument data
  unsigned long step_num = 0;
  unsigned long size = 0;
  ext_step_from_arg(data, &step_num, &size, world_history);
#else
  ext_step_from_arg(data, &step_num, &size, world_history, &fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD
  // log debugging info
  dprintf(fd, "ext_step() on %s. uid=%d\n", compart_name(), getuid());

  // add ten to unpacked value
  // 3. perform computation w/ output of (2)
  step(size, step_num);
#ifndef LC_ALLOW_EXCHANGE_FD
  // return value, now modified, by repacking (i.e. serializing) to
  // `extension_data` type
  // 4. serialize value to return
  // 5. return output of (4)
  return ext_step_to_arg(step_num, size, world_history);
#else
  return ext_step_to_arg(step_num, size, world_history, fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD
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

  for (unsigned long i = 1; i < cycles; i++) {
    // step(size, i);
    compart_call_fn(step_ext, ext_step_to_arg(i, size, world_history));
    print_world(size, i);
    printf("\n");
  }

  free(world_history);
  free(init_world);

  close(fd);
  return EXIT_SUCCESS;
}
