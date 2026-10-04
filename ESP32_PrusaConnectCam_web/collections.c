/* This file is part of the OpenMV project.
 * Copyright (c) 2013-2017 Ibrahim Abdelkader <iabdalkader@openmv.io> & Kwabena W. Agyeman <kwagyeman@openmv.io>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 */

#include "collections.h"
#include <stdint.h>
#define CHAR_BITS (sizeof(char) * 8)
#define CHAR_MASK (CHAR_BITS - 1)
#define CHAR_SHIFT IM_LOG2(CHAR_MASK)

//////////
// lifo //
//////////

void lifo_alloc(lifo_t *ptr, size_t size, size_t data_len)
{
  ptr->len = 0;
  ptr->size = 0;
  ptr->data_len = data_len;
  ptr->data = NULL;
  if (!data_len || !size || size > SIZE_MAX / data_len) return;
  ptr->data = (char *)malloc(size * data_len);
  if (ptr->data) ptr->size = size;
}
void lifo_alloc_all(lifo_t *ptr, size_t *size, size_t data_len)
{
  lifo_alloc(ptr, data_len ? 255 / data_len : 0, data_len);
  *size = ptr->size;
}
void lifo_free(lifo_t *ptr)
{
  free(ptr->data);
  ptr->data = NULL;
  ptr->len = ptr->size = 0;
}

void lifo_clear(lifo_t *ptr)
{
  ptr->len = 0;
}

size_t lifo_size(lifo_t *ptr)
{
  return ptr->len;
}

bool lifo_is_not_empty(lifo_t *ptr)
{
  return ptr->len;
}

bool lifo_is_not_full(lifo_t *ptr)
{
  return ptr->len < ptr->size;
}

void lifo_enqueue(lifo_t *ptr, void *data)
{
  if (!data || !lifo_is_not_full(ptr)) return;
  memcpy(ptr->data + (ptr->len * ptr->data_len), data, ptr->data_len);

  ptr->len += 1;
}

void lifo_dequeue(lifo_t *ptr, void *data)
{
  if (!lifo_is_not_empty(ptr)) return;
  if (data)
  {
    memcpy(data, ptr->data + ((ptr->len - 1) * ptr->data_len), ptr->data_len);
  }

  ptr->len -= 1;
}

void lifo_poke(lifo_t *ptr, void *data)
{
  if (!data || !lifo_is_not_full(ptr)) return;
  memcpy(ptr->data + (ptr->len * ptr->data_len), data, ptr->data_len);
}

void lifo_peek(lifo_t *ptr, void *data)
{
  if (!data || !lifo_is_not_empty(ptr)) return;
  memcpy(data, ptr->data + ((ptr->len - 1) * ptr->data_len), ptr->data_len);
}

/* EOF */
