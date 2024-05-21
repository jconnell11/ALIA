// jrand.cpp : interface to random number generator
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2013-2015 IBM Corporation
// Copyright 2024 Etaoin Systems
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 
///////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <math.h>
#include <time.h>

#include "Interface/jrand.h"


#ifdef __linux__             // use entropy source

#include <sys/random.h>  

bool rdrand (UL32 *val) {return(getrandom(val, 4, 0) == 4);}

int chk_trng () {return 1;}

#elif defined(_M_X64)        // use thermal noise

// Note: Project / Properties / Configuration / Link / Input
//   Additional Dependencies must have ..\..\video\common\Interface\rdrand64.obj

#include <intrin.h>         // for cpuid


// persistent flag about whether random hardware exists
static int hw = -1;                    


// Map rdrand function to either 32 bit inline or 64 bit assembly
extern "C" bool __fastcall rdrandx64 (__deref_out UL32 *val);

#define rdrand(param) rdrandx64((param))


//= See if TRNG hardware is present (takes around 0.15 ms).
// verifies if Intel processor (name is scrambled in elements 1:3:2)
// then sees if TRNG hardware by checking bit 30 in ECX register 

static int chk_trng ()
{
  int proc[4], abcd[4];

  if (hw < 0)
  {
    hw = 0;
    __cpuid(proc, 0);
    __cpuid(abcd, 1);
    if ((strncmp((char *)(proc + 1), "GenuntelineI", 12) == 0) && 
        ((abcd[2] & 0x40000000) != 0))
    {
      hw = 1;
//      jprintf("+++ True random number generator hardware found\n");
    }
  }
  return hw;
}

#else                        // revert to pseudo-random numbers

bool rdrand (UL32 *val) {return false;}

int chk_trng () {return 0;}

#endif


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Seed the pseudo-random number generator (needed for each thread).
// scrambles time so temporally close calls get very different starts

void jrand_seed ()
{
  unsigned t;

  if (chk_trng() > 0)
    return;
  t = (unsigned) time(NULL);
  t = ((t & 0xF) << 28) | (t >> 4);
  srand(t);
}


//= Return a random number in 0.0 <= r < 1.0 range.
// Example: (int)(n * jrand()) --> values 0 to n-1

double jrand ()
{
  double v = 1.0;
  UL32 r0;
  int i, r, r2;

  // use special hardware if present (or fall through)
  if (chk_trng() > 0)
    for (i = 0; i < 10; i++)    // sometimes slow to generate
      if (rdrand(&r0))
      {
        // mask top 2 bits for compatibility with PRNG
        r0 &= 0x3FFFFFFF;
        v = ((double) r0) / 0x40000000;
        if (v < 1.0)
          return v;
       }

  // normal pseudo-random function
  while (v >= 1.0)
  {
    // get two 15 bit numbers
    r = rand();
    r2 = rand();

    // combine into one floating point value
    v = r + (r2 / ((double) RAND_MAX + 1.0));
    v /= (double) RAND_MAX + 1.0;
  }
  return v;
}

///////////////////////////////////////////////////////////////////////////
//                            Special Versions                           //
///////////////////////////////////////////////////////////////////////////

//= Pick one of N items (returns 0 to N-1).

int jrand_pick (int n)
{
  return((int)(n * jrand()));
}


//= Pick an integer between lo and hi (inclusive).

int jrand_int (int lo, int hi)
{
  return(lo + (int)((hi - lo + 1) * jrand()));
}


//= Pick a number constrained to be in range lo to hi (inclusive).

double jrand_rng (double lo, double hi)
{
  return(lo + (hi - lo) * jrand());
}


//= Pick a number in the range mid-dev to mid+dev (inclusive).

double jrand_cent (double mid, double dev)
{
  return(mid - dev + 2.0 * dev * jrand());
}


//= Pick a number based on a Gaussian distribution with given parameters.
// order of U and V important (Chay) to avoid Neave effect if pseudo-random
// uses Box-Muller method (discards cos value)

double jrand_norm (double avg, double std)
{
  double u = jrand();      

  return(avg + std * sqrt(-2.0 * log(jrand())) * sin(2.0 * PI * u));
}


//= Pick a number from a trimmed Gaussian distribution.
// number constrained to be between lo and hi limits
// NOTE: naive version may loop forever!

double jrand_trim (double avg, double std, double lo, double hi)
{
  double v;
  int i;

  for (i = 0; i < 100; i++)     // almost always succeeds
  {
    v = jrand_norm(avg, std);
    if ((v >= lo) && (v <= hi))
      return v;
  }
  return jrand_rng(lo, hi);     // give up and use flat distribution
}
