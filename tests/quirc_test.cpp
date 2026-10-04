#include <cassert>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <iostream>
extern "C" {
#include "../ESP32_PrusaConnectCam_web/quirc.h"
#include "../ESP32_PrusaConnectCam_web/collections.h"
}
#include "../ESP32_PrusaConnectCam_web/protocol.h"
#include "qr_fixtures.h"
static int allocationBudget = -1;
static int outstanding = 0;
extern "C" void *test_malloc(size_t size) {
  if (!allocationBudget) return nullptr;
  if (allocationBudget > 0) --allocationBudget;
  void *p = std::malloc(size);
  if (p) ++outstanding;
  return p;
}
extern "C" void test_free(void *p) {
  if (p) --outstanding;
  std::free(p);
}
static void recognize(const char *const *matrix, size_t size, const char *payload, bool mirror) {
  quirc *q = quirc_new();
  assert(q && quirc_resize(q, 320, 240) == 0);
  uint8_t *image = quirc_begin(q, nullptr, nullptr);
  std::memset(image, 255, 320 * 240);
  const int scale = 4, left = (320 - size * scale) / 2, top = (240 - size * scale) / 2;
  for (size_t y = 0; y < size; ++y)
    for (size_t x = 0; x < size; ++x)
      for (int dy = 0; dy < scale; ++dy)
        for (int dx = 0; dx < scale; ++dx)
          image[(top + y * scale + dy) * 320 + left + x * scale + dx] =
              matrix[y][mirror ? size - x - 1 : x] == 'X' ? 0 : 255;
  quirc_end(q);
  assert(quirc_count(q) == 1);
  quirc_code code;
  quirc_data data;
  quirc_extract(q, 0, &code);
  quirc_decode_error_t error = quirc_decode(&code, &data);
  if (error == QUIRC_ERROR_DATA_ECC) { quirc_flip(&code); error = quirc_decode(&code, &data); }
  assert(error == QUIRC_SUCCESS);
  assert(data.payload_len == static_cast<int>(std::strlen(payload)));
  assert(!std::memcmp(data.payload, payload, data.payload_len));
  char token[21];
  assert(prusa::pairingToken(reinterpret_cast<char *>(data.payload), data.payload_len, token));
  assert(!std::strcmp(token, expected_token));
  allocationBudget = 0;
  assert(quirc_decode(&code, &data) == QUIRC_ERROR_MEMORY);
  allocationBudget = -1;
  quirc_destroy(q);
  assert(outstanding == 0);
}
int main() {
  quirc_destroy(nullptr);
  allocationBudget = 0;
  assert(!quirc_new());
  allocationBudget = -1;
  quirc *q = quirc_new();
  assert(q && quirc_resize(q, 32, 32) == 0);
  uint8_t *before = quirc_begin(q, nullptr, nullptr);
  std::memset(before, 255, 32 * 32);
  allocationBudget = 0;
  assert(quirc_resize(q, 64, 64) == -1);
  assert(quirc_begin(q, nullptr, nullptr) == before);
  quirc_end(q);
  allocationBudget = -1;
  assert(quirc_resize(q, -1, 32) == -1);
  assert(quirc_resize(q, 0, 32) == -1);
  assert(quirc_resize(q, INT_MAX, INT_MAX) == -1);
  assert(quirc_resize(nullptr, 32, 32) == -1);
  quirc_destroy(q);
  assert(outstanding == 0);
  lifo_t stack;
  size_t capacity = 99;
  allocationBudget = 0;
  lifo_alloc_all(&stack, &capacity, sizeof(int));
  assert(capacity == 0 && !lifo_is_not_full(&stack));
  int item = 42;
  lifo_enqueue(&stack, &item);
  lifo_dequeue(&stack, &item);
  assert(lifo_size(&stack) == 0);
  lifo_free(&stack);
  allocationBudget = -1;
  lifo_alloc_all(&stack, &capacity, 0);
  assert(capacity == 0);
  lifo_free(&stack);
  recognize(legacy_matrix, sizeof(legacy_matrix) / sizeof(*legacy_matrix), legacy_payload, false);
  recognize(current_matrix, sizeof(current_matrix) / sizeof(*current_matrix), current_payload, false);
  recognize(current_matrix, sizeof(current_matrix) / sizeof(*current_matrix), current_payload, true);
  assert(outstanding == 0);
  std::cout << "QR tests passed: recognition, old/new links, mirroring, allocation failures\n";
}
