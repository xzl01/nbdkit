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

/* This test constructs a plugin and 3 layers of filters:
 *
 *     NBD     ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌────────┐
 *  client ───▶│ filter3 │───▶│ filter2 │───▶│ filter1 │───▶│ plugin │
 * request     └─────────┘    └─────────┘    └─────────┘    └────────┘
 *
 * We then run every possible request and ensure that each method in
 * each filter and the plugin is called in the right order.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>

#include <pthread.h>

#include <libnbd.h>

#include "byte-swapping.h"
#include "cleanup.h"

/* Declare program_name. */
#if HAVE_DECL_PROGRAM_INVOCATION_SHORT_NAME == 1
#include <errno.h>
#define program_name program_invocation_short_name
#else
#define program_name "nbdkit"
#endif

#ifndef WIN32

static void *start_log_capture (void *);
static void log_verify_seen (const char *msg);
static void log_verify_seen_in_order (const char *msg, ...)
  __attribute__ ((sentinel));
static void log_free (void);
static void short_sleep (void);

static int extent (void *opaque, const char *context, uint64_t offset,
                   uint32_t *entries, size_t nr_entries, int *error)
{
  return 0;
}

#if LIBNBD_HAVE_NBD_SET_OPT_MODE
static int export (void *opaque, const char *name, const char *desc)
{
  return 0;
}
#endif

