/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue Dec 14 15:57:31 2010 EST
 * BASED ON THE FILE: sobel.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: sobel
 */
#include <string.h>
#include "sobel_Worker.h"

// Algorithm specifics:
typedef uint8_t Pixel;      // the data type of pixels
#define FRAME_BYTES (p->height * p->width * sizeof(Pixel)) // pixels per frame
typedef int16_t PixelTemp;  // the data type for intermediate pixel math
#define MAX UINT8_MAX       // the maximum pixel value
#define KERNEL_SIZE 3       // the size of the kernel

// Define state between runs.  Will be initialized to zero for us.
#define LINE_BYTES (p->width * sizeof(Pixel))
#define HISTORY_SIZE (KERNEL_SIZE - 1)
typedef struct {
  unsigned inLine;                 // The next line# of input I expect
  RCCBuffer buffers[HISTORY_SIZE]; // buffer history (ring)
  unsigned next;                   // next buffer slot to use
  RCCBuffer *oldest;               // oldest in history, initially null
} SobelState;

static uint32_t sizes[] = {sizeof(SobelState), 0};
static RCCPortInfo pinfo[] = { {SOBEL_IN,0,KERNEL_SIZE+1},{-1,0,0} };

SOBEL_METHOD_DECLARATIONS;
RCCDispatch sobel = {
  /* insert any custom initializations here */
  .memSizes = sizes,
  .portInfo = pinfo,
  SOBEL_DISPATCH
};

// Compute one line of output
static void
doLine(Pixel *l0, Pixel *l1, Pixel *l2, Pixel *out, unsigned width) {
  unsigned i; // don't depend on c99 yet
  for (i = 1; i < width - 1; i++) {
    PixelTemp t =
      -1 * l0[i-1] + 1 * l0[i+1] +
      -2 * l1[i-1] + 2 * l1[i+1] +
      -1 * l2[i-1] + 1 * l2[i+1];
    out[i] = t < 0 ? 0 : (t > MAX ? MAX : t);
  }    
  out[0] = out[width-1] = 0; // boundary conditions
}


// A run condition for flushing zero-length message
// Basically we only wait for an output buffer, not an input
// This will be unnecessary when we have a separate EOS indication.
static RCCPortMask endMask[] = {1 << SOBEL_OUT, 0};
static RCCRunCondition end = {endMask, 0, 0};

/*
 * Methods to implement for worker sobel, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  SobelProperties *p = self->properties;
  SobelState *s = self->memories[0];
  RCCPort *in = &self->ports[SOBEL_IN],
          *out = &self->ports[SOBEL_OUT];
  const RCCContainer *c = &self->container;  
  (void)timedOut;

  // End state:  just send the zero length message to indicate "done"
  // This will be unnecessary when EOS indication is fixed

  // Arrange to send the zero-length message after the last line of last image
  // This will be unnecessary when EOS indication is fixed
  if (in->input.length == 0) {
    self->runCondition = &end;
    *newRunCondition = 1;
    out->output.length = 0;
    c->advance(out, 0);
    return RCC_DONE;
  }

  if ( (in->input.length>0) && (in->input.length>FRAME_BYTES) ) {
    return RCC_ERROR;
  }

  // Normal state (not first, not end)
  if (s->oldest && s->oldest->data ) {
    // except for initial condition, we always output a buffer. first and last per image are zeroes.
    if (s->inLine == 1 || s->inLine == p->height)
      memset(out->current.data, 0, LINE_BYTES);
    else
      doLine(s->oldest->data,                               // l0: the oldest buffer
	     s->buffers[(s->next + 1) % HISTORY_SIZE].data, // l1: next oldest
	     in->current.data,                               // l2: just arrived
	     out->current.data, p->width);
    if (++s->inLine > p->height)
      s->inLine = 0;
    out->output.length = LINE_BYTES;
    c->advance(out, LINE_BYTES);
  }

  // Manage a history of HISTORY_SIZE buffers in our state structure
  // Note that s->oldest will be initially NULL so the first time
  // "take" is called, no old buffer is released
  c->take(in, s->oldest, &s->buffers[s->next]);
  s->oldest = &s->buffers[s->next];
  s->next++;
  if (s->next >= HISTORY_SIZE) {
    s->next = 0;
  }
  return RCC_OK;
}
