/*
  =====
  Copyright (C) 2011 Massachusetts Institute of Technology


  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
  ======
*/

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Fri May 6 03:38:44 2011 EDT
 * BASED ON THE FILE: optical_flow_pyr_lk.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: optical_flow_pyr_lk
 */
#include "optical_flow_pyr_lk_Worker.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

typedef struct {
  uint16_t init;
  float *derivIBuf;
  float *derivJBuf;
  float *derivIWinBuf;

  // derivatives
  float *adx;
  float *ady;
  float *ad2x;
  float *adxdy;
  float *ad2y;
  float *bdx;
  float *bdy;
} OFState;

static uint32_t sizes[] = {sizeof(OFState), 0 };

OPTICAL_FLOW_PYR_LK_METHOD_DECLARATIONS;
RCCDispatch optical_flow_pyr_lk = {
  /* insert any custom initializations here */
  .memSizes = sizes,
  OPTICAL_FLOW_PYR_LK_DISPATCH
};


static RCCResult start(RCCWorker *self)
{
  OFState *myState = self->memories[0];  
  Optical_flow_pyr_lkProperties *p = self->properties;
  if ( myState->init == 0 ) {
    myState->derivIBuf = malloc( (p->height+2*p->win_height)*(p->width+2*p->win_width)*sizeof(float)*6 );
    myState->derivJBuf = malloc( (p->height+2*p->win_height)*(p->width+2*p->win_width)*sizeof(float)*6 );
    myState->derivIWinBuf = malloc( p->win_height*p->win_width*sizeof(float)*6 );
  }
  myState->init = 1;
  return RCC_OK;
}


static RCCResult release(RCCWorker *self)
{
  OFState *myState = self->memories[0];  
  if ( myState->init ) {
    free( myState->derivIBuf);
    free( myState->derivJBuf);
    free( myState->derivIWinBuf);
  }
  return RCC_OK;
}


// Lucas-Kanade optical flow
struct Point2f { float x, y; };
struct Point2i { int x, y; };
#define uchar unsigned char

