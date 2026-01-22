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

#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifndef PITCHFORK_DBGSTDOUT
#include <sys/stat.h>
#include <fcntl.h>
/* PITCHFORK_DBGSTDOUT */
#endif

#include "compart_base.h"
#include "compost.h"

#ifndef GENERAL_ARG_BUF_SIZE
#define GENERAL_ARG_BUF_SIZE 1024
/* GENERAL_ARG_BUF_SIZE */
#endif

#ifndef PITCHFORK_LOG_ENVAR
#define PITCHFORK_LOG_ENVAR "PITCHFORK_LOG"
/* PITCHFORK_LOG_ENVAR */
#endif

/* Exit codes */
#define EXIT_OK 0
#define EXIT_NO_LOGPATH 1
#define EXIT_INVALID_LOGPATH 2
#define EXIT_WRITE_ERROR_LOGPATH 3
#define EXIT_FAILED_CHDIR 10
#define EXIT_FAILED_CHROOT 4
#define EXIT_FAILED_SETGID 5
#define EXIT_FAILED_SETUID 6
#define EXIT_OK_CHILD_TOO 7
#define EXIT_SERVER_INSTRUCTED_CLIENT 8
/* FIXME EXIT_NEGATIVEONE is a generic error code */
#define EXIT_NEGATIVEONE 9
#define EXIT_SIGACTION 12
#define EXIT_NO_LOG_CHAN 13
#define EXIT_MULTIPLE_COMPART_AS 14
#define EXIT_CANNOT_COMPART_AS 15
#define EXIT_COMPART_CHAN_BROKE 16
#define EXIT_LOGGED_OFFSET 50

/* error checking */
#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

/* FIXME CHECK_NEGATIVE_ONE doesn't respond to client with terminate=1 before exiting. */
#define CHECK_NEGATIVE_ONE(call) \
  do { \
    int result = (call); \
    if(result == -1) { \
      perror("Error " __FILE__ ":" STRINGIFY(__LINE__));\
      exit(EXIT_NEGATIVEONE); \
    } \
  } while(0) \

/* We only set exit_code once, to indicate the first error that
   resulted in the exit.
*/
#define SET_EXIT_CODE(code) \
  { \
    if (0 == exit_code) {\
      exit_code = code;\
    } \
  }

#ifndef PITCHFORK_NOLOG
#define LOG(code, msg) \
  { \
    char msg_buf[MSG_BUF_SIZE]; \
    if (code <= 0) \
      snprintf(msg_buf, MSG_BUF_SIZE - 1, "<?> %s", msg); \
    else \
      snprintf(msg_buf, MSG_BUF_SIZE - 1, "<%d> %s", code, msg); \
    compart_log(msg_buf, strlen(msg_buf)); \
  }
#else
/* def PITCHFORK_NOLOG */
#define LOG(code, msg) ;
/* PITCHFORK_NOLOG */
#endif

int libcompart_general_arg_buf_size(void) {
  return GENERAL_ARG_BUF_SIZE;
}

int libcompart_msg_buf_size(void) {
  return MSG_BUF_SIZE;
}

const char* libcompart_pitchfork_log_envar(void) {
  return PITCHFORK_LOG_ENVAR;
}

int libcompart_ext_arg_buf_size(void) {
  return EXT_ARG_BUF_SIZE;
}

typedef enum {
  NO_REQ,
  CALL_FN,
  RET_FN,
} RequestType;

typedef struct
{
/* FIXME include tid too? */
#ifdef INCLUDE_PID
  pid_t pid;
/* INCLUDE_PID */
#endif
  RequestType type;
  char data[GENERAL_ARG_BUF_SIZE];
} Request;

typedef struct
{
  int result;
  int err_no;
  char terminate;
  char data[GENERAL_ARG_BUF_SIZE];
} Response;

static Request empty_request =
  {
  #ifdef INCLUDE_PID
    .pid = 0,
  /* INCLUDE_PID */
  #endif
    .type = NO_REQ,
    .data = ""
  };

static Response empty_response =
  {
    .result = 0,
    .err_no = 0,
    .terminate = 0,
    .data = ""
  };

static const struct compost *channel = NULL;

static bool am_i_monitor = true;
static bool am_i_main = false;
static int main_compart_idx = -1;
static const int monitor_compart_idx = -1;
static const char* compartment_name = NULL;

static const char* const monitor_name = "(monitor)";
static const char *comparting_as = NULL;

static FILE* log_fd = NULL;
static int started = 0;
static int initialised = 0;

