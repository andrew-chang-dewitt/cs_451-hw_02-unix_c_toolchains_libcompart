#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "part_interface.h"

/* * * * * * * * * * * * * * * * * * * * * * * *
 * Globals
 * -------
 *
 * includes comparts map, extension id,
 * & world history state
 *
 * declared as extern to be available to all
 * compartments
 * * * * * * * * * * * * * * * * * * * * * * * */

// not important to deeply understand for our purposes for now, but this part
// defines how to create the separate binaries needed to run this program on
// different nodes
// FIXME: no idea what this does or if it's right at all...
struct combin combins[NO_COMPARTS] = {
    {.path = "/home/andrew/chopchop/compartmenting/partitioned_life/step"}};

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

// init public extension identifer pointers as NULL
// they must be redefined by user of this code
struct extension_id *step_ext = NULL;

char *world_history = NULL;

/* * * * * * * * * * * * * * * * * * * * * * * *
 * Compartment extenion fn
 * * * * * * * * * * * * * * * * * * * * * * * */

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
  unsigned long step_num = 0;
  unsigned long size = 0;
#ifndef LC_ALLOW_EXCHANGE_FD
  fd = STDOUT_FILENO;
  dprintf(fd, "[%s]:%d BEGIN\n", compart_name(), getuid());
  // unpack the integer given in `data`
  // 2. deserialize argument data
  ext_step_from_arg(data, &step_num, &size, world_history);
#else
  dprintf(fd, "[%s]:%d BEGIN\n", compart_name(), getuid());
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
  struct extension_data result = ext_step_to_arg(step_num, size, world_history);
#else
  struct extension_data result =
      ext_step_to_arg(step_num, size, world_history, fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD

  dprintf(fd, "[%s]:%d END\n", compart_name(), getuid());
  return result;
}

/* * * * * * * * * * * * * * * * * * * * * * * *
 * (De)Serializers
 * -------
 *
 * for inter-compart comms
 * * * * * * * * * * * * * * * * * * * * * * * */

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

// unpack a step number (integer), size (integer), & world history (string of
// states) from a given argument & store each in given pointers
#ifndef LC_ALLOW_EXCHANGE_FD
void ext_history_from_arg(struct extension_data data, unsigned long offset,
                          char *history_ptr)
#else
void ext_history_from_arg(struct extension_data data, unsigned long offset,
                          char *history_ptr, int *fd)
#endif // ndef LC_ALLOW_EXCHANGE_FD
{
  int size_history = data.bufc - offset;
  memcpy(history_ptr, data.buf[offset], size_history);
  // read history states from buffer
#ifdef LC_ALLOW_EXCHANGE_FD
  // FIXME assert result.fdc == 1;
  *fd = data.fd[0];
#endif // LC_ALLOW_EXCHANGE_FD
}
