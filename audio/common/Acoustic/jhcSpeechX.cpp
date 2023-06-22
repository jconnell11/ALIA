// jhcSpeechX.cpp : more advanced speech functions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2020 IBM Corporation
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

#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "Data/jhcParam.h"             // common video
#include "Interface/jhcMessage.h"
#include "Interface/jms_x.h"

#include "Acoustic/jhcSpeechX.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcSpeechX::jhcSpeechX ()
{
  // no speech result ready
  *utt0 = '\0';
  *ph   = '\0';
  *utt  = '\0';
  *conf = '\0';
  cf = 0;

  // setup default DLLs to use
  *ifile = '\0';
  strcpy_s(rfile, "sp_reco_ms");
  strcpy_s(tfile, "sp_tts_ms");
  strcpy_s(pfile, "sp_reco_ms");

  // default user name and voice model
  *user = '\0'; 
  *model = '\0';

  // no configuration or grammar loaded
  *rcfg = '\0';
  *pcfg = '\0';
  *tcfg = '\0';
  *gram = '\0';

  // no grammar result ready
  *frag = '\0';
  nw = 0;

  // no pronunciations output, no alternates
  phon = 0;
  nalt = 0;

  // clear state
  Reset();
}


//= Default destructor does necessary cleanup.

jhcSpeechX::~jhcSpeechX ()
{
  Listen(0);
  Finish();
}


// Sets up components of speech system.
// needs to be called before using recognition or TTS
// returns positive if successful, 0 or negative for failure

int jhcSpeechX::Init (int dbg, int noisy)
{
  int ans = Ready();

  // report source (if any)
  if (noisy > 0)
    jprintf("Speech system initialized from:\n  %s\n", ifile);

  // RECO
  if (r_ok <= 0)
  {
    // try to attach to specified reco DLL and configure 
    if (noisy > 0)
    {
      jprintf("\n-------- reco --------\n");
      jprintf("DLL = %s\ncfg = %s\n", rfile, rcfg);
    }
    ans = BindReco(rfile, rcfg, 1 + dbg);
    if (ans <= 0)
    {
      if (noisy > 0)
        jprintf("\n>>> reco FAILED!\n");
    }
    else if (SetUser(user) <= 0)
    {
      if (noisy > 0)
        jprintf("\n>>> user FAILED!\n");
    }
  }

  // PARSE
  // try to attach to specified parser DLL and configure 
  if (noisy > 0)
  {
    jprintf("\n-------- parse -------\n");
    jprintf("DLL = %s\ncfg = %s\ngrm = %s\n", pfile, pcfg, gram);
  }
  ClearGrammar();                      
  ans = BindParse(pfile, pcfg);
  if (ans <= 0) 
  {  
    if (noisy > 0)
      jprintf("\n>>> parse FAILED!\n");
  }
  else if (LoadSpGram(gram) <= 0)
  {
    if (noisy > 0)
      jprintf("\n>>> grammar FAILED!\n");
  }
   
  // TTS
  if (t_ok <= 0)
  {
    LoadAlt("pronounce.map");
    if (noisy > 0)
    {
      jprintf("\n--------- TTS --------\n");
      jprintf("DLL = %s\ncfg = %s\n", tfile, tcfg);
      jprintf("  %d re-spellings from: pronounce.map\n", Fixes());
    }
    ans = BindTTS(tfile, tcfg);
    if ((ans <= 0) && (noisy > 0))
      jprintf("\n>>> TTS FAILED!\n");
  }

  // see if everything worked
  ans = Ready();
  if (noisy > 0)
  {
    jprintf("\nSpeech -> %s\n", ((ans > 0) ? "OK" : "FAILED !!!"));
    jprintf("=========================\n");
  }
  return ans;
}


//= Basic initialization of just text-to-speech system.

int jhcSpeechX::InitTTS (int noisy)
{
  char info[80];
  int ans;

  LoadAlt("pronounce.map");
  ans = BindTTS(tfile, tcfg);
  if (noisy > 0)
  {
    jprintf("TTS\t= DLL version %s\n", tts_version(info, 80));
    jprintf("Voice\t= %s\n", tts_voice(info, 80));
    jprintf("Output\t= %s\n", tts_output(info, 80));
    jprintf("  %d re-spellings from: pronounce.map\n", Fixes());
    jprintf("\nTTS -> %s\n", ((ans > 0) ? "OK" : "FAILED !!!"));
    jprintf("=========================\n\n");
  }
  return ans;
}


//= Clear state for beginning of run.
// assumes Init has already been called to set up system

void jhcSpeechX::Reset ()
{
  hear = 0;
  talk = 0;
  tlast = 0;
  tlock = 0;
  *qtext = '\0'; 
  emit = NULL;
  rcv = NULL;
  now = 0;
  last = 0;
  suspend = 0;
  reco_list_users(model);
  acc = 0;
  txtin = 0;
}


//= See if recognition loop should exit.

bool jhcSpeechX::Escape ()
{
  // look for escape key
  if (_kbhit() == 0) 
    return false;
  if (_getch() == '\x1B')
    return true;

  // toggle Paused() state
  if (suspend > 0)
  {
    suspend = 0;
    reco_listen(1);
    jprintf("resume\n\n");
  }
  else
  {
    suspend = 1;
    reco_listen(0);
    jprintf("\n>> Pause ... ");
  }
  return false;
}