static int no_comparts = 0; /* As declared by the user. Dimensions "comparts". */
static struct compart *comparts = NULL;

struct compart_metadata {
  const struct compost *channel;
  int num_registrations;
  pid_t pid; /* FIXME make optional, in case unavailable */
};
static struct compart_metadata *comparts_metadata = NULL;

static int compart_count = 0; /* As counted by compart_start */
static bool main_compart_dead = false;

static struct compart_config my_config;

static int my_compart_idx = -1;

void default_on_termination(int compart_idx)
{
#ifndef PITCHFORK_NOLOG
  if (!main_compart_dead && compart_idx != main_compart_idx) {
      const char termination_s[] = "crashed compartment: ";
      char *msg = malloc(strlen(termination_s) + strlen(comparts[compart_idx].name) + 1);
      strcpy(msg, termination_s);
      strcpy(msg + strlen(termination_s), comparts[compart_idx].name);
      LOG(61, msg)
      free(msg);
  } else {
      const char termination_s[] = "terminated: ";
      char *msg = malloc(strlen(termination_s) + strlen(comparts[compart_idx].name) + 1);
      strcpy(msg, termination_s);
      strcpy(msg + strlen(termination_s), comparts[compart_idx].name);
      LOG(50, msg)
      free(msg);
  }
/* ndef PITCHFORK_NOLOG */
#endif
  main_compart_dead = true;

  /* Don't exit if there are still children */
  if (0 == --compart_count) {
      LOG(37, "all children dead");
      exit(EXIT_LOGGED_OFFSET + 37);
  }

  int i = 0;
  for (i = 0; i < no_comparts; ++i) {
      if (compart_idx == i) {
          comparts_metadata[i].pid = 0;
      } else if (0 != comparts_metadata[i].pid) {
          kill(comparts_metadata[i].pid, SIGKILL); /* FIXME check result */
      }
  }
}

void default_on_comm_break(int compart_idx)
{
  (void)compart_idx; /* This is to pacify warnings about unused parameters. */
#ifndef PITCHFORK_NOLOG
  char msg[MSG_BUF_SIZE];
  const char *target_name = NULL;
  if (-1 == compart_idx) {
    if (NULL == comparting_as) {
      target_name = monitor_name;
    } else {
      target_name = "(main)"/*FIXME const*/;
    }
  } else {
    target_name = comparts[compart_idx].name;
  }
  snprintf(msg, MSG_BUF_SIZE, "communication break: between %s (pid %d) and %s", compartment_name, getpid(), target_name);
  LOG(38, msg)
/* ndef PITCHFORK_NOLOG */
#endif
  if (NULL == comparting_as) {
    while(1); /* Wait to be killed by the monitor */
  } else {
    exit(EXIT_COMPART_CHAN_BROKE);
  }
}

struct compart_config default_config = {
#ifdef INCLUDE_PID
  .CFG_INCLUDE_PID = 0,
/* INCLUDE_PID */
#endif
  .call_timeout = -1,
  .activity_timeout = -1,
  .on_call_timeout = NULL,
  .on_activity_timeout = NULL,
  .on_termination = &default_on_termination,
  .on_comm_break = &default_on_comm_break,
  .start_subs = 1
};

struct extension_id {
  int extension_idx;
};
struct extension_info {
  int compart_idx;
  struct extension_data (*fn)(struct extension_data);
};

#ifndef MAX_COMPART_REGS
#define MAX_COMPART_REGS 10
/* MAX_COMPART_REGS */
#endif
static unsigned current_compart_reg = 0;
static struct extension_info registrations[MAX_COMPART_REGS];

static void call_fn(struct extension_id *eid, struct extension_data *arg, struct extension_data *resp)
{
  if (1 != started) {
    LOG(21, "call_fn() on unstarted monitor")
    exit(21 + EXIT_LOGGED_OFFSET);
  }

  if (NULL == registrations[eid->extension_idx].fn) {
    LOG(64, "call_fn() attempting to call NULL")
    exit(64 + EXIT_LOGGED_OFFSET);
  }

  struct extension_data (*fn)(struct extension_data) = registrations[eid->extension_idx].fn;
  *resp = fn(*arg);
}

enum Mode {Idle, Call};
static enum Mode monitor_mode = Idle;
static int called_compart_idx = -1;

