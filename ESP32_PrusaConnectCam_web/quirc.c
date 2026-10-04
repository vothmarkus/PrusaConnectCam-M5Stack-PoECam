/* quirc -- QR-code recognition library
 * Copyright (C) 2010-2012 Daniel Beer <dlbeer@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include "quirc_internal.h"
#include <limits.h>
#ifdef ESP_PLATFORM
#include <esp_heap_caps.h>
#endif

void *quirc_alloc(size_t size)
{
#ifdef ESP_PLATFORM
  void *memory = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (memory) return memory;
#endif
  return malloc(size);
}

const char *quirc_version(void)
{
  return "1.0";
}

//static struct quirc _q;
struct quirc *quirc_new(void)
{
  struct quirc *q = quirc_alloc(sizeof(*q));

  if (!q)
    return NULL;

  memset(q, 0, sizeof(*q));
  return q;
}

void quirc_destroy(struct quirc *q)
{
  if (!q) return;
  free(q->image);
  if (sizeof(*q->image) != sizeof(*q->pixels)) free(q->pixels);
  free(q);
}
int quirc_resize(struct quirc *q, int w, int h)
{
  if (!q || w <= 0 || h <= 0 || w > INT_MAX / h ||
      (size_t)w * (size_t)h > SIZE_MAX / sizeof(quirc_pixel_t)) return -1;
  const size_t size = (size_t)w * (size_t)h;
  uint8_t *new_image = quirc_alloc(size);
  if (!new_image) return -1;
  quirc_pixel_t *new_pixels = NULL;
  if (sizeof(*q->image) != sizeof(*q->pixels)) {
    new_pixels = quirc_alloc(size * sizeof(quirc_pixel_t));
    if (!new_pixels) {
      free(new_image);
      return -1;
    }
  }
  // Commit only after all allocations succeed; old buffers survive OOM.
  free(q->image);
  if (sizeof(*q->image) != sizeof(*q->pixels)) free(q->pixels);
  q->image = new_image;
  q->pixels = new_pixels;
  q->w = w;
  q->h = h;
  return 0;
}

int quirc_count(const struct quirc *q)
{
  return q->num_grids;
}

static const char *const error_table[] = {
    [QUIRC_SUCCESS] = "Success",
    [QUIRC_ERROR_INVALID_GRID_SIZE] = "Invalid grid size",
    [QUIRC_ERROR_INVALID_VERSION] = "Invalid version",
    [QUIRC_ERROR_FORMAT_ECC] = "Format data ECC failure",
    [QUIRC_ERROR_DATA_ECC] = "ECC failure",
    [QUIRC_ERROR_UNKNOWN_DATA_TYPE] = "Unknown data type",
    [QUIRC_ERROR_DATA_OVERFLOW] = "Data overflow",
    [QUIRC_ERROR_MEMORY] = "Out of memory",
    [QUIRC_ERROR_DATA_UNDERFLOW] = "Data underflow"};

const char *quirc_strerror(quirc_decode_error_t err)
{
  if (err >= 0 && err < sizeof(error_table) / sizeof(error_table[0]))
    return error_table[err];

  return "Unknown error";
}

/* EOF */