///////////////////////////////////////////////////////////////////////////
//                             Configuration                             //
///////////////////////////////////////////////////////////////////////////

//= Read names of DLLs from a text file.
// looks for lines beginning with sp_reco, sp_parse, and sp_tts
// returns 1 if found and bound, 0 if not bound, -1 for bad file
// Note: must call Init after this to set up system

int jhcSpeechX::Defaults (const char *fname)
{
  jhcParam ps;
  int ok = 1;

  // save configuration source file (if any)
  if (fname == NULL)
    return -1;
  strcpy_s(ifile, fname);
/*
  // get dll names from configuration file (overrides defaults)
  ps.LoadText(rfile, fname, "reco_dll",  NULL, 200);
  ps.LoadText(pfile, fname, "parse_dll", NULL, 200);
  ps.LoadText(tfile, fname, "tts_dll",   NULL, 200);

  // get individual cfg files from master file
  ps.LoadText(rcfg, fname, "reco_cfg",  NULL, 200);
  ps.LoadText(pcfg, fname, "parse_cfg", NULL, 200);
  ps.LoadText(tcfg, fname, "tts_cfg",   NULL, 200);
*/

  // get audio input 
  ok &= ps.LoadText(mic, fname, "reco_mic", NULL, 80);

  // get default grammar
  ok &= ps.LoadText(gram, fname, "grammar", NULL, 200);
  return ok;
}


//= Read just body specific values from a file.

int jhcSpeechX::LoadCfg (const char *fname)
{
  jhcParam ps;
  int ok = 1;

  ok &= ps.LoadText(vname, fname, "voice",     NULL, 80);
  ok &= ps.LoadText(user,  fname, "user_name", NULL, 80);
  return ok;
}


//= Write current processing variable values to a file.

int jhcSpeechX::SaveVals (const char *fname) const
{
  jhcParam ps;
  char abbr[80];
  int ok = 1;
/*
  // speech recognition
  ok &= ps.SaveText(fname, "reco_cfg",  rcfg);
  ok &= ps.SaveText(fname, "reco_dll",  rfile);

  // parsing
  ok &= ps.SaveText(fname, "parse_cfg", pcfg);
  ok &= ps.SaveText(fname, "parse_dll", pfile);

  // text to speech
  ok &= ps.SaveText(fname, "tts_cfg",   tcfg);
  ok &= ps.SaveText(fname, "tts_dll",   tfile);
*/

  // save audio input 
  MicName(abbr, 1, 80);
  ok &= ps.SaveText(fname, "reco_mic", abbr);

  // save top level grammar
  ok &= ps.SaveText(fname, "grammar", gram);
  return ok;
}


//= Write current body specific values to a file.

int jhcSpeechX::SaveCfg (const char *fname) const
{
  jhcParam ps;
  char abbr[80];
  int ok = 1;

  VoiceName(abbr, 1, 80);
  ok &= ps.SaveText(fname, "voice", abbr);
  ok &= ps.SaveText(fname, "user_name", user);
  return ok;
}


//= Change prefix for debugging message (negative to suppress).

void jhcSpeechX::SetTag (int n)
{
  sprintf_s(tag, "%d ", n);
}


//= Change microphone input to speech recognizer.
// should only call after Init
// returns 1 if successful, 0 for failure

int jhcSpeechX::SetMic (const char *name)
{
  return reco_set_src(name, 0);
}


//= Get name of audio input (possibly most specific part).
// nick: 0 = full, 1 = specific, 2 = suffix

const char *jhcSpeechX::MicName (char *name, int nick, int ssz) const
{
  char full[80];
  char *start, *next;
  int n = 0;

  // get full name and check simplest cases
  if (name == NULL)
    return NULL;
  if (nick <= 0)
    return reco_input(name, ssz);
  reco_input(full, 80);

  // start at opening parenthesis but skip over "(2- "
  if ((start = strchr(full, '(')) == NULL)
    start = full;
  else if (isdigit(start[1]) == 0)
    start++;
  else if ((next = strchr(start + 1, ' ')) != NULL)
    start = next + 1;

  // remove final parenthesis (if any)
  if (nick < 2)
  {
    if ((n = (int) strlen(full)) > 0)
      if (full[n - 1] == ')')
        full[n - 1] = '\0';
    strcpy_s(name, ssz, start);
    return name;
  }

  // gather first three starting letters
  name[n++] = start[0];
  if ((next = strchr(start, ' ')) != NULL)
  {
    name[n++] = next[1];
    if ((next = strchr(next + 1, ' ')) != NULL)
      name[n++] = next[1];
  }
  name[n] = '\0';
  return name;
}


//= Change the acoustic model used for recognition.
// force: 0 = await pause, 1 = pause in background, 2 = block (~360ms)
// can add dashes and copy suffix if needed (strict <= 0)
// example: "Chris Murasso" --> "Chris_Murasso_VTI"
// caches pretty version of name, should only call after Init
// call UserName(1) after change has taken effect to get updated name
// returns 1 if successful, 0 for failure