static void sigchld_handler (int sig)
{
    switch (sig) {
    case SIGCHLD:
        {
            int status;
            pid_t pid;
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                int idx = -1;
                int i = 0;
                for (i = 0; i < no_comparts; ++i) {
                    if (pid == comparts_metadata[i].pid) {
                        idx = i;
                        break;
                    }
                }

                if (-1 == idx) {
                    LOG(9, "unknown compartment was terminated");
                }

                my_config.on_termination(idx);
            }
            break;
        }
    case SIGALRM:
        {
            if (!am_i_monitor) {
                if (my_config.activity_timeout > -1 && NULL != my_config.on_activity_timeout) {
                    my_config.on_activity_timeout();
                    alarm(my_config.activity_timeout);
                }
            } else {
                if (Idle == monitor_mode) {
                    if (my_config.activity_timeout > -1 && NULL != my_config.on_activity_timeout) {
                        my_config.on_activity_timeout();
                        alarm(my_config.activity_timeout);
                    }
                } else if (Call == monitor_mode) {
                    /* FIXME assume called_compart_idx > -1 */
                    my_config.on_call_timeout(called_compart_idx);
                    monitor_mode = Idle;
                    if (my_config.activity_timeout > -1 && NULL != my_config.on_activity_timeout) {
                        alarm(my_config.activity_timeout);
                    }
                } else {
                    LOG(54, "unknown mode")
                    exit(54 + EXIT_LOGGED_OFFSET);
                }
            }
            break;
        }
    default:
        LOG(53, "unexpected signal received")
        exit(53 + EXIT_LOGGED_OFFSET);
    }
}

static void parent_loop(void)
{
  if (1 != started) {
    LOG(25, "parent_loop() on unstarted monitor")
    exit(25 + EXIT_LOGGED_OFFSET);
  }

  struct sigaction act;
  memset (&act, 0, sizeof(act));
  act.sa_handler = sigchld_handler;
  if (sigaction(SIGCHLD, &act, 0)) {
    exit(EXIT_SIGACTION);
  }
  if (sigaction(SIGALRM, &act, 0)) {
    exit(EXIT_SIGACTION);
  }

  if (my_config.activity_timeout == 0 && NULL != my_config.on_activity_timeout) {
    my_config.on_activity_timeout();
  } else if (my_config.activity_timeout > 0 && NULL != my_config.on_activity_timeout) {
    alarm(my_config.activity_timeout);
  }

  while(1) {
    Request request = empty_request;
    int result = compost_recv(channel, &request, sizeof(Request));
    if (0 >= result)/*FIXME check if this is the right range*/ {
      my_config.on_comm_break(main_compart_idx);
    }

    Response response = empty_response;

    struct extension_data arg;

    int exit_code = 0;

    /* FIXME check pid and tid? */

    switch(request.type)
    {
      case RET_FN:
        {
          monitor_mode = Idle;
          if (my_config.activity_timeout > -1 && NULL != my_config.on_activity_timeout) {
              alarm(my_config.activity_timeout);
          }

          struct extension_id eid;
          memcpy(&eid, request.data, sizeof(eid));
          /* FIXME assume (am_i_monitor) */

#ifndef PITCHFORK_NOLOG
          const char from_s[] = "(monitor) return from ";
          char *msg = malloc(strlen(from_s) + strlen(comparts[registrations[eid.extension_idx].compart_idx].name) + 1);
          strcpy(msg, from_s);
          strcpy(msg + strlen(from_s), comparts[registrations[eid.extension_idx].compart_idx].name);
          LOG(46, msg)
          free(msg);
/* ndef PITCHFORK_NOLOG */
#endif
          break;
        }
      case CALL_FN:
        {
          struct extension_id eid;
          struct extension_data resp;
          memcpy(&eid, request.data, sizeof(eid));
          memcpy(&arg, request.data + sizeof(eid), sizeof(arg));

          /* FIXME check if function has been registered to this compartment. */

          if (!am_i_monitor) {
#ifdef LC_ALLOW_EXCHANGE_FD
            /* FIXME assert resp.fdc < LC_ALLOW_EXCHANGE_FD */
            int i = 0;
            for (i = 0; i < arg.fdc; i++) {
              compost_recv_fd(channel, &(arg.fd[i]));
            }
/* LC_ALLOW_EXCHANGE_FD */
#endif
            call_fn(&eid, &arg, &resp);
            response.result = 0; /* FIXME const -- field is redundant. */
            response.err_no = errno;
            if (0 != response.result) {
              LOG(23, "call_fn failed")
              SET_EXIT_CODE(23 + EXIT_LOGGED_OFFSET)
            } else {
              memcpy(response.data, &resp, sizeof(resp));
            }
          } else {
            called_compart_idx = registrations[eid.extension_idx].compart_idx;
            monitor_mode = Call;
            alarm(my_config.call_timeout);

#ifndef PITCHFORK_NOLOG
            const char to_s[] = "(monitor) call to ";
            char *msg = malloc(strlen(to_s) + strlen(comparts[registrations[eid.extension_idx].compart_idx].name) + 1);
            strcpy(msg, to_s);
            strcpy(msg + strlen(to_s), comparts[registrations[eid.extension_idx].compart_idx].name);
            LOG(45, msg)
            free(msg);
/* ndef PITCHFORK_NOLOG */
#endif
          }
          break;
        }
      default:
        {
          LOG(4, "Invalid request from child")
          SET_EXIT_CODE(4 + EXIT_LOGGED_OFFSET)
        }
    }

    if (0 != exit_code) {
      response.terminate = 1;
    }

    result = compost_send(channel, &response, sizeof(Response));
    if (0 >= result)/*FIXME check if this is the right range*/ {
      my_config.on_comm_break(main_compart_idx);
    }
#ifdef LC_ALLOW_EXCHANGE_FD
    if (!am_i_monitor) {
      /* FIXME assert resp.fdc < LC_ALLOW_EXCHANGE_FD */
      int i = 0;
      for (i = 0; i < arg.fdc; i++) {
        compost_send_fd(channel, arg.fd[i]);
      }
    }
/* LC_ALLOW_EXCHANGE_FD */
#endif

    if (0 != exit_code) {
      exit(exit_code);
    }
  }
}

