// jhcSpeechDLL.cpp : wrapper for speech-related DLL functions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2012 IBM Corporation
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

#include "Interface/jhcMessage.h"

#include "Acoustic/jhcSpeechDLL.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcSpeechDLL::jhcSpeechDLL ()
{
  // no DLLs bound
  r_dll = NULL;
  p_dll = NULL;
  t_dll = NULL;
  r_release();
  p_release();
  t_release();
  *r_path = '\0';
  *p_path = '\0';
  *t_path = '\0';
}


//= Default destructor does necessary cleanup.

jhcSpeechDLL::~jhcSpeechDLL ()
{
  // close down systems
  if (r_ok > 0)
    r_cleanup();
  if (p_ok > 0)
    p_cleanup();
  if (t_ok > 0)
    t_cleanup();

  // unbind DLLs
  r_release();
  p_release();
  t_release();
}


//= Open some named DLL at run-time and try to bind recognition functions in it.
// can optionally start reco running if cfg file is non-NULL (even just "")
// returns 1 if completely successful, 0 or negative for failure

int jhcSpeechDLL::BindReco (const char *fname, const char *cfg)
{
  // check arguments 
  if ((fname == NULL) || (*fname == '\0'))
    return -2;

  // keep or stop any running version
  if (r_ok > 0)
  {
    if (strcmp(fname, r_path) == 0)
      return 1;
    r_cleanup();
  }

  // try binding to suggested DLL
  if (r_bind(fname) <= 0)
    return -1;
  strcpy(r_path, fname);

  // try starting up bound subsystems
  if (cfg == NULL)
    return 1;
  if (reco_setup(cfg) > 0)
    if (reco_start() > 0)
      return 1;
  return 0;
}


//= Open some named DLL at run-time and try to bind parsing functions in it.
// can optionally start parser running if cfg file is non-NULL (even just "")
// returns 1 if completely successful, 0 or negative for failure

int jhcSpeechDLL::BindParse (const char *fname, const char *cfg)
{
  // check arguments 
  if ((fname == NULL) || (*fname == '\0'))
    return -2;

  // keep or stop any running version
  if (p_ok > 0)
  {
    if (strcmp(fname, p_path) == 0)
      return 1;
    p_cleanup();
  }

  // try binding to suggested DLL
  if (p_bind(fname) <= 0)
    return -1;
  strcpy(p_path, fname);

  // try starting up bound subsystems
  if (cfg == NULL)
    return 1;
  if (parse_setup(cfg) > 0)
    if (parse_start() > 0)
      return 1;
  return 0;
}


//= Open some named DLL at run-time and try to bind speaking functions in it.
// can optionally start text-to-speech running if cfg file is non-NULL (even just "")
// returns 1 if completely successful, 0 or negative for failure

