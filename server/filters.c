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
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "internal.h"

/* We extend the generic backend struct with extra fields relating
 * to this filter.
 */
struct backend_filter {
  struct backend backend;
  struct nbdkit_filter filter;
};

/* Note this frees the whole chain. */
static void
filter_free (struct backend *b)
{
  struct backend_filter *f = container_of (b, struct backend_filter, backend);

  b->next->free (b->next);

  backend_unload (b, f->filter.unload);
  free (f);
}

static int
filter_thread_model (struct backend *b)
{
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  int filter_thread_model = NBDKIT_THREAD_MODEL_PARALLEL;
  int model = b->next->thread_model (b->next);

  if (f->filter.thread_model) {
    filter_thread_model = f->filter.thread_model ();
    if (filter_thread_model == -1)
      exit (EXIT_FAILURE);
  }

  if (filter_thread_model < model) /* more serialized */
    model = filter_thread_model;

  return model;
}

/* This is actually passing the request through to the final plugin,
 * hence the function name.
 */
static const char *
plugin_name (struct backend *b)
{
  return b->next->plugin_name (b->next);
}

static const char *
filter_version (struct backend *b)
{
  struct backend_filter *f = container_of (b, struct backend_filter, backend);

  return f->filter._version;
}

static void
filter_usage (struct backend *b)
{
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  const char *p;

  printf ("filter: %s", b->name);
  if (f->filter.longname)
    printf (" (%s)", f->filter.longname);
  printf ("\n");
  printf ("(%s)\n", b->filename);
  if (f->filter.description) {
    printf ("%s", f->filter.description);
    if ((p = strrchr (f->filter.description, '\n')) == NULL || p[1])
      printf ("\n");
  }
  if (f->filter.config_help) {
    printf ("%s", f->filter.config_help);
    if ((p = strrchr (f->filter.config_help, '\n')) == NULL || p[1])
      printf ("\n");
  }
}

/* This implements the --dump-plugin option which can dump additional
 * fields for any filters present.
 */
static void
filter_dump_fields (struct backend *b)
{
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  char *path;

  b->next->dump_fields (b->next);

  debug ("%s: dump_plugin", b->name);

  /* Dump some information about the filter. */
  path = nbdkit_realpath (b->filename);
  printf ("%s_path=%s\n", b->name, path);
  free (path);

  printf ("%s_name=%s\n", b->name, b->name);

  /* We could add filter struct fields here, but they are not so
   * interesting for filters because they are always tied to the exact
   * current version of nbdkit so we can determine which fields are
   * present just by looking at the source.
   */

  /* Custom fields. */
  if (f->filter.dump_plugin) {
    f->filter.dump_plugin ();
  }
}

static int
next_config (struct backend *b, const char *key, const char *value)
{
  b->config (b, key, value);
  return 0;
}

static void
filter_config (struct backend *b, const char *key, const char *value)
{
  struct backend_filter *f = container_of (b, struct backend_filter, backend);

  debug ("%s: config key=%s, value=%s",
         b->name, key, value);

  if (f->filter.config) {
    if (f->filter.config (next_config, b->next, key, value) == -1)
      exit (EXIT_FAILURE);
  }
  else
    b->next->config (b->next, key, value);
}

static int
next_config_complete (struct backend *b)
{
  b->config_complete (b);
  return 0;
}

static void
filter_config_complete (struct backend *b)
{
  struct backend_filter *f = container_of (b, struct backend_filter, backend);

  debug ("%s: config_complete", b->name);

  if (f->filter.config_complete) {
    if (f->filter.config_complete (next_config_complete, b->next) == -1)
      exit (EXIT_FAILURE);
  }
  else
    b->next->config_complete (b->next);
}

static void
filter_get_ready (struct backend *b)
{
  struct backend_filter *f = container_of (b, struct backend_filter, backend);

  b->next->get_ready (b->next); /* exits on failure */

  debug ("%s: get_ready thread_model=%d", b->name, thread_model);

  if (f->filter.get_ready) {
    if (f->filter.get_ready (thread_model) == -1)
      exit (EXIT_FAILURE);
  }
}

