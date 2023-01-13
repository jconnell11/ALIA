// jhcAliaSpeech.cpp : speech and loop timing interface for ALIA reasoner
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2022 Etaoin Systems
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

#include <ctype.h>

#include "Acoustic/jhcAliaSpeech.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcAliaSpeech::~jhcAliaSpeech ()
{
  // for debugging - only happens when program closes
  DumpAll();
}


//= Default constructor initializes certain values.

jhcAliaSpeech::jhcAliaSpeech ()
{
  strcpy_s(gram,  "language/alia_top.sgm");    // main grammar file
  strcpy_s(kdir,  "KB0/");                     // for kernels
  strcpy_s(kdir2, "KB2/");                     // extra abilities
  spin  = 0;                                   // no speech reco
  amode = 2;                                   // attn word at front
  tts   = 0;                                   // no speech out
  acc   = 0;                                   // forget rules/ops
  time_params(NULL);
}


//= Most recent result of speech input (0 = none, 1 = speaking, 2 = reco).

int jhcAliaSpeech::SpeechRC () const 
{
  if (spin == 2)
    return web.Hearing();
  if (spin == 1)
    return sp.Hearing();
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for overall control of timing.
// this should be called in Defaults and tps used in SaveVals

int jhcAliaSpeech::time_params (const char *fname)
{
  jhcParam *ps = &tps;
  int ok;

  ps->SetTag("asp_time", 0);
  ps->NextSpecF( &stretch,  3.5, "Attention window (sec)");    // was 2.5 (local)
  ps->NextSpecF( &splag,    0.5, "Post speech delay (sec)");   // was 0.1 (local)
  ps->NextSpecF( &wait,     0.3, "Text out delay (sec)");      // was 0.15
  ps->Skip(3);

  ps->NextSpecF( &thz,     80.0, "Thought cycle rate (Hz)");  
  ps->NextSpecF( &shz,     30.0, "Default body rate (Hz)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.
// can also set up robot name as an attention word
// returns 1 if okay, 0 or negative for error

int jhcAliaSpeech::Reset (const char *rname, const char *vname)
{
  char extra[200], extra2[200];

  // remember interface choice and set attentional state
  sprintf_s(extra,  "%sbaseline.lst", kdir2);
  sprintf_s(extra2, "%svolition.lst", kdir2);
  awake = 0;
  if (spin == 3)
    jprintf(">>> Windows 11 Live Caption interface not written yet!\n");
  else if (spin == 2)
  {
    // configure online speech recognition (requires internet connection)
    jprintf(1, noisy, "SPEECH online %s ...\n\n", web.Version());
    if (web.Init(1) <= 0)
    {
      jprintf(1, noisy, "\nSPEECH -> FAILED !!!\n");
      jprintf(1, noisy, "=========================\n\n");
      return 0;
    }
    jprintf(1, noisy, "  %d recognition fixes from: misheard.map\n", web.Fixes());
    jprintf(1, noisy, "\nSPEECH -> OK\n");
    jprintf(1, noisy, "=========================\n\n");
  }
  else if (spin == 1)
  {
    // constrain local speech recognition by same grammar as core
    jprintf(1, noisy, "SPEECH local ...\n\n");
    sp.SetGrammar(gram); 
    if (sp.Init(1, 0) <= 0)            // show partial transcriptions
    {
      jprintf(1, noisy, "\nSPEECH -> FAILED !!!\n");
      jprintf(1, noisy, "=========================\n\n");
      return 0;
    }
    if (noisy > 0)
      sp.PrintCfg();

    // add words associated with kernels (slow second time?)
    jprintf(1, noisy, "Augmenting speech grammar with:\n");
    (net.mf).AddSpVocab(&sp, "language/vocabulary.sgm", 1);
    kern_gram();
    base_gram(extra);                  // includes graphizer.sgm
    if (vol > 0)
      base_gram(extra2);

    // add robot name as attention word (speech only)
    self_name(rname);
    sp.MarkRule("toplevel");          
    sp.Listen(1);
    jprintf(1, noisy, "\nSPEECH -> OK\n");
    jprintf(1, noisy, "=========================\n\n");
  }
  
  // set TTS and speech state
  if ((tts > 0) && (spin != 1))        // use just TTS from jhcSpeechX                              
    sp.InitTTS(noisy);                      
  if ((tts > 0) && (vname != NULL) && (*vname != '\0'))
    sp.SetVoice(vname);
  sp.Reset();

  // set basic grammar for core and clear state (speech already set)
  jprintf("Initializing ALIA core %4.2f\n\n", Version());
  MainGrammar(gram, "toplevel", rname);
  jhcAliaCore::Reset(1, rname, 1);

  // load rules, operators, and words for kernels (speech already set)
  (net.mf).AddVocab(&gr, "language/vocabulary.sgm", 0, -1);
  KernExtras(kdir);
  Baseline(extra, 1, 2);               // includes graphizer.sgm
  if (vol > 0)
    Baseline(extra2, 1, 2);
  if (acc >= 1)
    LoadLearned();

  // clear text input and output buffers
  *lastin = '\0';
  *input = '\0';
  done = 0;
  *output = '\0';
  *pend = '\0';
  yack = 0;

  // reset loop timing
  start = 0;
  rem = 0.0;
  sense = 0;
  think = 0;
  return 1;
}


//= Load local speech system with extra grammar pieces associated with kernels.

void jhcAliaSpeech::kern_gram ()
{
  char fname[200];
  const jhcAliaKernel *k = &kern;
  const char *tag; 

  while (k != NULL)
  {
    tag = k->BaseTag();
    if (*tag != '\0')
    {
      sprintf_s(fname, "%s%s.sgm", kdir, tag);
      (net.mf).AddSpVocab(&sp, fname, 1);
    }
    k = k->NextPool();
  }
  sp.Listen(1);
}


//= Load local speech system with extra grammar pieces associated with baseline knowledge.

void jhcAliaSpeech::base_gram (const char *list)
{
  char dir[80], line[80], fname[200];
  FILE *in;
  char *end;
  int n;

  // make list of files exists
  if (fopen_s(&in, list, "r") != 0)
    return;

  // save directory part of path
  strcpy_s(dir, list);
  if ((end = strrchr(dir, '/')) == NULL)
    end = strrchr(dir, '\\');
  if (end != NULL)
    *(end + 1) = '\0';
  else
    *dir = '\0';

  // read list of local file names
  while (fgets(line, 80, in) != NULL)
  {
    // ignore commented out lines 
    if (strncmp(line, "//", 2) == 0)
      continue;

    // trim trailing whitespace
    n = 0;
    end = line;
    while (line[n] != '\0')
      if (strchr(" \t\n", line[n++]) == NULL)
        end = line + n;
    if (end == line)
      continue;
    *end = '\0';

    // read various types of input 
    sprintf_s(fname, "%s%s.sgm", dir, line);  
    (net.mf).AddSpVocab(&sp, fname, 1);
  } 

  // clean up
  fclose(in);
}


//= Add the robot's own name as an attention word to local speech system.

void jhcAliaSpeech::self_name (const char *name)
{
  char first[80];
  char *sep;

  if ((name == NULL) || (*name == '\0'))
    return;
  jprintf("\nAdding robot name: %s\n", name);
  sp.ExtendRule("ATTN", name, 0); 
  strcpy_s(first, name);
  if ((sep = strchr(first, ' ')) != NULL)
  {
    *sep = '\0';
    sp.ExtendRule("ATTN", first, 0); 
  }
  AddName(name);                       // adds full and first to <NAME>
}


//= Add the name of a particular person to parsing and speech grammars.
// make sure to call Listen(1) afterward to re-engage speech recognition

void jhcAliaSpeech::AddName (const char* name)
{
  char first[80];
  char* sep;

  // get first name from full name
  if ((name == NULL) || (*name == '\0'))
    return;
  strcpy_s(first, name);
  if ((sep = strchr(first, ' ')) != NULL)
    *sep = '\0';

  // update parsing grammar and possibly speech front-end also 
  sp_listen(0);
  gram_add("NAME", name, 0);
  gram_add("NAME-P", (net.mf).SurfWord(name, JTAG_NAMEP), 0);
  if (sep != NULL)
  {
    gram_add("NAME", first, 0);
    gram_add("NAME-P", (net.mf).SurfWord(first, JTAG_NAMEP), 0);
  }
}


//= Disable local speech recognition to add a new word.

void jhcAliaSpeech::sp_listen (int doit)
{
  if (spin == 1)
    sp.Listen(doit);
}


//= Extend a local speech recognition grammar rule for a new word.
// lvl: -1 = vocab, 0 = kernel, 1 = extras, 2 = previous accumulation, 3 = newly added
// Note: should call sp_listen(0) before this

void jhcAliaSpeech::gram_add (const char *cat, const char *wd, int lvl)
{
  if (wd == NULL)
    return;
  gr.ExtendRule(cat, wd, lvl);         // jhcAliaCore::gram_add
  if (spin == 1)
    sp.ExtendRule(cat, wd, 0);
}


//= Initialize just the local speech component for use with remote ALIA brain.

int jhcAliaSpeech::VoiceInit ()
{
  sp.SetGrammar(gram);      
  if (sp.Init(0, noisy) <= 0)
    return 0;
  sp.MarkRule("toplevel");    
  sp.Reset();
  kern_gram();      
  return 1;
}


//= Just do basic speech recognition (no reasoning) for debugging.
// returns 1 if happy, 0 to end interaction 

int jhcAliaSpeech::UpdateSpeech ()
{
  // check for new input 
  if (done > 0)
    return 0;
  if ((tts > 0) && (spin != 1))
    sp.Update(0, web.Hearing());       // just TTS (web may be inactive)
  if (spin == 2)
    web.Update(sp.Talking());
  else if (spin == 1)
    sp.Update();                       // does TTS also

  // see if exit requested
  if (((spin == 2) && web.Escape()) || 
      ((spin == 1) && sp.Escape()))
    return 0;
  return 1;
}


//= Generate actions in response to update sensory information.
// if "alert" > 0 then forces system to pay attention to input
// should mark all seed nodes to retain before calling this
// returns 1 if happy, 0 to end interaction 
// NOTE: should call UpdateSpeech before this and DayDream after this

int jhcAliaSpeech::Respond (int alert)
{
  char mark[80] = "##  +------------------------------------------------------------------------";
  int bid, n;

jtimer(16, "Respond");
  // possibly wake up system then evaluate any language input
  now = jms_now();
  if (alert > 0)
    awake = now;
  xfer_input();

  // process current foci to generate commands for body
  RunAll(1);
  bid = Response(output);

  // send commands to body and language output channel (no delay)
  if (tts > 0)
  {
    sp.Say(bid, output);
    sp.Issue();
  }

  // show prominently in log file (capitalize first letter)
  if ((n = (int) strlen(output)) > 0)
  {
    n = __min(n, 70);
    mark[n + 9] = '\0';
    jprintf("\n%s+\n##  | \"%c%s\" |\n%s+\n\n", mark, toupper(output[0]), output + 1, mark);
  }
jtimer_x(16);
  return 1;
}


//= If grammatical utterance then show parse and network.
// expects member variable "now" to hold current time
// state retained in member variable "wake" (timestamp of last activity)

void jhcAliaSpeech::xfer_input ()
{
  const char *sent;
  int hear, attn = ((amode <= 0) ? 1 : Attending()), perk = 0;

  // get language status and input string 
  if (spin == 2)
  {
    hear = web.Hearing();
    sent = web.Heard();
  }
  else if (spin == 1)
  {
    hear = sp.Hearing();
    sent = sp.Heard();
  }
  else  
  {
    hear = ((*input != '\0') ? 2 : 0);
    sent = input;
  }

  // reject spurious motor noise
  if ((spin >= 1) && (hear == 2) && (syllables(sent) < 2))
    hear = 0;

  // pass input (if any) to semantic network generator
  if (hear < 0)
    perk = Interpret(NULL, attn, amode, -1);               // for "huh?" response
  else if (hear >= 2)
    perk = Interpret(sent, attn, amode, spin);
  if (perk >= 2)                                           // attention word heard
    attn = 1;
  if ((attn > 0) && (hear != 0))
    awake = now;

  // see if system should continue paying attention
  if (awake != 0) 
  {
    if (sp.Talking() > 0)                                  // prolong during TTS output
      awake = now;
    else if ((attn > 0) && (sp.Silence() > splag) && (jms_secs(now, awake) > stretch))
    {
      jprintf(1, noisy, "\n  ... timeout ... verbal attention off\n\n");
      awake = 0;
    }
  }

  // percolate saved input strings
  strcpy_s(lastin, input);
  *input = '\0';
}


//= Guess how many syllables heard as an aid to rejecting spurious noises (e.g. "spin").
// counts vowel clusters ("ia" = 2) but adjusts for word breaks, leading "y", and final "e"

int jhcAliaSpeech::syllables (const char *txt) const
{
  const char *t = txt;
  char t0, v = '\0';
  int sp = 1, n = 0;

  // scan through lowercase string
  while (*t != '\0')
  {
    t0 = (char) tolower(*t++);
    if ((strchr("aiou", t0) != NULL) ||
        ((t0 == 'e') && (*t != ' ') && (*t != '\0')) ||        
        ((t0 == 'y') && (sp <= 0)))
    {
      // vowel cluster (except leading "y" or final "e", split "ia")
      if ((v == '\0') || ((v == 'i') && (t0 == 'a')))
        n++;
      v = t0;
      sp = 0;
    }
    else 
    { 
      // consonant or word break
      v = '\0';
      sp = ((t0 == ' ') ? 1 : 0);
    }
  }
  return n;
}


//= Perform several cycles of reasoning disconnected from sensors and actuators.
// quits early if some Run() takes too long to prevent missing sensor schedule

void jhcAliaSpeech::DayDream ()
{
  double frac = 1.0;
  int cyc, n = 1, ms = ROUND(1000.0 / shz);      // standard sensing time

jtimer(17, "DayDream");
  // determine how many total thought cycles to run right now
  if (start == 0)
    start = now;
  else 
  {
    frac = thz * jms_secs(now, last) + rem;
    n = ROUND(frac);
  }
  last = now;

  // possibly catch up on thinking (commands will be ignored)
  for (cyc = 1; cyc < n; cyc++)
    if (jms_diff(jms_now(), last) < ms)
      RunAll(0);
  rem = frac - cyc;          // how many thought cycles NOT completed
  think += cyc;
  sense++;
jtimer_x(17);
}


//= Call at end of run to put robot in stable state and possibly save knowledge.

void jhcAliaSpeech::Done ()
{
  // stop running actions
  StopAll();
  atree.ClrFoci();

  // stop speech input and output
  if (spin == 2)
    web.Close();
  else if (spin == 1)
    sp.Listen(0);
  CloseCvt();

  // possibly save state
  if (acc >= 2)
    DumpLearned();
}


///////////////////////////////////////////////////////////////////////////
//                            Intercepted I/O                            //
///////////////////////////////////////////////////////////////////////////

//= Force a string into the input buffer.
// returns true if some valid input available for processing

bool jhcAliaSpeech::Accept (const char *in, int quit) 
{
  // process string through speech or handle separately
  if (spin == 2)
    web.Inject(in, quit);
  else if (spin == 1)
    sp.Inject(in, quit); 
  else
  {
    if (in == NULL)
      *input = '\0';
    else
      strcpy_s(input, in);
    done = quit;
  }

  // no attention word ever needed for text input
  if ((in != NULL) && (*in != '\0'))
    awake = jms_now();
  return((in != NULL) && (*in != '\0') && (quit <= 0));
}


//= Show input received on last cycle.
// if parsable, can return cleaned up version

const char *jhcAliaSpeech::NewInput (int clean)
{
  const char *last;

  // get last raw input string (if any)
  if (spin == 2)
    last = web.LastIn();
  else if (spin == 1)
    last = sp.LastIn();
  else
    last = lastin;

  // possibly nothing valid on this cycle
  if ((last == NULL) || (*last == '\0'))
    return NULL;
  if ((spin > 0) && (awake == 0))
    return NULL;

  // return raw, cleanly parsed, or unknown marked version 
  if (clean <= 0)
    return last;
  if (gr.NumTrees() > 0)
    return gr.Clean();            
  return vc.Marked();    
}


//= Show output completed on last cycle (delays text for "typing").

const char *jhcAliaSpeech::NewOutput () 
{
  const char *msg = NULL;
  UL32 now;

  // see if last output delayed long enough or interrupted
  now = jms_now();
  if (*pend != '\0')
  {
    if (jms_secs(now, yack) > wait)
      msg = blip_txt(0);
    else if (*output != '\0')
      msg = blip_txt(1);
  }

  // queue any new output for printing after slight delay
  if (*output != '\0')
  {
    mood.Speak((int) strlen(output));
    strcpy_s(pend, output);
    yack = now;
    if ((spin <= 0) && (tts > 0))
    {
      // start TTS immediately but allow for later override
      sp.Say(sense, output);               
      sp.Utter();
    }
  }
  return msg;
}


//= Possibly terminate message after first word by inserting ellipsis.
// copies "pend" string to ephemeral "delayed" variable and erases "pend"

const char *jhcAliaSpeech::blip_txt (int cutoff)
{
  char *end;

  if (cutoff > 0)
  {
    if ((end = strchr(pend, ' ')) != NULL)
      *end = '\0';
    strcat_s(pend, " ...");
  }
  strcpy_s(delayed, pend);
  *pend = '\0';
  return delayed;
}


