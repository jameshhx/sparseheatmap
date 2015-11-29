#include "sparsescroll.h"
#include <stddef.h>
#include <node.h>
#include <math.h>

/**
 * Constructor for a new sparse matrix
 */
Sparsescroll::Sparsescroll(int w, int h, int yaxismultiplier) {
  yMult = yaxismultiplier;
  set_size(w, h);
}

/**
 * Set the size of the matrix
 */
void Sparsescroll::set_size(int w, int h) {
  width = w;
  height = h;
  max = 0;
  data = new unsigned long[height];
  for (int d = 0; d < height; d++) {
    data[d] = 0;
  }
};

/**
 * Destructor
 */
Sparsescroll::~Sparsescroll() {
  delete[] data;
  data = NULL;
}

/**
 * Integrate a sparse array
 */
void Sparsescroll::integrate_sparsearray(Sparsearray *sp) {
  int dlen = sp->datalen;
  int tidx = 0;
  for (int idx = 0; idx < dlen; idx++) {
    unsigned int srcVal = sp->data[idx];
    for (int k = 0; k < yMult; k++) {
      if (tidx < height - 1) {
        data[tidx++] += srcVal;
      }
    }
  }
}

/**
 * Get an intensity map
 */
char *Sparsescroll::get_intensity_map(int w) {
  int h = (int) (((float) w / (float) width) * (float) height);
  lastIntensityHeight = h;
  int sz = w * h;
  lastIntensitySize = sz;
  int osz = height;
  for (int idx = 0; idx < osz; idx++) {
    max = (max < data[idx]) ? data[idx] : max;
  }
  long double fmax = (long double) max;
  for (int idx = 0; idx < osz; idx++) {
    long double val = (long double) data[idx];
    data[idx] = (unsigned long) ((val / fmax) * 255);
  }
  max = 255;
  char *targ = new char[sz];
  double hf = (double) h - (double) 1;
  double myheight = (double) height - (double) 1;
  int index = 0;
  for (int y = 0; y < h; y++) {
    double yp = (double) y / hf;
    double srcy = yp * myheight;
    int floory = (int) floor(srcy);
    int ceily = (int) ceil(srcy);
    double tt = (double) data[floory];
    double bb = (double) data[ceily];
    double yprog = srcy - (double) floory;
    double tots = ((tt * (1.0 - yprog)) + (bb * yprog));
    char ctots = (char) tots;
    for (int x = 0; x < w; x++) {
      targ[index++] = ctots;
    }
  }
  lastIntensityIndex = index - 1;
  return targ;
}