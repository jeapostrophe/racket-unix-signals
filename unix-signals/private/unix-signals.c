#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>

/* This implementation uses djb's "self-pipe trick".
 * See http://cr.yp.to/docs/selfpipe.html. */

static int self_pipe_initialized = 0;
static int self_pipe_read_end = -1;
static int self_pipe_write_end = -1;

int setup_self_pipe(void) {
  {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
      perror("unix-signals-extension pipe(2)");
      goto error;
    }
    self_pipe_read_end = pipefd[0];
    self_pipe_write_end = pipefd[1];
  }

  {
    int flags = fcntl(self_pipe_write_end, F_GETFL, 0);
    if (flags == -1) {
      perror("unix-signals-extension F_GETFL");
      goto error;
    }
    if (fcntl(self_pipe_write_end, F_SETFL, flags | O_NONBLOCK) == -1) {
      perror("unix-signals-extension F_SETFL");
      goto error;
    }
  }

  return 0;

 error:
  if (self_pipe_write_end != -1) {
    int tmp = self_pipe_write_end;
    self_pipe_write_end = -1;
    close(tmp);
  }
  if (self_pipe_read_end != -1) {
    int tmp = self_pipe_read_end;
    self_pipe_read_end = -1;
    close(tmp);
  }
  return -1;
}

static void signal_handler_fn(int signum) {
  if (self_pipe_write_end == -1) {
    return;
  }

  {
    uint8_t b;
    b = (uint8_t) (signum & 0xff);
    if (write(self_pipe_write_end, &b, 1) == -1) {
      perror("unix-signals-extension write");
    }
  }
}

int prim_get_signal_fd() {
  if (self_pipe_read_end == -1) {
    return 0;
  } else {
    return self_pipe_read_end;
  }
}

typedef struct {
  const char* name;
  const int num; }
  signal_t;

signal_t SIGNALS[] =
  {
#define ADD_SIGNAL_NAME(n) { #n, n }
   
   /* POSIX.1-1990 */
   ADD_SIGNAL_NAME(SIGHUP),
   ADD_SIGNAL_NAME(SIGINT),
   ADD_SIGNAL_NAME(SIGQUIT),
   ADD_SIGNAL_NAME(SIGILL),
   ADD_SIGNAL_NAME(SIGABRT),
   ADD_SIGNAL_NAME(SIGFPE),
   ADD_SIGNAL_NAME(SIGKILL),
   ADD_SIGNAL_NAME(SIGSEGV),
   ADD_SIGNAL_NAME(SIGPIPE),
   ADD_SIGNAL_NAME(SIGALRM),
   ADD_SIGNAL_NAME(SIGTERM),
   ADD_SIGNAL_NAME(SIGUSR1),
   ADD_SIGNAL_NAME(SIGUSR2),
   ADD_SIGNAL_NAME(SIGCHLD),
   ADD_SIGNAL_NAME(SIGCONT),
   ADD_SIGNAL_NAME(SIGSTOP),
   ADD_SIGNAL_NAME(SIGTSTP),
   ADD_SIGNAL_NAME(SIGTTIN),
   ADD_SIGNAL_NAME(SIGTTOU),

  /* Not POSIX.1-1990, but SUSv2 and POSIX.1-2001 */
   ADD_SIGNAL_NAME(SIGBUS),
#if !defined(__APPLE__)
   ADD_SIGNAL_NAME(SIGPOLL),
#endif
   ADD_SIGNAL_NAME(SIGPROF),
   ADD_SIGNAL_NAME(SIGSYS),
   ADD_SIGNAL_NAME(SIGTRAP),
   ADD_SIGNAL_NAME(SIGURG),
   ADD_SIGNAL_NAME(SIGVTALRM),
   ADD_SIGNAL_NAME(SIGXCPU),
   ADD_SIGNAL_NAME(SIGXFSZ),

  /* Misc, that we hope are widely-supported enough not to have to
     bother with a feature test. */
   ADD_SIGNAL_NAME(SIGIO),
   ADD_SIGNAL_NAME(SIGWINCH),

#undef ADD_SIGNAL_NAME

   { 0, 0 }
  };

int prim_get_signal_names_count() {
  int i = 0;
  while ( SIGNALS[i].name ) {
    i++; }
  return i;
}

const char *prim_get_signal_names_name(int i) {
  return SIGNALS[i].name;
}

int prim_get_signal_names_num(int i) {
  return SIGNALS[i].num;
}

int prim_capture_signal(int signum, int code) {
  switch (code) {
    case 0:
      if (signal(signum, signal_handler_fn) == SIG_ERR) {
        perror("unix-signals-extension signal(2) install");
        return 0;
      }
      break;
    case 1:
      if (signal(signum, SIG_IGN) == SIG_ERR) {
        perror("unix-signals-extension signal(2) ignore");
        return 0;
      }
      break;
    case 2:
      if (signal(signum, SIG_DFL) == SIG_ERR) {
        perror("unix-signals-extension signal(2) default");
        return 0;
      }
      break;
    default:
      return 0;
  }
  return 1;
}

int lowlevel_send_signal(pid_t pid, int sig) {
  if (kill(pid, sig) == -1) {
    perror("unix-signals-extension kill(2)");
    return 0;
  }
  return 1;
}