void calcOpticalFlowPyrLK( OFState *myState,
			   unsigned H, unsigned W,
			   const char *prevImg, const char *nextImg, // input ports
			   struct Point2f *prevPts, // input port
			   struct Point2f *nextPts, // output port
			   char *status, float *err, // output ports
			   int winHeight, int winWidth, // 10, 10
			   unsigned level, // 0
			   double termMaxCount, // = 20,
			   double termEpsilon, // = 0.01,
			   double derivLambda, // = 0.5,
			   size_t npoints)
{
  // init parameters
  if(derivLambda < 0.) derivLambda = 0.;
  if(derivLambda > 1.) derivLambda = 1.;

  double lambda1 = 1. - derivLambda, lambda2 = derivLambda;
  const float deriv1Scale = 0.5f/4.f;
  const float deriv2Scale = 0.25f/4.f;
  struct Point2f halfWin;
  halfWin.x = (winWidth-1)*0.5f;
  halfWin.y = (winHeight-1)*0.5f;

  size_t i, j;
  for( i = 0; i < npoints; i++ )
    status[i] = 1; // true;

  if( npoints == 0 )
    return;
    
  int cn = 1;
  // substitute parameters
  size_t derivIStep = 6 * cn * (W+2*winWidth) * sizeof(float);
  size_t derivJStep = 3 * cn * (W+2*winWidth) * sizeof(float);
  size_t derivIWinBufStep = 6 * cn * winWidth * sizeof(float);
  size_t derivIElemSize1 = sizeof(float);
  size_t derivJElemSize1 = sizeof(float);
  size_t derivIRows = H+2*winHeight;
  size_t derivJRows = H+2*winHeight;
  int derivICols = W+2*winWidth;
  int derivJCols = W+2*winWidth;


  const char *derivIData = (const char *) myState->derivIBuf;
  const char *derivJData = (const char *) myState->derivJBuf;
  const char *derivIWinBufData = (const char *) myState->derivIWinBuf;

  if(termMaxCount > 100) termMaxCount = 100;
  if(termMaxCount < 0) termMaxCount = 0;

  if(termEpsilon > 10.) termEpsilon = 10.;
  if(termEpsilon < 0.) termEpsilon = 0.;
  termEpsilon *= termEpsilon; // work with squared distances

  // one level
  {
    // I, dI/dx ~ Ix, dI/dy ~ Iy, d2I/dx2 ~ Ixx, d2I/dxdy ~ Ixy, d2I/dy2 ~ Iyy
    // derivI
    for( i = 0; i < H; i++ ) {
      float *dI = myState->derivIBuf + (i+winHeight)*(W+2*winWidth)*6;
      for( j = 0; j < W; j++ ) {
	size_t jj = j + winWidth;
	dI[6*jj] = (float) prevImg[i*W+j];
	dI[6*jj+1] = myState->adx[i*W+j] * deriv1Scale;
	dI[6*jj+2] = myState->ady[i*W+j] * deriv1Scale;
	dI[6*jj+3] = myState->ad2x[i*W+j] * deriv2Scale;
	dI[6*jj+4] = myState->adxdy[i*W+j] * deriv2Scale;
	dI[6*jj+5] = myState->ad2y[i*W+j] * deriv2Scale;
      }
    }

    // J, dJ/dx ~ Jx, dJ/dy ~ Jy
    // derivJ
    for( i = 0; i < H; i++ ) {
      float *dJ = myState->derivJBuf + (i+winHeight)*(W+2*winWidth)*6;
      for( j = 0; j < W; j++ ) {
	size_t jj = j + winWidth;
	dJ[6*jj] = (float) nextImg[i*W+j];
	dJ[6*jj+1] = myState->bdx[i*W+j] * deriv1Scale;
	dJ[6*jj+2] = myState->bdy[i*W+j] * deriv1Scale;
      }
    }

    size_t ptidx;
    for( ptidx = 0; ptidx < npoints; ptidx++ )
      {
	struct Point2f prevPt = {prevPts[ptidx].x*(float)(1./(1 << level)),
				 prevPts[ptidx].y*(float)(1./(1 << level))};
	struct Point2f nextPt;
	if( level == 0 ) // maxLevel )
	  {
	    nextPt = prevPt;
	  }
	/*
	  else
	  nextPt = nextPts[ptidx]*2.f;
	*/
	nextPts[ptidx] = nextPt;
            
	struct Point2i iprevPt, inextPt;
	prevPt.x -= halfWin.x;
	prevPt.y -= halfWin.y;//-= halfWin;
	iprevPt.x = (int) (prevPt.x);
	iprevPt.y = (int) (prevPt.y);

	if( iprevPt.x < -winWidth || iprevPt.x >= derivICols ||
	    iprevPt.y < -winHeight || iprevPt.y >= derivIRows )
	  {
	    if( level == 0 )
	      {
		status[ptidx] = 0; // false;
		err[ptidx] = 1e9; // FLT_MAX;
	      }
	    continue;
	  }
            
	float a = prevPt.x - iprevPt.x;
	float b = prevPt.y - iprevPt.y;
	float w00 = (1.f - a)*(1.f - b), w01 = a*(1.f - b);
	float w10 = (1.f - a)*b, w11 = a*b;
	size_t stepI = derivIStep/derivIElemSize1;
	size_t stepJ = derivJStep/derivJElemSize1;
	int cnI = cn*6, cnJ = cn*3;
	double A11 = 0, A12 = 0, A22 = 0;
	double iA11 = 0, iA12 = 0, iA22 = 0;
            
	// extract the patch from the first image
	int x, y;
	for( y = 0; y < winHeight; y++ )
	  {
	    const float* src = (const float*)(derivIData +
					      (y + iprevPt.y)*derivIStep) + iprevPt.x*cnI;
	    float* dst = (float*)(derivIWinBufData + y*derivIWinBufStep);

	    for( x = 0; x < winWidth*cnI; x += cnI, src += cnI )
	      {
		float I = src[0]*w00 + src[cnI]*w01 + src[stepI]*w10 + src[stepI+cnI]*w11;
		dst[x] = I;
                    
		float Ix = src[1]*w00 + src[cnI+1]*w01 + src[stepI+1]*w10 + src[stepI+cnI+1]*w11;
		float Iy = src[2]*w00 + src[cnI+2]*w01 + src[stepI+2]*w10 + src[stepI+cnI+2]*w11;
		dst[x+1] = Ix; dst[x+2] = Iy;
                    
		float Ixx = src[3]*w00 + src[cnI+3]*w01 + src[stepI+3]*w10 + src[stepI+cnI+3]*w11;
		float Ixy = src[4]*w00 + src[cnI+4]*w01 + src[stepI+4]*w10 + src[stepI+cnI+4]*w11;
		float Iyy = src[5]*w00 + src[cnI+5]*w01 + src[stepI+5]*w10 + src[stepI+cnI+5]*w11;
		dst[x+3] = Ixx; dst[x+4] = Ixy; dst[x+5] = Iyy;

		iA11 += (double)Ix*Ix;
		iA12 += (double)Ix*Iy;
		iA22 += (double)Iy*Iy;

		A11 += (double)Ixx*Ixx + (double)Ixy*Ixy;
		A12 += Ixy*((double)Ixx + Iyy);
		A22 += (double)Ixy*Ixy + (double)Iyy*Iyy;
	      }
	  }

	A11 = lambda1*iA11 + lambda2*A11;
	A12 = lambda1*iA12 + lambda2*A12;
	A22 = lambda1*iA22 + lambda2*A22;

	double D = A11*A22 - A12*A12;
	double minEig = (A22 + A11 - sqrt((A11-A22)*(A11-A22) +
					  4.*A12*A12))/(2*winWidth*winHeight);
	err[ptidx] = (float)minEig;

	if( D < DBL_EPSILON )
	  {
	    if( level == 0 )
	      status[ptidx] = 0; // false;
	    continue;
	  }
            
	D = 1./D;

	nextPt.x -= halfWin.x;
	nextPt.y -= halfWin.y;//-= halfWin;
	struct Point2f prevDelta;

	for( j = 0; j < termMaxCount; j++ )
	  {
	    inextPt.x = (int) (nextPt.x);
	    inextPt.y = (int) (nextPt.y);

	    if( inextPt.x < -winWidth || inextPt.x >= derivJCols ||
		inextPt.y < -winHeight || inextPt.y >= derivJRows )
	      {
		if( level == 0 )
		  status[ptidx] = 0; // false;
		break;
	      }

	    a = nextPt.x - inextPt.x;
	    b = nextPt.y - inextPt.y;
	    w00 = (1.f - a)*(1.f - b); w01 = a*(1.f - b);
	    w10 = (1.f - a)*b; w11 = a*b;

	    double b1 = 0, b2 = 0, ib1 = 0, ib2 = 0;

	    for( y = 0; y < winHeight; y++ )
	      {
		const float* src = (const float*)(derivJData +
						  (y + inextPt.y)*derivJStep) + inextPt.x*cnJ;
		const float* Ibuf = (float*)(derivIWinBufData + y*derivIWinBufStep);

		for( x = 0; x < winWidth; x++, src += cnJ, Ibuf += cnI )
		  {
		    double It = src[0]*w00 + src[cnJ]*w01 + src[stepJ]*w10 +
		      src[stepJ+cnJ]*w11 - Ibuf[0];
		    double Ixt = src[1]*w00 + src[cnJ+1]*w01 + src[stepJ+1]*w10 +
		      src[stepJ+cnJ+1]*w11 - Ibuf[1];
		    double Iyt = src[2]*w00 + src[cnJ+2]*w01 + src[stepJ+2]*w10 +
		      src[stepJ+cnJ+2]*w11 - Ibuf[2];
		    b1 += Ixt*Ibuf[3] + Iyt*Ibuf[4];
		    b2 += Ixt*Ibuf[4] + Iyt*Ibuf[5];
		    ib1 += It*Ibuf[1];
		    ib2 += It*Ibuf[2];
		  }
	      }

	    b1 = lambda1*ib1 + lambda2*b1;
	    b2 = lambda1*ib2 + lambda2*b2;
	    struct Point2f delta;
	    delta.x =  (float)((A12*b2 - A22*b1) * D);
	    delta.y =  (float)((A12*b1 - A11*b2) * D);
	    //delta = -delta;

	    nextPt.x += delta.x;
	    nextPt.y += delta.y;
	    nextPts[ptidx].x = nextPt.x + halfWin.x;
	    nextPts[ptidx].y = nextPt.y + halfWin.y;//= nextPt + halfWin;

	    //if( delta.ddot(delta) <= criteria.epsilon )
	    if(delta.x*delta.x + delta.y*delta.y <= termEpsilon)
	      break;

	    if( j > 0 && abs(delta.x + prevDelta.x) < 0.01 &&
		abs(delta.y + prevDelta.y) < 0.01 )
	      {
		//nextPts[ptidx] -= delta*0.5f;
		nextPts[ptidx].x -= delta.x*0.5f;
		nextPts[ptidx].y -= delta.y*0.5f;
		break;
	      }
	    prevDelta = delta;
	  }
      }
  }

}



