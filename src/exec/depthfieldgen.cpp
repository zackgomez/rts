#include <cstdio>
#include <cstdint>
#include <algorithm>
#include <glm/glm.hpp>
#include "stb_image.c"

const int DSIZE = 256;
const int RADIUS = 200;

float calculate_distance(uint8_t *data, int width, int height, const glm::vec2 &pt) {
  const glm::vec2 dim(width, height);
  const glm::vec2 pixelCoord = glm::floor(pt * glm::vec2(width, height));

  const int xmin = std::max((int)pixelCoord.x - RADIUS, 0);
  const int xmax = std::min((int)pixelCoord.x + RADIUS + 1, width);
  const int ymin = std::max((int)pixelCoord.y - RADIUS, 0);
  const int ymax = std::min((int)pixelCoord.y + RADIUS + 1, height);
  float dist = HUGE_VAL;
  for (int y = ymin; y < height && y < ymax; y++) {
    for (int x = xmin; x < width && x < xmax; x++) {
      if (data[y * width + x]) {
        glm::vec2 sourcePixelCenter = glm::vec2(x + 0.5f, y + 0.5f) / dim;
        float curDist = glm::distance(sourcePixelCenter, pt);
        dist = std::min(curDist, dist);
      }
    }
  }

  return dist;
}

float *calculate_distance_field(uint8_t *data, int width, int height) {
  const glm::vec2 output_dim(DSIZE, DSIZE);
  const glm::vec2 input_dim(width, height);
  float *depth_field = (float *)malloc(sizeof(*depth_field) * DSIZE * DSIZE);
  for (int y = 0; y < DSIZE; y++) {
    for (int x = 0; x < DSIZE; x++) {
      // center of output pixel
      glm::vec2 pt = (glm::vec2(x, y) + 0.5f) / output_dim;
      // test for 'in' or 'out'
      glm::vec2 sampled_pt = glm::floor(pt * input_dim);
      uint8_t val = data[(int)sampled_pt.x + (int)sampled_pt.y * width];
      float distance = val ? 0.f : calculate_distance(data, width, height, pt);
      depth_field[y * DSIZE + x] = distance;
    }
  }

  return depth_field;
}

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

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s imagefile\n", argv[0]);
    exit(1);
  }
  int width, height, depth;
  uint8_t *pixels = stbi_load(argv[1], &width, &height, &depth, 1);
  if (!pixels) {
    fprintf(stderr, "Unable to load image from %s\n", argv[1]);
    exit(1);
  }

  float *distance_field = calculate_distance_field(pixels, width, height);
  write_pgm("out.pgm", distance_field, DSIZE, DSIZE, 0.f, (float)RADIUS / width);

  stbi_image_free(pixels);
}