static bool privileged(void)
{
  if (getuid() != 0/*NOTE const -- uid for root*/) {
    return false;
  }
  return true;
}

static void drop_privs(int no_comparts, struct compart comparts[])
{
  if (!privileged()) {
    LOG(32, "drop_privs() called in non-privileged process")
    exit(32 + EXIT_LOGGED_OFFSET);
  }

  if (NULL == compartment_name) {
    LOG(33, "NULL == compartment_name")
    exit(33 + EXIT_LOGGED_OFFSET);
  }

  struct compart* compart = NULL;
  int i = 0;
  for (i = 0; i < no_comparts; ++i) {
    if (0 == strcmp(compartment_name, comparts[i].name)) {
      compart = &(comparts[i]);
    }
  }

  if (NULL == compart) {
    LOG(34, "NULL == compart")
    exit(34 + EXIT_LOGGED_OFFSET);
  }

  if (NULL != compart->path) {
    if (chdir(compart->path) != 0) {
#ifndef PITCHFORK_NOLOG
      const char* chdir_err = strerror(errno);
      const char* separator = ": ";
      char *msg = malloc(strlen(chdir_err) + strlen(separator) + strlen(compart->path) + 1);
      strcpy(msg, chdir_err);
      strcpy(msg + strlen(chdir_err), separator);
      strcpy(msg + strlen(chdir_err) + strlen(separator), compart->path);
      LOG(9, msg)
      free(msg);
/* ndef PITCHFORK_NOLOG */
#endif
      exit(EXIT_FAILED_CHDIR);
    }

    if (chroot(compart->path) != 0) {
      LOG(6, strerror(errno))
      exit(EXIT_FAILED_CHROOT);
    }
  }

  if (setgid(compart->gid) != 0) {
    LOG(7, strerror(errno))
    exit(EXIT_FAILED_SETGID);
  }

  if (setuid(compart->uid) != 0) {
    LOG(8, strerror(errno))
    exit(EXIT_FAILED_SETGID);
  }
}

