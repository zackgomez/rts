#pragma once
#include <cstdint>

void write_pgm(const char *filename, float *data, int width, int height,
               float min, float max);
void write_pgm(const char *filename, uint8_t *data, int width, int height);