int
main (int argc, char *argv[])
{
  struct nbd_handle *nbd;
  int pfd[2];
  int r;
  int err;
  pthread_t thread;
  int orig_stderr;
  char data[512];

  if (system ("nbdkit --exit-with-parent --version") != 0) {
    printf ("%s: this test requires --exit-with-parent functionality\n",
            program_name);
    exit (77);
  }

  /* Prepare libnbd. */
  fprintf (stderr, "%s: beginning test\n", program_name);
  nbd = nbd_create ();
  if (nbd == NULL) {
    fprintf (stderr, "nbd_create: %s\n", nbd_get_error ());
    exit (EXIT_FAILURE);
  }
  if (nbd_add_meta_context (nbd, LIBNBD_CONTEXT_BASE_ALLOCATION) == -1) {
    fprintf (stderr, "nbd_add_meta_context: %s\n", nbd_get_error ());
    exit (EXIT_FAILURE);
  }
#if LIBNBD_HAVE_NBD_SET_OPT_MODE
  if (nbd_set_opt_mode (nbd, true) == -1) {
    fprintf (stderr, "nbd_set_opt_mode: %s\n", nbd_get_error ());
    exit (EXIT_FAILURE);
  }
#endif

  /* Start a thread which will just listen on the pipe and
   * place the log messages in a memory buffer.
   */
#ifdef HAVE_PIPE2
  r = pipe2 (pfd, O_CLOEXEC);
#else
  /* Just ignore the O_CLOEXEC requirement, it's only a test. */
  r = pipe (pfd);
#endif
  if (r == -1) {
    perror ("pipe2");
    exit (EXIT_FAILURE);
  }
  err = pthread_create (&thread, NULL, start_log_capture, &pfd[0]);
  if (err) {
    errno = err;
    perror ("pthread_create");
    exit (EXIT_FAILURE);
  }
  err = pthread_detach (thread);
  if (err) {
    errno = err;
    perror ("pthread_detach");
    exit (EXIT_FAILURE);
  }

  /* Shuffle stderr. Until we restore it later, avoid direct use of stderr.  */
  orig_stderr = fcntl (STDERR_FILENO, F_DUPFD_CLOEXEC, STDERR_FILENO);
  if (orig_stderr == -1) {
    perror ("fcntl");
    exit (EXIT_FAILURE);
  }
  if (dup2 (pfd[1], STDERR_FILENO) == -1) {
    dprintf (orig_stderr, "dup2: %s\n", strerror (errno));
    exit (EXIT_FAILURE);
  }

  /* Start nbdkit. */
  if (nbd_connect_command (nbd,
                           (char *[]) {
                             "nbdkit", "--exit-with-parent", "-fvns",
                             /* Because of asynchronous shutdown with
                              * threads, finalize isn't reliably
                              * called unless we disable parallel.
                              */
                             "-t", "1",
                             "--filter", ".libs/test-layers-filter3." SOEXT,
                             "--filter", ".libs/test-layers-filter2." SOEXT,
                             "--filter", ".libs/test-layers-filter1." SOEXT,
                             ".libs/test-layers-plugin." SOEXT,
                             "foo=bar",
                             NULL }) == -1) {
    dprintf (orig_stderr, "nbd_connect_command: %s\n", nbd_get_error ());
    exit (EXIT_FAILURE);
  }

  /* Restore normal stderr, now that child is forked. */
  close (pfd[1]);
  dup2 (orig_stderr, STDERR_FILENO);
  close (orig_stderr);

  short_sleep ();
  fprintf (stderr, "%s: nbdkit passed preconnect\n", program_name);

  /* Note for the purposes of this test we're not very careful about
   * checking for errors (except for the bare minimum).  This is
   * because we can be certain about exactly which server we are
   * connecting to and what it supports.  Don't use this as example
   * code for connecting to NBD servers.
   *
   * Expect to receive newstyle handshake.
   */
  if (strcmp (nbd_get_protocol (nbd), "newstyle-fixed") != 0) {
    fprintf (stderr, "%s: unexpected NBDMAGIC or version\n",
             program_name);
    exit (EXIT_FAILURE);
  }

  /* Plugin and 3 filters should run the load method in any order. */
  log_verify_seen ("test_layers_plugin_load");
  log_verify_seen ("filter1: test_layers_filter_load");
  log_verify_seen ("filter2: test_layers_filter_load");
  log_verify_seen ("filter3: test_layers_filter_load");

  /* config methods called in order. */
  log_verify_seen_in_order
    ("testlayersfilter3: config key=foo, value=bar",
     "filter3: test_layers_filter_config",
     "testlayersfilter2: config key=foo, value=bar",
     "filter2: test_layers_filter_config",
     "testlayersfilter1: config key=foo, value=bar",
     "filter1: test_layers_filter_config",
     "testlayersplugin: config key=foo, value=bar",
     "test_layers_plugin_config",
     NULL);

  /* config_complete methods called in order. */
  log_verify_seen_in_order
    ("testlayersfilter3: config_complete",
     "filter3: test_layers_filter_config_complete",
     "testlayersfilter2: config_complete",
     "filter2: test_layers_filter_config_complete",
     "testlayersfilter1: config_complete",
     "filter1: test_layers_filter_config_complete",
     "testlayersplugin: config_complete",
     "test_layers_plugin_config_complete",
     NULL);

  /* thread_model methods called in inner-to-outer order. */
  log_verify_seen_in_order
    ("test_layers_plugin_thread_model",
     "filter1: test_layers_filter_thread_model",
     "filter2: test_layers_filter_thread_model",
     "filter3: test_layers_filter_thread_model",
     NULL);

  /* get_ready methods called in inner-to-outer order. */
  log_verify_seen_in_order
    ("testlayersplugin: get_ready",
     "test_layers_plugin_get_ready",
     "testlayersfilter1: get_ready",
     "filter1: test_layers_filter_get_ready",
     "testlayersfilter2: get_ready",
     "filter2: test_layers_filter_get_ready",
     "testlayersfilter3: get_ready",
     "filter3: test_layers_filter_get_ready",
     NULL);

  /* after_fork methods called in inner-to-outer order. */
  log_verify_seen_in_order
    ("testlayersplugin: after_fork",
     "test_layers_plugin_after_fork",
     "testlayersfilter1: after_fork",
     "filter1: test_layers_filter_after_fork",
     "testlayersfilter2: after_fork",
     "filter2: test_layers_filter_after_fork",
     "testlayersfilter3: after_fork",
     "filter3: test_layers_filter_after_fork",
     NULL);

  /* preconnect methods called in outer-to-inner order, complete
   * in inner-to-outer order.
   */
  log_verify_seen_in_order
    ("testlayersfilter3: preconnect",
     "filter3: test_layers_filter_preconnect",
     "testlayersfilter2: preconnect",
     "filter2: test_layers_filter_preconnect",
     "testlayersfilter1: preconnect",
     "filter1: test_layers_filter_preconnect",
     "testlayersplugin: preconnect",
     "test_layers_plugin_preconnect",
     NULL);

#if LIBNBD_HAVE_NBD_SET_OPT_MODE
  /* We can only test .list_exports if we can send NBD_OPT_INFO; if we
   * can test it, they are called in order.
   */
  if (nbd_opt_list (nbd, (nbd_list_callback) { .callback = export }) == -1) {
    fprintf (stderr, "nbd_opt_list: %s\n", nbd_get_error ());
    exit (EXIT_FAILURE);
  }
  if (nbd_opt_go (nbd) == -1) {
    fprintf (stderr, "nbd_opt_go: %s\n", nbd_get_error ());
    exit (EXIT_FAILURE);
  }

  short_sleep ();
  log_verify_seen_in_order
    ("testlayersfilter3: list_exports",
     "filter3: test_layers_filter_list_exports",
     "testlayersfilter2: list_exports",
     "filter2: test_layers_filter_list_exports",
     "testlayersfilter1: list_exports",
     "filter1: test_layers_filter_list_exports",
     "testlayersplugin: list_exports",
     "test_layers_plugin_list_exports",
     NULL);
#endif /* LIBNBD_HAVE_NBD_SET_OPT_MODE */

  fprintf (stderr, "%s: nbdkit running\n", program_name);

  /* Verify export size (see tests/test-layers-plugin.c). */
  if (nbd_get_size (nbd) != 1024) {
    fprintf (stderr, "%s: unexpected export size %" PRIu64 " != 1024\n",
             program_name, nbd_get_size (nbd));
    exit (EXIT_FAILURE);
  }

  /* Verify export flags. */
  if (nbd_is_read_only (nbd) != 0) {
    fprintf (stderr, "%s: unexpected eflags: NBD_FLAG_READ_ONLY not clear\n",
             program_name);
    exit (EXIT_FAILURE);
  }
  if (nbd_can_flush (nbd) != 1) {
    fprintf (stderr, "%s: unexpected eflags: NBD_FLAG_SEND_FLUSH not set\n",
             program_name);
    exit (EXIT_FAILURE);
  }
  if (nbd_can_fua (nbd) != 1) {
    fprintf (stderr, "%s: unexpected eflags: NBD_FLAG_SEND_FUA not set\n",
             program_name);
    exit (EXIT_FAILURE);
  }
  if (nbd_is_rotational (nbd) != 1) {
    fprintf (stderr, "%s: unexpected eflags: NBD_FLAG_ROTATIONAL not set\n",
             program_name);
    exit (EXIT_FAILURE);
  }
  if (nbd_can_trim (nbd) != 1) {
    fprintf (stderr, "%s: unexpected eflags: NBD_FLAG_SEND_TRIM not set\n",
             program_name);
    exit (EXIT_FAILURE);
  }
  if (nbd_can_zero (nbd) != 1) {
    fprintf (stderr,
             "%s: unexpected eflags: NBD_FLAG_SEND_WRITE_ZEROES not set\n",
             program_name);
    exit (EXIT_FAILURE);
  }
  if (nbd_can_meta_context (nbd, LIBNBD_CONTEXT_BASE_ALLOCATION) != 1) {
    fprintf (stderr,
             "%s: unexpected setup: META_CONTEXT not supported\n",
             program_name);
    exit (EXIT_FAILURE);
  }

  /* default_export called in outer-to-inner order. */
  log_verify_seen_in_order
    ("testlayersfilter3: default_export",
     "filter3: test_layers_filter_default_export",
     "testlayersfilter2: default_export",
     "filter2: test_layers_filter_default_export",
     "testlayersfilter1: default_export",
     "filter1: test_layers_filter_default_export",
     "testlayersplugin: default_export",
     "test_layers_plugin_default_export",
     NULL);

  /* open methods called in outer-to-inner order, but thanks to next
   * pointer, complete in inner-to-outer order. */
  log_verify_seen_in_order
    ("testlayersfilter3: open readonly=0",
     "testlayersfilter2: open readonly=0",
     "testlayersfilter1: open readonly=0",
     "testlayersplugin: open readonly=0",
     "test_layers_plugin_open",
     "filter1: test_layers_filter_open",
     "filter2: test_layers_filter_open",
     "filter3: test_layers_filter_open",
     NULL);

  /* prepare methods called in inner-to-outer order.
   *
   * Note that prepare methods only exist for filters, and they must
   * be called from inner to outer (but finalize methods below are
   * called the other way around).
   */
  log_verify_seen_in_order
    ("filter1: test_layers_filter_prepare",
     "filter2: test_layers_filter_prepare",
     "filter3: test_layers_filter_prepare",
     NULL);

  /* get_size methods called in order. */
  log_verify_seen_in_order
    ("filter3: test_layers_filter_get_size",
     "filter2: test_layers_filter_get_size",
     "filter1: test_layers_filter_get_size",
     "test_layers_plugin_get_size",
     NULL);

  /* can_* / is_* methods called in order. */
  log_verify_seen_in_order
    ("filter3: test_layers_filter_can_write",
     "filter2: test_layers_filter_can_write",
     "filter1: test_layers_filter_can_write",
     "test_layers_plugin_can_write",
     NULL);
  log_verify_seen_in_order
    ("filter3: test_layers_filter_can_zero",
     "filter2: test_layers_filter_can_zero",
     "filter1: test_layers_filter_can_zero",
     "test_layers_plugin_can_zero",
     NULL);
  log_verify_seen_in_order
    ("filter3: test_layers_filter_can_fast_zero",
     "filter2: test_layers_filter_can_fast_zero",
     "filter1: test_layers_filter_can_fast_zero",
     "test_layers_plugin_can_fast_zero",
     NULL);
  log_verify_seen_in_order
    ("filter3: test_layers_filter_can_trim",
     "filter2: test_layers_filter_can_trim",
     "filter1: test_layers_filter_can_trim",
     "test_layers_plugin_can_trim",
     NULL);
  log_verify_seen_in_order
    ("filter3: test_layers_filter_can_fua",
     "filter2: test_layers_filter_can_fua",
     "filter1: test_layers_filter_can_fua",
     "test_layers_plugin_can_fua",
     NULL);
  log_verify_seen_in_order
    ("filter3: test_layers_filter_can_flush",
     "filter2: test_layers_filter_can_flush",
     "filter1: test_layers_filter_can_flush",
     "test_layers_plugin_can_flush",
     NULL);
  log_verify_seen_in_order
    ("filter3: test_layers_filter_is_rotational",
     "filter2: test_layers_filter_is_rotational",
     "filter1: test_layers_filter_is_rotational",
     "test_layers_plugin_is_rotational",
     NULL);
  log_verify_seen_in_order
    ("filter3: test_layers_filter_can_multi_conn",
     "filter2: test_layers_filter_can_multi_conn",
     "filter1: test_layers_filter_can_multi_conn",
     "test_layers_plugin_can_multi_conn",
     NULL);
  log_verify_seen_in_order
    ("filter3: test_layers_filter_can_extents",
     "filter2: test_layers_filter_can_extents",
     "filter1: test_layers_filter_can_extents",
     "test_layers_plugin_can_extents",
     NULL);
  log_verify_seen_in_order
    ("filter3: test_layers_filter_can_cache",
     "filter2: test_layers_filter_can_cache",
     "filter1: test_layers_filter_can_cache",
     "test_layers_plugin_can_cache",
     NULL);

  fprintf (stderr, "%s: protocol connected\n", program_name);

  /* Send one command of each type. */
  if (nbd_pread (nbd, data, sizeof data, 0, 0) != 0) {
    fprintf (stderr, "%s: NBD_CMD_READ failed with %d %s\n",
             program_name, nbd_get_errno (), nbd_get_error ());
    exit (EXIT_FAILURE);
  }

  short_sleep ();
  log_verify_seen_in_order
    ("testlayersfilter3: pread count=512 offset=0",
     "filter3: test_layers_filter_pread",
     "testlayersfilter2: pread count=512 offset=0",
     "filter2: test_layers_filter_pread",
     "testlayersfilter1: pread count=512 offset=0",
     "filter1: test_layers_filter_pread",
     "testlayersplugin: pread count=512 offset=0",
     "test_layers_plugin_pread",
     NULL);

  if (nbd_pwrite (nbd, data, sizeof data, 0, 0) != 0) {
    fprintf (stderr, "%s: NBD_CMD_WRITE failed with %d %s\n",
             program_name, nbd_get_errno (), nbd_get_error ());
    exit (EXIT_FAILURE);
  }

  short_sleep ();
  log_verify_seen_in_order
    ("testlayersfilter3: pwrite count=512 offset=0 fua=0",
     "filter3: test_layers_filter_pwrite",
     "testlayersfilter2: pwrite count=512 offset=0 fua=0",
     "filter2: test_layers_filter_pwrite",
     "testlayersfilter1: pwrite count=512 offset=0 fua=0",
     "filter1: test_layers_filter_pwrite",
     "testlayersplugin: pwrite count=512 offset=0 fua=0",
     "test_layers_plugin_pwrite",
     NULL);

  if (nbd_flush (nbd, 0) != 0) {
    fprintf (stderr, "%s: NBD_CMD_FLUSH failed with %d %s\n",
             program_name, nbd_get_errno (), nbd_get_error ());
    exit (EXIT_FAILURE);
  }

  short_sleep ();
  log_verify_seen_in_order
    ("testlayersfilter3: flush",
     "filter3: test_layers_filter_flush",
     "testlayersfilter2: flush",
     "filter2: test_layers_filter_flush",
     "testlayersfilter1: flush",
     "filter1: test_layers_filter_flush",
     "testlayersplugin: flush",
     "test_layers_plugin_flush",
     NULL);

  if (nbd_trim (nbd, sizeof data, 0, 0) != 0) {
    fprintf (stderr, "%s: NBD_CMD_TRIM failed with %d %s\n",
             program_name, nbd_get_errno (), nbd_get_error ());
    exit (EXIT_FAILURE);
  }

  short_sleep ();
  log_verify_seen_in_order
    ("testlayersfilter3: trim count=512 offset=0 fua=0",
     "filter3: test_layers_filter_trim",
     "testlayersfilter2: trim count=512 offset=0 fua=0",
     "filter2: test_layers_filter_trim",
     "testlayersfilter1: trim count=512 offset=0 fua=0",
     "filter1: test_layers_filter_trim",
     "testlayersplugin: trim count=512 offset=0 fua=0",
     "test_layers_plugin_trim",
     NULL);

  if (nbd_zero (nbd, sizeof data, 0, 0) != 0) {
    fprintf (stderr, "%s: NBD_CMD_WRITE_ZEROES failed with %d %s\n",
             program_name, nbd_get_errno (), nbd_get_error ());
    exit (EXIT_FAILURE);
  }

  short_sleep ();
  log_verify_seen_in_order
    ("testlayersfilter3: zero count=512 offset=0 may_trim=1 fua=0 fast=0",
     "filter3: test_layers_filter_zero",
     "testlayersfilter2: zero count=512 offset=0 may_trim=1 fua=0 fast=0",
     "filter2: test_layers_filter_zero",
     "testlayersfilter1: zero count=512 offset=0 may_trim=1 fua=0 fast=0",
     "filter1: test_layers_filter_zero",
     "testlayersplugin: zero count=512 offset=0 may_trim=1 fua=0 fast=0",
     "test_layers_plugin_zero",
     NULL);

  if (nbd_cache (nbd, sizeof data, 0, 0) != 0) {
    fprintf (stderr, "%s: NBD_CMD_CACHE failed with %d %s\n",
             program_name, nbd_get_errno (), nbd_get_error ());
    exit (EXIT_FAILURE);
  }

  short_sleep ();
  log_verify_seen_in_order
    ("testlayersfilter3: cache count=512 offset=0",
     "filter3: test_layers_filter_cache",
     "testlayersfilter2: cache count=512 offset=0",
     "filter2: test_layers_filter_cache",
     "testlayersfilter1: cache count=512 offset=0",
     "filter1: test_layers_filter_cache",
     "testlayersplugin: cache count=512 offset=0",
     "test_layers_plugin_cache",
     NULL);

  if (nbd_block_status (nbd, sizeof data, 0,
                        (nbd_extent_callback) { .callback = extent }, 0) != 0) {
    fprintf (stderr, "%s: NBD_CMD_BLOCK_STATUS failed with %d %s\n",
             program_name, nbd_get_errno (), nbd_get_error ());
    exit (EXIT_FAILURE);
  }

  short_sleep ();
  log_verify_seen_in_order
    ("testlayersfilter3: extents count=512 offset=0",
     "filter3: test_layers_filter_extents",
     "testlayersfilter2: extents count=512 offset=0",
     "filter2: test_layers_filter_extents",
     "testlayersfilter1: extents count=512 offset=0",
     "filter1: test_layers_filter_extents",
     "testlayersplugin: extents count=512 offset=0",
     "test_layers_plugin_extents",
     NULL);

  /* Close the connection. */
  fprintf (stderr, "%s: closing the connection\n", program_name);
  if (nbd_shutdown (nbd, 0) != 0) {
    fprintf (stderr, "%s: NBD_CMD_DISC failed with %d %s\n",
             program_name, nbd_get_errno (), nbd_get_error ());
    exit (EXIT_FAILURE);
  }
  nbd_close (nbd);

  /* finalize methods called in reverse order of prepare */
  short_sleep ();
  log_verify_seen_in_order
    ("filter3: test_layers_filter_finalize",
     "filter2: test_layers_filter_finalize",
     "filter1: test_layers_filter_finalize",
     NULL);

  /* close methods called outer-to-inner, which is reverse of completion
   * of open */
  log_verify_seen_in_order
    ("filter3: test_layers_filter_close",
     "filter2: test_layers_filter_close",
     "filter1: test_layers_filter_close",
     "test_layers_plugin_close",
     NULL);

  /* cleanup methods called in outer-to-inner order. */
  log_verify_seen_in_order
    ("testlayersfilter3: cleanup",
     "filter3: test_layers_filter_cleanup",
     "testlayersfilter2: cleanup",
     "filter2: test_layers_filter_cleanup",
     "testlayersfilter1: cleanup",
     "filter1: test_layers_filter_cleanup",
     "testlayersplugin: cleanup",
     "test_layers_plugin_cleanup",
     NULL);

  /* unload methods should be run in any order. */
  log_verify_seen ("test_layers_plugin_unload");
  log_verify_seen ("filter1: test_layers_filter_unload");
  log_verify_seen ("filter2: test_layers_filter_unload");
  log_verify_seen ("filter3: test_layers_filter_unload");

  log_free ();

  exit (EXIT_SUCCESS);
}