static void
filter_after_fork (struct backend *b)
{
  struct backend_filter *f = container_of (b, struct backend_filter, backend);

  b->next->after_fork (b->next); /* exits on failure */

  debug ("%s: after_fork", b->name);

  if (f->filter.after_fork) {
    if (f->filter.after_fork (b->next) == -1)
      exit (EXIT_FAILURE);
  }
}

static void
filter_cleanup (struct backend *b)
{
  struct backend_filter *f = container_of (b, struct backend_filter, backend);

  debug ("%s: cleanup", b->name);
  if (f->filter.cleanup)
    f->filter.cleanup (b->next);

  b->next->cleanup (b->next);
}

static int
filter_preconnect (struct backend *b, int readonly)
{
  struct backend_filter *f = container_of (b, struct backend_filter, backend);

  debug ("%s: preconnect", b->name);

  if (f->filter.preconnect)
    return f->filter.preconnect (b->next->preconnect, b->next, readonly);
  else
    return b->next->preconnect (b->next, readonly);
}

/* magic_config_key only applies to plugins, so this passes the
 * request through to the plugin (hence the name).
 */
static const char *
plugin_magic_config_key (struct backend *b)
{
  return b->next->magic_config_key (b->next);
}

static int
filter_list_exports (struct backend *b, int readonly, int is_tls,
                     struct nbdkit_exports *exports)
{
  struct backend_filter *f = container_of (b, struct backend_filter, backend);

  if (f->filter.list_exports)
    return f->filter.list_exports (backend_list_exports, b->next,
                                   readonly, is_tls, exports);
  return backend_list_exports (b->next, readonly, exports);
}

static const char *
filter_default_export (struct backend *b, int readonly, int is_tls)
{
  struct backend_filter *f = container_of (b, struct backend_filter, backend);

  if (f->filter.default_export)
    return f->filter.default_export (backend_default_export, b->next,
                                     readonly, is_tls);
  return backend_default_export (b->next, readonly);
}

static int
next_open (struct context *c, int readonly, const char *exportname)
{
  struct backend *b = nbdkit_context_get_backend (c);
  struct context *c_next = nbdkit_next_context_open (b, readonly, exportname,
                                                     false);
  struct context *old;

  if (c_next == NULL)
    return -1;
  old = nbdkit_context_set_next (c, c_next);
  assert (old == NULL);
  return 0;
}

static void *
filter_open (struct context *c, int readonly, const char *exportname,
             int is_tls)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  void *handle;

  /* Most filters will call next_open first, resulting in
   * inner-to-outer ordering.
   */
  if (f->filter.open)
    handle = f->filter.open (next_open, c, readonly, exportname,
                             is_tls);
  else if (next_open (c, readonly, exportname) == -1)
    handle = NULL;
  else
    handle = NBDKIT_HANDLE_NOT_NEEDED;
  return handle;
}

static void
filter_close (struct context *c)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);

  assert (c->handle);
  if (f->filter.close)
    f->filter.close (c->handle);
}

static int
filter_prepare (struct context *c, int readonly)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.prepare &&
      f->filter.prepare (c_next, c->handle, readonly) == -1)
    return -1;

  return 0;
}

static int
filter_finalize (struct context *c)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.finalize &&
      f->filter.finalize (c_next, c->handle) == -1)
    return -1;
  return 0;
}

static const char *
filter_export_description (struct context *c)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.export_description)
    return f->filter.export_description (c_next, c->handle);
  else
    return backend_export_description (c_next);
}

static int64_t
filter_get_size (struct context *c)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.get_size)
    return f->filter.get_size (c_next, c->handle);
  else
    return backend_get_size (c_next);
}

static int
filter_block_size (struct context *c,
                   uint32_t *minimum, uint32_t *preferred, uint32_t *maximum)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.block_size)
    return f->filter.block_size (c_next, c->handle,
                                 minimum, preferred, maximum);
  else
    return backend_block_size (c_next,
                               minimum, preferred, maximum);
}

