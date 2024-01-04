/* nbdkit
 * Copyright Red Hat
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * * Neither the name of Red Hat nor the names of its contributors may be
 * used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY RED HAT AND CONTRIBUTORS ''AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RED HAT OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include "test.h"

#ifndef WIN32

static char pidpath[] = "/tmp/nbdkitpidXXXXXX";

static void run_test (void);

int
main (int argc, char *argv[])
{
  if (system ("nbdkit --exit-with-parent --version") != 0) {
    printf ("%s: --exit-with-parent is not implemented on this platform, "
            "skipping\n",
            argv[0]);
    exit (77);
  }

  run_test ();
  exit (EXIT_SUCCESS);
}

static void
run_test (void)
{
  pid_t ppid, nbdpid;
  int i, fd, status;
  FILE *fp;
  ssize_t r;
  size_t n;
  char *pidstr = NULL;

  fd = mkstemp (pidpath);
  if (fd == -1) {
    perror ("mkstemp");
    exit (EXIT_FAILURE);
  }
  close (fd);
  unlink (pidpath);

  /* We're going to create:
   *
   *    monitoring process (this)
   *       |
   *       `--- parent of nbdkit: waits for nbdkit to start then exits (ppid)
   *                |
   *                `--- exec nbdkit --exit-with-parent (pidpath)
   *
   * We can read the nbdkit PID in the monitoring process using
   * the pidpath file.
   */
  ppid = fork ();
  if (ppid == 0) {              /* parent of nbdkit */
    nbdpid = fork ();
    if (nbdpid == 0) {             /* exec nbdkit process */
      const char *argv[] = {
        "nbdkit", "-U", "-", "-P", pidpath, "-fv", "--exit-with-parent",
        "example1",
        NULL
      };

      execvp ("nbdkit", (char **) argv);
      perror ("exec: nbdkit");
      _exit (EXIT_FAILURE);
    }

    printf ("parent of nbdkit: "
            "monitoring process (test) = %d, "
            "parent of nbdkit = %d, "
            "nbdkit = %d\n",
            (int) getppid (), (int) getpid (), nbdpid);
    fflush (stdout);

    /* Wait for the pidfile to turn up, which indicates that nbdkit has
     * started up successfully and is ready to serve requests.  However
     * if 'nbdpid' exits in this time it indicates a failure to start up.
     * Also there is a timeout in case nbdkit hangs.
     */
    for (i = 0; i < NBDKIT_START_TIMEOUT; ++i) {
      if (waitpid (nbdpid, NULL, WNOHANG) == nbdpid)
        goto early_exit;

      if (kill (nbdpid, 0) == -1) {
        if (errno == ESRCH) {
        early_exit:
          fprintf (stderr,
                   "%s FAILED: nbdkit exited before starting to serve files\n",
                   program_name);
          nbdpid = 0;
          exit (EXIT_FAILURE);
        }
        perror ("kill");
      }

      if (access (pidpath, F_OK) == 0)
        break;

      sleep (1);
    }

    printf ("parent of nbdkit: exiting\n");
    fflush (stdout);

    /* nbdkit is now running, check that --exit-with-parent works
     * by exiting abruptly here.
     */
    _exit (EXIT_SUCCESS);
  }

  /* Monitoring process. */
  printf ("monitor: waiting for parent of nbdkit to finish and exit\n");
  fflush (stdout);

  if (waitpid (ppid, &status, 0) == -1) {
    perror ("waitpid (ppid)");
    exit (EXIT_FAILURE);
  }
  if (WIFEXITED (status) && WEXITSTATUS (status) != 0) {
    fprintf (stderr, "child exited unexpectedly with non-zero exit code %d\n",
             WEXITSTATUS (status));
    exit (WEXITSTATUS (status));
  }
  if (WIFSIGNALED (status)) {
    fprintf (stderr, "child terminated by signal %d\n", WTERMSIG (status));
    exit (EXIT_FAILURE);
  }
  if (WIFSTOPPED (status)) {
    fprintf (stderr, "child stopped by signal %d\n", WSTOPSIG (status));
    exit (EXIT_FAILURE);
  }

  /* Get the PID of nbdkit from the pidpath file. */
  fp = fopen (pidpath, "r");
  if (fp == NULL) {
    perror (pidpath);
    exit (EXIT_FAILURE);
  }
  r = getline (&pidstr, &n, fp);
  if (r == -1) {
    perror ("read");
    exit (EXIT_FAILURE);
  }
  if (sscanf (pidstr, "%d", &nbdpid) != 1) {
    fprintf (stderr, "could not read nbdkit PID from -P pidfile (%s)\n",
             pidpath);
    exit (EXIT_FAILURE);
  }
  fclose (fp);
  free (pidstr);
  unlink (pidpath);

  printf ("monitor: found PID of nbdkit = %d\n", nbdpid);
  fflush (stdout);

  /* We expect PID to go away, but it might take a few seconds. */
  for (i = 0; i < NBDKIT_START_TIMEOUT; ++i) {
    if (kill (nbdpid, 0) == -1) {
      if (errno == ESRCH) goto done; /* good - gone away */
      perror ("kill");
      exit (EXIT_FAILURE);
    }

    sleep (1);
  }

  fprintf (stderr, "--exit-with-parent does not appear to work\n");
  exit (EXIT_FAILURE);

 done:
  printf ("monitor: success: nbdkit exited with parent\n");
  fflush (stdout);
}

#else /* WIN32 */

/* The test above relies on Unixisms like fork so it won't work on
 * Windows as written.  Since we don't implement --exit-with-parent
 * anyway, bail for now and worry about it later.
 */

int
main (int argc, char *argv[])
{
  printf ("%s: neither --exit-with-parent nor this test "
          "are implemented on Windows, skipping\n",
          argv[0]);
  exit (77);
}

#endif /* WIN32 */
