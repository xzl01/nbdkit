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

#include <nbdkit-filter.h>

#include "byte-swapping.h"
#include "cleanup.h"

#include "partition.h"

unsigned partnum = 0;

/* sector_size will default to 512, may get set to 4096.
 * finalized in prepare. user can override via config.
 * if still zero at prepare time, apply default.
 */
unsigned sector_size = 0;

/* Called for each key=value passed on the command line. */
static int
partition_config (nbdkit_next_config *next, nbdkit_backend *nxdata,
                  const char *key, const char *value)
{
  if (strcmp (key, "partition") == 0) {
    if (nbdkit_parse_unsigned ("partition", value, &partnum) == -1)
      return -1;
    if (partnum == 0) {
      nbdkit_error ("invalid partition number");
      return -1;
    }
    return 0;
  }
  else if (strcmp (key, "partition-sectorsize") == 0) {
    sector_size = nbdkit_parse_size (value);
    /* N.B. error from parse_size() -> -1 is covered by below: -1 != [512, 4096] */
    if ( sector_size != SECTOR_SIZE_512 && sector_size != SECTOR_SIZE_4K ) {
      nbdkit_error ("invalid partition-sectorsize, it must be '512' or '4086'");
      return -1;
    }
    return 0;
  }
  else
    return next (nxdata, key, value);
}

/* Check the user did pass partition number. */
static int
partition_config_complete (nbdkit_next_config_complete *next,
                           nbdkit_backend *nxdata)
{
  if (partnum == 0) {
    nbdkit_error ("you must supply the partition parameter on the command "
                  "line");
    return -1;
  }

  return next (nxdata);
}

#define partition_config_help \
  "partition=<PART>    (required) The partition number (counting from 1)."

struct handle {
  int64_t offset;
  int64_t range;
  const char *type;
};

/* Open a connection. */
static void *
partition_open (nbdkit_next_open *next, nbdkit_context *nxdata,
                int readonly, const char *exportname, int is_tls)
{
  struct handle *h;

  if (next (nxdata, readonly, exportname) == -1)
    return NULL;

  h = malloc (sizeof *h);
  if (h == NULL) {
    nbdkit_error ("malloc: %m");
    return NULL;
  }

  /* These are set in the prepare method. */
  h->offset = h->range = -1;
  return h;
}

static void
partition_close (void *handle)
{
  struct handle *h = handle;

  free (h);
}

static int
partition_prepare (nbdkit_next *next,
                   void *handle, int readonly)
{
  struct handle *h = handle;
  int64_t size;
  uint8_t lba01[2*SECTOR_SIZE_4K]; /* LBA 0 and 1, may only use 2*512 bytes */
  int r;
  int err;

  if (sector_size == 0) {
    uint32_t minimum, preferred, maximum;
    if (next->block_size (next, &minimum, &preferred, &maximum) != 0)
      return -1;
    if (minimum == SECTOR_SIZE_512 || minimum == SECTOR_SIZE_4K) {
      nbdkit_debug ("underlying storage has a minimum read blocksize: "
                    "setting partition-sectorsize=%d",
                    minimum);
      sector_size = minimum;
    } else {
      sector_size = SECTOR_SIZE_DEFAULT; /* aka 512 */
    }
  }

  size = next->get_size (next);
  if (size == -1)
    return -1;
  if (size < 2 * sector_size) {
    nbdkit_error ("disk is too small to be a partitioned disk");
    return -1;
  }

  nbdkit_debug ("disk size=%" PRIi64, size);

  if (next->pread (next, lba01, 2 * sector_size, 0, 0, &err) == -1)
    return -1;

  /* Is it GPT? */
  /* for 512b sectors, this used to be 2 * 34 * sector_size.
   * Which was pmbr sector + primary gpt header sector + 32 sectors of entries,
   * and then doubled.
   * (though the secondary only has header & entries, so the pmbr shouldn't
   * have been part of the doubling, but was harmless overcounting,
   * since the minimum disk should have at least one LBA in a partition, and
   * in practice is much, much larger.
   * Now that we are 512b/4k sector-aware, count the entries as the 16kb they
   * are.  Only the pmbr and headers are tied to the sector size.
   * This also now more closely matches the similar calculation done in
   * partition-gpt.c
   */
  if (size >= 3 * sector_size + 2 * 128 * 128  &&
      memcmp (&lba01[sector_size], "EFI PART", 8) == 0) {
    r = find_gpt_partition (next, size, &lba01[sector_size], &h->offset,
                            &h->range);
    h->type = "GPT";
  }
  /* Is it MBR? */
  else if (lba01[0x1fe] == 0x55 && lba01[0x1ff] == 0xAA) {
    r = find_mbr_partition (next, size, lba01, &h->offset, &h->range);
    h->type = "MBR";
  }
  else {
    nbdkit_error ("disk does not contain MBR or GPT partition table signature");
    r = -1;
  }
  if (r == -1)
    return -1;

  /* The find_*_partition functions set h->offset & h->range to the
   * partition boundaries.  We additionally check that they are inside
   * the underlying disk.
   */
  if (h->offset < 0 || h->range < 0 || h->offset + h->range > size) {
    nbdkit_error ("partition is outside the disk");
    return -1;
  }

  nbdkit_debug ("partition offset=%" PRIi64 " range=%" PRIi64,
                h->offset, h->range);

  return 0;
}