int jhcSpeechDLL::BindTTS (const char *fname, const char *cfg)
{
  // check arguments 
  if ((fname == NULL) || (*fname == '\0'))
    return -2;

  // keep or stop any running version
  if (t_ok > 0)
  {
    if (strcmp(fname, t_path) == 0)
      return 1;
    t_cleanup();
  }

  // try binding to suggested DLL
  if (t_bind(fname) <= 0)
    return -1;
  strcpy(t_path, fname);

  // try starting up bound subsystems
  if (cfg == NULL)
    return 1;
  if (tts_setup(cfg) > 0)
    if (tts_start() > 0)
      return 1;
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                   Binding Speech Recognition DLL                      //
///////////////////////////////////////////////////////////////////////////

//= Attempts to attach itself to some particular recognition DLL at run-time.
// returns 0 if cannot get functions, 1 if successful

int jhcSpeechDLL::r_bind (const char *dll_name)
{
  int all = 1;

  // try to connect to DLL
  r_release();
  r_dll = LoadLibrary((LPCTSTR) dll_name);
  if (r_dll == NULL)
    return -1;

  // configuration
  if ((r_version = GetProcAddress(r_dll, "reco_version")) == NULL)
    all = 0;
  if ((r_setup   = GetProcAddress(r_dll, "reco_setup"))   == NULL)
    all = 0;
  if ((r_input   = GetProcAddress(r_dll, "reco_input"))   == NULL)
    all = 0;
  if ((r_engine  = GetProcAddress(r_dll, "reco_engine"))  == NULL)
    all = 0;
  if ((r_start   = GetProcAddress(r_dll, "reco_start"))   == NULL)
    all = 0;
  if ((r_cleanup = GetProcAddress(r_dll, "reco_cleanup")) == NULL)
    all = 0;

  // run-time modification
  if ((r_uloc  = GetProcAddress(r_dll, "reco_loc_user"))    == NULL)
    all = 0;
  if ((r_uadd  = GetProcAddress(r_dll, "reco_add_user"))    == NULL)
    all = 0;
  if ((r_udel  = GetProcAddress(r_dll, "reco_del_user"))    == NULL)
    all = 0;
  if ((r_uclr  = GetProcAddress(r_dll, "reco_clr_users"))   == NULL)
    all = 0;
  if ((r_ulist = GetProcAddress(r_dll, "reco_list_users"))  == NULL)
    all = 0;
  if ((r_madd  = GetProcAddress(r_dll, "reco_add_model"))   == NULL)
    all = 0;
  if ((r_mdel  = GetProcAddress(r_dll, "reco_del_model"))   == NULL)
    all = 0;
  if ((r_mclr  = GetProcAddress(r_dll, "reco_clr_models"))  == NULL)
    all = 0;
  if ((r_mlist = GetProcAddress(r_dll, "reco_list_models")) == NULL)
    all = 0;

  // results
  if ((r_listen  = GetProcAddress(r_dll, "reco_listen"))   == NULL)
    all = 0;
  if ((r_status  = GetProcAddress(r_dll, "reco_status"))   == NULL)
    all = 0;
  if ((r_heard   = GetProcAddress(r_dll, "reco_heard"))    == NULL)
    all = 0;
  if ((r_phone   = GetProcAddress(r_dll, "reco_phonetic")) == NULL)
    all = 0;
  if ((r_speaker = GetProcAddress(r_dll, "reco_speaker"))  == NULL)
    all = 0;

  // see if all functions found
  if (all > 0)
    r_ok = -1;
  return(r_ok + 2);
}


//= Disconnects class from whatever sp_reco DLL it was bound to.
// make sure speech reco is shutdown (r_cleanup) before calling this
// always returns 0 for convenience

int jhcSpeechDLL::r_release ()
{
  // unbind any previous libraries
  if (r_dll != NULL)
    FreeLibrary(r_dll);
  r_dll = NULL;
  *r_path = '\0';

  // set all configuration functions pointer to NULL (for safety)
  r_version = NULL;
  r_setup   = NULL;
  r_input   = NULL;
  r_engine  = NULL;
  r_start   = NULL;
  r_cleanup = NULL;

  // set all modification functions pointer to NULL (for safety)
  r_uloc  = NULL;
  r_uadd  = NULL;
  r_udel  = NULL;
  r_uclr  = NULL;
  r_ulist = NULL;
  r_madd  = NULL;
  r_mdel  = NULL;
  r_mclr  = NULL;
  r_mlist = NULL;

  // set all result functions pointer to NULL (for safety)
  r_listen  = NULL;
  r_status  = NULL;
  r_heard   = NULL;
  r_phone   = NULL;
  r_speaker = NULL;

  // mark as unloaded
  r_ok = -2;
  return 0;
}


// the calling sequences for the functions in the sp_reco DLL

typedef const char * (*FCN_R_VER)  (char *detail);
typedef          int (*FCN_R_SET)  (const char *cfg_file);
typedef const char * (*FCN_R_IN)   (char *detail);
typedef const char * (*FCN_R_ENG)  (char *detail);
typedef          int (*FCN_R_GO)   (int level, char *log_file);
typedef         void (*FCN_R_DONE) ();

typedef void (*FCN_R_ULOC) (int azim, int elev, int dist);
typedef  int (*FCN_R_UADD) (const char *name);
typedef void (*FCN_R_UDEL) (const char *name);
typedef  int (*FCN_R_UCLR) ();
typedef  int (*FCN_R_ULST) (char *list);
typedef  int (*FCN_R_MADD) (const char *name);
typedef void (*FCN_R_MDEL) (const char *name);
typedef  int (*FCN_R_MCLR) ();
typedef  int (*FCN_R_MLST) (char *list);

typedef void (*FCN_R_LIS) (int doit);
typedef  int (*FCN_R_ST)  ();
typedef  int (*FCN_R_HRD) (char *text, char *conf, int choice);
typedef void (*FCN_R_PH)  (char *pseq, int choice);
typedef  int (*FCN_R_SPK) (char *name);


///////////////////////////////////////////////////////////////////////////
//               Low-level Speech Recognition Functions                  //
///////////////////////////////////////////////////////////////////////////

//= Fills string with version number of DLL.
// returns pointer to input string for convenience

const char *jhcSpeechDLL::reco_version (char *detail)
{
  if (r_version == NULL)
    Fatal("Function reco_version unbound in jhcSpeechDLL");
  return ((FCN_R_VER) r_version)(detail);
}


//= Loads all speech engine and input device parameters based on the file given.
// the single configuration file can point to other files as needed (REQUIRED)
// returns 1 if successful, 0 or negative for some error

int jhcSpeechDLL::reco_setup (const char *cfg_file)
{
  int rc;

  // make sure function exists and DLL is bound
  if (r_setup == NULL)
    Fatal("Function reco_setup unbound in jhcSpeechDLL");
  if (r_ok != -1)
    return -1;

  // save status of subsystem
  rc = ((FCN_R_SET) r_setup)(cfg_file);
  if (rc > 0)
    r_ok = 0;
  return(r_ok + 1);
}


//= Fills string with description of audio source.
// returns pointer to input string for convenience

const char *jhcSpeechDLL::reco_input (char *detail)
{
  if (r_input == NULL)
    Fatal("Function reco_input unbound in jhcSpeechDLL");
  return ((FCN_R_IN) r_input)(detail);
}


//= Fills string with description of underlying speech recognition engine.
// returns pointer to input string for convenience

const char *jhcSpeechDLL::reco_engine (char *detail)
{
  if (r_engine == NULL)
    Fatal("Function reco_engine unbound in jhcSpeechDLL");
  return ((FCN_R_ENG) r_engine)(detail);
}


//= Start processing speech from the pre-designated audio source.
// takes a debugging level specification and log file designation
// use this to initially fire up the system (REQUIRED)
// returns 1 if successful, 0 or negative for some error

int jhcSpeechDLL::reco_start (int level, char *log_file)
{
  int rc;

  // make sure function exists and setup has been called
  if (r_start == NULL)
    Fatal("Function reco_start unbound in jhcSpeechDLL");
  if (r_ok != 0)
    return -1;

  // save status of subsystem
  rc = ((FCN_R_GO) r_start)(level, log_file);
  if (rc > 0)
    r_ok = 1;
  return r_ok;
}


//= Stop recognizing speech and clean up all objects and files.
// only call this at the end of a processing session (REQUIRED)

void jhcSpeechDLL::reco_cleanup ()
{
  if (r_cleanup == NULL)
    Fatal("Function reco_cleanup unbound in jhcSpeechDLL");
  ((FCN_R_DONE) r_cleanup)();
}


///////////////////////////////////////////////////////////////////////////

//= Provide a hint about user position to assist acoustic adaptation.
// angles are in degrees, distance is in inches relative to microphone

void jhcSpeechDLL::reco_loc_user (int azim, int elev, int dist)
{
  if (r_uloc == NULL)
    Fatal("Function reco_loc_user unbound in jhcSpeechDLL");
  ((FCN_R_ULOC) r_uloc)(azim, elev, dist);
}


//= Reconfigure the engine for a new user (acoustic model).
// this can be done without explicitly pausing the system
// if only one acoustic model is allowed, this overwrites previous
// can also be used for visual hints about the correct speaker ID
// returns 1 if successful, 0 or negative for some error

int jhcSpeechDLL::reco_add_user (const char *name)
{
  if (r_uadd == NULL)
    Fatal("Function reco_add_user unbound in jhcSpeechDLL");
  return ((FCN_R_UADD) r_uadd)(name);
}


//= Disable the acoustic model for a particular user.
// sometimes useful if a certain speaker is known to have left

void jhcSpeechDLL::reco_del_user (const char *name)
{
  if (r_udel == NULL)
    Fatal("Function reco_del_user unbound in jhcSpeechDLL");
  ((FCN_R_UDEL) r_udel)(name);
}


//= Disable all current acoustic models (users).
// this may also load up some background acoustic model instead

void jhcSpeechDLL::reco_clr_users ()
{
  if (r_uclr == NULL)
    Fatal("Function reco_clr_users unbound in jhcSpeechDLL");
  ((FCN_R_UCLR) r_uclr)();
}


//= Give the ID strings associate with the current users (acoustic models).
// elements are separated with newline characters
// returns the count of elements in the string (separator = new line)

int jhcSpeechDLL::reco_list_users (char *list)
{
  if (r_ulist == NULL)
    Fatal("Function reco_list_users unbound in jhcSpeechDLL");
  return ((FCN_R_ULST) r_ulist)(list);
}


//= Enable a certain (or additional) language model (grammar / vocabulary).
// this can be done without explicitly pausing the system
// if only one langauge model is allowed, this overwrites previous
// returns 1 if successful, 0 or negative for some error

int jhcSpeechDLL::reco_add_model (const char *topic)
{
  if (r_madd == NULL)
    Fatal("Function reco_add_model unbound in jhcSpeechDLL");
  return ((FCN_R_MADD) r_madd)(topic);
}


//= Disable a certain language model (grammar / vocabulary).

void jhcSpeechDLL::reco_del_model (const char *topic)
{
  if (r_mdel == NULL)
    Fatal("Function reco_del_model unbound in jhcSpeechDLL");
  ((FCN_R_MDEL) r_mdel)(topic);
}


//= Disable all current language models (grammars / vocabularies).
// this may also load up some background language model instead

void jhcSpeechDLL::reco_clr_models ()
{
  if (r_mclr == NULL)
    Fatal("Function reco_clr_models unbound in jhcSpeechDLL");
  ((FCN_R_MCLR) r_mclr)();
}


//= Generate a list of all currently enabled language models. 
// elements are separated with newline characters
// returns the count of elements in the string (separator = new line)

int jhcSpeechDLL::reco_list_models (char *list)
{
  if (r_mlist == NULL)
    Fatal("Function reco_list_models unbound in jhcSpeechDLL");
  return ((FCN_R_MLST) r_mlist)(list);
}


///////////////////////////////////////////////////////////////////////////

//= Temporarily stop or resume processing speech (e.g. to free up CPU).
// also useful for half-duplex mode to ignore text-to-speech utterances
// the reco_start function automatically calls reco_listen(1) 

void jhcSpeechDLL::reco_listen (int doit)
{
  if (r_listen == NULL)
    Fatal("Function reco_listen unbound in jhcSpeechDLL");
  ((FCN_R_LIS) r_listen)(doit);
}


//= See if any utterances are ready for harvesting.
// returns 0 for silence, 1 for speech detected, 2 for ready, negative for some error

int jhcSpeechDLL::reco_status ()
{
  if (r_status == NULL)
    Fatal("Function reco_status unbound in jhcSpeechDLL");
  return ((FCN_R_ST) r_status)();
}


//= Get the nth alternative for the last recognition result as a text string.
// assumes reco_status called immediately before this and returns 2
// also generates a string with the confidences (0-100) for each word
// and gives the confidence in this whole alternative (0-100)
// zero confidence generally means you have gotten to the end of the list
// returns confidence, 0 if nothing ready, -1 or less for some error

int jhcSpeechDLL::reco_heard (char *text, char *conf, int choice)
{
  if (r_heard == NULL)
    Fatal("Function reco_heard unbound in jhcSpeechDLL");
  return ((FCN_R_HRD) r_heard)(text, conf, choice);
}


//= Get the phoneme string for the nth alternative of the last recognition result.
// phonemes are one or two characters separated by spaces with "&" between words

void jhcSpeechDLL::reco_phonetic (char *pseq, int choice)
{
  if (r_phone == NULL)
    Fatal("Function reco_phonetic unbound in jhcSpeechDLL");
  ((FCN_R_PH) r_phone)(pseq, choice);
}


//= Give ID string associated with the most likely speaker of the last utterance.
// if unknown then function returns 0 and clears string

int jhcSpeechDLL::reco_speaker (char *name)
{
  if (r_speaker == NULL)
    Fatal("Function reco_speaker unbound in jhcSpeechDLL");
  return ((FCN_R_SPK) r_speaker)(name);
}


///////////////////////////////////////////////////////////////////////////
//                       Binding Parsing DLL                             //
///////////////////////////////////////////////////////////////////////////

//= Attempts to attach itself to some particular parsing DLL at run-time.
// returns 0 if cannot get functions, 1 if successful

int jhcSpeechDLL::p_bind (const char *dll_name)
{
  int all = 1;

  // try to connect to DLL
  p_release();
  p_dll = LoadLibrary((LPCTSTR) dll_name);
  if (p_dll == NULL)
    return -1;

  // configuration
  if ((p_version = GetProcAddress(p_dll, "parse_version")) == NULL)
    all = 0;
  if ((p_setup   = GetProcAddress(p_dll, "parse_setup"))   == NULL)
    all = 0;
  if ((p_start   = GetProcAddress(p_dll, "parse_start"))   == NULL)
    all = 0;
  if ((p_cleanup = GetProcAddress(p_dll, "parse_cleanup")) == NULL)
    all = 0;

  // run-time modification
  if ((p_load    = GetProcAddress(p_dll, "parse_load"))    == NULL)
    all = 0;
  if ((p_clear   = GetProcAddress(p_dll, "parse_clear"))   == NULL)
    all = 0;
  if ((p_enable  = GetProcAddress(p_dll, "parse_enable"))  == NULL)
    all = 0;
  if ((p_disable = GetProcAddress(p_dll, "parse_disable")) == NULL)
    all = 0;
  if ((p_extend  = GetProcAddress(p_dll, "parse_extend"))  == NULL)
    all = 0;

  // results
  if ((p_analyze = GetProcAddress(p_dll, "parse_analyze")) == NULL)
    all = 0;
  if ((p_focus   = GetProcAddress(p_dll, "parse_focus"))   == NULL)
    all = 0;
  if ((p_span    = GetProcAddress(p_dll, "parse_span"))    == NULL)
    all = 0;
  if ((p_top     = GetProcAddress(p_dll, "parse_top"))     == NULL)
    all = 0;
  if ((p_next    = GetProcAddress(p_dll, "parse_next"))    == NULL)
    all = 0;
  if ((p_down    = GetProcAddress(p_dll, "parse_down"))    == NULL)
    all = 0;
  if ((p_up      = GetProcAddress(p_dll, "parse_up"))      == NULL)
    all = 0;

  // see if all functions found
  if (all > 0)
    p_ok = -1;
  return(p_ok + 2);
}


//= Disconnects class from whatever sp_parse DLL it was bound to.
// make sure parsing is shutdown (p_cleanup) before calling this
// always returns 0 for convenience

int jhcSpeechDLL::p_release ()
{
  // unbind any previous libraries
  if (p_dll != NULL)
    FreeLibrary(p_dll);
  p_dll = NULL;
  *p_path = '\0';

  // set all configuration function pointers to NULL (for safety)
  p_version = NULL;
  p_setup   = NULL;
  p_start   = NULL;
  p_cleanup = NULL;

  // set all modification function pointers to NULL (for safety)
  p_load    = NULL;
  p_clear   = NULL;
  p_enable  = NULL;
  p_disable = NULL;
  p_extend  = NULL;

  // set all result function pointers to NULL (for safety)
  p_analyze = NULL;
  p_focus   = NULL;
  p_span    = NULL;
  p_top     = NULL;
  p_next    = NULL;
  p_down    = NULL;
  p_up      = NULL;

  // mark as unloaded
  p_ok = -2;
  return 0;
}


// the calling sequences for the functions in the sp_parse DLL

typedef const char * (*FCN_P_VER)  (char *detail);
typedef          int (*FCN_P_SET)  (const char *cfg_file);
typedef          int (*FCN_P_GO)   (int level, char *log_file);
typedef         void (*FCN_P_DONE) ();

typedef  int (*FCN_P_LOAD) (const char *grammar);
typedef void (*FCN_P_CLR)  ();
typedef  int (*FCN_P_ENA)  (const char *rule);
typedef  int (*FCN_P_DIS)  (const char *rule);
typedef  int (*FCN_P_EXT)  (const char *rule, const char *option);

typedef int (*FCN_P_ANA)  (const char *text, const char *conf);
typedef int (*FCN_P_FOC)  (char *token);
typedef int (*FCN_P_SPAN) (int *first, int *last);
typedef int (*FCN_P_TOP)  ();
typedef int (*FCN_P_NXT)  ();
typedef int (*FCN_P_DN)   ();
typedef int (*FCN_P_UP)   ();


///////////////////////////////////////////////////////////////////////////
//                    Low-level Parsing Functions                        //
///////////////////////////////////////////////////////////////////////////

//= Fills string with version number of DLL and possibly other information.
// returns pointer to input string for convenience

const char *jhcSpeechDLL::parse_version (char *detail)
{
  if (p_version == NULL)
    Fatal("Function parse_version unbound in jhcSpeechDLL");
  return ((FCN_P_VER) p_version)(detail);
}


//= Loads all common grammar and parsing parameters based on the file given.
// the single configuration file can point to other files as needed (REQUIRED)
// returns 1 if successful, 0 or negative for some error

int jhcSpeechDLL::parse_setup (const char *cfg_file)
{
  int rc;

  // make sure function exists and DLL is bound
  if (p_setup == NULL)
    Fatal("Function parse_setup unbound in jhcSpeechDLL");
  if (p_ok != -1)
    return -1;

  // save status of subsystem
  rc = ((FCN_P_SET) p_setup)(cfg_file);
  if (rc > 0)
    p_ok = 0;
  return(p_ok + 1);
}


//= Start accepting utterances to parse according to some grammar(s).
// takes a debugging level specification and log file designation
// use this to initially fire up the system (REQUIRED)
// returns 1 if successful, 0 or negative for some error

int jhcSpeechDLL::parse_start (int level, char *log_file)
{
  int rc;

  // make sure function exists and setup has been called
  if (p_start == NULL)
    Fatal("Function parse_start unbound in jhcSpeechDLL");
  if (p_ok != 0)
    return -1;

  // save status of subsystem
  rc = ((FCN_P_GO) p_start)(level, log_file);
  if (rc > 0)
    p_ok = 1;
  return p_ok;
}


//= Stop accepting utterances and clean up all objects and files.
// only call this at the end of a processing session (REQUIRED)

void jhcSpeechDLL::parse_cleanup ()
{
  if (p_cleanup == NULL)
    Fatal("Function parse_cleanup unbound in jhcSpeechDLL");
  ((FCN_P_DONE) p_cleanup)();
}


///////////////////////////////////////////////////////////////////////////

//= Load a certain (or additional) grammar from a file.
// this can be done without explicitly pausing the system
// if only one grammar is allowed, this overwrites previous
// initially all rules are disabled (call parse_enable)
// returns 1 if successful, 0 or negative for some error

int jhcSpeechDLL::parse_load (const char *grammar)
{
  if (p_load == NULL)
    Fatal("Function parse_load unbound in jhcSpeechDLL");
  return ((FCN_P_LOAD) p_load)(grammar);
}


//= Disable all grammars that may have been loaded.

void jhcSpeechDLL::parse_clear ()
{
  if (p_clear == NULL)
    Fatal("Function parse_clear unbound in jhcSpeechDLL");
  ((FCN_P_CLR) p_clear)();
}


//= Enable some top-level (i.e. sentence) rule within the grammar.
// returns 1 if successful, 0 if not found, negative for some error

int jhcSpeechDLL::parse_enable (const char *rule)
{
  if (p_enable == NULL)
    Fatal("Function parse_enable unbound in jhcSpeechDLL");
  return ((FCN_P_ENA) p_enable)(rule);
}


//= Disable some top-level (i.e. sentence) rule within the grammar.
// a NULL rule name serves to disable ALL top level rules
// returns 1 if successful, 0 if not found, negative for some error

int jhcSpeechDLL::parse_disable (const char *rule)
{
  if (p_disable == NULL)
    Fatal("Function parse_disable unbound in jhcSpeechDLL");
  return ((FCN_P_DIS) p_disable)(rule);
}


//= Add a new expansion to some existing rule in the grammar.
// alters internal graph and attempts to change original grammar file also
// returns 2 if ok, 1 if only run-time changed, 0 or negative for error

int jhcSpeechDLL::parse_extend (const char *rule, const char *option)
{
  if (p_extend == NULL)
    Fatal("Function parse_extend unbound in jhcSpeechDLL");
  return ((FCN_P_EXT) p_extend)(rule, option);
}


///////////////////////////////////////////////////////////////////////////

//= Accept an utterance for parsing by currently active grammar(s).
// can optionally take list of confidences (0-100) for each word
// automatically sets focus to top of parse tree (if found)
// returns number of words matched, 0 if no valid parse, negative if error

int jhcSpeechDLL::parse_analyze (const char *text, const char *conf)
{
  if (p_analyze == NULL)
    Fatal("Function parse_analyze unbound in jhcSpeechDLL");
  return ((FCN_P_ANA) p_analyze)(text, conf);
}


//= Returns the name or string associated with the current focus node.
// returns 1 if successful, 0 or negative for error

int jhcSpeechDLL::parse_focus (char *token)
{
  if (p_focus == NULL)
    Fatal("Function parse_focus unbound in jhcSpeechDLL");
  return ((FCN_P_FOC) p_focus)(token);
}


//= Returns the range of surface words covered by the current focus node.
// word 0 is the initial word in the utterance
// returns total number of words in utterance, 0 or negative for error

int jhcSpeechDLL::parse_span (int *first, int *last)
{
  if (p_span == NULL)
    Fatal("Function parse_span unbound in jhcSpeechDLL");
  return ((FCN_P_SPAN) p_span)(first, last);
}


//= Reset the focus to the top most node of the parse tree.
// returns 1 if successful, 0 or negative for error

int jhcSpeechDLL::parse_top ()
{
  if (p_top == NULL)
    Fatal("Function parse_top unbound in jhcSpeechDLL");
  return ((FCN_P_TOP) p_top)();
}


//= Move the focus one element to the right in the current expansion.
// returns 1 if successful, 0 if focus unchanged, negative for error

int jhcSpeechDLL::parse_next ()
{
  if (p_next == NULL)
    Fatal("Function parse_next unbound in jhcSpeechDLL");
  return ((FCN_P_NXT) p_next)();
}


//= Move focus down one level (i.e. expand a non-terminal node).
// automatically sets focus to leftmost branch of parse tree
// returns 1 if successful, 0 if focus unchanged, negative for error

int jhcSpeechDLL::parse_down ()
{
  if (p_down == NULL)
    Fatal("Function parse_down unbound in jhcSpeechDLL");
  return ((FCN_P_DN) p_down)();
}


//= Move focus up one level (i.e. restore it to location before call to down). 
// returns 1 if successful, 0 if focus unchanged, negative for error

int jhcSpeechDLL::parse_up ()
{
  if (p_up == NULL)
    Fatal("Function parse_up unbound in jhcSpeechDLL");
  return ((FCN_P_UP) p_up)();
}


///////////////////////////////////////////////////////////////////////////
//                    Binding Speech Synthesis DLL                       //
///////////////////////////////////////////////////////////////////////////

//= Attempts to attach itself to some particular text-to-speech DLL at run-time.
// returns 0 if cannot get functions, 1 if successful

int jhcSpeechDLL::t_bind (const char *dll_name)
{
  int all = 1;

  // try to connect to DLL
  t_release();
  t_dll = LoadLibrary((LPCTSTR) dll_name);
  if (t_dll == NULL)
    return -1;

  // configuration
  if ((t_version = GetProcAddress(t_dll, "tts_version")) == NULL)
    all = 0;
  if ((t_setup   = GetProcAddress(t_dll, "tts_setup"))   == NULL)
    all = 0;
  if ((t_voice   = GetProcAddress(t_dll, "tts_voice"))   == NULL)
    all = 0;
  if ((t_output  = GetProcAddress(t_dll, "tts_output"))  == NULL)
    all = 0;
  if ((t_start   = GetProcAddress(t_dll, "tts_start"))   == NULL)
    all = 0;
  if ((t_cleanup = GetProcAddress(t_dll, "tts_cleanup")) == NULL)
    all = 0;

  // speaking
  if ((t_say    = GetProcAddress(t_dll, "tts_say"))    == NULL)
    all = 0;
  if ((t_status = GetProcAddress(t_dll, "tts_status")) == NULL)
    all = 0;
  if ((t_wait   = GetProcAddress(t_dll, "tts_wait"))   == NULL)
    all = 0;
  if ((t_shutup = GetProcAddress(t_dll, "tts_shutup")) == NULL)
    all = 0;

  // see if all functions found
  if (all > 0)
    t_ok = -1;
  return(t_ok + 2);
}


//= Disconnects class from whatever sp_tts DLL it was bound to.
// make sure text-to-speech is shutdown (t_cleanup) before calling this
// always returns 0 for convenience

int jhcSpeechDLL::t_release ()
{
  // unbind any previous libraries
  if (t_dll != NULL)
    FreeLibrary(t_dll);
  t_dll = NULL;
  *t_path = '\0';

  // set all configuration function pointers to NULL (for safety)
  t_version = NULL;
  t_setup   = NULL;
  t_voice   = NULL;
  t_output  = NULL;
  t_start   = NULL;
  t_cleanup = NULL;

  // set all speaking function pointers to NULL (for safety)
  t_say    = NULL;
  t_status = NULL;
  t_wait   = NULL;
  t_shutup = NULL;

  // mark as unloaded
  t_ok = -2;
  return 0;
}


// the calling sequences for the functions in the sp_tts DLL

typedef const char * (*FCN_T_VER)  (char *detail);
typedef          int (*FCN_T_SET)  (const char *cfg_file);
typedef const char * (*FCN_T_VOX)  (char *detail);
typedef const char * (*FCN_T_OUT)  (char *detail);
typedef          int (*FCN_T_GO)   (int level, char *log_file);
typedef         void (*FCN_T_DONE) ();

typedef int (*FCN_T_SAY)  (const char *msg);
typedef int (*FCN_T_ST)   ();
typedef int (*FCN_T_WAIT) ();
typedef int (*FCN_T_SHUT) ();


///////////////////////////////////////////////////////////////////////////
//                Low-level Speech Synthesis Functions                   //
///////////////////////////////////////////////////////////////////////////

//= Fills string with version number of DLL and possibly other information.
// returns pointer to input string for convenience

const char *jhcSpeechDLL::tts_version (char *detail)
{
  if (t_version == NULL)
    Fatal("Function tts_version unbound in jhcSpeechDLL");
  return ((FCN_T_VER) t_version)(detail);
}


//= Loads all voice and output device parameters based on the file given.
// the single configuration file can point to other files as needed (REQUIRED)
// returns 1 if successful, 0 or negative for some error

int jhcSpeechDLL::tts_setup (const char *cfg_file)
{
  int rc;

  // make sure function exists and DLL is bound
  if (t_setup == NULL)
    Fatal("Function tts_setup unbound in jhcSpeechDLL");
  if (t_ok != -1)
    return -1;

  // save status of subsystem
  rc = ((FCN_T_SET) t_setup)(cfg_file);
  if (rc > 0)
    t_ok = 0;
  return(t_ok + 1);
}


//= Fills string with description of voice being used for output.
// returns pointer to input string for convenience

const char *jhcSpeechDLL::tts_voice (char *detail)
{
  if (t_voice == NULL)
    Fatal("Function tts_voice unbound in jhcSpeechDLL");
  return ((FCN_T_VOX) t_voice)(detail);
}


//= Fills string with description of output device being used.
// returns pointer to input string for convenience

const char *jhcSpeechDLL::tts_output (char *detail)
{
  if (t_output == NULL)
    Fatal("Function tts_output unbound in jhcSpeechDLL");
  return ((FCN_T_OUT) t_output)(detail);
}


//= Start the text-to-speech system running and awaits input.
// takes a debugging level specification and log file designation
// use this to initially fire up the system (REQUIRED)
// returns 1 if successful, 0 or negative for some error

int jhcSpeechDLL::tts_start (int level, char *log_file)
{
  int rc;

  // make sure function exists and setup has been called
  if (t_start == NULL)
    return Fatal("Function tts_start unbound in jhcSpeechDLL");
  if (t_ok != 0)
    return -1;

  // save status of subsystem
  rc = ((FCN_T_GO) t_start)(level, log_file);
  if (rc > 0)
    t_ok = 1;
  return t_ok;
}


//= Stop all speech output and clean up all objects and files.
// only call this at the end of a processing session (REQUIRED)

void jhcSpeechDLL::tts_cleanup ()
{
  if (t_cleanup == NULL)
    Fatal("Function tts_cleanup unbound in jhcSpeechDLL");
  ((FCN_T_DONE) t_cleanup)();
}


///////////////////////////////////////////////////////////////////////////

//= Speak some message after filling in fields (like printf).
// queues utterance if already speaking, does not wait for completion
// returns 1 if successful, 0 or negative for some error

int jhcSpeechDLL::tts_say (const char *msg)
{
  if (t_say == NULL)
    Fatal("Function tts_say unbound in jhcSpeechDLL");
  return ((FCN_T_SAY) t_say)(msg);
}


//= Tells if system has completed emitting utterance yet.
// returns 1 if talking, 0 if queue is empty, negative for some error

int jhcSpeechDLL::tts_status ()
{
  if (t_status == NULL)
    Fatal("Function tts_status unbound in jhcSpeechDLL");
  return ((FCN_T_ST) t_status)();
}


//= Wait until system finishes speaking (BLOCKS).
// returns 1 if successful, 0 or negative for some error

int jhcSpeechDLL::tts_wait ()
{
  if (t_wait == NULL)
    Fatal("Function tts_wait unbound in jhcSpeechDLL");
  return ((FCN_T_WAIT) t_wait)();
}


//= Immediately terminate whatever is being said and anything queued.
// returns 1 if successful, 0 or negative for some error

int jhcSpeechDLL::tts_shutup ()
{
  if (t_shutup == NULL)
    Fatal("Function tts_shutup unbound in jhcSpeechDLL");
  return ((FCN_T_SHUT) t_shutup)();
}