static int
filter_can_write (struct context *c)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.can_write)
    return f->filter.can_write (c_next, c->handle);
  else
    return backend_can_write (c_next);
}

static int
filter_can_flush (struct context *c)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.can_flush)
    return f->filter.can_flush (c_next, c->handle);
  else
    return backend_can_flush (c_next);
}

static int
filter_is_rotational (struct context *c)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.is_rotational)
    return f->filter.is_rotational (c_next, c->handle);
  else
    return backend_is_rotational (c_next);
}

static int
filter_can_trim (struct context *c)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.can_trim)
    return f->filter.can_trim (c_next, c->handle);
  else
    return backend_can_trim (c_next);
}

static int
filter_can_zero (struct context *c)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.can_zero)
    return f->filter.can_zero (c_next, c->handle);
  else
    return backend_can_zero (c_next);
}

static int
filter_can_fast_zero (struct context *c)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.can_fast_zero)
    return f->filter.can_fast_zero (c_next, c->handle);
  else
    return backend_can_fast_zero (c_next);
}

static int
filter_can_extents (struct context *c)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.can_extents)
    return f->filter.can_extents (c_next, c->handle);
  else
    return backend_can_extents (c_next);
}

static int
filter_can_fua (struct context *c)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.can_fua)
    return f->filter.can_fua (c_next, c->handle);
  else
    return backend_can_fua (c_next);
}

static int
filter_can_multi_conn (struct context *c)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.can_multi_conn)
    return f->filter.can_multi_conn (c_next, c->handle);
  else
    return backend_can_multi_conn (c_next);
}

static int
filter_can_cache (struct context *c)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.can_cache)
    return f->filter.can_cache (c_next, c->handle);
  else
    return backend_can_cache (c_next);
}

static int
filter_pread (struct context *c,
              void *buf, uint32_t count, uint64_t offset,
              uint32_t flags, int *err)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.pread)
    return f->filter.pread (c_next, c->handle,
                            buf, count, offset, flags, err);
  else
    return backend_pread (c_next, buf, count, offset, flags, err);
}

static int
filter_pwrite (struct context *c,
               const void *buf, uint32_t count, uint64_t offset,
               uint32_t flags, int *err)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.pwrite)
    return f->filter.pwrite (c_next, c->handle,
                             buf, count, offset, flags, err);
  else
    return backend_pwrite (c_next, buf, count, offset, flags, err);
}

static int
filter_flush (struct context *c,
              uint32_t flags, int *err)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.flush)
    return f->filter.flush (c_next, c->handle, flags, err);
  else
    return backend_flush (c_next, flags, err);
}

static int
filter_trim (struct context *c,
             uint32_t count, uint64_t offset,
             uint32_t flags, int *err)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.trim)
    return f->filter.trim (c_next, c->handle, count, offset,
                           flags, err);
  else
    return backend_trim (c_next, count, offset, flags, err);
}

static int
filter_zero (struct context *c,
             uint32_t count, uint64_t offset, uint32_t flags, int *err)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.zero)
    return f->filter.zero (c_next, c->handle,
                           count, offset, flags, err);
  else
    return backend_zero (c_next, count, offset, flags, err);
}

static int
filter_extents (struct context *c,
                uint32_t count, uint64_t offset, uint32_t flags,
                struct nbdkit_extents *extents, int *err)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.extents)
    return f->filter.extents (c_next, c->handle,
                              count, offset, flags,
                              extents, err);
  else
    return backend_extents (c_next, count, offset, flags,
                            extents, err);
}

static int
filter_cache (struct context *c,
              uint32_t count, uint64_t offset,
              uint32_t flags, int *err)
{
  struct backend *b = c->b;
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  struct context *c_next = c->c_next;

  if (f->filter.cache)
    return f->filter.cache (c_next, c->handle,
                            count, offset, flags, err);
  else
    return backend_cache (c_next, count, offset, flags, err);
}