/* Description. */
static const char *
partition_export_description (nbdkit_next *next,
                              void *handle)
{
  struct handle *h = handle;
  const char *base = next->export_description (next);

  assert (h->type);
  if (!base)
    return NULL;
  return nbdkit_printf_intern ("partition %d of %s disk: %s", partnum, h->type,
                               base);
}

/* Get the file size. */
static int64_t
partition_get_size (nbdkit_next *next,
                    void *handle)
{
  struct handle *h = handle;

  return h->range;
}

/* Read data. */
static int
partition_pread (nbdkit_next *next,
                 void *handle, void *buf, uint32_t count, uint64_t offs,
                 uint32_t flags, int *err)
{
  struct handle *h = handle;

  return next->pread (next, buf, count, offs + h->offset, flags, err);
}

/* Write data. */
static int
partition_pwrite (nbdkit_next *next,
                  void *handle,
                  const void *buf, uint32_t count, uint64_t offs,
                  uint32_t flags, int *err)
{
  struct handle *h = handle;

  return next->pwrite (next, buf, count, offs + h->offset, flags, err);
}

/* Trim data. */
static int
partition_trim (nbdkit_next *next,
                void *handle, uint32_t count, uint64_t offs, uint32_t flags,
                int *err)
{
  struct handle *h = handle;

  return next->trim (next, count, offs + h->offset, flags, err);
}

/* Zero data. */
static int
partition_zero (nbdkit_next *next,
                void *handle, uint32_t count, uint64_t offs, uint32_t flags,
                int *err)
{
  struct handle *h = handle;

  return next->zero (next, count, offs + h->offset, flags, err);
}

/* Extents. */
static int
partition_extents (nbdkit_next *next,
                   void *handle, uint32_t count, uint64_t offs, uint32_t flags,
                   struct nbdkit_extents *extents, int *err)
{
  struct handle *h = handle;
  size_t i;
  CLEANUP_EXTENTS_FREE struct nbdkit_extents *extents2 = NULL;
  struct nbdkit_extent e;

  extents2 = nbdkit_extents_new (offs + h->offset, h->offset + h->range);
  if (extents2 == NULL) {
    *err = errno;
    return -1;
  }
  if (next->extents (next, count, offs + h->offset, flags, extents2,
                     err) == -1)
    return -1;

  for (i = 0; i < nbdkit_extents_count (extents2); ++i) {
    e = nbdkit_get_extent (extents2, i);
    e.offset -= h->offset;
    if (nbdkit_add_extent (extents, e.offset, e.length, e.type) == -1) {
      *err = errno;
      return -1;
    }
  }
  return 0;
}

/* Cache data. */
static int
partition_cache (nbdkit_next *next,
                 void *handle, uint32_t count, uint64_t offs, uint32_t flags,
                 int *err)
{
  struct handle *h = handle;

  return next->cache (next, count, offs + h->offset, flags, err);
}

static struct nbdkit_filter filter = {
  .name               = "partition",
  .longname           = "nbdkit partition filter",
  .config             = partition_config,
  .config_complete    = partition_config_complete,
  .config_help        = partition_config_help,
  .open               = partition_open,
  .prepare            = partition_prepare,
  .close              = partition_close,
  .export_description = partition_export_description,
  .get_size           = partition_get_size,
  .pread              = partition_pread,
  .pwrite             = partition_pwrite,
  .trim               = partition_trim,
  .zero               = partition_zero,
  .extents            = partition_extents,
  .cache              = partition_cache,
};

NBDKIT_REGISTER_FILTER (filter)
