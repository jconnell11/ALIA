// jhcAliaSpeech.cpp : speech and loop timing interface for ALIA reasoner
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2023 Etaoin Systems
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
}


//= Default constructor initializes certain values.

jhcAliaSpeech::jhcAliaSpeech ()
{
  spin  = 0;                                   // no speech reco
  amode = 2;                                   // attn word at front
  tts   = 0;                                   // no speech out
  acc   = 0;                                   // forget rules/ops
  time_params(NULL);
  gr.SetBonus("ACT-2");                        // prefer these trees
}


//= Most recent result of speech input (0 = none, 1 = speaking, 2 = reco).

int jhcAliaSpeech::SpeechRC () const 
{
  if (spin == 2)
    return web.Hearing();
  return sp.Hearing();                 // can set with Click()
}


//= Switch recognition acoustic model based on user's identity.
// examines semantic net if "name" from face reco not valid
// Note: only really needed for Microsoft SAPI (spin = 1)

void jhcAliaSpeech::UserVoice (const char *name)
{
  jhcNetNode *user = atree.Human();
  const char *who; 
  int cnt, w = 0, best = 0;

  // possibly set speech model based on face recognition (never a denial)
  if ((name != NULL) && (*name != '\0'))
  {
    if (strcmp(name, sp.UserName()) == 0)            // full string matches
      return;
    if (sp.SetUser(name, 0, 1) > 0)
    {
      jprintf(1, noisy, "  ... acoustic model => %s ...\n\n", sp.VoiceModel());
      return;
    }
  }

  // try setting speech model to longest lexical tag (ignore restrictions)
  while ((who = atree.Name(user, w++)) != NULL)
  {
    cnt = (int) strlen(who);
    if (cnt > best)
    {
      name = who;
      best = cnt;
    }
  }
  if (best > 0)
    if (strncmp(name, sp.UserName(), best) != 0)     // beginning matches ("Jon")
      if (sp.SetUser(name, 0, 1) > 0)
        jprintf(1, noisy, "  ... acoustic model => %s ...\n\n", sp.VoiceModel());
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
  ps->NextSpecF( &stretch, 3.5, "Attention window (sec)");     // was 2.5 (local)
  ps->NextSpecF( &splag,   0.5, "Post speech delay (sec)");    // was 0.1 (local)
  ps->NextSpecF( &wait,    0.3, "Text out delay (sec)");       // was 0.15
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

int jhcAliaSpeech::Reset (const char *rname, const char *vname, int cvt)
{
  // dispatch based on interface choice 
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
  else if (spin == 1)        // maintains separate copy of grammar
  {
    // constrain local speech recognition by same grammar as core
    jprintf(1, noisy, "SPEECH local ...\n\n");
    sp.SetGrammar("language/alia_top.sgm"); 
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
    add_sp_vocab("language/vocabulary.sgm", 1);
    kern_gram();
    base_gram("KB2/baseline.lst");     // includes graphizer.sgm
    if (vol > 0)
      base_gram("KB2/volition.lst");

    // start parsed speech recognition from main rule
    sp.MarkRule("toplevel");          
    sp.Listen(1);
    jprintf(1, noisy, "\nSPEECH -> OK\n");
    jprintf(1, noisy, "=========================\n\n");
  }
  else if (spin == 0)
  {
    jprintf(1, noisy, "\nSPEECH -> OFF (text only)\n");
    jprintf(1, noisy, "=========================\n\n");
  }
  
  // set TTS and speech state
  if ((tts > 0) && (spin != 1))        // use just TTS from jhcSpeechX                              
    sp.InitTTS(noisy);                      
  if ((tts > 0) && (vname != NULL) && (*vname != '\0'))
    sp.SetVoice(vname);
  sp.Reset();

  // clear text input and output buffers
  *lastin = '\0';
  *input = '\0';
  done = 0;
  *output = '\0';
  *pend = '\0';
  yack = 0;
  awake = 0;                           // attention state

  // initialize underlying reasoning system
  jhcAliaCore::Reset(rname, cvt);
  return 1;
}


//= Loads a grammar file to speech recognition as well as all morphological variants.
// assumes original grammar file has some section labelled "=[XXX-morph]"
// returns positive if successful, 0 or negative for problem
// NOTE: use instead of base LoadGrammar function since adds proper variants

int jhcAliaSpeech::add_sp_vocab (const char *fname, int rpt)
{
  const char deriv[80] = "jhc_temp.txt";
  char strip[80];
  char *end;

  // load basic grammar file like usual
  if ((fname == NULL) || (*fname == '\0'))
    return -3;
  if (sp.LoadSpGram(fname) <= 0)
    return -2;
  if (rpt > 0)
  {
    // list file as loaded
    strcpy_s(strip, fname);
    if ((end = strrchr(strip, '.')) != NULL)
      *end = '\0';
    jprintf("   %s\n", strip);
  }

  // read and apply morphology to add derived forms as well
  if ((net.mf).LoadExcept(fname) < 0)
    return 1;
  if ((net.mf).LexDeriv(fname, 0, deriv) < 0)
    return -1;
  if (sp.LoadSpGram(deriv) <= 0)
    return 0;
  return 2;
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
      sprintf_s(fname, "KB0/%s.sgm", tag);
      add_sp_vocab(fname, 1);
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
    add_sp_vocab(fname, 1);
  } 

  // clean up
  fclose(in);
}