static void split_parent_child(const char * const new_compartment_name, int no_comparts, struct compart comparts[])
{
    if (1 != started) {
      LOG(13, "split_parent_child() on unstarted monitor")
      exit(13 + EXIT_LOGGED_OFFSET);
    }

    pid_t pid = fork(); /* FIXME check success of this */

    if (!pid) /* Child */
    {
      am_i_monitor = false;
      compart_count = -1; /* Children don't track this info. */
      compartment_name = strdup(new_compartment_name);
      am_i_main = true;

      int found = 0;
      int i = 0;
      for (i = 0; i < no_comparts; ++i) {
        if (0 == strcmp(new_compartment_name, comparts[i].name)) {
          my_compart_idx = i;
          main_compart_idx = i;
          found++;
        }
      }

      if (0 == found) {
#ifndef PITCHFORK_NOLOG
        const char msg_prefix[] = "split_parent_child() found 0 instances of sought compartment: ";
        char *msg = malloc(strlen(msg_prefix) + strlen(new_compartment_name) + 1);
        strcpy(msg, msg_prefix);
        strcpy(msg + strlen(msg_prefix), new_compartment_name);
        LOG(57, msg)
        free(msg);
/* ndef PITCHFORK_NOLOG */
#endif
        exit(57 + EXIT_LOGGED_OFFSET);
      } else if (found > 1) {
#ifndef PITCHFORK_NOLOG
        const char msg_prefix[] = "split_parent_child() found multiple instances of sought compartment: ";
        char *msg = malloc(strlen(msg_prefix) + strlen(new_compartment_name) + 1);
        strcpy(msg, msg_prefix);
        strcpy(msg + strlen(msg_prefix), new_compartment_name);
        LOG(58, msg)
        free(msg);
/* ndef PITCHFORK_NOLOG */
#endif
        exit(58 + EXIT_LOGGED_OFFSET);
      }

      channel = compost_m2mon();

      if (my_config.start_subs == 1) {
        drop_privs(no_comparts, comparts);
      }

#ifndef PITCHFORK_NOLOG
      char msg[MSG_BUF_SIZE];
      snprintf(msg, MSG_BUF_SIZE, "starting %d %s", getpid(), new_compartment_name);
      LOG(11, msg)
/* ndef PITCHFORK_NOLOG */
#endif
    }
    else /* Parent */
    { 
      /* NOTE expect that (am_i_monitor == true); */
      am_i_monitor = true;
      am_i_main = false;
      my_compart_idx = -1;

      compart_count++;

#ifndef PITCHFORK_NOLOG
      char msg[MSG_BUF_SIZE];
      snprintf(msg, MSG_BUF_SIZE, "starting monitor %d %s", getpid(), compartment_name);
      LOG(62, msg)
/* ndef PITCHFORK_NOLOG */
#endif

      int found = 0;
      int i = 0;
      for (i = 0; i < no_comparts; ++i) {
        if (0 == strcmp(new_compartment_name, comparts[i].name)) {
          comparts_metadata[i].pid = pid;
          main_compart_idx = i;
          found++;
        }
      }

      /* FIXME DRY principle */
      if (0 == found) {
#ifndef PITCHFORK_NOLOG
        const char msg_prefix[] = "split_parent_child() found 0 instances of sought compartment: ";
        char *msg = malloc(strlen(msg_prefix) + strlen(new_compartment_name) + 1);
        strcpy(msg, msg_prefix);
        strcpy(msg + strlen(msg_prefix), new_compartment_name);
        LOG(48, msg)
        free(msg);
/* ndef PITCHFORK_NOLOG */
#endif
        exit(48 + EXIT_LOGGED_OFFSET);
      } else if (found > 1) {
#ifndef PITCHFORK_NOLOG
        const char msg_prefix[] = "split_parent_child() found multiple instances of sought compartment: ";
        char *msg = malloc(strlen(msg_prefix) + strlen(new_compartment_name) + 1);
        strcpy(msg, msg_prefix);
        strcpy(msg + strlen(msg_prefix), new_compartment_name);
        LOG(49, msg)
        free(msg);
/* ndef PITCHFORK_NOLOG */
#endif
        exit(49 + EXIT_LOGGED_OFFSET);
      } else {
        /* Call pre init */
        if(comparts[main_compart_idx].preinit_fn != NULL)
        {
            comparts[main_compart_idx].preinit_fn();
        }
      }

      channel = compost_mon2m();

      compartment_name = monitor_name;
      parent_loop();
    }
}