static struct backend filter_functions = {
  .free = filter_free,
  .thread_model = filter_thread_model,
  .plugin_name = plugin_name,
  .usage = filter_usage,
  .version = filter_version,
  .dump_fields = filter_dump_fields,
  .config = filter_config,
  .config_complete = filter_config_complete,
  .magic_config_key = plugin_magic_config_key,
  .get_ready = filter_get_ready,
  .after_fork = filter_after_fork,
  .cleanup = filter_cleanup,
  .preconnect = filter_preconnect,
  .list_exports = filter_list_exports,
  .default_export = filter_default_export,
  .open = filter_open,
  .prepare = filter_prepare,
  .finalize = filter_finalize,
  .close = filter_close,
  .export_description = filter_export_description,
  .get_size = filter_get_size,
  .block_size = filter_block_size,
  .can_write = filter_can_write,
  .can_flush = filter_can_flush,
  .is_rotational = filter_is_rotational,
  .can_trim = filter_can_trim,
  .can_zero = filter_can_zero,
  .can_fast_zero = filter_can_fast_zero,
  .can_extents = filter_can_extents,
  .can_fua = filter_can_fua,
  .can_multi_conn = filter_can_multi_conn,
  .can_cache = filter_can_cache,
  .pread = filter_pread,
  .pwrite = filter_pwrite,
  .flush = filter_flush,
  .trim = filter_trim,
  .zero = filter_zero,
  .extents = filter_extents,
  .cache = filter_cache,
};

/* Register and load a filter. */
struct backend *
filter_register (struct backend *next, size_t index, const char *filename,
                 void *dl, filter_init_function filter_init)
{
  struct backend_filter *f;
  const struct nbdkit_filter *filter;

  f = calloc (1, sizeof *f);
  if (f == NULL) {
    perror ("strdup");
    exit (EXIT_FAILURE);
  }

  f->backend = filter_functions;
  backend_init (&f->backend, next, index, filename, dl, "filter");

  /* Call the initialization function which returns the address of the
   * filter's own 'struct nbdkit_filter'.
   */
  filter = filter_init ();
  if (!filter) {
    fprintf (stderr, "%s: %s: filter registration function failed\n",
             program_name, filename);
    exit (EXIT_FAILURE);
  }

  /* We do not provide API or ABI guarantees for filters, other than
   * the ABI position and API contents of _api_version and _version to
   * diagnose mismatch from the current nbdkit version.
   */
  if (filter->_api_version != NBDKIT_FILTER_API_VERSION) {
    fprintf (stderr,
             "%s: %s: filter is incompatible with this version of nbdkit "
             "(_api_version = %d, need %d)\n",
             program_name, filename, filter->_api_version,
             NBDKIT_FILTER_API_VERSION);
    exit (EXIT_FAILURE);
  }
  if (filter->_version == NULL ||
      strcmp (filter->_version, PACKAGE_VERSION) != 0) {
    fprintf (stderr,
             "%s: %s: filter is incompatible with this version of nbdkit "
             "(_version = %s, need %s)\n",
             program_name, filename, filter->_version ?: "<null>",
             PACKAGE_VERSION);
    exit (EXIT_FAILURE);
  }

  f->filter = *filter;

  backend_load (&f->backend, f->filter.name, f->filter.load);

  return (struct backend *) f;
}

NBDKIT_DLL_PUBLIC struct backend *
nbdkit_context_get_backend (struct context *c)
{
  assert (c);
  return c->b->next;
}

NBDKIT_DLL_PUBLIC struct context *
nbdkit_next_context_open (struct backend *b,
                          int readonly, const char *exportname, int shared)
{
  struct context *c = threadlocal_get_context ();

  assert (b);
  assert (!c || b == c->b->next);
  return backend_open (b, readonly, exportname, shared || !c || !c->conn);
}

NBDKIT_DLL_PUBLIC void
nbdkit_next_context_close (struct context *c)
{
  if (c)
    backend_close (c);
}

NBDKIT_DLL_PUBLIC struct context *
nbdkit_context_set_next (struct context *c, struct context *next)
{
  struct context *old;

  assert (c);
  if (next)
    assert (next->b == c->b->next);
  old = c->c_next;
  c->c_next = next;
  return old;
}