int jhcSpeechX::SetUser (const char *name, int strict, int force)
{
  char full[200];
  const char *s = name;
  char *d = full;
  int ssz = 200;

  // check arguments
  if ((name == NULL) || (*name == '\0'))
    return 0;
  update_model();

  // add underscores between first and last names
  while (*s != '\0')
  {
    *d++ = ((*s == ' ') ? '_' : *s);
    ssz--;
    s++;
  }

  // terminate Firstname-Lastname string
  *d = '\0';
  ssz--;

  // try user model name verbatim
  if (strict > 0)
  {
    if (reco_add_user(full, force) > 0)
      return 1;
    return 0;
  }

  // add suffix from audio source (eg. "_VTI")
  *d = '_';
  MicName(d + 1, 2, ssz);
  if (reco_add_user(full, force) > 0)
    return 1;

  // try copying from previous (e.g. "_SB8") if different
  if ((s = strrchr(model, '_')) != NULL)    
    if (strcmp(s, strrchr(full, '_')) != 0)
    {
      strcpy_s(d, ssz, s);
      if (reco_add_user(full, force) > 0)
        return 1;
    }

  // look for naked user name as substring
  *d = '\0';
  if (reco_add_user(full, force) > 0)
    return 1;  
  return 0;
}


//= Get pretty name of current user based on acoustic model.
// unless chk > 0, first sees if user name already known (cached)

const char *jhcSpeechX::UserName (int chk)
{
  if ((chk > 0) || (*user == '\0') || (strncmp(user, "human", 5) == 0))
    update_model();
  return user;
}


//= Update voice model name and user name by interacting with speech engine.
// remove dashes and strip any suffix from acoustic model name
// example: "Chris_Murasso_VTI" --> "Chris Murasso"
// results stored in member variables "model" and "user"

int jhcSpeechX::update_model ()
{
  const char *s = model;
  char *d = user;
  int any = 0;

  // get current acoustic model
  if (reco_list_users(model) <= 0)
  {
    strcpy_s(user, "unknown");
    return 0;
  }

  // strip out underscores
  while ((*s != '\0') && (*s != '\n'))
    if (*s != '_')                        // used to be '-'
      *d++ = *s++;
    else
    {
      if (++any > 1)
        break;
      *d++ = ' ';
      s++;
    }

  // make sure user string is terminated
  *d = '\0';
  return 1;
}


//= Copies out just the first part of the full user name.
// returns 0 if name string not changed

int jhcSpeechX::FirstName (char *name, int ssz)
{
  const char *full;
  char *end;

  full = UserName();
  if (*full == '\0')
    return 0;
  strcpy_s(name, ssz, full);
  if ((end = strchr(name, ' ')) != NULL)
    *end = '\0';
  return 1;
}


//= Print out full configuration data for current system.
// returns overall status of system

int jhcSpeechX::PrintCfg () 
{
  char info[80];

  // possibly prefix
  if (*tag != '\0')
    jprintf("======================== UNIT %s========================\n", tag);

  // speech recognition
  if (r_ok <= 0)
    jprintf(">>> Failed to set up recognizer (%d) !\n", r_ok);
  else
  {
    jprintf("Reco\t= DLL version %s\n", reco_version(info, 80));
    jprintf("Input\t= %s\n", reco_input(info, 80));
    jprintf("Engine\t= %s\n", reco_engine(info, 80));
    reco_list_users(info);
    jprintf("User\t= %s\n", info);
  }

  // parsing
  jprintf("---\n");
  if (p_ok <= 0)
    jprintf(">>> Failed to set up parser (%d) !\n", p_ok);
  else
  {
    jprintf("Parser\t= DLL version %s\n", parse_version(info, 80));
    jprintf("Grammar\t= %s\n", gram);
  }

  // text to speech
  jprintf("---\n");
  if (t_ok <= 0)
    jprintf(">>> Failed to set up text-to-speech (%d) !\n", t_ok);
  else
  {
    jprintf("TTS\t= DLL version %s\n", tts_version(info, 80));
    jprintf("Voice\t= %s\n", tts_voice(info, 80));
    jprintf("Output\t= %s\n", tts_output(info, 80));
    jprintf("  %d re-spellings from: pronounce.map\n", Fixes());
  }

  // return system status
  jprintf("\n");
  return Ready();
}


///////////////////////////////////////////////////////////////////////////
//                          Convenience Functions                        //
///////////////////////////////////////////////////////////////////////////

//= Possibly stop processing via some other signal.

void jhcSpeechX::Inject (const char *txt, int stop)
{
  if (stop > 0)
    _ungetch(0x1B);
  if ((txt != NULL) && (*txt != '\0'))
  {
    // prefer this string to reco result
    strcpy_s(utt, txt);
    rcv = utt;
    hear = 2;
    txtin = 1;               
  }
}


//= Update status of all speech-related status variables.
// can use result "prolong" from alternative speech reco when just TTS in use
// returns status of recognition (0 = quiet, 1 = speech, 2 = recognition)

