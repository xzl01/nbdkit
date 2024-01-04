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
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <sys/time.h>

#include <nbdkit-filter.h>

#include "cleanup.h"
#include "windows-compat.h"

static unsigned retries = 5;    /* 0 = filter is disabled */
static unsigned initial_delay = 2;
static bool exponential_backoff = true;
static bool force_readonly = false;

/* Currently nbdkit_backend_reopen is not safe if another thread makes a
 * request on the same connection (but on other connections it's OK).
 * To work around this for now we limit the thread model here, but
 * this is something we could improve in server/backend.c in future.
 */
static int
retry_thread_model (void)
{
  return NBDKIT_THREAD_MODEL_SERIALIZE_REQUESTS;
}

static int
retry_config (nbdkit_next_config *next, nbdkit_backend *nxdata,
              const char *key, const char *value)
{
  int r;

  if (strcmp (key, "retries") == 0) {
    if (nbdkit_parse_unsigned ("retries", value, &retries) == -1)
      return -1;
    return 0;
  }
  else if (strcmp (key, "retry-delay") == 0) {
    if (nbdkit_parse_unsigned ("retry-delay", value, &initial_delay) == -1)
      return -1;
    if (initial_delay == 0) {
      nbdkit_error ("retry-delay cannot be 0");
      return -1;
    }
    return 0;
  }
  else if (strcmp (key, "retry-exponential") == 0) {
    r = nbdkit_parse_bool (value);
    if (r == -1)
      return -1;
    exponential_backoff = r;
    return 0;
  }
  else if (strcmp (key, "retry-readonly") == 0) {
    r = nbdkit_parse_bool (value);
    if (r == -1)
      return -1;
    force_readonly = r;
    return 0;
  }

  return next (nxdata, key, value);
}

#define retry_config_help \
  "retries=<N>              Number of retries (default: 5).\n" \
  "retry-delay=<N>          Seconds to wait before retry (default: 2).\n" \
  "retry-exponential=yes|no Exponential back-off (default: yes).\n" \
  "retry-readonly=yes|no    Force read-only on failure (default: no).\n"

struct retry_handle {
  int readonly;                 /* Save original readonly setting. */
  char *exportname;             /* Client exportname. */
  nbdkit_context *context;      /* Context learned during .open. */
  unsigned reopens;
  bool open;
};

/* This function encapsulates the common retry logic used across all
 * data commands.  If it returns true then the data command will retry
 * the operation.  ‘struct retry_data’ is stack data saved between
 * retries within the same command, and is initialized to zero.
 */
struct retry_data {
  int retry;                    /* Retry number (0 = first time). */
  int delay;                    /* Seconds to wait before retrying. */
};

static bool
valid_range (nbdkit_next *next,
             uint32_t count, uint64_t offset, bool is_write, int *err)
{
  if ((int64_t) offset + count > next->get_size (next)) {
    *err = is_write ? ENOSPC : EIO;
    return false;
  }
  return true;
}

static bool
do_retry (struct retry_handle *h, struct retry_data *data,
          nbdkit_next **next, const char *method, int *err)
{
  nbdkit_next *new_next, *old_next;

  /* If it's the first retry, initialize the other fields in *data. */
  if (data->retry == 0)
    data->delay = initial_delay;

 again:
  if (data->retry >= retries) {
    nbdkit_debug ("could not recover after %d retries", retries);
    return false;
  }

  /* Since we will retry, log the original errno otherwise it will be lost. */
  nbdkit_debug ("%s failed: original errno = %d", method, *err);

  nbdkit_debug ("retry %d: waiting %d seconds before retrying",
                data->retry+1, data->delay);
  if (nbdkit_nanosleep (data->delay, 0) == -1) {
    /* We could do this but it would overwrite the more important
     * errno from the underlying data call.
     */
    if (*err == 0)
      *err = errno;
    return false;
  }

  /* Update *data in case we are called again. */
  data->retry++;
  if (exponential_backoff)
    data->delay *= 2;

  /* Close the old connection. */
  h->reopens++;
  h->open = false;
  if (*next != NULL) {
    /* Failure to finalize a connection indicates permanent data loss,
     * which we treat the same as the original command failing.
     */
    if ((*next)->finalize (*next) == -1) {
      *err = ESHUTDOWN;
      goto again;
    }
    nbdkit_next_context_close (*next);
    old_next = nbdkit_context_set_next (h->context, NULL);
    assert (old_next == *next);
    *next = NULL;
  }
  /* Open a new connection. */
  new_next = nbdkit_next_context_open (nbdkit_context_get_backend (h->context),
                                       h->readonly || force_readonly,
                                       h->exportname, false);
  if (new_next == NULL) {
    *err = ESHUTDOWN;
    goto again;
  }
  if (new_next->prepare (new_next) == -1) {
    new_next->finalize (new_next);
    nbdkit_next_context_close (new_next);
    *err = ESHUTDOWN;
    goto again;
  }
  old_next = nbdkit_context_set_next (h->context, new_next);
  assert (old_next == NULL);
  *next = new_next;
  h->open = true;

  /* Retry the data command. */
  return true;
}