//= Disable local speech recognition to add a new word.

void jhcAliaSpeech::sp_listen (int doit)
{
  if (spin == 1)
    sp.Listen(doit);
}


//= Extend a local speech recognition grammar rule for a new word.
// lvl: -1 = vocab, 0 = kernel, 1 = extras, 2 = previous accumulation, 3 = newly added
// Note: takes the place of jhcAliaCore::gram_add, should call sp_listen(0) before this

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
  sp.SetGrammar("language/alia_top.sgm");      
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
    sp.Update(0, 1, web.Hearing());    // just TTS (web may be inactive)
  if (spin == 2)
    web.Update(sp.Talking());
  else if (spin == 1)
    sp.Update(1, tts);                 // can do reco and TTS 

  // catalog previous acoustic state
  stat.Speech(SpeechRC(), BusyTTS(), Attending());

  // see if exit requested
  if (((spin == 2) && web.Escape()) || 
      ((spin == 1) && sp.Escape()))
    return 0;
  return 1;
}


//= Generate actions in response to update sensory information.
// if "stare" > 0 then forces system to pay attention to input
// should mark all seed nodes to retain before calling this
// returns 1 always for convenienace
// NOTE: should call UpdateSpeech before this and DayDream after this

int jhcAliaSpeech::Respond (int stare)
{
  int bid;

jtimer(16, "Respond");
  // possibly wake up system then evaluate any language input
  if (stare > 0)
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
  else
    sp.Blurt((bid > 0) ? 1 : 0);
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
    sp.Click(hear);
    sent = input;
  }

  // reject spurious motor noise
  if ((spin >= 1) && (hear == 2) && (syllables(sent) < 2))
    hear = 0;

  // pass input (if any) to semantic network generator
  if (hear < 0)
    perk = Interpret(NULL, attn, amode);         // for "huh?" response
  else if (hear >= 2)
    perk = Interpret(sent, attn, amode);
  if (perk >= 2)                                 // attention word heard
    attn = 1;
  if ((attn > 0) && (hear != 0))
    awake = now;

  // see if system should continue paying attention
  if (awake != 0) 
  {
    if (BusyTTS() > 0)                           // prolong during TTS output
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


//= Call at end of run to put robot in stable state and possibly save knowledge.

void jhcAliaSpeech::Done (int save)
{
  if (spin == 2)
    web.Close();
  else if (spin == 1)
    sp.Listen(0);
  jhcAliaCore::Done(save);
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
  int n;

  // see if last output delayed long enough or interrupted
  now = jms_now();
  if (*pend != '\0')
  {
    if (jms_secs(now, yack) > wait)
      msg = blip_txt(0);
    else if (*output != '\0')
      msg = blip_txt(1);
  }

  // see if any message for user
  if (*output != '\0')
  {
    // queue for printing after slight delay
    mood.Speak((int) strlen(output));
    strcpy_s(pend, output);
    yack = now;

    // start TTS immediately but allow for later override
    if ((spin <= 0) && (tts > 0))
    {
      sp.Say(SenseCnt(), output);               
      sp.Utter();
    }

    // possibly open speech gate for user response
    n = (int) strlen(output);
    if (output[n - 1] == '?')
      awake = now;
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


