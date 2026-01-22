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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "compost.h"
#include "combin_tcp.h"

#define BACKLOG 2

struct compost {
  int fd_tx;
  int fd_rx;
  int sock;
};


void free_combin(struct combin *combin)
{
  free((char *)combin->address);
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

int connect_combin(struct combin *combin)
{
  int fd;
  struct sockaddr_in servaddr;

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    /* FIXME report error */
    exit(1);
  }

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(combin->address);
  servaddr.sin_port = htons(combin->port);

  if (connect(fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
    /* FIXME report error */
    exit(1);
  }

  return fd;
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
  cached_mon2m->sock = -1;

  pipe(raw_pipe);
  cached_m2mon->fd_rx = raw_pipe[0];
  cached_mon2m->fd_tx = raw_pipe[1];
  cached_m2mon->sock = -1;

  int i = 0;
  for (i = 0; i < no_comparts; i++) {
    if (0 != strcmp(comparts[i].name, compartment_name)) {
      struct combin* combin = (struct combin*)(comparts[i].comms);

      char *buf = malloc(MSG_BUF_SIZE);
      sprintf(buf, "<COMBIN> %s connecting to %s:%d", compartment_name, combin->address, combin->port);
      compart_log(buf, strlen(buf));
      free(buf);

      cached_m2[i].fd_tx = -1;
      cached_m2[i].fd_rx = -1;
      cached_m2[i].sock = connect_combin(combin); /* FIXME check that not -1 */
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

  char *buf = malloc(MSG_BUF_SIZE);
  sprintf(buf, "<COMBIN> %s listening on %s:%d", compartment_name, combin->address, combin->port);
  compart_log(buf, strlen(buf));
  free(buf);

  struct sockaddr_in servaddr, clientaddr;
  int sockfd;
  unsigned clientaddr_size;
  bzero(&servaddr, sizeof(servaddr));

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    /* FIXME report error */
    exit(1);
  }

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(combin->address);
  servaddr.sin_port = htons(combin->port);

  if ((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0) {
    /* FIXME report error */
    exit(1);
  }

  if ((listen(sockfd, BACKLOG)) != 0) {
    /* FIXME report error */
    exit(1);
  }

  clientaddr_size = sizeof(clientaddr);
  cached_2m[idx].sock = accept(sockfd, (struct sockaddr*)&clientaddr, &clientaddr_size);
  if (cached_2m[idx].sock < 0) {
    /* FIXME report error */
    exit(1);
  }

  cached_2m[idx].fd_rx = -1;
  cached_2m[idx].fd_tx = -1;
  /* FIXME not closing sockfd */
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
  int fd = cp->sock;
  if (cp->sock == -1) {
    fd = cp->fd_tx;
  }
  return write(fd, buf, count);
}

ssize_t compost_recv(const struct compost *cp, void *buf, size_t count)
{
  int fd = cp->sock;
  if (cp->sock == -1) {
    fd = cp->fd_rx;
  }
  return read(fd, buf, count);
}

void compost_close(struct compost *cp)
{
  if (cp->sock == -1) {
    close(cp->fd_tx);
    close(cp->fd_rx);
  } else {
    close(cp->sock);
  }
  free(cp);
}
