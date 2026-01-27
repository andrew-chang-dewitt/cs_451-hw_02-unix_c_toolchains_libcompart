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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "hello_interface.h"

int main(int argc, char **argv) {
  (void)argc; // These are to pacify warnings about unused variables.
  (void)argv;

  compart_check();
  compart_init(NO_COMPARTS, comparts, default_config);
  // register pointer to function extension add ten to run on other
  // compartment, saving returned value (a function pointer) to `add_ten_ext`
  // (provided by/declared in `./hello_interface.h`)
  add_ten_ext = compart_register_fn("other compartment", &ext_add_ten);
  // same, but for extension add uid & third compartment
  add_zero_ext = compart_register_fn("third compartment", &ext_add_uid);
  compart_start("hello compartment");
  // since "hello compartment" is the first one started, it becomes "main"

  // get a handle on stdout
#ifndef LC_ALLOW_EXCHANGE_FD
  int fd = STDOUT_FILENO;
#else
  int fd = open("stdout", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
  printf("(%d) open fd=%d\n", getpid(), fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD

  int original_value = -5;
  // uses `dprintf` to specify target output stream
  dprintf(fd, "Old value: %d\n", original_value);

  // BEGIN inter-compartment function call
#ifndef LC_ALLOW_EXCHANGE_FD
  // setup/serialize args
  struct extension_data arg = ext_add_int_to_arg(original_value);
  // make inter-compartment function call & get output
  int new_value = ext_add_int_from_arg( // deserialize output from below
      compart_call_fn(add_ten_ext, arg));
#else

  // repeat to call again
  struct extension_data arg = ext_add_int_to_arg(original_value, fd);
  int new_value = ext_add_int_from_arg(compart_call_fn(add_ten_ext, arg), &fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD
  // END inter-compartment function call

  // BEGIN inter-compartment function call
#ifndef LC_ALLOW_EXCHANGE_FD
  arg = ext_add_int_to_arg(new_value);
  new_value = ext_add_int_from_arg(compart_call_fn(add_zero_ext, arg));
#else
  arg = ext_add_int_to_arg(new_value, fd);
  new_value = ext_add_int_from_arg(compart_call_fn(add_zero_ext, arg), &fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD
  // END inter-compartment function call

  dprintf(fd, "After adding 10+0 to old value\n");
  dprintf(fd, "New value: %d\n", new_value);

  close(fd);
  return EXIT_SUCCESS;
}