int jhcSpeechX::Update (int reco, int tts, int prolong) 
{
  // see if finished talking and check grammar parser
  if (tts > 0)
    ChkOutput(); 
  if ((reco > 0) && (txtin <= 0))
  {
    rcv = NULL;
    AwaitPhrase(0.0);
  }
  txtin = 0;
  emit = NULL;

  // adjust silence counter (neither user or robot speaking)
  now = jms_now();
  if ((hear > 0) || (talk > 0) || (prolong > 0))
    last = now;
//    last = 0;
//  else if (last == 0)
//    last = now;
  acc = 1;
  return hear;
}


//= Start any pending actions.

void jhcSpeechX::Issue ()  
{ 
  Utter();
  tlast = tlock;
  tlock = 0;
  acc = 0;
}


//= Length of the current silence interval (in seconds).

double jhcSpeechX::Silence () const
{
  if (last == 0)
    return 300.0;                      // 5 minutes
  return(0.001 * (now - last));
}


///////////////////////////////////////////////////////////////////////////
//                           Speech Recognition                          //
///////////////////////////////////////////////////////////////////////////

//= Turns on (1) or off (0) speech recognition functionality.
// NOTE: if block > 0 then waits for 40-300ms 

void jhcSpeechX::Listen (int reco, int block)
{
  if (r_ok <= 0)
   return;
  reco_listen(reco, block);
  if (reco <= 0)
    hear = 0;
}


//= Specify where sound seems to be coming from to help speech recognition.
// this info could come from the microphone or from face recognition

void jhcSpeechX::SuggestPos (double azim, double elev, double dist) 
{
  if (r_ok <= 0)
    return; 
  reco_loc_user(ROUND(azim), ROUND(elev), ROUND(dist));
}


//= Try to set the speech recognition engine for a particular user's voice.
// can be useful for priming speech recogniton based on face recognition

int jhcSpeechX::SuggestUser (const char *name, ...) 
{
  va_list args;
  char user[80], info[200];
  char *end, *all = info;
  int n;

  if (r_ok <= 0)
    return -2; 

  // build requested name
  va_start(args, name); 
  vsprintf_s(user, name, args);

  // see if already active (could be several in list)
  reco_list_users(all, 200);
  while (*all != '\0')
  {
    if ((end = strchr(all, '\n')) != NULL)
    {
      // user names listed on separate lines
      n = (int)(end - all);
      if (strncmp(all, user, n) == 0)
        return 1;
      all = end + 1;
    }
    else if (strcmp(all, user) == 0)
      return 1;   
    break;
  }

  // add or switch to this user
  return reco_add_user(user);
}


//= Waits until phrase is finished, a timeout occurs, or some key is hit.

int jhcSpeechX::AwaitOrQuit (double secs)
{
  int i, n = ROUND(secs / 0.5);

  if (r_ok <= 0)
    return -2; 

  for (i = 0; i < n; i++)
  {
    if (_kbhit())
      return -1;
    if (AwaitPhrase(0.5) >= 2)
      return 1;
  }
  return 0;
}


//= Wait for a complete phrase, always return after maximum of "secs".
// returns 2 if recognition complete, 1 if not done yet, 0 if no sound
// can use to poll background recognition engine status
// leaves focus on first important (capitalized) non-terminal

int jhcSpeechX::AwaitPhrase (double secs)
{
  char t[500], c[200];
  int rc, rc2, i = 0, n = ROUND(secs / 0.1), nmax = 0, hyp = 100;

  // make sure recognizer is initialized and robot is not talking
  if (r_ok <= 0)
    return -2; 
  if (talk > 0)
    return 0;

  // clear result then make sure engine is listening for speech
  *utt0 = '\0';
  *ph   = '\0';
  *utt  = '\0';
  *conf = '\0';
  cf = 0;
  nw = 0;
  Listen(1, 0);

  // keep checking recognition engine at regular intervals
  while (1)
  {
    if ((hear = reco_status()) < 0)
      return hear;
    if (hear == 1)
      reco_partial(utt0);
    if ((hear == 2) || (i++ >= n))
      break;
    jms_sleep(100);
  }
  if (hear < 2)          // timeout
    return hear;

  // go through first N acoustic matches in order of confidence
  hear = 0;
  for (i = 0; i < hyp; i++)
  {
    // get next interpretation (if any) and send it to the parser
    if ((rc = reco_heard(t, c, i, 500, 200)) < 0)
      return rc;
    if (rc == 0)
      break;
    if ((rc2 = parse_analyze(t, c)) < 0)
      return -1;
    if (rc2 == 0)
      continue;
    
    // save results if more words matched than best so far
    n = parse_span();
    if (n > nmax)
    {
      reco_phonetic(ph, i, 1500);
      strcpy_s(utt, t);
      strcpy_s(conf, c);
      cf = rc;
      nw = n;
      nmax = n;
    }
  }

  // restore parser to state of best match (reject likely motor noise)
  if (i > 1)
    parse_analyze(utt, conf);
  rcv = utt;
  if ((nw > 0) && (cf > 0))
    hear = 2;
  return hear;
}


//= Block until nothing heard on input line.
// useful at startup to let engine establish squelch level

int jhcSpeechX::AwaitQuiet (double secs)
{
  int i, n = ROUND(secs / 0.1);

  // make sure engine is listening for speech
  if (r_ok <= 0)
    return -1; 
  Listen(1);

  // keep checking recognition engine at regular intervals
  for (i = 0; i < n; i++)
  {
    if ((hear = reco_status()) < 0)
      return hear;
    if (hear == 0)
      return 1;
    jms_sleep(100);
  }
  return 0;            // timeout
}