void compart_init(int local_no_comparts, struct compart local_comparts[], struct compart_config config)
{
    log_fd = stderr;
#ifndef PITCHFORK_NOLOG
#ifndef PITCHFORK_DBGSTDOUT
    char* log_path = getenv(PITCHFORK_LOG_ENVAR); /* FIXME use secure_getenv? */
    if (NULL == log_path) {
        fprintf(log_fd, "Must set environment variable %s since libcompart not compiled with PITCHFORK_DBGSTDOUT\n", PITCHFORK_LOG_ENVAR);
        exit(EXIT_NO_LOGPATH);
    } else {
        /* Wipe our variables from the environment. */
        unsetenv(PITCHFORK_LOG_ENVAR);

        log_fd = fopen(log_path, "w");
        if (log_fd == NULL) {
            exit(EXIT_INVALID_LOGPATH);
        }
    }
/* ndef PITCHFORK_DBGSTDOUT */
#endif
/* ndef PITCHFORK_NOLOG */
#endif

    if (my_config.start_subs == 1 && !privileged()) {
      LOG(39, "compart_init() called in non-privileged process")
      exit(39 + EXIT_LOGGED_OFFSET);
    }

    if (!am_i_monitor) {
      LOG(26, "compart_init() called by non-monitor")
      exit(26 + EXIT_LOGGED_OFFSET);
    }

    if (0 != started) {
      LOG(47, "compart_init() on already-started monitor")
      exit(47 + EXIT_LOGGED_OFFSET);
    }

    if (config.call_timeout > -1 && NULL == config.on_call_timeout) {
      LOG(51, "config.call_timeout > -1 && NULL == config.on_call_timeout")
      exit(51 + EXIT_LOGGED_OFFSET);
    }

    if (config.activity_timeout > -1 && NULL == config.on_activity_timeout) {
      LOG(52, "config.activity_timeout > -1 && NULL == config.on_activity_timeout")
      exit(52 + EXIT_LOGGED_OFFSET);
    }

    int i = 0;
    int j = 0;
    for (i = 0; i < no_comparts; ++i) {
      for (j = 0; j < no_comparts; ++j) {
        if (0 == strcmp(comparts[j].name, comparts[i].name) && j != i) {
          LOG(56, "different compartments must have different names")
          exit(56 + EXIT_LOGGED_OFFSET);
        }
      }
    }

    if (NULL == config.on_comm_break) {
      config.on_comm_break = &default_on_comm_break;
    }
    if (NULL == config.on_termination) {
      config.on_termination = &default_on_termination;
    }

    LOG(42, "initialising")

    no_comparts = local_no_comparts;
    int comparts_size = no_comparts * sizeof(struct compart);
    comparts = malloc(comparts_size);
    memcpy(comparts, local_comparts, comparts_size);
    compost_init(no_comparts, comparts);

    comparts_metadata = malloc(no_comparts * sizeof(struct compart_metadata));

    for (i = 0; i < no_comparts; i++) {
      comparts_metadata[i].num_registrations = 0;
      comparts_metadata[i].channel = NULL;
      comparts_metadata[i].pid = 0; /* FIXME make optional, in case unavailable */
    }

    for (i = 0; i < MAX_COMPART_REGS; i++) {
      registrations[i].compart_idx = -1;
      registrations[i].fn = NULL;
    }

    my_config = config;

    initialised = 1;
}

void compart_as(const char * const as_compartment_name)
{
  if (1 != initialised) {
    LOG(59, "compart_as() on before compart_init() is called")
    exit(59 + EXIT_LOGGED_OFFSET);
  }

  if (NULL != comparting_as) {
    LOG(EXIT_MULTIPLE_COMPART_AS, "multiple calls to compart_as()")
    exit(EXIT_MULTIPLE_COMPART_AS);
  }

  bool found = false;
  int i = 0;
  for (i = 0; i < no_comparts; ++i) {
    if (0 == strcmp(as_compartment_name, comparts[i].name)) {
      found = true;
      my_compart_idx = i;
      break;
    }
  }

  if (!found) {
    LOG(EXIT_CANNOT_COMPART_AS, "multiple calls to compart_as()")
    exit(EXIT_CANNOT_COMPART_AS);
  }

  am_i_monitor = false;
  compart_count = -1; /* Children don't track this info. */
  compartment_name = as_compartment_name;
  comparting_as = as_compartment_name;
  compost_as(comparting_as);
  channel = compost_2m(my_compart_idx);

  if (my_config.start_subs == 1) {
    /* Otherwise compart_as runs with user privileges. */
    drop_privs(no_comparts, comparts);
  }

  started = 1;

  /* FIXME add atexit() to close(*log_fd), free(log_fd), and free the eids and other mallocs in all compartments. */

#ifndef PITCHFORK_NOLOG
  char msg[MSG_BUF_SIZE];
  snprintf(msg, MSG_BUF_SIZE, "starting sub %d %s", getpid(), compartment_name);
  LOG(43, msg)
/* ndef PITCHFORK_NOLOG */
#endif

  parent_loop();
}