/* The log from nbdkit is captured in a separate thread. */
static char *log_buf = NULL;
static size_t log_len = 0;
static size_t last_out = 0;
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

static void *
start_log_capture (void *arg)
{
  int fd = *(int *)arg;
  size_t allocated = 0;
  ssize_t r;

  for (;;) {
    {
      ACQUIRE_LOCK_FOR_CURRENT_SCOPE (&log_lock);
      if (allocated <= log_len) {
        allocated += 4096;
        log_buf = realloc (log_buf, allocated);
        if (log_buf == NULL) {
          perror ("log: realloc");
          exit (EXIT_FAILURE);
        }
      }
    }

    r = read (fd, &log_buf[log_len], allocated-log_len);
    if (r == -1) {
      perror ("log: read");
      exit (EXIT_FAILURE);
    }
    if (r == 0)
      break;

    ACQUIRE_LOCK_FOR_CURRENT_SCOPE (&log_lock);
    log_len += r;
  }

  /* nbdkit closed the connection. */
  pthread_exit (NULL);
}

static void short_sleep (void)
{
  sleep (2);
  /* Copy what we have received so far into stderr */
  ACQUIRE_LOCK_FOR_CURRENT_SCOPE (&log_lock);
  if (fwrite (&log_buf[last_out], log_len - last_out, 1, stderr) == -1)
    perror ("log: fwrite");
  last_out = log_len;
}