//= Give name of likely speaker of last utterance.
// returns pointer to internal string **WHICH MAY GET REWRITTEN**
// copy the returned string if you want it to persist

const char *jhcSpeechX::SpeakerId () 
{
  if (reco_speaker(model) <= 0)
    return NULL;
  update_model();
  return user;
}


///////////////////////////////////////////////////////////////////////////
//                                Parsing                                //
///////////////////////////////////////////////////////////////////////////

//= Remembers grammar to load but does NOT load it yet.
// used as default when calling LoadSpGram(NULL) or Init()
// generally "gfile" is only the first grammar added
// NOTE: most common pattern is SetGrammar then Init 

void jhcSpeechX::SetGrammar (const char *fname, ...)
{
  va_list args;

  if ((fname == NULL) || (*fname == '\0'))
    return;
  va_start(args, fname); 
  vsprintf_s(gram, fname, args);
  if (strchr(gram, '.') == NULL)
    strcat_s(gram, ".sgm");
}


//= Get rid of any loaded grammar rules but generally keep file name.

void jhcSpeechX::ClearGrammar (int keep)
{
  char first = *gram;

  parse_clear();
  if (keep > 0)
    *gram = first;  
}


//= Load a recognition grammar from an XML, binary, or generic file.
// appends new rules if some other grammar(s) already loaded
// to get rid of old rules first call ClearGrammar()
// all rules initially unmarked (i.e. not active top level)
// returns 0 if some error, else 1
// NOTE: most common pattern is SetGrammar then Init 

int jhcSpeechX::LoadSpGram (const char *fname, ...)
{
  char append[200];
  char *gf = (char *)((*gram == '\0') ? &gram : &append);
  va_list args;
  int rc;

  if (p_ok < 0) 
    return -1;

  // assemble file name
  if (fname == NULL)
    gf = gram;
  else if (*fname == '\0')
    return 0;
  else
  {
    va_start(args, fname); 
    vsprintf_s(gf, 200, fname, args);
    if (strchr(gf, '.') == NULL)
      strcat_s(gf, 200, ".sgm");
  }

  // try loading
  if ((rc = parse_load(gf)) > 0)
    return 1;
  return rc;
}


//= Activate (val = 1) or deactivate (val = 0) a grammar rule.
// needs name attribute, e.g. <RULE NAME="foo" TOPLEVEL="ACTIVE">
// use NULL as name to mark all top level rules
// returns 0 if could not find rule, else 1

int jhcSpeechX::MarkRule (const char *name, int val)
{
  if (p_ok < 0)
    return -1;

  if (val <= 0)
    return parse_disable(name);
  return parse_enable(name);
}


//= Add another valid expansion for some non-terminal.
// returns 2 if full update, 1 if not in file, 0 or negative for failure

int jhcSpeechX::ExtendRule (const char *name, const char *phrase, int file)
{
  if ((name == NULL) || (phrase == NULL) || (*name == '\0') || (*phrase == '\0'))
    return 0;
  return parse_extend(name, phrase, file);
}


///////////////////////////////////////////////////////////////////////////
//                           Association List                            //
///////////////////////////////////////////////////////////////////////////

//= Returns the non-terminal associated with the root of the parse tree.
// returns pointer to internal string **WHICH MAY GET REWRITTEN**
// copy the returned string if you want it to persist

const char *jhcSpeechX::Root ()
{
  if (p_ok <= 0)
    return NULL;

  parse_top();
  parse_focus(frag);
  return frag;
}


//= Moves focus to highest important (capitalized) non-terminal.
// returns name of this rule (i.e. typically the category)
// returns pointer to internal string **WHICH MAY GET REWRITTEN**
// copy the returned string if you want it to persist

const char *jhcSpeechX::TopCat ()
{
  if (p_ok <= 0)
    return NULL;

  parse_top();
  if (tree_major(frag, 80) > 0)
    return frag;
  return NULL;
}


//= Do depth first search of tree until first all capitalized rule is found.

int jhcSpeechX::tree_major (char *ans, int ssz)
{
  // see if current focus is a winner
  parse_focus(ans, ssz);
  if (all_caps(ans) > 0)
    return 1;

  // try going down further
  if (parse_down() > 0)
  {
    if (tree_major(ans, ssz) > 0)
      return 1;
    parse_up();
  }

  // else try next part of current expansion
  if (parse_next() > 0)
    return tree_major(ans, ssz);
  return 0;
}


//= Generates a string encoding an association list of slots and values.
// finds all major (capitalized) categories and first non-terminal underneath (if any).
// e.g. "GRAB=nil SIZE=red COLOR=red" for "Grab the big red block" all prefixed by tabs
// can return a semi-phonetic word for dictation items if "phon" variable is positive

char *jhcSpeechX::SlotValuePairs (char *alist, int close, int ssz)
{
  // start with empty list 
  *alist = '\0';
  if ((p_ok <= 0) || (hear < 2))
    return alist;

  // add other capitalized rules from grammar
  parse_top();
  tree_slots(alist, phon, close, ssz);
  return alist;
}


