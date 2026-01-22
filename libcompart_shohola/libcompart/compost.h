/*
Part of the "chopchop" project at the University of Pennsylvania, USA.
Authors: Nik Sultana. 2019.

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

#ifndef __LIBCOMPOST_API
#define __LIBCOMPOST_API

#include "compart_api.h"

struct compost;

void compost_init(int local_no_comparts, struct compart *comparts);
void compost_start(const char * const compartment_name);
void compost_as(const char * const compartment_name);
const struct compost *compost_m2mon(void);
const struct compost *compost_mon2m(void);
const struct compost *compost_m2(int compart_idx);
const struct compost *compost_2m(int compart_idx);
ssize_t compost_send(const struct compost *cp, const void *buf, size_t count);
ssize_t compost_recv(const struct compost *cp, void *buf, size_t count);
#ifdef LC_ALLOW_EXCHANGE_FD
void compost_send_fd(const struct compost *cp, int fd);
void compost_recv_fd(const struct compost *cp, int *fd);
/* LC_ALLOW_EXCHANGE_FD */
#endif
void compost_close(struct compost *cp);

/* __LIBCOMPOST_API */
#endif
