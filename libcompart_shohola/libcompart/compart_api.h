/*
Part of the "chopchop" project at the University of Pennsylvania, USA.
Authors: Ke Zhong, Henry Zhu, Zhilei Zheng, Nik Sultana. 2018, 2019.

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

#ifndef __LIBCOMPART_API
#define __LIBCOMPART_API


/* FIXME since client code might depend on the size of this value, but this library and client code might be compiled
       at different times, then it might help to allow the client to query this value at runtime to check if the
       library's been compiled with parameters that are agreeable to the client.*/
#ifndef EXT_ARG_BUF_SIZE
#define EXT_ARG_BUF_SIZE 512
/* EXT_ARG_BUF_SIZE */
#endif

#ifndef MSG_BUF_SIZE
#define MSG_BUF_SIZE 100
/* MSG_BUF_SIZE */
#endif

#include <sys/types.h>

struct compart {
  const char * const name;
  uid_t uid;
  gid_t gid;
  const char * const path;
  void *comms;
  void (*preinit_fn)(void);
};

#define LIBCOMPART_VERSION_SHOHOLA

struct compart_config {
#ifdef INCLUDE_PID
  int CFG_INCLUDE_PID : 1;
/* INCLUDE_PID */
#endif
  int call_timeout;
  int activity_timeout;
  void (*on_call_timeout)(int compart_idx);
  void (*on_activity_timeout)();
  void (*on_termination)(int compart_idx);
  void (*on_comm_break)(int compart_idx);
  unsigned start_subs : 1; /* FIXME linked to drop_privs */
};

void default_on_termination(int compart_idx);
void default_on_comm_break(int compart_idx);

extern struct compart_config default_config;

struct extension_data {
  size_t bufc;
  char buf[EXT_ARG_BUF_SIZE];
#ifdef LC_ALLOW_EXCHANGE_FD
#if LC_ALLOW_EXCHANGE_FD < 1
#error "LC_ALLOW_EXCHANGE_FD must be >= 1"
#endif
/* LC_ALLOW_EXCHANGE_FD is a compile-time-specified array-size
 for an array that holds fds. These are populated by an external
 de/marshaller, as with the serialised data.
 Not all compost instantiation are required to support fd-
 transferring. */
  ssize_t fdc;
  int fd[LC_ALLOW_EXCHANGE_FD];
/* LC_ALLOW_EXCHANGE_FD */
#endif
};

#ifndef LC_ALLOW_EXCHANGE_FD
#define compart_check()
#else
#define compart_check() \
{ \
  if (LC_ALLOW_EXCHANGE_FD != compart_allowed_fd()) { \
    exit(99/*FIXME const*/); \
  } \
}
/* ndef LC_ALLOW_EXCHANGE_FD */
#endif

void compart_init(int no_comparts, struct compart comparts[], struct compart_config config);
void compart_start(const char * const new_compartment_name);
void compart_as(const char * const compartment_name);
struct extension_id;
struct extension_id *compart_register_fn(const char * const new_compartment_name, struct extension_data (*fn)(struct extension_data));
struct extension_data compart_call_fn(struct extension_id *, struct extension_data);
void compart_log(const char *buf, const size_t count);
int compart_allowed_fd(void);

/* __LIBCOMPART_API */
#endif
