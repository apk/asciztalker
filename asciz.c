#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>

struct buf {
  char buf [1024];
  int fill;
  int pos;
};

void buf_init (struct buf *b) {
  b->pos = 0;
  b->fill = 0;
}

void buf_appendz (struct buf *b, char *p) {
  do {
    if (b->fill >= sizeof (b->buf)) {
      fprintf (stderr, "Overflow\n");
      exit (1);
    }
  } while ((b->buf [b->fill ++] = *p ++));
}

int buf_rd_p (struct buf *b) {
  return b->fill < sizeof (b->buf);
}

int buf_wr_p (struct buf *b) {
  return b->pos < b->fill;
}

void buf_rd_x (struct buf *b, int fd) {
  int sz = sizeof (b->buf) - b->fill;
  int r = read (fd, b->buf + b->fill, sz);
  // fprintf (stderr, "%d..%d read(%d,%d)=>%d\n", b->pos, b->fill, fd, sz, r);
  if (r == -1) {
    perror ("read");
    exit (1);
  }
  if (r == 0) {
    fprintf (stderr, "EOF\n");
    exit (1);
  }
  b->fill += r;
  // fprintf (stderr, "buf: %d..%d\n", b->pos, b->fill);
}

void buf_wr_x (struct buf *b, int fd) {
  int sz = b->fill - b->pos;
  int r = write (fd, b->buf + b->pos, sz);
  // fprintf (stderr, "write(%d,%d)=>%d\n", fd, sz, r);
  if (r == -1) {
    perror ("read");
    exit (1);
  }
  if (r == 0) {
    fprintf (stderr, "FULL?\n");
    exit (1);
  }
  b->pos += r;
  if (b->pos > sizeof (b->buf) / 2) {
    memmove (b->buf, b->buf + b->pos, b->fill - b->pos);
    b->fill -= b->pos;
    b->pos = 0;
  }
}

int main (int argc, char **argv) {
  int fds [2];
  char *tg = getenv ("yyx");
  int r = socketpair (AF_LOCAL, SOCK_STREAM, 0, fds);
  if (r == -1) {
    perror ("socketpair");
    exit (1);
  }
  if (!tg) {
    fprintf (stderr, "no target");
    exit (1);
  }
  pid = fork ();
  if (pid == -1) {
    perror ("fork");
    exit (1);
  } else if (pid == 0) {
    dup2 (fds [1], 0);
    dup2 (fds [1], 1);
    close (fds [0]);
    execvp (argv [1], argv + 1);
    _exit (4);
  } else {
    int op = 0;
    struct buf up, down;
    buf_init (&up);
    buf_init (&down);
    buf_appendz (&down, tg);
    close (fds [1]);
    while (1) {
      fd_set R, W;
      FD_ZERO (&R);
      FD_ZERO (&W);
      if (op && buf_rd_p (&down)) {
	FD_SET (0, &R);
      }
      if (buf_wr_p (&down)) {
	FD_SET (fds [0], &W);
      }
      if (buf_rd_p (&up)) {
	FD_SET (fds [0], &R);
      }
      if (op && buf_wr_p (&up)) {
	FD_SET (1, &W);
      }
      r = select (fds [0] + 1, &R, &W, 0, 0);
      if (r == -1) {
	perror ("select");
	kill (pid, SIGINT);
	exit (1);
      }
      if (r == 0) {
	fprintf (stderr, "Oops\n");
	kill (pid, SIGINT);
	exit (1);
      }
      if (op && FD_ISSET (0, &R)) {
	buf_rd_x (&down, 0);
      }
      if (FD_ISSET (fds [0], &W)) {
	buf_wr_x (&down, fds [0]);
      }
      if (FD_ISSET (fds [0], &R)) {
	buf_rd_x (&up, fds [0]);
      }
      if (op && FD_ISSET (1, &W)) {
	buf_wr_x (&up, 1);
      }
      if (!op) {
	int i;
	fprintf (stderr, "up: %d..%d\n", up.pos, up.fill);
	for (i = up.pos; i < up.fill; i ++) {
	  fprintf (stderr, "  [%d]: %d\n", i, up.buf [i]);
	  if (up.buf [i] == 0) {
	    if (i == up.pos) {
	      /* Got the empty string */
	      up.pos ++;
	      fprintf (stderr, "Good!\n");
	      op = 1;
	      continue;
	    } else {
	      /* Something's amiss */
	      fprintf (stderr, "FAIL '%s'\n", up.buf + up.pos);
	      kill (pid, SIGINT);
	      exit (1);
	    }
	  }
	}
      }
    }
  }
  return 0;
}

/*
 * Local Variables:
 * compile-command: "make -k asciz && ./asciz /bin/cat"
 * End:
 */
