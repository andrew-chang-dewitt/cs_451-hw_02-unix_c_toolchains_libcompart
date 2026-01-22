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

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "compost.h"
#include "combin.h"

struct compost {
  int fd_tx;
  int fd_rx;
};

const char * const suffix_2m_w = "_2m_w";
const char * const suffix_2m_r = "_2m_r";
const char * const suffix_m2_w = "_m2_w";
const char * const suffix_m2_r = "_m2_r";

struct combin *derive_simplex_path(struct combin *combin, const char * const suffix)
{
  struct combin *result = malloc(sizeof(struct combin));
  char *path = malloc(strlen(combin->path) + strlen(suffix) + 1);
  memcpy(path, combin->path, strlen(combin->path));
  memcpy(path + strlen(combin->path), suffix, strlen(suffix));
  result->path = path;
  return result;
}

void free_combin(struct combin *combin)
{
  free((char *)combin->path);
  free(combin);
}

static struct compost *cached_m2mon = NULL;
static struct compost *cached_mon2m = NULL;
static struct compost *cached_2m = NULL;
static struct compost *cached_m2 = NULL;

static int no_comparts = 0;
static struct compart *comparts = NULL;

void compost_init(int local_no_comparts, struct compart *local_comparts)
{
  no_comparts = local_no_comparts;
  comparts = local_comparts;

  cached_2m = malloc(no_comparts * sizeof(struct compost));
  cached_m2 = malloc(no_comparts * sizeof(struct compost));
}

void compost_start(const char * const compartment_name)
{
  (void)compartment_name;

  cached_m2mon = malloc(sizeof(*cached_m2mon));
  cached_mon2m = malloc(sizeof(*cached_mon2m));

  int raw_pipe[2];

  pipe(raw_pipe);
  cached_m2mon->fd_tx = raw_pipe[1];
  cached_mon2m->fd_rx = raw_pipe[0];

  pipe(raw_pipe);
  cached_m2mon->fd_rx = raw_pipe[0];
  cached_mon2m->fd_tx = raw_pipe[1];

  int i = 0;
  for (i = 0; i < no_comparts; i++) {
    if (0 != strcmp(comparts[i].name, compartment_name)) {
      struct combin* combin = (struct combin*)(comparts[i].comms);

#ifndef PITCHFORK_NOLOG
      char *buf = malloc(MSG_BUF_SIZE);
      struct combin *combin_m2_w = derive_simplex_path(combin, suffix_m2_w); /* FIXME check that file exists */
      sprintf(buf, "<COMBIN> %s opening %s", compartment_name, combin_m2_w->path);
      compart_log(buf, strlen(buf));

      struct combin *combin_m2_r = derive_simplex_path(combin, suffix_m2_r); /* FIXME check that file exists */
      sprintf(buf, "<COMBIN> %s opening %s", compartment_name, combin_m2_r->path);
      compart_log(buf, strlen(buf));
      free(buf);
#endif

      cached_m2[i].fd_tx = open(combin_m2_w->path, O_WRONLY);
      cached_m2[i].fd_rx = open(combin_m2_r->path, O_RDONLY);
      free_combin(combin_m2_w);
      free_combin(combin_m2_r);
    }
  }
}

void compost_as(const char * const compartment_name)
{
  (void)compartment_name;

  int idx = -1;
  int i = 0;
  for (i = 0; i < no_comparts; i++) {
    if (0 == strcmp(comparts[i].name, compartment_name)) {
      idx = i;
      break;
    }
  }

  struct combin* combin = (struct combin*)(comparts[idx].comms);

#ifndef PITCHFORK_NOLOG
  char *buf = malloc(MSG_BUF_SIZE);
  struct combin *combin_2m_w = derive_simplex_path(combin, suffix_2m_w); /* FIXME check that file exists */
  sprintf(buf, "<COMBIN> %s opening %s", compartment_name, combin_2m_w->path);
  compart_log(buf, strlen(buf));

  struct combin *combin_2m_r = derive_simplex_path(combin, suffix_2m_r); /* FIXME check that file exists */
  sprintf(buf, "<COMBIN> %s opening %s", compartment_name, combin_2m_r->path);
  compart_log(buf, strlen(buf));
  free(buf);
#endif

  cached_2m[idx].fd_rx = open(combin_2m_r->path, O_RDONLY);
  cached_2m[idx].fd_tx = open(combin_2m_w->path, O_WRONLY);
  free_combin(combin_2m_w);
  free_combin(combin_2m_r);
}

const struct compost *compost_m2mon(void)
{
  return cached_m2mon;
}

const struct compost *compost_mon2m(void)
{
  return cached_mon2m;
}

const struct compost *compost_m2(int compart_idx)
{
  return &(cached_m2[compart_idx]);
}

const struct compost *compost_2m(int compart_idx)
{
  return &(cached_2m[compart_idx]);
}

ssize_t compost_send(const struct compost *cp, const void *buf, size_t count)
{
  return write(cp->fd_tx, buf, count);
}

ssize_t compost_recv(const struct compost *cp, void *buf, size_t count)
{
  return read(cp->fd_rx, buf, count);
}

void compost_close(struct compost *cp)
{
  close(cp->fd_tx);
  close(cp->fd_rx);
  free(cp);
}

#ifdef LC_ALLOW_EXCHANGE_FD
#error "-DLC_ALLOW_EXCHANGE_FD is not supported for combin.c"
void compost_send_fd(const struct compost *cp, int fd)
{
  cp;
  fd;
  fprintf(stderr, "(%s) %s is not supported for this instance\n", __FILE__, "compost_send_fd()");
  exit(EXIT_FAILURE);
}

void compost_recv_fd(const struct compost *cp, int *fd)
{
  cp;
  fd;
  fprintf(stderr, "(%s) %s is not supported for this instance\n", __FILE__, "compost_recv_fd()");
  exit(EXIT_FAILURE);
}
#endif // LC_ALLOW_EXCHANGE_FD