static void *
retry_open (nbdkit_next_open *next, nbdkit_context *nxdata,
            int readonly, const char *exportname, int is_tls)
{
  struct retry_handle *h;
  struct retry_data data = {0};

  h = malloc (sizeof *h);
  if (h == NULL) {
    nbdkit_error ("malloc: %m");
    return NULL;
  }

  h->readonly = readonly;
  h->exportname = strdup (exportname);
  h->context = nxdata;
  if (h->exportname == NULL) {
    nbdkit_error ("strdup: %m");
    free (h);
    return NULL;
  }
  h->reopens = 0;

  if (next (nxdata, readonly, exportname) != -1)
    h->open = true;
  else {
    /* Careful - our .open must not return a handle unless do_retry()
     * works, as the caller's next action will be calling .get_size
     * and similar probe functions which we do not bother to wire up
     * into retry logic because they only need to be used right after
     * connecting.
     */
    nbdkit_next *next_handle = NULL;
    int err = ESHUTDOWN;

    h->open = false;
    while (! h->open && do_retry (h, &data, &next_handle, "open", &err))
      ;

    if (! h->open) {
      free (h->exportname);
      free (h);
      return NULL;
    }
  }
  return h;
}

static void
retry_close (void *handle)
{
  struct retry_handle *h = handle;

  nbdkit_debug ("reopens needed: %u", h->reopens);
  free (h->exportname);
  free (h);
}

static int
retry_pread (nbdkit_next *next,
             void *handle, void *buf, uint32_t count, uint64_t offset,
             uint32_t flags, int *err)
{
  struct retry_handle *h = handle;
  struct retry_data data = {0};
  int r;

 again:
  if (! (h->open && valid_range (next, count, offset, false, err)))
    r = -1;
  else
    r = next->pread (next, buf, count, offset, flags, err);
  if (r == -1 && do_retry (h, &data, &next, "pread", err))
    goto again;

  return r;
}

/* Write. */
static int
retry_pwrite (nbdkit_next *next,
              void *handle,
              const void *buf, uint32_t count, uint64_t offset,
              uint32_t flags, int *err)
{
  struct retry_handle *h = handle;
  struct retry_data data = {0};
  int r;

 again:
  if (h->reopens && force_readonly) {
    *err = EROFS;
    return -1;
  }
  if (! (h->open && valid_range (next, count, offset, true, err)))
    r = -1;
  else if (next->can_write (next) != 1) {
    *err = EROFS;
    r = -1;
  }
  else if (flags & NBDKIT_FLAG_FUA &&
           next->can_fua (next) <= NBDKIT_FUA_NONE) {
    *err = EIO;
    r = -1;
  }
  else
    r = next->pwrite (next, buf, count, offset, flags, err);
  if (r == -1 && do_retry (h, &data, &next, "pwrite", err))
    goto again;

  return r;
}

/* Trim. */
static int
retry_trim (nbdkit_next *next,
            void *handle,
            uint32_t count, uint64_t offset, uint32_t flags,
            int *err)
{
  struct retry_handle *h = handle;
  struct retry_data data = {0};
  int r;

 again:
  if (h->reopens && force_readonly) {
    *err = EROFS;
    return -1;
  }
  if (! (h->open && valid_range (next, count, offset, true, err)))
    r = -1;
  else if (next->can_trim (next) != 1) {
    *err = EROFS;
    r = -1;
  }
  else if (flags & NBDKIT_FLAG_FUA &&
           next->can_fua (next) <= NBDKIT_FUA_NONE) {
    *err = EIO;
    r = -1;
  }
  else
    r = next->trim (next, count, offset, flags, err);
  if (r == -1 && do_retry (h, &data, &next, "trim", err))
    goto again;

  return r;
}

