// jhcEmotion.cpp : autonomous nagging and conscious acess to feelings
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
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

#include "Interface/jms_x.h"           // common video

#include "Kernel/jhcEmotion.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcEmotion::~jhcEmotion ()
{
}


//= Default constructor initializes certain values.

jhcEmotion::jhcEmotion ()
{
  strcpy_s(tag, "Emotion");
  mood = NULL;
  rpt = NULL;
  Defaults();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for avoiding distraction.

int jhcEmotion::time_params (const char *fname)
{
  jhcParam *ps = &tps;
  int ok;

  ps->SetTag("emo_time", 0);
  ps->NextSpecF( delay,      45.0, "Bored nag (sec)");  
  ps->NextSpecF( urge,       30.0, "Very bored nag (sec)");  
  ps->NextSpecF( delay + 1,  60.0, "Lonely nag (sec)");  
  ps->NextSpecF( urge  + 1, 120.0, "Very lonely nag (sec)"); 
  ps->NextSpecF( delay + 2,  30.0, "Tired nag (sec)");  
  ps->NextSpecF( urge  + 2,  15.0, "Very tired nag (sec)"); 

  ps->NextSpecF( &suffer,     1.0, "Delay per goal (sec)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcEmotion::Defaults (const char *fname)
{
  int ok = 1;

  ok &= time_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcEmotion::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= tps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                          Overridden Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Set up for new run of system.

void jhcEmotion::local_reset (jhcAliaNote& top)
{
  rpt = &top;
  first = 1; 
  yikes = 0;
  reported = 0;
  onset = 0;
  nag[0] = 0;
  nag[1] = 0;
  nag[2] = 0;
}


//= Post any spontaneous observations to attention queue.

void jhcEmotion::local_volunteer ()
{
  if ((mood == NULL) || (rpt == NULL))
    return;
  wake_up();
  freak_out();
  mark_onset();
  auto_nag();
}


//= Start up a new instance of some named function.
// starting time and bid are already speculatively bound by base class
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if successful, -1 for problem, -2 if function unknown

int jhcEmotion::local_start (const jhcAliaDesc& desc, int i)
{
  JCMD_SET(emo_test);
  JCMD_SET(emo_list);
  return -2;
}


//= Check on the status of some named function.
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if done, 0 if still working, -1 if failed, -2 if function unknown

int jhcEmotion::local_status (const jhcAliaDesc& desc, int i)
{
  JCMD_CHK(emo_test);
  JCMD_CHK(emo_list);
  return -2; 
}


///////////////////////////////////////////////////////////////////////////
//                            Event Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Notice that the system has been started up.

void jhcEmotion::wake_up ()
{
  if (first <= 0)
    return;
  first = 0;
  rpt->StartNote();
  rpt->NewProp(rpt->Self(), "hq", "awake");
  rpt->FinishNote();
}


//= Issue message when reasoning engine is working very hard.

void jhcEmotion::freak_out ()
{
  int m = mood->MeltDown();

  rpt->StartNote();
  if ((m > 0) && (yikes <= 0))
  {
    rpt->NewProp(rpt->Self(), "hq", "overwhelmed");
    yikes = 1;
  }
  else if ((m <= 0) && (yikes > 0))
  {
    rpt->NewProp(rpt->Self(), "hq", "overwhelmed", 1);
    yikes = 0;
  }
  rpt->FinishNote();
}


//= When a new emotional state is entered, mark it for reporting.
// makes a separate NOTE for each emotion if several reported at once
// emotion bits:
// [ surprised angry scared happy : unhappy bored lonely tired ]

void jhcEmotion::mark_onset ()
{
  double delay = 0.0;
  int bit, vect = mood->Quantized(), start = (vect ^ reported) & vect;

  // see if any new conditions (incl. "very")
  reported &= vect;
  if (start == 0)
  {
    onset = 0;
    return;
  }

  // delay report if busy thinking
  if (onset == 0)
    onset = jms_now();
  else
    delay = jms_elapsed(onset);
  if (delay < (suffer * mood->Busy()))
    return;

  // report new conditions (base emotion or added "very")
  for (bit = 0; bit <= 7; bit++)
    if ((start & (0x0101 << bit)) != 0)
    {
      emo_assert(bit, -1);
      if (bit <= 2)
        nag[bit] = 0;        // restart nag timer
    } 
  reported = vect;
}


//= Persistently complain about bored, lonely, and tired.

void jhcEmotion::auto_nag ()
{
  double repeat, overdue;
  UL32 now = jms_now();
  int bit, vect = mood->Quantized();

  // skip if some onset of bored/lonely/tired not reported yet
  if (((vect ^ reported) & vect & 0x0707) != 0)
    return;

  // only for bored, lonely, and tired
  for (bit = 0; bit <= 2; bit++)
  {
    // turn off nagging if not present, start after reported once
    if ((vect & (0x01 << bit)) == 0)
      nag[bit] = 0;
    else if ((nag[bit] == 0) && ((reported & (0x0101 << bit)) != 0))
      nag[bit] = now;
    if (nag[bit] == 0)
      continue;

    // report condition again if sufficient time has elapsed
    repeat = delay[bit];
    if ((vect & (0x0100 << bit)) != 0)
      repeat = urge[bit];
    overdue = jms_secs(now, nag[bit]) - repeat;
    if (overdue >= (suffer * mood->Busy()))
    {
      emo_assert(bit, -1);
      nag[bit] = now;
    }
  }
}


///////////////////////////////////////////////////////////////////////////
//                             Main Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Start trying to determine if currently experiencing some emotion.
// returns 1 if okay, -1 for interpretation error

int jhcEmotion::emo_test0 (const jhcAliaDesc& desc, int i)
{
  const jhcAliaDesc *hq;

  if ((mood == NULL) || (rpt == NULL))
    return -1;
  if ((hq = desc.Val("arg")) == NULL)
    return -1;
  if (hq->Lex() == NULL)
    return -1;
  if ((cst[i] = mood_bit(cmode[i], hq)) < 0)
    return -1;
  return 1;
}


//= Continue trying to determine if currently experiencing some emotion.
// returns 1 if done, 0 if still working, -1 for failure

int jhcEmotion::emo_test (const jhcAliaDesc& desc, int i)
{
  emo_assert(cst[i], cmode[i]);
  return 1;
}


//= Start trying to list all emotions currently being experiences.
// returns 1 if okay, -1 for interpretation error

int jhcEmotion::emo_list0 (const jhcAliaDesc& desc, int i)
{
  if ((mood == NULL) || (rpt == NULL))
    return -1;
  return 1;
}


//= Continue trying to list all emotions currently being experiences.
// makes a separate NOTE for each emotion experienced
// returns 1 if done, 0 if still working, -1 for failure

int jhcEmotion::emo_list (const jhcAliaDesc& desc, int i)
{
  int bit;

  for (bit = 0; bit <= 7; bit++)
    emo_assert(bit, -1);
  return 1;
}


//= Returns bit number for checking against mood vector.
// [ surprised angry scared  happy : unhappy bored lonely tired ]
// sets deg = 1 if "very" query detected

int jhcEmotion::mood_bit (int& deg, const jhcAliaDesc *hq) const
{
  const char emo[8][20] = {"surprised", "angry", "scared", "happy", 
                           "unhappy", "bored", "lonely", "tired"}; 
  const jhcAliaDesc *d;
  int bit, i = 0;

  // match basic emotion term
  deg = 0;
  for (bit = 7; bit >= 0; bit--)
    if (hq->LexMatch(emo[7 - bit]))
    {
      // see if question about "very" something
      while ((d = hq->Fact("deg", i++)) != NULL)
        if (d->Visible() && d->LexMatch("very"))
        {
          deg = 1;
          break;
        }
      return bit;
    }
  return -1;
}


//= Possibly create a new fact about some emotional state based on mood vector.
// detail: negative = only if present, 0 = confirm/deny base, 1 = check "very" also

void jhcEmotion::emo_assert (int bit, int detail)
{
  const char emo[8][20] = {"surprised", "angry", "scared", "happy", 
                           "unhappy", "bored", "lonely", "tired"}; 
  jhcAliaDesc *hq;
  int vect = mood->Quantized(), neg = 0, nv = 0;

  // see if anything should be reported
  if ((bit < 0) || (bit > 7))
    return;
  if ((vect & (0x01 << bit)) == 0)
    neg = 1;
  if ((neg > 0) && (detail < 0))
    return;

  // add property to self, possibly with "very"
  rpt->StartNote();
  hq = rpt->NewProp(rpt->Self(), "hq", emo[7 - bit], neg);
  if ((vect & (0x01 << (bit + 8))) == 0)
    nv = 1;
  if ((nv <= 0) || (detail >= 1))
    rpt->NewProp(hq, "deg", "very", nv);
  rpt->FinishNote();
}