void compart_start(const char * const new_compartment_name)
{
    unsigned total_registrations = 0;
    int i = 0;
    for (i = 0; i < no_comparts; ++i) {
      total_registrations += comparts_metadata[i].num_registrations;
    }
    /* FIXME assert total_registrations == current_compart_reg */

    if (0 == total_registrations) {
      LOG(60, "compart_start() called before registrations were made")
      exit(60 + EXIT_LOGGED_OFFSET);
    }

    if (0 != started) {
      LOG(12, "compart_start() on already-started monitor")
      exit(12 + EXIT_LOGGED_OFFSET);
    }

    for (i = 0; i < no_comparts; ++i) {
      if (0 == strcmp(new_compartment_name, comparts[i].name) && comparts_metadata[i].num_registrations > 0) {
        LOG(55, "functions should not be registered with the main compartment")
        exit(55 + EXIT_LOGGED_OFFSET);
      }
    }

    if (my_config.start_subs == 1) {
      for (i = 0; i < no_comparts; ++i) {
        if (0 != strcmp(new_compartment_name, comparts[i].name) && comparts_metadata[i].num_registrations > 0) {
          pid_t pid = fork(); /* FIXME check success of this */

          if (!pid) {
            /* Call pre init */
            if(comparts[i].preinit_fn != NULL)
            {
                comparts[i].preinit_fn();
            }

            compart_as(comparts[i].name);
            
          } else {
            comparts_metadata[i].pid = pid;
            
            /* NOTE expect that (am_i_monitor == true); */
            compart_count++;
          }
        }
      }
    }

    started = 1;
    compost_start(new_compartment_name);

    for (i = 0; i < no_comparts; ++i) {
      if (0 != strcmp(new_compartment_name, comparts[i].name) && comparts_metadata[i].num_registrations > 0) {
        comparts_metadata[i].channel = compost_m2(i);
      }
    }

    split_parent_child(new_compartment_name, no_comparts, comparts);
}

#ifdef PITCHFORK_NOLOG
void compart_log(const char *buf, const size_t count) {
  (void)buf; /* This is to pacify warnings about unused parameters. */
  (void)count;
}
#else
/* ndef PITCHFORK_NOLOG */
void compart_log(const char *buf, const size_t count)
{
        if (NULL == log_fd) {
            exit(EXIT_LOGGED_OFFSET);
        }

        int written = fprintf(log_fd, "%s\n", buf);
        if (written < 0 || count != ((size_t)written - 1/*Accounting for the \n*/)) {
            exit(EXIT_WRITE_ERROR_LOGPATH);
        }
}
/* PITCHFORK_NOLOG */
#endif

struct extension_id *compart_register_fn(const char * const compartment_name, struct extension_data (*fn)(struct extension_data))
{
    if (!am_i_monitor) {
      LOG(40, "compart_register_fn() called by non-monitor")
      exit(40 + EXIT_LOGGED_OFFSET);
    }

    if (1 == started) {
      LOG(20, "compart_register_fn() on started monitor")
      exit(20 + EXIT_LOGGED_OFFSET);
    }

    if (1 != initialised) {
      LOG(31, "compart_register_fn() on before compart_init() is called")
      exit(31 + EXIT_LOGGED_OFFSET);
    }

    if (MAX_COMPART_REGS <= current_compart_reg) {
      LOG(63, "compart_register_fn() exceeded MAX_COMPART_REGS")
      exit(63 + EXIT_LOGGED_OFFSET);
    }

    int compart_idx = -1;
    int i = 0;
    for (i = 0; i < no_comparts; ++i) {
      if (0 == strcmp(compartment_name, comparts[i].name)) {
        compart_idx = i;
        comparts_metadata[i].num_registrations += 1;
        break;
      }
    }

    if (-1 == compart_idx) {
      /* FIXME message can be made more helpful by specifying that compartment_name hadn't previously been declared. */
      LOG(41, "-1 == compart_idx")
      exit(41 + EXIT_LOGGED_OFFSET);
    }

    /* FIXME check if function already registered */
    registrations[current_compart_reg].compart_idx = compart_idx;
    registrations[current_compart_reg].fn = fn;
    struct extension_id * eid = malloc(sizeof(*eid));
    eid->extension_idx = current_compart_reg;
    current_compart_reg += 1;
    return eid;
}

struct extension_data compart_call_fn(struct extension_id *eid, struct extension_data arg)
{
  if (1 != started) {
    LOG(24, "compart_call_fn() on unstarted compartment")
    exit(24 + EXIT_LOGGED_OFFSET);
  }

  if (NULL == eid) {
    LOG(35, "compart_call_fn() on unregistered function")
    exit(35 + EXIT_LOGGED_OFFSET);
  }