/* Flush. */
static int
retry_flush (nbdkit_next *next,
             void *handle, uint32_t flags,
             int *err)
{
  struct retry_handle *h = handle;
  struct retry_data data = {0};
  int r;

 again:
  if (! h->open)
    r = -1;
  else if (next->can_flush (next) != 1) {
    *err = EIO;
    r = -1;
  }
  else
    r = next->flush (next, flags, err);
  if (r == -1 && do_retry (h, &data, &next, "flush", err))
    goto again;

  return r;
}

/* Zero. */
static int
retry_zero (nbdkit_next *next,
            void *handle,
            uint32_t count, uint64_t offset, uint32_t flags,
            int *err)
{
  struct retry_handle *h = handle;
  struct retry_data data = {0};
  int r;

 again:
  if (h->reopens && force_readonly) {
    *err = EROFS;
    return -1;
  }
  if (flags & NBDKIT_FLAG_FAST_ZERO &&
      (! h->open || next->can_fast_zero (next) != 1)) {
    *err = EOPNOTSUPP;
    return -1;
  }
  if (! (h->open && valid_range (next, count, offset, true, err)))
    r = -1;
  else if (next->can_zero (next) <= NBDKIT_ZERO_NONE) {
    *err = EROFS;
    r = -1;
  }
  else if (flags & NBDKIT_FLAG_FUA &&
           next->can_fua (next) <= NBDKIT_FUA_NONE) {
    *err = EIO;
    r = -1;
  }
  else
    r = next->zero (next, count, offset, flags, err);
  if (r == -1 && do_retry (h, &data, &next, "zero", err))
    goto again;

  return r;
}

/* Extents. */
static int
retry_extents (nbdkit_next *next,
               void *handle,
               uint32_t count, uint64_t offset, uint32_t flags,
               struct nbdkit_extents *extents, int *err)
{
  struct retry_handle *h = handle;
  struct retry_data data = {0};
  CLEANUP_EXTENTS_FREE struct nbdkit_extents *extents2 = NULL;
  int r;
  size_t i;

 again:
  if (! (h->open && valid_range (next, count, offset, false, err)))
    r = -1;
  else if (next->can_extents (next) != 1) {
    *err = EIO;
    r = -1;
  }
  else {
    /* Each retry must begin with extents reset to the right beginning. */
    nbdkit_extents_free (extents2);
    extents2 = nbdkit_extents_new (offset, next->get_size (next));
    if (extents2 == NULL) {
      *err = errno;
      return -1; /* Not worth a retry after ENOMEM. */
    }
    r = next->extents (next, count, offset, flags, extents2, err);
  }
  if (r == -1 && do_retry (h, &data, &next, "extents", err))
    goto again;

  if (r == 0) {
    /* Transfer the successful extents back to the caller. */
    for (i = 0; i < nbdkit_extents_count (extents2); ++i) {
      struct nbdkit_extent e = nbdkit_get_extent (extents2, i);

      if (nbdkit_add_extent (extents, e.offset, e.length, e.type) == -1) {
        *err = errno;
        return -1;
      }
    }
  }

  return r;
}

/* Cache. */
static int
retry_cache (nbdkit_next *next,
             void *handle,
             uint32_t count, uint64_t offset, uint32_t flags,
             int *err)
{
  struct retry_handle *h = handle;
  struct retry_data data = {0};
  int r;

 again:
  if (! (h->open && valid_range (next, count, offset, false, err)))
    r = -1;
  else if (next->can_cache (next) <= NBDKIT_CACHE_NONE) {
    *err = EIO;
    r = -1;
  }
  else
    r = next->cache (next, count, offset, flags, err);
  if (r == -1 && do_retry (h, &data, &next, "cache", err))
    goto again;

  return r;
}

static struct nbdkit_filter filter = {
  .name              = "retry",
  .longname          = "nbdkit retry filter",
  .thread_model      = retry_thread_model,
  .config            = retry_config,
  .config_help       = retry_config_help,
  .open              = retry_open,
  .close             = retry_close,
  .pread             = retry_pread,
  .pwrite            = retry_pwrite,
  .trim              = retry_trim,
  .flush             = retry_flush,
  .zero              = retry_zero,
  .extents           = retry_extents,
  .cache             = retry_cache,
};

NBDKIT_REGISTER_FILTER (filter)