//= Do depth first search of tree to find capitalized nodes and children.
// if first character of a node is "^" then bind covered string instead
// prefixes "!" and "$" emit tag but do not block descent
// will mark ends of !, $, and % phrases if close > 0
// can optionally return a fake semi-phonetic word for dictation items

void jhcSpeechX::tree_slots (char *alist, int fake, int close, int ssz)
{
  char node[40], val[200];
  int first, last;

  // see if current node is a phrase boundary
  parse_focus(node);
  if (strchr("!$%", *node) != NULL)
  {
    append(alist, "\t", ssz);
    append(alist, node, ssz);
  }

  // see if current focus is a winner (capitalized)
  if (all_caps(node) > 0)
  {
    // add rule name (needs to have leading tab always)
    append(alist, "\t", ssz);
    append(alist, node, ssz);
    append(alist, "=", ssz);

    // add first subcategory (or full node expansion)
    if ((*node != '^') && (parse_down() > 0))
    {
      // minor node name verbatim
      parse_focus(node);
      append(alist, node, ssz);
      parse_up();
    }
    else
    {
      // possibly clean up dictation results
      parse_span(&first, &last);
      if ((fake > 0) && (strncmp(node, "DICT", 4) == 0))
        fake_words(val, first, last, 200);
      else
        get_words(val, first, last, 200);
      strcat_s(alist, ssz, val);
    }
  }
  else if (parse_down() > 0)
  {
    // try going down further
    tree_slots(alist, fake, close, ssz);
    parse_up();
  }

  // possibly mark phrase ending
  if ((close > 0) && (strchr("!$%", *node) != NULL))
  {
    node[1] = '\0';
    append(alist, "\t", ssz);
    append(alist, node, ssz);
  }

  // try next part of current expansion
  if (parse_next() > 0)
    tree_slots(alist, fake, close, ssz);
}


//= Test whether all alphabetic characters are uppercase.

int jhcSpeechX::all_caps (const char *name) const
{
  const char *c = name;

  if (name == NULL)
    return 0;

  while (*c != '\0')
    if (islower(*c))
      return 0;
    else
      c++;
  return 1;
}


//= Get a sequence of words from the input to the parser.
// returns a pointer to an internal string which may get overwritten 

char *jhcSpeechX::get_words (char *dest, int first, int last, int ssz) const
{
  char input[500], sep[20] = " \t\n.";
  char *word, *w0 = input;
  char **src = &w0;
  int i;

  // check for valid parse and word range spec
  if ((p_ok <= 0) || (first < 0) || (last < first))
    return NULL;

  // make a copy of the input and clear output
  strcpy_s(input, utt);
  *dest = '\0';

  // skip words until requested start
  for (i = 0; i < first; i++) 
    if ((word = next_token(src, sep)) == NULL)
      return NULL;

  // accumulate words until requested end
  for (i = first; i <= last; i++)
  {
    if ((word = next_token(src, sep)) == NULL)
      break;
    if (i > first)
      append(dest, " ", ssz);
    append(dest, word, ssz);
  }
  return dest;
}
    

//= Generate synthetic words based on the phonetic transcription.

char *jhcSpeechX::fake_words (char *dest, int first, int last, int ssz) const
{
  char key[40][5] = {"aa", "ae", "ah", "ao", "aw", "ax", "ay",  "b", "ch",  "d",
                     "dh", "eh", "er", "ey",  "f",  "g",  "h", "ih", "iy", "jh",
                      "k",  "l",  "m",  "n", "ng", "ow", "oy",  "p",  "r",  "s",
                     "sh",  "t", "th", "uh", "uw",  "v",  "w",  "y",  "z", "zh"};
  char alt[40][5] = {"ah",  "a",  "u", "aw", "ow", "uh", "ai",  "b", "ch",  "d",
                     "th",  "e", "ur", "ay",  "f",  "g",  "h",  "i", "ee",  "j",
                      "k",  "l",  "m",  "n", "ng", "oe", "oy",  "p",  "r",  "s",
                     "sh",  "t", "th", "oo", "ew",  "v",  "w",  "y",  "z", "zh"};
  int hard[40]    = {  0,    0,    0,    0,    0,    0,    0,    2,    2,    2,
                       2,    0,    0,    0,    2,    2,    2,    0,    0,    2, 
                       2,    1,    2,    2,    2,    0,    0,    2,    1,    1,
                       1,    2,    2,    0,    0,    2,    2,    2,    1,    1 };
  int snd[80], sep[80];    
  char input[500];
  char *p, *w, *w0 = input;
  char **phones = &w, **src = &w0;
  int i, j, n = 0, any = 0, run = 0;

  // check for valid parse and word range spec
  if ((p_ok <= 0) || (first < 0) || (last < first))
    return NULL;

  // make a copy of the phonemes and clear output
  strcpy_s(input, ph);
  *dest = '\0';

  // skip words until requested start
  for (i = 0; i < first; i++) 
    if ((w = next_token(src, "\n")) == NULL)
      return NULL;

  // accumulate words until requested end
  for (i = first; i <= last; i++)
  {
    // get phoneme sequence for this word
    if ((w = next_token(src, "\n")) == NULL)
      break;

    // lookup all phonemes in sequence
    while ((p = next_token(phones, " ")) != NULL)
      for (j = 0; j < 40; j++)
        if (strcmp(p, key[j]) == 0)
        {
          // save phoneme index
          snd[n] = j;
          sep[n] = 0;
          n++;
          break;
        }
  }

  // figure out where syllable separators should be
  for (i = 0; i < n; i++)
  {
    // measure length of consonant clusters
    if (hard[snd[i]] > 0)  
    {
      run += hard[snd[i]];                  
      continue;
    }

    // keep full first cluster
    if (any++ <= 0)
    {
      run = 0;
      continue;
    }

    // always separate adjacent vowels
    if (run <= 0)
    {
      sep[i] = 1;
      continue;
    }

    // backup half size of consonant cluster 
    if (run > 1)
      run /= 2;
    for (j = i - 1; j >= 0; j--)
      if (hard[snd[j]] > 0)
      {
        run -= hard[snd[j]];
        if (run <= 0)
        {
          // add separator
          sep[j] = 1;
          run = 0;
          break;
        }
      }
  }

  // render full fake words with separators
  for (i = 0; i < n; i++)
  {
    if (sep[i] > 0)
      append(dest, " ", ssz);
    append(dest, alt[snd[i]], ssz);
  }
  return dest;
}


