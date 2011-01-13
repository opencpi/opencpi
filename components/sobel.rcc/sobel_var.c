/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue Dec 14 15:57:31 2010 EST
 * BASED ON THE FILE: sobel.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: sobel
 */
#include "sobel_Worker.h"

// Define state between runs.  Known to be initialized to zero.
typedef struct {
  unsigned inLine;  // The next line of input I expect
  unsigned outLine; // The next line of output I will produce
  Pixel *outPixels;   // The next line in the output buffer
  unsigned outLines;  // Lines left in image
  uint32_t imageBytes;
} State;

static uint32 sizes[] = {sizeof(State), 0};

SOBEL_METHOD_DECLARATIONS;
RCCDispatch sobel = {
  /* insert any custom initializations here */
  .memories = sizes
  SOBEL_DISPATCH
};

/*
 * Methods to implement for worker sobel, based on metadata.
*/

static RCCResult initialize(RCCWorker *self) {
  return RCC_OK;
}

// Define pixel data type
typedef uint8_t Pixel;
typedef int16_t PixelTemp;
#define MAX UINT8_MAX

// Compute one line of output
static void doLine(Pixel *l0, Pixel *l1, Pixel *l2, Pixel *out, unsigned width) {
  for (unsigned i = 1; i < width - 1; i++) {
    PixelTemp t =
      -1 * l0[i-1] + 1 * l0[i+1] +
      -2 * l1[i-1] + 2 * l1[i+1] +
      -1 * l2[i-1] + 1 * l2[i+1];
    out[i] = t < 0 ? 0 : (t > MAX ? MAX : t);
  }    
  out[0] = out[width-1] = 0; // boundary conditions
}


static RCCResult run(RCCWorker *self,
                     RCCBoolean timedOut,
                     RCCBoolean *newRunCondition) {
  SobelProperties *p = self->properties;
  State *s = self->memories[0];
  RCCPort
    *in = self->port[SOBEL_IN],
    *out = self->port[SOBEL_OUT];
  unsigned inLines = in->current.length / p->width * sizeof(Pixel);
  
  // If we are looking at a new output buffer...
  if (!s->outPixels) {
    s->outPixels = out->current.data;                   // line pointer in output
    s->outLines = out->current.maxLength / p->width; // lines left in buffer
    out->output.length = 0;
  }
  if (!s->inPixels) {
    s->inPixels = in->current.data;
    s->inLines = out->current.maxLength / p->width; // lines left in buffer
  }



  // Produce a line of output
  if (s->imageLine == 0 || s->imageLine == p->height - 1)
    // If we are at the first line of an output image, it is zeroed
    memset(s->outPixels, 0, s->lineBytes);
 else if (
    out->out.length += s->lineBytes;
    // 
  }

  }
  // If this line is #2 or more, we have work to do
  if (s->inLine >= 2) {
    doLine(l0, l1, l2, s->outPixels, p->width);
    s->imageLine++;
    s->bufferLines--;
    out->output.length += s->lineBytes;
    if (s->imageLine == p->height)
      s->bufferLines = 0;
    if (s->bufferLines == 0) {
      // This buffer should be shipped, and the next one can be
      // as large as the rest of the image
      self->container.advance(out, s->imageLines * s->lineBytes);
      s->outPixels = 0; // indicate new output buffer
    }}

  return RCC_ADVANCE;
}
