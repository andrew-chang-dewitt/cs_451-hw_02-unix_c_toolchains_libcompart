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
#include <unistd.h>
#include <sys/types.h>

#include "hello_interface.h"

struct combin combins[NO_COMPARTS] =
  {{.path = "/home/nik/chopchop/compartmenting/hello_compartment/other"},
   {.path = "/home/nik/chopchop/compartmenting/hello_compartment/third"}};
struct compart comparts[NO_COMPARTS] =
  {{.name = "hello compartment", .uid = 65534, .gid = 65534, .path = "/tmp", .comms = NULL},
   {.name = "other compartment", .uid = 65534, .gid = 65534, .path = "/tmp", .comms = &combins[0]},
   {.name = "third compartment", .uid = 0, .gid = 0, .path = "/tmp", .comms = &combins[1]}};

struct extension_id *add_ten_ext = NULL;
struct extension_id *add_zero_ext = NULL;

#ifndef LC_ALLOW_EXCHANGE_FD
struct extension_data ext_add_int_to_arg(int num)
#else
struct extension_data ext_add_int_to_arg(int num, int fd)
#endif // ndef LC_ALLOW_EXCHANGE_FD
{
  struct extension_data result;
  result.bufc = sizeof(num);
  memcpy(result.buf, &num, sizeof(num));
#ifdef LC_ALLOW_EXCHANGE_FD
  result.fdc = 1;
  printf("(%d) ext_add_int_to_arg fd=%d\n", getpid(), fd);
  result.fd[0] = fd;
#endif // LC_ALLOW_EXCHANGE_FD
  return result;
}

#ifndef LC_ALLOW_EXCHANGE_FD
int ext_add_int_from_arg(struct extension_data data)
#else
int ext_add_int_from_arg(struct extension_data data, int *fd)
#endif // ndef LC_ALLOW_EXCHANGE_FD
{
  int result;
  memcpy(&result, data.buf, sizeof(result));
#ifdef LC_ALLOW_EXCHANGE_FD
  // FIXME assert result.fdc == 1;
  *fd = data.fd[0];
#endif // LC_ALLOW_EXCHANGE_FD
  return result;
}

struct extension_data ext_add_ten(struct extension_data data)
{
  int fd = -1;
#ifndef LC_ALLOW_EXCHANGE_FD
  fd = STDOUT_FILENO;
  int num = ext_add_int_from_arg(data);
#else
  int num = ext_add_int_from_arg(data, &fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD
  dprintf(fd, "ext_add_ten() on %s. uid=%d\n", compart_name(), getuid());

  num = num + 10;
#ifndef LC_ALLOW_EXCHANGE_FD
  return ext_add_int_to_arg(num);
#else
  return ext_add_int_to_arg(num, fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD
}

struct extension_data ext_add_uid(struct extension_data data)
{
  int fd = -1;
#ifndef LC_ALLOW_EXCHANGE_FD
  fd = STDOUT_FILENO;
  int num = ext_add_int_from_arg(data);
#else
  int num = ext_add_int_from_arg(data, &fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD
  dprintf(fd, "ext_add_uid() on %s. uid=%d\n", compart_name(), getuid());

  num = num + getuid();
#ifndef LC_ALLOW_EXCHANGE_FD
  return ext_add_int_to_arg(num);
#else
  return ext_add_int_to_arg(num, fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD
}