//= Safer form of strtok function that allows call nesting.
// alters input to point to remainder of original string (NULL when done)
// returns pointer to found token (original string is altered with '\0')

char *jhcSpeechX::next_token (char **src, const char *delim) const
{
  char *start, *end;

  if ((src == NULL) || (*src == NULL))
    return NULL;
    
  // skip over delimiters to start of token
  start = *src;
  while ((*start != '\0') && (strchr(delim, *start) != NULL))
    start++;
  if (*start == '\0')
  {
    *src = NULL;       // no more tokens
    return NULL;
  }

  // pass through valid characters to first delimiter
  end = start + 1;
  while ((*end != '\0') && (strchr(delim, *end) == NULL))
    end++;
  if (*end == '\0')
  {
    *src = NULL;       // no more tokens
    return start;
  }

  // terminate token and point at where to look next
  *end = '\0';
  *src = end + 1;
  return start;
}


//= Alternative to strcat, which sometimes gives crazy results.

void jhcSpeechX::append (char *dest, const char *extra, int ssz) const
{
  int n = (int) strlen(dest);

  strcpy_s(dest + n, ssz - n, extra);
}


//////////////////////////////////////////////////////////////////////////
//                              Debugging                                //
///////////////////////////////////////////////////////////////////////////

//= Print out sections of parse tree for debugging.
// if top positive then resets to tree root, else starts at current node

void jhcSpeechX::PrintTree (int top)
{
  if (p_ok <= 0)
    return;

  if (top > 0)
    parse_top();
  print_focus(0, 0, 0);
}


//= Go depth-first through parse tree showing non-terminals and expansions

void jhcSpeechX::print_focus (int indent, int start, int end)
{
  char val[200], node[40], leader[40] = "";
  int i, first, last;

  // check for valid node then get surface coverage
  if (parse_focus(node) <= 0)
    return;
  parse_span(&first, &last);

  // build indentation white space 
  for (i = 0; i < indent; i++)
    strcat_s(leader, "  ");

  // print any leading terminals then rule name
  if ((indent > 0) && (first > start))
    jprintf("%s%s\n", leader, get_words(val, start, first - 1, 200));
  jprintf("%s<%s>\n", leader, node);

  // expand non-terminal or just print surface words
  if (parse_down() > 0)
  {
    print_focus(indent + 1, first, last);
    parse_up();
  }
  else
    jprintf("%s  %s\n", leader, get_words(val, first, last, 200));

  // go on to next non-terminal or print trailing terminals
  if (parse_next() > 0)
    print_focus(indent, last + 1, end);
  else if ((indent > 0) && (last < end))
    jprintf("%s%s\n", leader, get_words(val, last + 1, end, 200));
}


///////////////////////////////////////////////////////////////////////////
//                            Speech Synthesis                           //
///////////////////////////////////////////////////////////////////////////

//= Use text to speech engine to say something immediately.
// assembles full message then starts right away
// returns 1 for convenience

int jhcSpeechX::Say (const char *msg, ...)
{
  va_list args;

  *qtext = '\0';
  if (msg != NULL)
  {
    va_start(args, msg); 
    vsprintf_s(qtext, msg, args);
  }
  emit = qtext;
  return Utter();
}


//= Propose saying something using text to speech engine.
// returns 1 if string accepted (call Utter to cause output)

int jhcSpeechX::Say (int bid, const char *msg, ...)
{
  va_list args;

  // see if previous command takes precedence
  if (bid <= tlock)
    return 0;
  tlock = bid;

  // assemble full message
  *qtext = '\0';
  if (msg != NULL)
  {
    va_start(args, msg); 
    vsprintf_s(qtext, msg, args);
  }

  // only start saying if bid is higher than what is in progress
  if (tlock > tlast)
    emit = qtext;
  return 1;
}


//= Actually cause text to speech engine to start speaking queued string.
// always speaks in the default voice selected in Control Panel
// returns 1 if something started, does not await completion