/*
 * Methods to implement for worker optical_flow_pyr_lk, based on metadata.
 */
static RCCResult run(RCCWorker *self,
                     RCCBoolean timedOut,
                     RCCBoolean *newRunCondition) {
  ( void ) timedOut;
  ( void ) newRunCondition;
  OFState *myState = self->memories[0];  
  Optical_flow_pyr_lkProperties *p = self->properties;
  RCCPort *in_a     = &self->ports[OPTICAL_FLOW_PYR_LK_IN_A],
    *in_adx   = &self->ports[OPTICAL_FLOW_PYR_LK_IN_ADX],
    *in_ady   = &self->ports[OPTICAL_FLOW_PYR_LK_IN_ADY],
    *in_ad2x  = &self->ports[OPTICAL_FLOW_PYR_LK_IN_AD2X],
    *in_ad2y  = &self->ports[OPTICAL_FLOW_PYR_LK_IN_AD2Y],
    *in_adxdy = &self->ports[OPTICAL_FLOW_PYR_LK_IN_ADXDY],
    *in_b     = &self->ports[OPTICAL_FLOW_PYR_LK_IN_B],
    *in_bdx   = &self->ports[OPTICAL_FLOW_PYR_LK_IN_BDX],
    *in_bdy   = &self->ports[OPTICAL_FLOW_PYR_LK_IN_BDX],
    *in_feature = &self->ports[OPTICAL_FLOW_PYR_LK_IN_FEATURE],
    *out      = &self->ports[OPTICAL_FLOW_PYR_LK_OUT],
    *out_status = &self->ports[OPTICAL_FLOW_PYR_LK_OUT_STATUS],
    *out_err    = &self->ports[OPTICAL_FLOW_PYR_LK_OUT_ERR];
  //  const RCCContainer *c = self->container;

  // set pointers to derivatives
  myState->adx = (float *) in_adx->current.data;
  myState->ady = (float *) in_ady->current.data;
  myState->ad2x = (float *) in_ad2x->current.data;
  myState->ad2y = (float *) in_ad2y->current.data;
  myState->adxdy = (float *) in_adxdy->current.data;
  myState->bdx = (float *) in_bdx->current.data;
  myState->bdy = (float *) in_bdy->current.data;

  // get number of features
  size_t npoints = in_feature->input.length / (2 * sizeof(float));

  // calc optical flow
  calcOpticalFlowPyrLK(myState, 
		       p->height, p->width,
		       in_a->current.data, in_b->current.data,
		       (struct Point2f *) in_feature->current.data,
		       (struct Point2f *) out->current.data, // output features
		       (char *) out_status->current.data, // output status
		       (float *) out_err->current.data, // output errors
		       p->win_height, p->win_width, p->level,
		       p->term_max_count, p->term_epsilon, p->deriv_lambda,
		       npoints
		       );

  // set output ports
  out->output.u.operation = in_a->input.u.operation;
  out->output.length = 2 * npoints * sizeof(float);

  out_status->output.u.operation = in_a->input.u.operation;
  out_status->output.length = npoints * sizeof(char);

  out_err->output.u.operation = in_a->input.u.operation;
  out_err->output.length = npoints * sizeof(float);

  return RCC_ADVANCE;
}
