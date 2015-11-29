#include "sparsematrix.h"
#include <stddef.h>
#include <node.h>
#include <math.h>

/**
 * Constructor for a new sparse matrix
 */
Sparsematrix::Sparsematrix(int w, int h, int bw, int bh, int lt, unsigned int *bdata) {
  blobwidth = bw;
  blobheight = bh;
  layout = lt;
  blob = bdata;
  blobhalfwidth = (int) bw / 2;
  blobhalfheight = (int) bh / 2;
  set_size(w, h);
}

/**
 * Set the size of the matrix
 */
void Sparsematrix::set_size(int x, int y) {
  width = x;
  height = y;
  max = 0;
  data = new unsigned long[width * height];
  for (int d = 0; d < width * height; d++) {
    data[d] = 0;
  }
};

/**
 * Get the area of the matrix
 */
unsigned int Sparsematrix::area() {
  return width * height;
}

/**
 * Destructor
 */
Sparsematrix::~Sparsematrix() {
  delete[] data;
  data = NULL;
}

/**
 * Get an intensity map
 */
char *Sparsematrix::get_intensity_map(int w) {
  int h = (int) (((float) w / (float) width) * (float) height);
  lastIntensityHeight = h;
  int sz = w * h;
  lastIntensitySize = sz;
  int osz = width * height;
  for (int idx = 0; idx < osz; idx++) {
    max = (max < data[idx]) ? data[idx] : max;
  }
  long double fmax = (long double) max;
  for (int idx = 0; idx < osz; idx++) {
    long double val = (long double)data[idx];
    data[idx] = (unsigned long)((val / fmax) * 255);
  }
  max = 255;
  char *targ = new char[sz];
  double hf = (double) h - (double) 1;
  double wf = (double) w - (double) 1;
  double myheight = (double) height - (double) 1;
  double mywidth = (double) width - (double) 1;
  int index = 0;
  for (int y = 0; y < h; y++) {
    double yp = (double) y / hf;
    double srcy = yp * myheight;
    int floory = (int) floor(srcy);
    int ceily = (int) ceil(srcy);
    int ybase = floory * width;
    int yceil = ceily * width;
    double yprog = srcy - (double) floory;
    for (int x = 0; x < w; x++) {
      double xp = (double) x / wf;
      double srcx = xp * mywidth;
      int floorx = (int) floor(srcx);
      int ceilx = (int) ceil(srcx);
      double xprog = srcx - (double) floorx;
      double tl = (double) data[ybase + floorx];
      double tr = (double) data[ybase + ceilx];
      double bl = (double) data[yceil + floorx];
      double br = (double) data[yceil + ceilx];
      double topavr = (tl * (1.0 - xprog)) + (tr * xprog);
      double botavr = (bl * (1.0 - xprog)) + (br * xprog);
      double tots = ((topavr * (1.0 - yprog)) + (botavr * yprog));
      targ[index++] = (char)tots;
    }
  }
  lastIntensityIndex = index - 1;
  return targ;
}

/**
 * Set a blob at a position
 */
void Sparsematrix::set_blob(int x, int y) {
  int ydesto = y - blobhalfheight + blobheight;
  int xdesto = x - blobhalfwidth + blobwidth;
  int blobidx = 0;
  if (xdesto < width && x - blobhalfwidth >= 0 && ydesto < height &&
      (int) y - (int) blobhalfheight >= 0) {
    for (int yp = y - blobhalfheight; yp < ydesto; yp++) {
      int ydpos = yp * width;
      for (int xp = x - blobhalfwidth; xp < xdesto; xp++) {
        data[ydpos + xp] += blob[blobidx++];
      }
    }
  } else {
    int nx = x;
    int ny = y;
    for (int yp = ny - blobhalfheight; yp < ydesto; yp++) {
      int ydpos = yp * width;
      if (yp >= 0 && yp < (int) height) {
        for (int xp = nx - blobhalfwidth; xp < xdesto; xp++) {
          if (xp >= 0 && xp < (int) width) {
            blobidx = ((yp - (ny - blobhalfheight)) * blobwidth) + ((xp - (nx - blobhalfwidth)));
            data[ydpos + xp] += blob[blobidx];
          }
        }
      }
    }
  }
}

/**
 * Integrate a sparse array
 */
void Sparsematrix::integrate_sparsearray(Sparsearray *sp) {
  float spw = (float) sp->width;
  float ow = (float) width;
  int dlen = sp->datalen;
  if (layout == 0) {
    // CENTERFIXEDWIDTH
    int xofs = (int) ((ow - spw) / 2.0);
    for (int idx = 0; idx < dlen; idx += 3) {
      int nx = (int) ((float) sp->data[idx + 1] + xofs);
      int ny = (int) sp->data[idx + 2];
      if (ny <= height + blobhalfheight) {
        set_blob(nx, ny);
      }
    }
  } else if (layout == 1) {
    // STRETCH
    for (int idx = 0; idx < dlen; idx += 3) {
      int nx = (int) (((float) sp->data[idx + 1] / spw) * ow);
      int ny = (int) sp->data[idx + 2];
      if (ny <= (int) height + (int) blobhalfheight) {
        set_blob(nx, ny);
      }
    }
  } else if (layout == 2) {
    // LEFTFIXEDWIDTH
    for (int idx = 0; idx < dlen; idx += 3) {
      int nx = (int) sp->data[idx + 1];
      int ny = (int) sp->data[idx + 2];
      if (ny <= (int) height + (int) blobhalfheight) {
        set_blob(nx, ny);
      }
    }
  }
}