int jhcSpeechX::Utter ()
{
  int rc = 0;

  if (t_ok <= 0)
    return -1;

  // see if anything new to say 
  if (emit != NULL)
  {
    // see if any ongoing utterance and whether it can be interrupted
    if (talk > 0)
    {
      if (tlock <= tlast)      // new must be higher than on-going
        return 0;
      tts_shutup();
    }

    // turn off recognition engine and start new sentence
    Listen(0, 0);
    jprintf("\n==> \"%c%s\"\n\n", toupper(emit[0]), emit + 1);
    rc = tts_say(alt_pron(emit));
    talk = 1;
  }
  return rc;
}


//= Blocks when any spoken utterance is in progress.
// returns when finished or given time is elapses
// value indicates whether still busy (0) or silent (1)
// re-enables recognition when complete 

int jhcSpeechX::Finish (double secs)
{
  int i, wait = ROUND(secs / 0.01);

  if (t_ok <= 0)
   return -2;

  // check at regular intervals whether done yet
  for (i = 0; i <= wait; i++)
    if ((talk = tts_status(NULL, 0)) <= 0)
      break;
    else
      jms_sleep(10);

  // check for errors or still speaking
  if (talk < 0)
    return talk;
  if (talk >= 1)
    return 0;

  // possibly restart recognition engine
  Listen(1, 0);
  return 1;
}


//= Checks whether the robot is still speaking.
// actually queries the underlying speech system each time

int jhcSpeechX::ChkOutput ()
{
  if ((t_ok > 0) && ((talk = tts_status(NULL, 0)) > 0))
    return 1;  
  return 0;
}


//= Stop talking right away and ignore any queued phrases.

void jhcSpeechX::ShutUp ()
{
  if (t_ok <= 0)
   return;

  // stop anything that is running
  if (talk > 0)
  {
    tts_shutup();
    talk = 0;
  }

  // stop anything that is queued
  emit = NULL;
  tlock = 0;
}


//= Pick a particular voice for output.
// can also adjust default volume as a percentage

int jhcSpeechX::SetVoice (const char *spec, int pct)
{
  return tts_set_voice(spec, pct);
}


//= Returns the name of the speaking voice (possibly a shortened form).

const char *jhcSpeechX::VoiceName (char *name, int nick, int ssz) const
{
  char *mid;

  // get full name and check simplest cases
  if (name == NULL)
    return NULL;
  tts_voice(name, ssz);

  // stop before first dash and trim preceding space
  if (nick > 0)
    if ((mid = strchr(name, '-')) != NULL)
    {
      *mid-- = '\0';
      if ((mid >= name) && (*mid == ' '))
        *mid = '\0';
    }
  return name;
}


///////////////////////////////////////////////////////////////////////////
//                        Alternate Pronunciations                       //
///////////////////////////////////////////////////////////////////////////

//= Load a list of word transformations from a file.
// stored as simple "key subst" pairs, one per line
// key must be single word, but subst can be many
// returns number of keys loaded, negative for error

int jhcSpeechX::LoadAlt (const char *fname, int clr)
{
  FILE *in;
  char *start, *end;
  char line[200];
  int n0, n;

  // try opening file and then erase old translations
  if (fopen_s(&in, fname, "r") != 0)
    return -1;
  if (clr > 0)
    nalt = 0;
  n0 = nalt;

  while (fgets(line, 200, in) != NULL)
  {
    // check if commented out
    if ((line[0] == '/') && (line[1] == '/'))
      continue;

    // strip leading white space then find first break
    start = line;
    while (*start != '\0')
    {
      if (strchr(" \t", *start) == NULL)
        break;
      start++;
    }
    end = start;
    while (*end != '\0')
    {
      if (strchr(" \t", *end) != NULL)
        break;
      end++;
    }

    // extract key (unless nothing follows)
    if (*end == '\0')
      continue;
    strncpy_s(key[nalt], start, end - start);    // always adds terminator

    // skip intervening white space then trim from far end
    start = end + 1;
    while (*start != '\0')
    {
      if (strchr(" \t", *start) == NULL)
        break;
      start++;
    }
    for (n = (int) strlen(start); n > 0; n--)
      if (strchr(" \t\n\r\x0A", start[n - 1]) == NULL)
        break;

    // copy substitution
    if (n <= 0)
      continue;
    strncpy_s(subst[nalt], start, n);    // always adds terminator
    nalt++;    
  }

  // clean up
  fclose(in);
  return(nalt - n0);
}


//= Substitutes more phonetic spelling for certain words.
// returns pointer to alternative string to send to TTS

const char *jhcSpeechX::alt_pron (const char *src)
{
  char raw[40];
  const char *w;
  int i;

  // simplest case
  if (nalt == 0)
    return src;

  // go through input string word by word
  *atext = '\0';
  txt.Bind(src);
  while (txt.ReadWord(raw, 1) > 0)
  {
    // if matches some key then get substitute
    w = raw;
    for (i = 0; i < nalt; i++)
      if (strcmp(raw, key[i]) == 0)
      {
        w = subst[i];
        break;
      }  

    // copy (possibly converted) word to output
    if ((*atext != '\0') && !txt.Punctuation(w))
      strcat_s(atext, " ");
    strcat_s(atext, w); 
  }
  return atext;
}

