#include "image.h"
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <glm/glm.hpp>

void write_pgm(const char *filename, float *data, int width, int height, float min, float max) {
  FILE *f = fopen(filename, "w");
  if (!f) {
    fprintf(stderr, "Unable to open %s for writing\n", filename);
    exit(1);
  }
  fprintf(f, "P2 %d %d 255\n", width, height);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      uint8_t val = 255 * glm::clamp(
          (data[y * width + x] - min) / (max - min),
          0.f,
          1.f);
      fprintf(f, "%d ", val);
    }
    fprintf(f, "\n");
  }
  fclose(f);
}

void write_pgm(const char *filename, uint8_t *data, int width, int height) {
  FILE *f = fopen(filename, "w");
  if (!f) {
    fprintf(stderr, "Unable to open %s for writing\n", filename);
    exit(1);
  }
  fprintf(f, "P2 %d %d 255\n", width, height);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      fprintf(f, "%d ", data[y * width + x]);
    }
    fprintf(f, "\n");
  }
  fclose(f);
}