  if (NULL == registrations[eid->extension_idx].fn || 0 > registrations[eid->extension_idx].compart_idx) {
    LOG(65, "compart_call_fn() attempting to call unregistered function")
    exit(65 + EXIT_LOGGED_OFFSET);
  }

#ifndef PITCHFORK_NOLOG
  const char to_s[] = " to ";
  char *msg = malloc(strlen(to_s) + strlen(compartment_name) + strlen(comparts[registrations[eid->extension_idx].compart_idx].name) + 1);
  strcpy(msg, compartment_name);
  strcpy(msg + strlen(compartment_name), to_s);
  strcpy(msg + strlen(compartment_name) + strlen(to_s), comparts[registrations[eid->extension_idx].compart_idx].name);
  LOG(44, msg)
  free(msg);
/* ndef PITCHFORK_NOLOG */
#endif

  /* FIXME should not work across compartments */
  Request request = empty_request;
  request.type = CALL_FN;
#ifdef INCLUDE_PID
  if (my_config.CFG_INCLUDE_PID) {
    request.pid = getpid();
  }
/* INCLUDE_PID */
#endif
  memcpy(request.data, eid, sizeof(*eid));

  int result = compost_send(channel, &request, sizeof(Request)/* FIXME make this message to monitor smaller -- we don't need to have space for the full arg in the request that we send to the monitor*/);
  if (0 >= result)/*FIXME check if this is the right range*/ {
    my_config.on_comm_break(monitor_compart_idx);
  }

  Response monitor_response = empty_response;
  result = compost_recv(channel, &monitor_response, sizeof(Response));
  if (0 >= result)/*FIXME check if this is the right range*/ {
    my_config.on_comm_break(monitor_compart_idx);
  }

  memcpy(request.data + sizeof(*eid), &arg, sizeof(arg));
  /* FIXME ensure registrations[eid->extension_idx].compart_idx < compart_count */
  result = compost_send(comparts_metadata[registrations[eid->extension_idx].compart_idx].channel, &request, sizeof(Request));
  if (0 >= result)/*FIXME check if this is the right range*/ {
    my_config.on_comm_break(registrations[eid->extension_idx].compart_idx);
  }
#ifdef LC_ALLOW_EXCHANGE_FD
  /* FIXME assert arg.fdc < LC_ALLOW_EXCHANGE_FD */
  int i = 0;
  for (i = 0; i < arg.fdc; i++) {
    compost_send_fd(comparts_metadata[registrations[eid->extension_idx].compart_idx].channel, arg.fd[i]);
  }
/* LC_ALLOW_EXCHANGE_FD */
#endif

  Response response = empty_response;
  result = compost_recv(comparts_metadata[registrations[eid->extension_idx].compart_idx].channel, &response, sizeof(Response));
  if (0 >= result)/*FIXME check if this is the right range*/ {
    my_config.on_comm_break(registrations[eid->extension_idx].compart_idx);
  }
  request.type = RET_FN;
  result = compost_send(channel, &request, sizeof(Request)); /* FIXME make message smaller */
  if (0 >= result)/*FIXME check if this is the right range*/ {
    my_config.on_comm_break(monitor_compart_idx);
  }
  result = compost_recv(channel, &monitor_response, sizeof(Response));
  if (0 >= result)/*FIXME check if this is the right range*/ {
    my_config.on_comm_break(monitor_compart_idx);
  }

  if (1 == response.terminate) {
    LOG(EXIT_SERVER_INSTRUCTED_CLIENT, "monitor asked for termination") /* FIXME this reply comes from the other compartment, not the monitor */
    exit(EXIT_SERVER_INSTRUCTED_CLIENT);
  }
  errno = response.err_no;

  struct extension_data resp;
  /* FIXME ensure that sizeof(response.data) >= sizeof(resp) */
  memcpy(&resp, &response.data, sizeof(resp));
#ifdef LC_ALLOW_EXCHANGE_FD
  /* FIXME assert resp.fdc < LC_ALLOW_EXCHANGE_FD */
  for (i = 0; i < resp.fdc; i++) {
    compost_recv_fd(comparts_metadata[registrations[eid->extension_idx].compart_idx].channel, &(resp.fd[i]));
  }
/* LC_ALLOW_EXCHANGE_FD */
#endif
  return resp;
}

const char * compart_name(void)
{
  return compartment_name;
}

int compart_allowed_fd(void)
{
#ifdef LC_ALLOW_EXCHANGE_FD
  return LC_ALLOW_EXCHANGE_FD;
#else
  return -1;
#endif
}
