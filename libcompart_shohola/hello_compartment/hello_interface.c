/*
Part of the "chopchop" project at the University of Pennsylvania, USA.
Authors: Nik Sultana. 2019, 2020.

   Copyright 2021 University of Pennsylvania

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "hello_interface.h"

// not important to deeply understand for our purposes for now, but this part
// defines how to create the separate binaries needed to run this program on
// different nodes
struct combin combins[NO_COMPARTS] = {
    {.path = "/home/nik/chopchop/compartmenting/hello_compartment/other"},
    {.path = "/home/nik/chopchop/compartmenting/hello_compartment/third"}};

// create separate compartments for each portion of our code (declare
// compartment name, user id & group id (to control ambient privileges),
// path (to set "root" of filesystem, meaning addresses outside the tree
// contained at path are not visible to this compartment) etc.)
struct compart comparts[NO_COMPARTS] = {{.name = "hello compartment",
                                         .uid = 65534,
                                         .gid = 65534,
                                         .path = "/tmp",
                                         .comms = NULL},
                                        {.name = "other compartment",
                                         .uid = 65534,
                                         .gid = 65534,
                                         .path = "/tmp",
                                         .comms = &combins[0]},
                                        // note third compartment uses uid of 0
                                        // (nobody) to limit privileges to
                                        // basically none
                                        {.name = "third compartment",
                                         .uid = 0,
                                         .gid = 0,
                                         .path = "/tmp",
                                         .comms = &combins[1]}};

// init public extension identifer pointers as NULL
// they must be redefined by user of this code
struct extension_id *add_ten_ext = NULL;
struct extension_id *add_zero_ext = NULL;

// pack (serialize) a given number as an integer
// copies 4-byte representation of given integer to a buffer in expected
// serialized output type, `extension_data`
//
// used in `ext_add_ten` & `ext_add_uid`
#ifndef LC_ALLOW_EXCHANGE_FD
struct extension_data ext_add_int_to_arg(int num)
#else
struct extension_data ext_add_int_to_arg(int num, int fd)
#endif // ndef LC_ALLOW_EXCHANGE_FD
{
  // create empty result value
  struct extension_data result;
  // declare size of buffer to be memory size of int
  result.bufc = sizeof(num);
  // copy 4-byte repr of number to buffer on result
  memcpy(result.buf, &num, sizeof(num));
#ifdef LC_ALLOW_EXCHANGE_FD
  result.fdc = 1;
  printf("(%d) ext_add_int_to_arg fd=%d\n", getpid(), fd);
  result.fd[0] = fd;
#endif // LC_ALLOW_EXCHANGE_FD
  // return result object
  return result;
}

// unpack (deserialize) a number from a given argument
// simply decodes a given buffer of 4-bytes, then returns the value
//
// used in `ext_add_ten` & `ext_add_uid`
#ifndef LC_ALLOW_EXCHANGE_FD
int ext_add_int_from_arg(struct extension_data data)
#else
int ext_add_int_from_arg(struct extension_data data, int *fd)
#endif // ndef LC_ALLOW_EXCHANGE_FD
{
  // declare null int to store encoded int
  int result;
  // use memcpy to read buffer from given data as an integer
  // & store in result
  memcpy(&result, data.buf, sizeof(result));
#ifdef LC_ALLOW_EXCHANGE_FD
  // FIXME assert result.fdc == 1;
  *fd = data.fd[0];
#endif // LC_ALLOW_EXCHANGE_FD
  // return value now populated from buffer
  return result;
}

// add ten to a given number
// shows pattern we'll see across other ext_* functions:
// 1. declare a file descriptor for logging
// 2. deserialize argument data
// 3. perform computation w/ output of (2)
// 4. serialize value to return
// 5. return output of (4)
struct extension_data ext_add_ten(struct extension_data data) {
  // 1. declare a file descriptor for logging
  int fd = -1;
#ifndef LC_ALLOW_EXCHANGE_FD
  fd = STDOUT_FILENO;
  // unpack the integer given in `data`
  // 2. deserialize argument data
  int num = ext_add_int_from_arg(data);
#else
  int num = ext_add_int_from_arg(data, &fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD
  // log debugging info
  dprintf(fd, "ext_add_ten() on %s. uid=%d\n", compart_name(), getuid());

  // add ten to unpacked value
  // 3. perform computation w/ output of (2)
  num = num + 10;
#ifndef LC_ALLOW_EXCHANGE_FD
  // return value, now modified, by repacking (i.e. serializing) to
  // `extension_data` type
  // 4. serialize value to return
  // 5. return output of (4)
  return ext_add_int_to_arg(num);
#else
  return ext_add_int_to_arg(num, fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD
}

// add current uid number to a given number
struct extension_data ext_add_uid(struct extension_data data) {
  // 1. declare a file descriptor for logging
  int fd = -1;
#ifndef LC_ALLOW_EXCHANGE_FD
  fd = STDOUT_FILENO;
  // 2. deserialize argument data
  int num = ext_add_int_from_arg(data);
#else
  int num = ext_add_int_from_arg(data, &fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD
  dprintf(fd, "ext_add_uid() on %s. uid=%d\n", compart_name(), getuid());

  // 3. perform computation w/ output of (2)
  num = num + getuid();
#ifndef LC_ALLOW_EXCHANGE_FD
  // 4. serialize value to return
  // 5. return output of (4)
  return ext_add_int_to_arg(num);
#else
  return ext_add_int_to_arg(num, fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD
}