/* These functions are called from the main thread to verify messages
 * appeared as expected in the log.
 *
 * NB: The log buffer is NOT \0-terminated.
 */

static void no_message_error (const char *msg) __attribute__ ((noreturn));

static void
no_message_error (const char *msg)
{
  fprintf (stderr, "%s: did not find expected message \"%s\"\n",
           program_name, msg);
  exit (EXIT_FAILURE);
}

static void
log_verify_seen (const char *msg)
{
  ACQUIRE_LOCK_FOR_CURRENT_SCOPE (&log_lock);
  if (memmem (log_buf, log_len, msg, strlen (msg)) == NULL)
    no_message_error (msg);
}

static void messages_out_of_order (const char *msg1, const char *msg2)
  __attribute__ ((noreturn));

static void
messages_out_of_order (const char *msg1, const char *msg2)
{
  fprintf (stderr, "%s: message \"%s\" expected before message \"%s\"\n",
           program_name, msg1, msg2);
  exit (EXIT_FAILURE);
}

static void
log_verify_seen_in_order (const char *msg, ...)
{
  va_list args;
  void *prev, *curr;
  const char *prev_msg, *curr_msg;

  ACQUIRE_LOCK_FOR_CURRENT_SCOPE (&log_lock);

  prev = memmem (log_buf, log_len, msg, strlen (msg));
  if (prev == NULL) no_message_error (msg);
  prev_msg = msg;

  va_start (args, msg);
  while ((curr_msg = va_arg (args, char *)) != NULL) {
    curr = memmem (log_buf, log_len, curr_msg, strlen (curr_msg));
    if (curr == NULL) no_message_error (curr_msg);
    if (prev > curr) messages_out_of_order (prev_msg, curr_msg);
    prev_msg = curr_msg;
    prev = curr;
  }
  va_end (args);
}

static void
log_free (void)
{
  ACQUIRE_LOCK_FOR_CURRENT_SCOPE (&log_lock);
  free (log_buf);
  log_buf = NULL;
  log_len = 0;
}

#else /* WIN32 */

/* A lot of porting work required for Windows.  For now, skip the test. */
int
main (int argc, char *argv[])
{
  fprintf (stderr, "%s: test skipped because not ported to Windows.\n",
           argv[0]);
  exit (77);
}

#endif /* WIN32 */
