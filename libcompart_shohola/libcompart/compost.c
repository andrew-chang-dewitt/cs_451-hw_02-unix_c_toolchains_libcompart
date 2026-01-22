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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h> /* FIXME for perror -- ideally remove this and use libcompart's LOG */
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include "compost.h"

struct compost {
  int tx_fd;
  int rx_fd;
#ifdef LC_ALLOW_EXCHANGE_FD
  int fdtx_fd;
  int fdrx_fd;
/* LC_ALLOW_EXCHANGE_FD */
#endif
};

static struct compost *cached_m2mon = NULL;
static struct compost *cached_mon2m = NULL;
static struct compost *cached_2m = NULL;
static struct compost *cached_m2 = NULL;

static int no_comparts = 0;
static struct compart *comparts = NULL;

void compost_init(int local_no_comparts, struct compart *local_comparts)
{
  /* FIXME ensure only called once.
     FIXME for other functions, check caller + recipient.
     FIXME close everything that isn't being used.
  */

  no_comparts = local_no_comparts;
  comparts = local_comparts;

  cached_m2mon = malloc(sizeof(*cached_m2mon));
  cached_mon2m = malloc(sizeof(*cached_mon2m));

  int ipc_pair[2];

  pipe(ipc_pair);
  cached_m2mon->tx_fd = ipc_pair[1];
  cached_mon2m->rx_fd = ipc_pair[0];

  pipe(ipc_pair);
  cached_m2mon->rx_fd = ipc_pair[0];
  cached_mon2m->tx_fd = ipc_pair[1];

  cached_2m = malloc(no_comparts * sizeof(struct compost));
  cached_m2 = malloc(no_comparts * sizeof(struct compost));

  int i = 0;
  for (i = 0; i < no_comparts; i++) {
    int ipc_pair[2];

    pipe(ipc_pair);
    cached_m2[i].rx_fd = ipc_pair[0];
    cached_2m[i].tx_fd = ipc_pair[1];

    pipe(ipc_pair);
    cached_m2[i].tx_fd = ipc_pair[1];
    cached_2m[i].rx_fd = ipc_pair[0];
  }

#ifdef LC_ALLOW_EXCHANGE_FD
  for (i = 0; i < no_comparts; i++) {
    int ipc_pair[2];

    socketpair(PF_UNIX, SOCK_DGRAM, 0, ipc_pair);
    cached_m2[i].fdrx_fd = ipc_pair[0];
    cached_2m[i].fdtx_fd = ipc_pair[1];

    socketpair(PF_UNIX, SOCK_DGRAM, 0, ipc_pair);
    cached_m2[i].fdtx_fd = ipc_pair[1];
    cached_2m[i].fdrx_fd = ipc_pair[0];
  }
/* LC_ALLOW_EXCHANGE_FD */
#endif
}

void compost_start(const char * const compartment_name)
{
  (void)compartment_name;
}

void compost_as(const char * const compartment_name)
{
  (void)compartment_name;
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
  return write(cp->tx_fd, buf, count);
}

ssize_t compost_recv(const struct compost *cp, void *buf, size_t count)
{
  return read(cp->rx_fd, buf, count);
}

void compost_close(struct compost *cp)
{
  close(cp->tx_fd);
  close(cp->rx_fd);
#ifdef LC_ALLOW_EXCHANGE_FD
  close(cp->fdtx_fd);
  close(cp->fdrx_fd);
/* LC_ALLOW_EXCHANGE_FD */
#endif
  free(cp);
}

#ifdef LC_ALLOW_EXCHANGE_FD
void compost_send_fd(const struct compost *cp, int fd)
{
  struct stat statbuf;
  if (fstat(fd, &statbuf)) {
    perror("composed_send_fd()");
    exit(1/*FIXME const*/);
  }

  char control[CMSG_SPACE(sizeof(int))];
  char buf = ' ';

  struct iovec iov;
  iov.iov_base = &buf;
  iov.iov_len = sizeof(buf);

  struct msghdr msg;
  msg.msg_name = NULL;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = control;
  msg.msg_controllen = sizeof(control);

  struct cmsghdr *cmptr;
  cmptr = CMSG_FIRSTHDR(&msg);
  cmptr->cmsg_len = CMSG_LEN(sizeof(int));
  cmptr->cmsg_level = SOL_SOCKET;
  cmptr->cmsg_type = SCM_RIGHTS;
  *((int*)CMSG_DATA(cmptr)) = fd;

  int len = sendmsg(cp->fdtx_fd, &msg, 0);
  if (len <= 0) {
    perror("composed_send_fd()");
    exit(1/*FIXME const*/);
  }
}

void compost_recv_fd(const struct compost *cp, int *fd)
{
  char control[CMSG_SPACE(sizeof(int))];
  char buf[1];

  struct iovec iov;
  iov.iov_base = buf;
  iov.iov_len = sizeof(buf);

  struct msghdr msg;
  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = control;
  msg.msg_controllen = sizeof(control);

  int len = recvmsg(cp->fdrx_fd, &msg, 0);

  if (len <= 0) {
    perror("composed_recv_fd()");
    *fd = -1;
    exit(1/*FIXME const*/);
  }

  struct cmsghdr *cmptr = CMSG_FIRSTHDR(&msg);
  if (NULL == cmptr || cmptr->cmsg_len != CMSG_LEN(sizeof(int)) ||
      cmptr->cmsg_level != SOL_SOCKET || cmptr->cmsg_type != SCM_RIGHTS) {
    *fd = -1;
    exit(1/*FIXME const*/);
  }
  *fd = *(int*)CMSG_DATA(cmptr);
}
/* LC_ALLOW_EXCHANGE_FD */
#endif
