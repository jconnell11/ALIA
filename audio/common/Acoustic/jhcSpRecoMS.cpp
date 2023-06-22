// jhcSpRecoMS.cpp : speech recognition and synthesis using Microsoft SAPI
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2020 IBM Corporation
// Copyright 2022-2023 Etaoin Systems
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
#include <tchar.h>
#include <process.h>

#include "Interface/jhcString.h"       // common video

#include <sapi.h>                      // used to have "sapi_xp.h"

#include "Acoustic/jhcSpRecoMS.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcSpRecoMS::jhcSpRecoMS ()
{
  HANDLE frob = CreateMutex(NULL, FALSE, NULL);

  CoInitialize(NULL);  
  f = (void *) frob;
  m = NULL;

  // speech recognition
  *rfile = '\0';
  e = NULL;	  
  c = NULL;   
  a = NULL;
  *select   = '\0';
  *mic      = '\0';
  *sfile    = '\0';
  *partial  = '\0';
  *result   = '\0';     
  *phonetic = '\0';     
  attn = 0;   
  ready = 0;  
  r_ok = -1;  
  noisy = 0;
  *tag = '\0';
  dict_wt = 1.0;

  // parsing
  *pfile = '\0';
  g = NULL;    
  d = NULL;  
  *gfile = '\0';
  focus = 0;      
  match = 0;      
  p_ok = -1;
}


//= Default destructor does necessary cleanup.

jhcSpRecoMS::~jhcSpRecoMS ()
{
  HANDLE frob = (HANDLE) f;

  parse_cleanup();
  reco_cleanup();
  CoUninitialize();  
  CloseHandle(frob);
}


//= Open some named DLL at run-time and try to bind recognition functions in it.
// start: 0 = do nothing, 1 = configure & start, 2+ = allow debugging messages
// returns 1 if completely successful, 0 or negative for failure

int jhcSpRecoMS::BindReco (const char *fname, const char *cfg, int start)
{
  // ignores any file name
  if (start <= 0)
    return 1;

  // try starting up bound subsystems
  if (reco_setup(cfg) > 0)
    if (reco_start(start - 1) > 0)
      return 1;
  return 0;
}


//= Open some named DLL at run-time and try to bind parsing functions in it.
// returns 1 if completely successful, 0 or negative for failure

int jhcSpRecoMS::BindParse (const char *fname, const char *cfg, int start)
{
  // ignores any file name
  if (start <= 0)
    return 1;

  // try starting up bound subsystems
  if (parse_setup(cfg) > 0)
    if (parse_start() > 0)
      return 1;
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                    Speech Recognition Configuration                   //
///////////////////////////////////////////////////////////////////////////

//= Fills string with version number and possibly other information.

const char *jhcSpRecoMS::reco_version (char *spec, int ssz) const 
{
  if (spec != NULL)
    strcpy_s(spec, ssz, "1.80 Microsoft");  // change in parse_version also !!!
  return spec;
}


//= Loads all speech engine and input device parameters based on the file given.
// the single configuration file can point to other files as needed (REQUIRED)
// returns 1 if successful, 0 or negative for some error

int jhcSpRecoMS::reco_setup (const char *cfg_file)
{
  ISpRecognizer *eng;
  ISpRecoContext *ctx;
  ISpRecoGrammar *gram;
  ULONGLONG notify = SPFEI(SPEI_RECOGNITION) | 
                     SPFEI(SPEI_PHRASE_START) |
                     SPFEI(SPEI_INTERFERENCE) |
                     SPFEI(SPEI_FALSE_RECOGNITION) | 
                     SPFEI(SPEI_END_SR_STREAM) |
                     SPFEI(SPEI_HYPOTHESIS);

  // clear any previous setup
  reco_cleanup();

  // clear local state 
  *sfile = '\0';
  *result = '\0';
  *phonetic = '\0';
  match = 0;
  attn = 0;
  ready = 0;

  // initialize backtracking history and parse result
  focus = 0;
  stack[focus] = NULL;

  // create a non-shared recognition engine to connect to audio input
  if (FAILED(CoCreateInstance(CLSID_SpInprocRecognizer,  
                              NULL, 
                              CLSCTX_ALL, 
                              IID_ISpRecognizer,          
                              (void **)(&eng))))
    return r_ok;
  e = (void *) eng;

  // quick hack: use config file as wav source, else use default mic
  if ((cfg_file != NULL) && (*cfg_file != '\0'))
  {
    if (connect_file(cfg_file) <= 0)
      return r_ok;
    strcpy_s(sfile, cfg_file);
  }
  else if (connect_mic(mic) <= 0)
    return r_ok;
  r_ok = 0;
  p_ok = 0;

  // communicate with Win32 events, create an empty grammar
  if (FAILED(eng->CreateRecoContext(&ctx)))
    return r_ok;
  c = (void *) ctx;
  if (FAILED(ctx->SetInterest(notify, notify)))
    return r_ok;
  if (FAILED(ctx->SetNotifyWin32Event()))
    return r_ok;
  if (FAILED(ctx->CreateGrammar(0x4A6863, &gram)))
    return r_ok;
  g = (void *) gram;
  ctx->SetContextState(SPCS_DISABLED);              // not listening
  r_ok = 1;
  p_ok = 1;

  // cache input source name
  reco_input(mic, 80);
  return r_ok;
}


//= Connect recognizer to default audio input source.

int jhcSpRecoMS::connect_mic (const char *name)
{
  jhcString m;
  ISpRecognizer *eng = (ISpRecognizer *) e;
  ISpObjectToken *token = NULL;
  ISpObjectTokenCategory *cat = NULL;
  IEnumSpObjectTokens *list = NULL; 
  ISpObjectToken *src = NULL; 
  char text[200] = "";
  WCHAR wide[200];
  WCHAR *w = wide;
  WCHAR *id = NULL;
  HRESULT hr = S_FALSE;
  int match = 0;

  while (1)
  {
    // find the default audio input device
    if (FAILED(CoCreateInstance(CLSID_SpObjectTokenCategory, 
                                NULL,
                                CLSCTX_ALL, 
                                IID_ISpObjectTokenCategory,          
                                (void **)(&cat))))
      break;
    if (FAILED(cat->SetId(SPCAT_AUDIOIN, TRUE)))
      break;

    // if no name given then get default audio input
    if ((name == NULL) || (*name == '\0'))
    {
      if (FAILED(cat->GetDefaultTokenId(&id)))
        break;
    }
    else
    {
      // check each speech model in the enumeration for a name match
      cat->EnumTokens(NULL, NULL, &list);
      while (list->Next(1, &src, NULL) == S_OK)
      {
        src->GetStringValue(NULL, &w);
        WideCharToMultiByte(CP_UTF8, 0, w, -1, text, 200, NULL, NULL);
        if (strstr(text, name) != NULL)
        {
          match = 1;
          break; 
        }
      }
      if (match <= 0)
        break;
      src->GetId(&id);
    }

    // create a streaming audio input (from default or search)
    if (FAILED(CoCreateInstance(CLSID_SpObjectToken, 
                                NULL,
                                CLSCTX_ALL, 
                                IID_ISpObjectToken,          
                                (void **)(&token))))
      break;
    if (FAILED(token->SetId(NULL, id, FALSE)))
      break;

    // attempt to connect to recognizer
    hr = eng->SetInput((IUnknown *) token, TRUE);
    break;
  }

  // cleanup temporary variables
  if (id != NULL)
    CoTaskMemFree(id);
  if (cat != NULL)
    cat->Release();
  if (token != NULL)
    token->Release();

  // make sure everything worked (mark as not from file)
  if (FAILED(hr))
    return 0;
  *sfile = '\0';

  // record true source (query in case default was used)
  reco_input(mic, 80);
  return 1;
}


//= Run speech recognition from a wav file.

int jhcSpRecoMS::connect_file (const char *fname)
{
  ISpRecognizer *eng = (ISpRecognizer *) e;
  ISpStream *src;
  GUID id = SPDFID_WaveFormatEx;
  WAVEFORMATEX fmt;  
  WCHAR wide[200];
  const char *end;

  // check for a valid wav file
  if ((fname == NULL) || (*fname == '\0'))
    return 0;
  if ((end = strrchr(fname, '.')) == NULL)
    return 0;
  if (_stricmp(end + 1, "wav") != 0)
    return 0;

  // create an audio stream object 
  if (FAILED(CoCreateInstance(CLSID_SpStream,
                              NULL,
                              CLSCTX_ALL, 
                              IID_ISpStream,
                              (void **)(&src))))
    return 0;
  a = (void *) src;

  // set correct format for speech (22KHz 16 bit stereo)
  fmt.wFormatTag = WAVE_FORMAT_PCM;
  fmt.nChannels = 2;                    
  fmt.nBlockAlign = 4;                  
  fmt.nSamplesPerSec = 22050;
  fmt.nAvgBytesPerSec = 88200;           
  fmt.cbSize = 0;

  // try to open audio file then bind to speech engine
  MultiByteToWideChar(CP_UTF8, 0, fname, -1, wide, 200);
  if (FAILED(src->BindToFile(wide, 
                             SPFM_OPEN_READONLY,
                             &id, 
                             &fmt, 
                             SPFEI_ALL_EVENTS)))
    return 0;
  if (FAILED(eng->SetInput(src, TRUE)))
    return 0;
  strcpy_s(sfile, fname);               // save name if successful
  return 1;
}


//= Fills string with description of audio source.

const char *jhcSpRecoMS::reco_input (char *spec, int ssz) const
{
  jhcString cvt;
  ISpRecognizer *eng = (ISpRecognizer *) e;
  HKEY h;
  ISpObjectToken *info;
  WCHAR *w;
  char key[200];
  char *subkey;
  unsigned long len = 200;
  int ans = 1;

  // see if system initialized yet and whether file input is being used
  if (spec == NULL)
    return NULL;
  *spec = '\0';
  if (r_ok <= 0) 
    return spec;
  if (*sfile != '\0')
  {
    strcpy_s(spec, ssz, sfile);
    return spec;
  }

  // find registry key for audio input device
  if (FAILED(eng->GetInputObjectToken(&info)))
    return spec;
  if (FAILED(info->GetId(&w)))
    ans = 0;
  else
  {
    WideCharToMultiByte(CP_UTF8, 0, w, -1, key, 200, NULL, NULL);
    CoTaskMemFree(w);
  }
  info->Release();
  if (ans <= 0)
    return spec;

  // get default value from registry (under CURRENT_USER not LOCAL_MACHINE)
  if ((subkey = strchr(key, '\\')) == NULL)
    return spec;
  cvt.Set(subkey + 1);
  if (FAILED(RegOpenKeyEx(HKEY_CURRENT_USER, cvt.Txt(), 0, KEY_READ, &h)))
    return spec;
  if (RegQueryValueEx(h, NULL, NULL, NULL, (LPBYTE) cvt.Txt(), &len) 
      != ERROR_SUCCESS)
    ans = 0;
  RegCloseKey(h);
  if (ans <= 0)
    return spec;

  // terminate and convert to normal string
  (cvt.Txt())[len] = _T('\0');
  cvt.Sync();
  strcpy_s(spec, ssz, cvt.ch);
  return spec;
}


//= Fills string with description of underlying speech recognition engine.

const char *jhcSpRecoMS::reco_engine (char *spec, int ssz) const
{
  jhcString cvt;
  ISpRecognizer *eng = (ISpRecognizer *) e;
  HKEY h;
  ISpObjectToken *info;
  WCHAR *w;
  char key[200];
  char *subkey;
  unsigned long len = 200;
  int ans = 1;

  // see if system initialized yet
  if ((r_ok <= 0) || (spec == NULL))
    return NULL;
  *spec = '\0'; 

  // extract name of recognizer as a registry key
  if (FAILED(eng->GetRecognizer(&info)))
    return spec;
  if (FAILED(info->GetId(&w)))
    ans = 0;
  else
  {
    WideCharToMultiByte(CP_UTF8, 0, w, -1, key, 200, NULL, NULL);
    CoTaskMemFree(w);
  }
  info->Release();
  if (ans <= 0)
    return spec;

  // get default value from registry
  if ((subkey = strchr(key, '\\')) == NULL)
    return spec;
  cvt.Set(subkey + 1);
  if (FAILED(RegOpenKeyEx(HKEY_LOCAL_MACHINE, cvt.Txt(), 0, KEY_READ, &h)))
    return spec;
  if (RegQueryValueEx(h, NULL, NULL, NULL, (LPBYTE) cvt.Txt(), &len) 
      != ERROR_SUCCESS)
    ans = 0;
  RegCloseKey(h);
  if (ans <= 0)
    return spec;

  // terminate and convert to normal string
  (cvt.Txt())[len] = _T('\0');
  cvt.Sync();
  strcpy_s(spec, ssz, cvt.ch);
  return spec;
}


//= Start processing speech from the pre-designated audio source.
// takes a debugging level specification and log file designation
// use this to initially fire up the system (REQUIRED)
// returns 1 if successful, 0 or negative for some error

int jhcSpRecoMS::reco_start (int level, const char *log_file)
{
  noisy = level;
  *partial = '\0';
  m = NULL;
  reco_listen(1); 
  return r_ok;
}


//= Stop recognizing speech and clean up all objects and files.
// only call this at the end of a processing session

void jhcSpRecoMS::reco_cleanup ()
{
  ISpRecognizer *eng = (ISpRecognizer *) e;	  
  ISpRecoContext *ctx = (ISpRecoContext *) c;  
  ISpStream *src = (ISpStream *) a;       
  ISpRecoGrammar *gram = (ISpRecoGrammar *) g; 
  SPPHRASE *detail = (SPPHRASE *) d;     

  // make sure recognition has stopped 
  reco_listen(0);

  // PARSE: get rid of any parse tree
  if (detail != NULL)
    CoTaskMemFree(detail);
  d = NULL;

  // deallocate grammar and context
  if (gram != NULL)
  {
    gram->Release();
    g = NULL;
  }
  if (ctx != NULL)
  {
    ctx->SetNotifySink(NULL);
    ctx->Release();
    c = NULL;
  }

  // mark parser as dead
  p_ok = -1;

  // RECO: release file input (if any) and reco engine 
  if (src != NULL)
  {
    src->Close();
    src->Release();
    a = NULL;
  }
  if (eng != NULL)
  {
    eng->Release();
    e = NULL;
  }

  // mark recognizer as dead
  r_ok = -1;
}


///////////////////////////////////////////////////////////////////////////
//                        Run-Time Modifications                         //
///////////////////////////////////////////////////////////////////////////

//= Take audio from the named live source or file.
// returns 1 if successful, 0 if failed
// NOTE: can block for ~360ms

int jhcSpRecoMS::reco_set_src (const char *name, int file)
{
  ISpRecognizer *eng = (ISpRecognizer *) e;	  
  ISpRecoContext *ctx = (ISpRecoContext *) c; 
  SPCONTEXTSTATE state; 
  SPRECOSTATE estate;
  int ans;

  // see if system initialized yet
  if (r_ok <= 0)
    return -1;

  // make sure engine is halted 
  ctx->GetContextState(&state);
  eng->GetRecoState(&estate);
  eng_stop(this);

  // attempt to bind new input
  if (file > 0)
    ans = connect_file(name);
  else if ((ans = connect_mic(name)) > 0)
    reco_input(mic, 80);

  // restart engine if needed
  if (estate == SPRST_ACTIVE)
    eng_start(this);
  ctx->SetContextState(state);
  return ans;
}


//= Provide a hint about user position to assist acoustic adaptation.
// angles are in degrees, distance is in inches relative to microphone

void jhcSpRecoMS::reco_loc_user (int azim, int elev, int dist)
{
  // ignored
}


//= Reconfigure the engine for a new primary user (acoustic model).
// this can be done without explicitly pausing the system
// if only one acoustic model is allowed, this overwrites previous
// can also be used for visual hints about the correct speaker ID
// force: 0 = await pause, 1 = pause in background, 2 = block (~360ms)
// generally need to call reco_listen(1) to restart recognition after this
// returns 1 if successful, 0 or negative for some error

int jhcSpRecoMS::reco_add_user (const char *name, int force)
{
  ISpObjectTokenCategory *cat = NULL;
  IEnumSpObjectTokens *list = NULL; 
  ISpObjectToken *prof = NULL; 
  char text[200];
  WCHAR wide[200];
  WCHAR *w = wide;
  int n, match = 0;

  // check for errors
  if (r_ok <= 0)
    return -1;
  if (*name == '\0')
    return 0;

  // get a token enumerator for available speech recognition profiles
  CoCreateInstance(CLSID_SpObjectTokenCategory,   
                   NULL, 
                   CLSCTX_ALL, 
                   IID_ISpObjectTokenCategory,   
                   (void **)(&cat)); 
  cat->SetId(SPCAT_RECOPROFILES, FALSE);
  cat->EnumTokens(NULL, NULL, &list);
  cat->Release();

  // check each profile in the enumeration for a name match
  n = (int) strlen(name);
  while (list->Next(1, &prof, NULL) == S_OK)
  {
    prof->GetStringValue(NULL, &w);
    WideCharToMultiByte(CP_UTF8, 0, w, -1, text, 200, NULL, NULL);
    if (strncmp(text, name, n) == 0)
    {
      match = 1;
      break; 
    }
  }  
  if (match <= 0)
    return 0;      

  // do changeover (possibly in background thread)
  strcpy_s(select, text);
  m = (void *) prof;
  if (force > 0)
  {
    if (force <= 1)
      _beginthread(eng_stop, 0, this);     
    else
      eng_stop(this);
  }
  return 1;
}


//= Disable the acoustic model for a particular user.
// sometimes useful if a certain speaker is known to have left

void jhcSpRecoMS::reco_del_user (const char *name)
{
  // only one user (required)
}


//= Disable all current acoustic models (users).
// this may also load up some background acoustic model instead

void jhcSpRecoMS::reco_clr_users ()
{
  // must have one user
}


//= Give the ID strings associated with the current users (acoustic models).
// elements are separated with newline characters
// returns the count of elements in the string (separator = new line)

int jhcSpRecoMS::reco_list_users (char *list, int ssz) const
{
  ISpRecognizer *eng = (ISpRecognizer *) e;	  
  WCHAR wide[80];
  WCHAR *w = wide;
  ISpObjectToken *info = NULL;
  HRESULT hr;

  // see if system initialized yet
  if ((r_ok <= 0) || (list == NULL))
    return -1;
  *list = '\0';  

  // find registry key value for current user voice profile
  if (FAILED(eng->GetRecoProfile(&info)))
    return -1;
  hr = info->GetStringValue(NULL, &w);
  info->Release();
  if (FAILED(hr))
    return -1;
  WideCharToMultiByte(CP_UTF8, 0, w, -1, list, ssz, NULL, NULL);
  return 1;
}


//= Enable a certain (or additional) language model (grammar / vocabulary).
// this can be done without explicitly pausing the system
// if only one langauge model is allowed, this overwrites previous
// returns 1 if successful, 0 or negative for some error

int jhcSpRecoMS::reco_add_model (const char *topic)
{
  // no language models, handled through grammar interface
  return 1;
}


//= Disable a certain language model (grammar / vocabulary).

void jhcSpRecoMS::reco_del_model (const char *topic)
{
  // no language models, handled through grammar interface
}


//= Disable all current language models (grammars / vocabularies).
// this may also load up some background language model instead

void jhcSpRecoMS::reco_clr_models ()
{
  // no language models, handled through grammar interface
}


//= Generate a list of all currently enabled language models. 
// elements are separated with newline characters
// returns the count of elements in the string (separator = new line)

int jhcSpRecoMS::reco_list_models (char *list, int ssz) const
{
  *list = '\0';  // no language models, only grammars
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                        Recognition Results                            //
///////////////////////////////////////////////////////////////////////////

//= Temporarily stop or resume processing speech (e.g. to free up CPU).
// also useful for half-duplex mode to ignore text-to-speech utterances
// the reco_start function automatically calls reco_listen(1) 
// NOTE: block <= 0 does not block, but change happens 40-300ms after call

void jhcSpRecoMS::reco_listen (int doit, int block)
{
  if (r_ok <= 0)
    return;
  if ((doit > 0) && (attn <= 0))
  {
    if (block <= 0)
      _beginthread(eng_start, 0, this);                  // do not wait 
    else
      eng_start(this);                                   // takes 40-60ms (Win7 needs!)
    attn = 1;
  }
  else if ((doit <= 0) && (attn > 0))
  {
    if (block <= 0)
      _beginthread(eng_stop, 0, this);                   // do not wait 
    else
      eng_stop(this);                                    // takes 300ms (Win7 needs!)
    attn = 0;
  }
}


//= Restart recognition engine after adjustments.
// call with "this" to use as blocking function for 40-60ms

void jhcSpRecoMS::eng_start (void *me)
{
  jhcSpRecoMS *self = (jhcSpRecoMS *) me;
  HANDLE frob = (HANDLE)(self->f);
  ISpRecognizer *eng = (ISpRecognizer *)(self->e);	  
  ISpRecoContext *ctx = (ISpRecoContext *)(self->c);  
  SPRECOSTATE state;

  // make sure any other engine operation has completed
  if (WaitForSingleObject(frob, INFINITE) != WAIT_OBJECT_0)
    return;

  // start engine if not running
  eng->GetRecoState(&state);
  if (state != SPRST_ACTIVE)
    eng->SetRecoState(SPRST_ACTIVE);
  ctx->SetContextState(SPCS_ENABLED);

  // allow future stop
  ReleaseMutex(frob);
}


//= Stop recognition engine so adjustments can be made.
// call with "this" to use as blocking function for ~300ms
// will automatically switch acoustic model if change pending

void jhcSpRecoMS::eng_stop (void *me)
{
  jhcSpRecoMS *self = (jhcSpRecoMS *) me;
  HANDLE frob = (HANDLE)(self->f);
  ISpRecognizer *eng = (ISpRecognizer *)(self->e);	  
  ISpRecoContext *ctx = (ISpRecoContext *)(self->c);  
  ISpObjectToken *prof, *oldp = NULL; 
  SPRECOSTATE state;

  // make sure any other engine operation has completed
  if (WaitForSingleObject(frob, INFINITE) != WAIT_OBJECT_0)
    return;
  prof = (ISpObjectToken *)(self->m);     // bind after grabbing token

  // stop engine if running
  ctx->SetContextState(SPCS_DISABLED);
  eng->GetRecoState(&state);
  if (state == SPRST_ACTIVE)
    eng->SetRecoState(SPRST_INACTIVE_WITH_PURGE);

  // bind new user speech model (if any)
  if (prof != NULL)
  {
    eng->GetRecoProfile(&oldp);              // required
    eng->SetRecoProfile(prof);
    self->m = NULL;
  }

  // allow future start
  ReleaseMutex(frob);
}


//= Check to see if any utterances are ready for harvesting.
// returns 0 for silence, 1 for speech detected, 2 for ready, negative for some error

int jhcSpRecoMS::reco_status () 
{
  ISpRecoContext *ctx = (ISpRecoContext *) c; 
  SPPHRASE *detail = (SPPHRASE *) d;     
  char ptable[50][5] = {  "",  "-",  "!",  "&",  ",",  ".",  "?",  "_",  "1",  "2", 
                        "aa", "ae", "ah", "ao", "aw", "ax", "ay",  "b", "ch",  "d",
                        "dh", "eh", "er", "ey",  "f",  "g",  "h", "ih", "iy", "jh",
                         "k",  "l",  "m",  "n", "ng", "ow", "oy",  "p",  "r",  "s",
                        "sh",  "t", "th", "uh", "uw",  "v",  "w",  "y",  "z", "zh"};
  SPEVENT evt;
  ISpPhrase *phrase = NULL; 
  SPPHRASE *hyp = NULL;  
  const SPPHRASEELEMENT *parts;
  const SPPHONEID *pron; 
  char txt[80];
  int i, j, n;

  // make sure grammar is loaded then start potential recognition
  if (r_ok <= 0)
    return -2;
  reco_listen(1);

  // handle intermediate messages while checking for full recognition
  while (1)
  {
    // get the next speech message (if any)
    if (ctx->GetEvents(1, &evt, NULL) != S_OK)   // needs S_OK not FAILED
      return ready;

    // successful path through grammar (handled below)
    if (evt.eEventId == SPEI_RECOGNITION)        
    {
      if (noisy > 0)
        jprintf("%s>>> full recognition\n", tag);
      break;
    }

    // possible speech heard
    if (evt.eEventId == SPEI_PHRASE_START)       
    {
      if (noisy > 0)
        jprintf("\n%s+++ speech start\n", tag);
      ready = 1;
    }

    // problem with processing chain
    if (evt.eEventId == SPEI_INTERFERENCE)       
    {
      if (evt.lParam == 7)                       // SPINTERFERENCE_LATENCY_WARNING
        if (noisy > 0)
          jprintf("\n%s~~~ audio lagging\n", tag);
    }   

    // silence or grammar timeout
    if (evt.eEventId == SPEI_FALSE_RECOGNITION)  
    {
      if (noisy > 0)
        jprintf("%s--- speech end\n", tag);
      *partial = '\0';
      ready = 0;
    }

    // audio file ends (or input stopped temporarily)
    if (evt.eEventId == SPEI_END_SR_STREAM) 
    {
//      if (noisy > 0)
//        jprintf("%s--- stream end\n", tag);
      if (a != NULL)
      {
        // only valid for files, not muting the microphone
        r_ok = 0;
        return ready;
      }
    }

    // examine partial recognitions 
    if (evt.eEventId == SPEI_HYPOTHESIS)
    {
      // get information about path through parse tree
      phrase = (ISpPhrase *)(evt.lParam);
      if (hyp != NULL)
        CoTaskMemFree(hyp);
      hyp = NULL;
      if (FAILED(phrase->GetPhrase(&hyp)))
        continue;

      // get text for partial result 
      *partial = '\0';
      n = (int)((hyp->Rule).ulCountOfElements);
      parts = hyp->pElements;
      for (i = 0; i < n; i++)
      {
        // add lexicalized version of word to sentence (e.g. "double-dash" not "- -")
        WideCharToMultiByte(CP_UTF8, 0, parts[i].pszLexicalForm, -1, txt, 80, NULL, NULL);
        strcat_s(partial, txt);
        strcat_s(partial, " ");
      }

      // print what was heard so far
      if (noisy > 0)
        jprintf("  %sheard: %s...\n", tag, partial);
    }
  }

  // save first reco event and parse tree
  *partial = '\0';
  phrase = (ISpPhrase *)(evt.lParam);
  if (detail != NULL)
    CoTaskMemFree(detail);
  d = NULL;
  focus = 0;
  stack[focus] = NULL;
  if (FAILED(phrase->GetPhrase(&detail)))
    return -1;
  d = (void *) detail;   // was missing prior to 7/14

  // get text and phonetic result
  *result = '\0';
  *phonetic = '\0';
  n = (int)((detail->Rule).ulCountOfElements);
  parts = detail->pElements;
  for (i = 0; i < n; i++)
  {
    // add lexicalized version of word to sentence (e.g. "double-dash" not "- -")
    WideCharToMultiByte(CP_UTF8, 0, parts[i].pszLexicalForm, -1, txt, 80, NULL, NULL);
    strcat_s(result, txt);
    strcat_s(result, " ");

    // add word separator then look at string of phonemes
    if (i > 0)
      strcat_s(phonetic, "\n");
    pron = parts[i].pszPronunciation;
    while (*pron != 0)
    {
      j = *pron++;
      if ((j >= 0) && (j < 50))
      {
        // convert from integer to textual form
        strcat_s(phonetic, ptable[j]);
        strcat_s(phonetic, " ");
      }
    }
  }

  // strip trailing spaces on output strings
  n = (int) strlen(result);
  if (n >= 1)
    result[n - 1] = '\0';
  n = (int) strlen(phonetic);
  if (n >= 1)
    phonetic[n - 1] = '\0';

  // signal that text is ready
  parse_top();                      // needed to set focus
  ready = 2;
  return ready;
}


//= Get the best guess as to what has been heard so far.
// returns 1 if user is speaking, 0 if no guess, negative for error

int jhcSpRecoMS::reco_partial (char *text, int ssz)
{
  if ((text == NULL) || (ssz <= 0))
    return -2;
  *text = '\0';
  if (r_ok <= 0)
    return -1;
  if (ready <= 0)
    return 0;
  strcpy_s(text, ssz, partial);
  return 1;
}


//= Get the nth alternative for the last recognition result as a text string.
// assumes reco_status called immediately before this and returns 2
// also generates a string with the confidences (0-100) for each word
// and gives the confidence in this whole alternative (0-100)
// zero confidence generally means you have gotten to the end of the list
// returns confidence, 0 if nothing ready, -1 or less for some error

int jhcSpRecoMS::reco_heard (char *text, char *conf, int choice, int tsz, int csz) 
{
  SPPHRASERULE *item;
  int i, n, cf;

  // clear results and check current status
  *text = '\0';
  if (conf != NULL)
    *conf = '\0';
  match = 0;
  if (r_ok <= 0)
    return -1;
  if ((ready < 2) || (choice != 0))
    return 0;

  // copy out saved surface string
  strcpy_s(text, tsz, result);
  ready = 0;

  // get word confidences (set default of 30 for dictation)
  if (conf != NULL)
  {
    item = (SPPHRASERULE *) stack[0];
    n = item->ulCountOfElements;
    for (i = 0; i < n; i++)
      strcat_s(conf, csz, "30 ");
    n = (int) strlen(conf);
    if (n > 1)
      conf[n - 1] = '\0';
    match = walk_tree(conf, stack[0]);
  }

  // mark overall confidence
  item = (SPPHRASERULE *) stack[focus];
  cf = item->Confidence;
  if (cf == SP_LOW_CONFIDENCE)
    return 50;
  if (cf == SP_NORMAL_CONFIDENCE)
    return 80;
  return 99;
}


//= Get the phoneme string for nth alternative of the last recognition result.
// phonemes are one or two characters separated by spaces with "&" between words

void jhcSpRecoMS::reco_phonetic (char *pseq, int choice, int ssz) const
{
  if (choice != 0)
    *pseq = '\0';
  else
    strcpy_s(pseq, ssz, phonetic);
}


//= Do depth first search of tree to pick up word confidences.
// returns number of words found

int jhcSpRecoMS::walk_tree (char *ctxt, const void *n)
{
  SPPHRASERULE *node = (SPPHRASERULE *) n;
  char *entry;
  int i, start, end, cf, nw;

  // check for end of some expansion (no sibling)
  if (node == NULL)
    return 0;
  
  // descend to a terminal node
  if (node->pFirstChild != NULL)
    nw = walk_tree(ctxt, node->pFirstChild);
  else
  {
    // find number of surface words and shared confidence
    cf = node->Confidence;
    start = node->ulFirstElement;
    nw = node->ulCountOfElements;
    end = start + nw;

    // convert default confidence values for particular words 
    entry = ctxt + 3 * start;
    for (i = start; i < end; i++, entry += 3)
      if (cf == SP_LOW_CONFIDENCE)               // default 30 -> 50
        entry[0] = '5';
      else if (cf == SP_NORMAL_CONFIDENCE)       // default 30 -> 80
        entry[0] = '8';
      else                                       // default 30 -> 99
      {
        entry[0] = '9';
        entry[1] = '9';
      }
  }

  // decode next thing in sentence order
  nw += walk_tree(ctxt, node->pNextSibling);
  return nw;
}


//= Give ID string associated with the most likely speaker of the last utterance.
// if unknown then function returns 0 and clears string

int jhcSpRecoMS::reco_speaker (char *name, int ssz) const
{
  return reco_list_users(name, ssz);
}


///////////////////////////////////////////////////////////////////////////
//                        Parsing Configuration                          //
///////////////////////////////////////////////////////////////////////////

//= Fills string with version number of DLL and possibly other information.

const char *jhcSpRecoMS::parse_version (char *spec, int ssz) const
{
  if (spec != NULL)
    strcpy_s(spec, ssz, "1.80 Microsoft");
  return spec;
}


//= Loads all common grammar and parsing parameters based on the file given.
// the single configuration file can point to other files as needed (REQUIRED)
// returns 1 if successful, 0 or negative for some error

int jhcSpRecoMS::parse_setup (const char *cfg_file)
{
  *gfile = '\0';
  return 1;  
}


//= Start accepting utterances to parse according to some grammar(s).
// takes a debugging level specification and log file designation
// use this to initially fire up the system (REQUIRED)
// returns 1 if successful, 0 or negative for some error

int jhcSpRecoMS::parse_start (int level, const char *log_file)
{
  return 1;
}


//= Stop accepting utterances and clean up all objects and files.
// only call this at the end of a processing session (REQUIRED)

void jhcSpRecoMS::parse_cleanup ()
{
  // nothing needs to be done
}


///////////////////////////////////////////////////////////////////////////
//                   Run-Time Parsing Modifications                      //
///////////////////////////////////////////////////////////////////////////

//= Load a certain (or additional) grammar from a file.
// this can be done without explicitly pausing the system
// if only one grammar is allowed, this overwrites previous
// initially all rules are disabled (call parse_enable)
// returns 2 if appended, 1 if exclusive, 0 or negative for some error

int jhcSpRecoMS::parse_load (const char *grammar)
{
  ISpRecognizer *eng = (ISpRecognizer *) e;
  ISpRecoGrammar *gram = (ISpRecoGrammar *) g;   
  SPPHRASE *detail = (SPPHRASE *) d;   
  SPRECOSTATE estate;
  WCHAR wide[200];
  const char *ext;
  int ans = 0;

  // make sure system is initialized
  if (p_ok <= 0)
    return -1;

  // stop audio and erase last result (if any) 
//  ctx->Pause(0);
  eng->GetRecoState(&estate);
  eng_stop(this);                        // (Win7 needs?)
  if (detail != NULL)
    CoTaskMemFree(detail);
  d = NULL;
  focus = 0;
  stack[focus] = NULL;

  // see if generic format grammar file specified
  ext = strrchr(grammar, '.');
  if ((ext != NULL) && 
      ((_stricmp(ext + 1, "sgm") == 0) || (_stricmp(ext + 1, "txt") == 0)))
  {
    // save name then process JHC format file(s)
    strcpy_s(gfile, grammar);
    ans = load_jhc(grammar, 0);
  }
  else if ((ext != NULL) && 
           ((_stricmp(ext + 1, "cfg") == 0) || (_stricmp(ext + 1, "bnf") == 0)))
  {
    // save name then process BNF format file(s)
    strcpy_s(gfile, grammar);
    ans = load_bnf(grammar, 0);
  }
  else
  {
    // load native Microsoft XML (or binary) format grammar 
    MultiByteToWideChar(CP_UTF8, 0, grammar, -1, wide, 200);
    if (SUCCEEDED(gram->LoadCmdFromFile(wide, SPLO_DYNAMIC)))
      ans = 1;
  }

  // save grammar then disable (restarts audio) 
  gram->Commit(0);
  parse_disable(NULL);
  if (estate == SPRST_ACTIVE)
    eng_start(this);                     // (Win7 needs?)
//  ctx->Resume(0);
  return ans;
}


//= Remove all grammars that may have been loaded.

void jhcSpRecoMS::parse_clear ()
{
  ISpRecoGrammar *gram = (ISpRecoGrammar *) g; 

  gram->ResetGrammar(MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT));
}


//= Enable some top-level (i.e. sentence) rule within the grammar.
// returns 1 if successful, 0 if not found, negative for some error

int jhcSpRecoMS::parse_enable (const char *rule)
{
  ISpRecognizer *eng = (ISpRecognizer *) e;	  
  ISpRecoGrammar *gram = (ISpRecoGrammar *) g;   
  SPRECOSTATE estate;
  WCHAR wide[200];
  int ans = 1;

  // make sure system is initialized
  if (p_ok <= 0)
    return 0;

  // temporarily suspend parsing and look for named rule
//  ctx->Pause(0);
  eng->GetRecoState(&estate);
  eng_stop(this);                        // (Win7 needs?)
  if (rule == NULL)
    gram->SetRuleState(NULL, NULL, SPRS_ACTIVE);
  else
  {
    MultiByteToWideChar(CP_UTF8, 0, rule, -1, wide, 200);
    if (FAILED(gram->SetRuleState(wide, NULL, SPRS_ACTIVE)))
      ans = 0;
  }
  if (estate == SPRST_ACTIVE)
    eng_start(this);                     // (Win7 needs?)
//  ctx->Resume(0);
  return ans;
}


//= Disable some top-level (i.e. sentence) rule within the grammar.
// a NULL rule name serves to disable ALL top level rules
// returns 1 if successful, 0 if not found, negative for some error

int jhcSpRecoMS::parse_disable (const char *rule)
{
  ISpRecognizer *eng = (ISpRecognizer *) e;	  
  ISpRecoGrammar *gram = (ISpRecoGrammar *) g;   
  SPRECOSTATE estate;
  WCHAR wide[200];
  int ans = 1;

  // make sure system is initialized
  if (p_ok <= 0)
    return 0;

  // temporarily suspend parsing and look for named rule
//  ctx->Pause(0);                                   
  eng->GetRecoState(&estate);
  eng_stop(this);                        // (Win7 needs?)
  if (rule == NULL)
    gram->SetRuleState(NULL, NULL, SPRS_INACTIVE);
  else
  {
    MultiByteToWideChar(CP_UTF8, 0, rule, -1, wide, 200);
    if (FAILED(gram->SetRuleState(wide, NULL, SPRS_INACTIVE)))
      ans = 0;
  }
  if (estate == SPRST_ACTIVE)
    eng_start(this);                     // (Win7 needs?)
//  ctx->Resume(0);
  return ans;
}


//= Add a new expansion to some existing rule in the grammar.
// alters internal graph and optionally changes original grammar file also
// returns 2 if ok, 1 if only run-time changed, 0 or negative for error

int jhcSpRecoMS::parse_extend (const char *rule, const char *option, int file)
{
  ISpRecoContext *ctx = (ISpRecoContext *) c;  
  ISpRecoGrammar *gram = (ISpRecoGrammar *) g;    
  SPSTATEHANDLE top;
  WCHAR wide[200];
  char parts[200];

  // ignore anything with a straight numeric wildcard
  if (strchr(option, '@') != NULL)
    return 0;

  // look for named rule (used to require pre-existing)
  ctx->Pause(0);
//  eng->SetRecoState(SPRST_INACTIVE_WITH_PURGE);            // (Win7 needs?)
  MultiByteToWideChar(CP_UTF8, 0, rule, -1, wide, 200);
  if (gram->GetRule(wide, 0, SPRAF_TopLevel | SPRAF_Dynamic, TRUE, &top) != S_OK)                 
  {
//    eng->SetRecoState(SPRST_ACTIVE);                       // (Win7 needs?)
    ctx->Resume(0);
    return 0;
  }

  // do run-time modification then restart system
  strcpy_s(parts, option);
  build_phrase(parts, &top, 0);    
  gram->Commit(0);
//  eng->SetRecoState(SPRST_ACTIVE);                         // (Win7 needs?)
  ctx->Resume(0);

  // try editing original grammar file (optional)
  if (file > 0)
    if (add_option(gfile, rule, option) > 0)
      return 2;
  return 1;
}


//= Modify grammar file so that given rule has one additional expansion.
// returns 1 if successful, zero or negative for failure

int jhcSpRecoMS::add_option (const char *fname, const char *rule, const char *option)
{
  FILE *in, *out;
  char line[500], tname[80] = "jhc_temp.txt";
  char *start, *end;
  int i, n = 0, found = 0;

  // try opening file 
  if (fopen_s(&in, fname, "r") != 0)
    return -1;

  // go through grammar file line by line
  while ((start = clean_line(line, 500, in, ';')) != NULL)
  {
    n++;
    if (strncmp(start, "#include", 8) == 0)
    {
      // try modifying another file first
      if ((start = strchr(start, '\"')) != NULL)
        if ((end = strchr(start + 1, '\"')) != NULL)
        {
          *end = '\0';
          if (add_option(start + 1, rule, option) > 0)
          {
            // new entry added to included file instead
            fclose(in);
            return 1;
          }
        }
    }
    else if (*start == '=')
    {
      // look for label at beginning of new rule paragraph
      if ((start = strpbrk(start + 1, "[<")) != NULL)
        if ((end = strpbrk(start + 1, "]>")) != NULL)
        {
          *end = '\0';
          start = trim_wh(start + 1);
          if (strcmp(start, rule) == 0)
          {
            // rule name matches this definition line
            found = 1;
            break;
          }
        }
    }
  }

  // try to open a temporary file and copy first n lines to it 
  if (fopen_s(&out, tname, "w") != 0)
  {
    fclose(in);
    return -1;
  }
  fseek(in, 0, SEEK_SET);
  for (i = 1; i <= n; i++)
  {
    fgets(line, 200, in);
    fputs(line, out);
  }

  // possibly add brand new rule definition at end
  if (found <= 0)
    fprintf(out, "\n=[%s]\n", rule);

  // write new expansion then copy rest of lines
  fprintf(out, "  %s\n", option);
  while (fgets(line, 200, in) != NULL)
    fputs(line, out);
  fclose(out);
  fclose(in);

  // copy altered temp file back to main grammar file
  if (fopen_s(&in, tname, "r") != 0)
    return 0;
  if (fopen_s(&out, fname, "w") != 0)
  {
    fclose(in);
    return 0;
  }
  while (fgets(line, 200, in) != NULL)
    fputs(line, out);
  fclose(out);
  fclose(in);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Parsing Results                              //
///////////////////////////////////////////////////////////////////////////

//= Accept an utterance for parsing by currently active grammar(s).
// can optionally take list of confidences (0-100) for each word
// automatically sets focus to top of parse tree (if found)
// returns number of interpretations, 0 if no valid parse, negative if error

int jhcSpRecoMS::parse_analyze (const char *text, const char *conf)
{
  if (strcmp(text, result) != 0)  // speech recognition and parsing combined
    return 0;
  return 1;
}


//= Returns the name or string associated with the current focus node.
// returns 1 if successful, 0 or negative for error

int jhcSpRecoMS::parse_focus (char *token, int ssz) const
{
  const SPPHRASERULE *item;

  // set default value
  *token = '\0';

  // check for various errors
  if (p_ok <= 0) 
    return -1;
  if (stack[focus] == NULL)
    return 0;

  // get name 
  item = (const SPPHRASERULE *) stack[focus];
  WideCharToMultiByte(CP_UTF8, 0, item->pszName, -1, token, ssz, NULL, NULL);
  return 1;
}


//= Returns the range of surface words covered by the current focus node.
// word 0 is the initial word in the utterance
// returns total number of words in utterance, 0 or negative for error

int jhcSpRecoMS::parse_span (int *first, int *last) const
{
  const SPPHRASERULE *item;
  int start;

  // set default values
  if (first != NULL)
    *first = 0;
  if (last != NULL)
    *last = 0;

  // check for various errors
  if (p_ok <= 0) 
    return -1;
  if (stack[focus] == NULL)
    return 0;

  // get specification of element
  item = (const SPPHRASERULE *) stack[focus];
  start = item->ulFirstElement;
  if (first != NULL)
    *first = start;
  if (last != NULL)
    *last = start + item->ulCountOfElements - 1;

  // find total
  item = (const SPPHRASERULE *) stack[0];
  return item->ulCountOfElements;
}


//= Reset the focus to the top most node of the parse tree.
// can select particular interpretation if more than one
// returns 1 if successful, 0 or negative for error

int jhcSpRecoMS::parse_top (int n)
{
  SPPHRASE *detail = (SPPHRASE *) d;   

  // set default values
  focus = 0;
  stack[focus] = NULL;

  // check for various errors
  if (p_ok <= 0)
    return -1;
  
  // set pointer to top of parse tree (if any)
  if (detail == NULL)
    return 0;
  stack[focus] = (const void *) &(detail->Rule);
  return 1;
}


//= Move focus to next non-terminal to the right in the current expansion.
// returns 1 if successful, 0 if focus unchanged, negative for error

int jhcSpRecoMS::parse_next ()
{
  const SPPHRASERULE *item, *next;

  // check for various errors
  if ((p_ok <= 0) || (stack[focus] == NULL))
    return -1;

  // see if move is valid
  item = (const SPPHRASERULE *) stack[focus];
  if ((next = item->pNextSibling) == NULL)
    return 0;
  stack[focus] = (void *) next;
  return 1;
}


//= Move focus down one level (i.e. expand a non-terminal node).
// automatically sets focus to leftmost branch of parse tree
// returns 1 if successful, 0 if focus unchanged, negative for error

int jhcSpRecoMS::parse_down ()
{
  const SPPHRASERULE *item, *next;

  // check for various errors
  if ((p_ok <= 0) || (stack[focus] == NULL) || (focus >= 49))
    return -1;

  // see if move is valid
  item = (const SPPHRASERULE *) stack[focus];
  if ((next = item->pFirstChild) == NULL)
    return 0;
  focus++;
  stack[focus] = (void *) next;
  return 1;
}


//= Move focus up one level (i.e. restore it to location before call to down). 
// returns 1 if successful, 0 if focus unchanged, negative for error

int jhcSpRecoMS::parse_up ()
{
  // check for various errors
  if (p_ok <= 0) 
    return -1;

  // move pointer if possible
  if (focus <= 0)
    return 0;
  focus--;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                        JHC Grammar Construction                       //
///////////////////////////////////////////////////////////////////////////

//= This function reads and load files with the standard JHC format.
// <pre>
// Standard JHC command and control grammar format:
//
//   ; first comment line
//   =[rule0]
//   word1 word2 (opt_word2) word3 <RULE1> word4 <rule2>
//   (word1) <rule2> *
//
//   // another comment
//   =[RULE1]
//   word1 ?
//   word2 word3, word4
//
// Left hand sides are preceeded by "=" and are in square or angle brackets. 
// Succeeding lines are possible disjunctive right hand side expansions. 
// 
// Terminals are unquoted words or numbers and can be broken into separate 
// parts using commas (to tolerate pauses better). 
//
// Nonterminals are enclosed in square or angle brackets. Non-terminals can 
// be declared "important" by putting their names all in caps. 
//
// Optional terminals and non-terminal elements are enclosed in parenthesis. 
//
// A dictation request is signalled with special characters. 
//   # = exactly 1 word 
//   ? = 0 or 1 word (same as "(#)")
//   + = at least 1 word but more allowed
//   * = 0 or more words (same as "(+)")
// These should not be used anywhere else (in terminals or non-terminals).
//
// Comments can be added either with "//" or with ";" to disregard the 
// remainder of the line. 
//
// Other grammar files can be embedded using #include "alt_gram.sgm" lines.
// Rules can span multiple files (i.e. each disjunctive expansion independent).
//
// </pre>

int jhcSpRecoMS::load_jhc (const char *fname, int flush)
{
  FILE *in;
  SPSTATEHANDLE top;
  char dir[200], extra[200], text[500], rpat[80];
  char *start, *end;
  int rule = 0;

  // try opening file 
  if (fopen_s(&in, fname, "r") != 0)
    return 0;

  // possibly clear out old grammar
  if (flush > 0)
    parse_clear();

  // save this file's directory (included files are relative)
  strcpy_s(dir, fname);
  if ((end = strrchr(dir, '/')) != NULL)
    *(end + 1) = '\0';
  else
    *dir = '\0';

  // go through file line by line
  while ((start = clean_line(text, 500, in, ';')) != NULL)
    if (strncmp(start, "#include", 8) == 0)
    {
      // load another file first
      if ((start = strchr(start, '\"')) != NULL)
        if ((end = strchr(start + 1, '\"')) != NULL)
        {
          *end = '\0';
          sprintf_s(extra, "%s%s", dir, start + 1);
          load_jhc(extra, 0);
          rule = 0;
        }
    }
    else if (*start == '=')
    {
      // look for label in beginning of new rule paragraph
      if ((start = strpbrk(start + 1, "[<")) != NULL)
        if ((end = strpbrk(start + 1, "]>")) != NULL)
        {
          if (_strnicmp(start + 1, "xxx", 3) == 0)
          {
            // ignore sections starting with XXX
            rule = 0;
            continue;
          }
          *end = '\0';
          start = trim_wh(start + 1);
          nonterm_chk(start, fname);
          add_rule(&top, start);
          sprintf_s(rpat, "<%s>", start);
          rule = 1;
        }
    }
    else if (rule > 0)
    {
      // cannot make directly recursive expansions
      if (strstr(start, rpat) != NULL)
        jprintf(">>> Direct self reference in %s from %s !\n", rpat, fname);
      if (strchr(start, '@') == NULL)
        build_phrase(start, &top, 0);
    }

  // close file
  fclose(in);
  return((flush > 0) ? 1 : 2);
}


//= Assemble one path of a rule or an optional conjunct.
// if optional, replaces top node with last node of current chain 

char *jhcSpRecoMS::build_phrase (char *line, void *t, int opt)
{
  SPSTATEHANDLE *top = (SPSTATEHANDLE *) t;
  SPSTATEHANDLE node;
  char *rest, *end;
  char first;
  int dict_num = 5, phrase = 0;

  // parse terms in disjunct (if any)
  node = *top;
  first = *line;
  rest = line + 1;
  while (first != '\0')
  {
    // end of optional elements
    if ((opt > 0) && (first == ')'))             
      break;

    // check for whitespace 
    if (strchr(" \t,", first) != NULL)     
    {
      first = *rest++;
      continue;
    }
    phrase = 1;

    // check for special characters
    if (strchr("(#?+*", first) != NULL)
    {
      if (first == '(')
        rest = build_phrase(rest, &node, 1);  // new optional part
      else if (first == '#')
        add_dict(&node, 1, 0);                // single word dictation
      else if (first == '?')
        add_dict(&node, 1, 1);                // optional single word 
      else if (first == '+')
        add_dict(&node, dict_num, 0);         // multi-word dictation
      else if (first == '*')
        add_dict(&node, dict_num, 1);         // optional multi-word
      first = *rest++;
      continue;
    }

    // start of non-terminal
    if (strchr("<[", first) != NULL)
    {
      if ((end = strpbrk(rest, ">]")) == NULL)
        break;
      *end = '\0';
      add_nonterm(&node, trim_wh(rest));
      first = *(end + 1);
      rest = end + 2;
      continue;
    }

    // look for end of terminal (allow embedded spaces)
    if ((end = strpbrk(rest, ",)(<[?*+#")) != NULL)      
    {
      first = *end;
      *end = '\0';
      add_term(&node, trim_wh(rest - 1));
      rest = end + 1;
      continue;
    }
      
    // series of terminals at end of line
    add_term(&node, trim_wh(rest - 1));
    rest += strlen(rest);
    break;
  }

  // add transition bypassing whole phrase or to rule end state 
  if (phrase > 0)
  {
    if (opt > 0)
      add_jump(top, &node);     
    else
      end_phrase(&node);          
  }
  return rest;
}


//= Strip off comment portion and newline character at end.
// also remove leading and trailing white space and eliminate tabs

char *jhcSpRecoMS::clean_line (char *ans, int len, FILE *in, char ignore)
{
  char *end;

  // get line from file (unless at end)
  if (fgets(ans, len, in) == NULL)
    return NULL;

  // remove final newline and anything after comment
  if ((end = strchr(ans, '\n')) != NULL)
    *end = '\0';
  if ((end = strchr(ans, ignore)) != NULL)
    *end = '\0';

  // remove double slashes
  end = ans;
  while ((end = strchr(end, '/')) != NULL)
    if (*(end + 1) == '/')
    {
      *end = '\0';
      break;
    }

  // whitespace normalization
  return trim_wh(ans);
}


//= Remove leading and trailing spaces from a string.
// also replaces internal tabs with spaces

char *jhcSpRecoMS::trim_wh (char *string)
{
  char *now, *last = NULL, *start = string;

  // trim leading white
  while (*start != '\0')
    if ((*start == ' ') || (*start == '\t'))
      start++;
    else
      break;

  // replace internal tabs
  now = start;
  while (*now != '\0')
  {
    if (*now == '\t')
      *now = ' ';
    else if (*now != ' ')
      last = now;
    now++;
  }

  // remove spaces after last real character
  if (last != NULL)
    *(last + 1) = '\0';
  return start;
}


//= Check that there are no common mistakes in the name of non-terminals.

void jhcSpRecoMS::nonterm_chk (const char *rname, const char *gram) const
{
  const char *r = rname;
  char c;
  int cap = 0, low = 0;

  // make sure parser will not get confused
  if (strpbrk(rname, "?#*+") != NULL)
  {
    jprintf(">>> Special character in =[%s] from %s !\n", rname, gram);
    return;
  }

  // count number of uppercase versus lower case characters
  while ((c = *r++) != '\0')
    if (isalpha(c) != 0)
    {
      if (isupper(c) != 0)
        cap++;
      else
        low++;
    }

  // check that a report category is all caps
  if ((low > 0) && (cap > low))
    jprintf(">>> Partial uppercase in =[%s] from %s !\n", rname, gram);
}


///////////////////////////////////////////////////////////////////////////
//                        BNF Grammar Construction                       //
///////////////////////////////////////////////////////////////////////////

//= This function reads and load files with simple BNF format.
// does not allow "include" directives or any sort of wildcards
// "#" is the comment character: rest of line will be ignored
// each valid line is one or more expansions of a non-terminal:
// <pre>
//   non1 -> "wd1" "wd2" non2 | non3 "wd3" | "wd4"
// </pre>
// non-terminals are connected words, terminals always enclosed by quotes
// embedded punctuation and numbers are okay (but no spaces or tabs)
// expansions are allowed have nulls (e.g. | at end)

int jhcSpRecoMS::load_bnf (const char *fname, int flush)
{
  FILE *in;
  const char *start, *end;
  char text[500], cat[80], tag[80];

  // try opening file 
  if (fopen_s(&in, fname, "r") != 0)
    return 0;

  // possibly clear out old grammar
  if (flush > 0)
    parse_clear();

  // go through file line by line
  while ((start = clean_line(text, 500, in, '#')) != NULL)
    if (*start != '\0')
    {
      // get part before arrow
      if ((end = strstr(start, "->")) == NULL)
        continue;
      strncpy_s(tag, start, end - start);            // always adds terminator

      // make sure it is a good non-terminal to expand
      bnf_token(cat, tag);
      if ((*cat != '\0') && (*cat != '"'))
        bnf_expansions(cat, end + 2);
    }

  // close file
  fclose(in);
  return((flush > 0) ? 1 : 2);
}


//= Creates SAPI structures for all right hand side expansions.

void jhcSpRecoMS::bnf_expansions (const char *cat, const char *tail)
{
  SPSTATEHANDLE top, node;
  const char *start = tail;
  char tag[80];

  // create a rule node for adding expansions to
  add_rule(&top, cat);
  node = top;
  while (1)
  {
    // read next RHS token (if any)
    start = bnf_token(tag, start);
    if (*tag == '\0')
       break;

    // possibly end expansion (if not empty)
    if (*tag == '|')
    {
      end_phrase(&node);
      node = top;
      continue;
    }

    // add a terminal or non-terminal to the sequence
    if (*tag == '"')
      add_term(&node, tag + 1);
    else
      add_nonterm(&node, tag);
  }

  // finish current expansion (if any)
  end_phrase(&node);
}


//= Reads next relevant BNF item and advance string pointer past it.
// trailing quote marks stripped, so tag starts with ", |, or alnum

const char *jhcSpRecoMS::bnf_token (char *tag, const char *line) const
{
  const char *next = line;
  char *dest = tag;

  // skip leading white space
  while (*next == ' ')
    next++;

  // copy characters until some delimiter found
  while (*next != '\0')
  {
    if (strchr(" |", *next) != NULL)
      break;
    if (*next != '\\')          // "I\'m" --> "I'm" 
      *dest++ = *next;
    next++;
  }

  // see if delimiter, else strip any trailing quote
  if ((dest == tag) && (*next == '|'))
    *dest++ = *next++;
  else if ((dest > tag) && (*(dest - 1) == '"'))
    dest--;
  *dest = '\0';
  return next;
} 


///////////////////////////////////////////////////////////////////////////
//                          Grammar Components                           //
///////////////////////////////////////////////////////////////////////////

//= Create a new rule entry and return handle in variable.
// always created in unmarked (i.e. not top-level) state

void jhcSpRecoMS::add_rule (void *r, const char *tag)
{
  ISpRecoGrammar *gram = (ISpRecoGrammar *) g; 
  SPSTATEHANDLE *rule = (SPSTATEHANDLE *) r;
  WCHAR wide[200];
  char text[200];

  // add rule exactly as requested
  MultiByteToWideChar(CP_UTF8, 0, tag, -1, wide, 200);
  gram->GetRule(wide, 0, 
                SPRAF_TopLevel | SPRAF_Dynamic,  // was SPRAF_Active also 
                TRUE, rule);

  // build other form of name (i.e. add capitals or remove)
  strcpy_s(text, tag);
  if (all_caps(text) > 0)
    _strlwr_s(text);
  else
    _strupr_s(text);

  // see if alternate rule exists (usually an error)
  MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, 200);
  if (gram->GetRule(wide, 0, 0, FALSE, NULL) == S_OK)
    printf(">>> Grammar has multiple forms of <%s>\n", tag); 
}


//= Test whether all alphabetic characters are uppercase.

int jhcSpRecoMS::all_caps (const char *name)
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


//= Create a new non-terminal link in clause.
// takes original node as input and returns new node in variable

void jhcSpRecoMS::add_nonterm (void *n, const char *tag)
{
  ISpRecoGrammar *gram = (ISpRecoGrammar *) g; 
  SPSTATEHANDLE *node = (SPSTATEHANDLE *) n;
  SPSTATEHANDLE rule, prev = *node;

  add_rule(&rule, tag);
  gram->CreateNewState(prev, node);
  gram->AddRuleTransition(prev, *node, rule, 1.0, NULL);
} 


//= Create a new terminal link in clause.
// takes original node as input and returns new node in variable

void jhcSpRecoMS::add_term (void *n, const char *tag)
{
  ISpRecoGrammar *gram = (ISpRecoGrammar *) g; 
  SPSTATEHANDLE *node = (SPSTATEHANDLE *) n;
  SPSTATEHANDLE prev = *node;
  WCHAR wide[200];

  gram->CreateNewState(prev, node);
  MultiByteToWideChar(CP_UTF8, 0, tag, -1, wide, 200);
  gram->AddWordTransition(prev, *node, wide, L" ", SPWT_LEXICAL, 1.0, NULL);
} 


//= Create a new dictation link in clause.
// will force recognition of 1 to cnt words (0 to cnt if opt positive)

void jhcSpRecoMS::add_dict (void *n, int cnt, int opt)
{
  ISpRecoGrammar *gram = (ISpRecoGrammar *) g; 
  SPSTATEHANDLE *node = (SPSTATEHANDLE *) n;
  SPSTATEHANDLE prev, end;
  int i;

  // build node for end of chain and possibly add skip transition
  gram->CreateNewState(*node, &end);
  if (opt > 0)
    gram->AddWordTransition(*node, end, NULL, L" ", SPWT_LEXICAL, 1.0, NULL);

  // build new nodes with dictation link from previous
  // also install a skip transition each new node to final node
  for (i = 1; i < cnt; i++)
  {
    prev = *node;
    gram->CreateNewState(prev, node);
    gram->AddRuleTransition(prev, *node, SPRULETRANS_DICTATION, (float) dict_wt, NULL);
    gram->AddWordTransition(*node, end, NULL, L" ", SPWT_LEXICAL, 1.0, NULL);
  }    

  // for final dictation link no new node or skip link needed
  // this enforces at least one dictated element (unless overall skip)
  gram->AddRuleTransition(*node, end, SPRULETRANS_DICTATION, (float) dict_wt, NULL);
  *node = end;
}


//= Create a link which is traversed by a single word (which is forgotten).
// advance starting node to end node - not currently used

void jhcSpRecoMS::add_ignore (void *n)
{
  ISpRecoGrammar *gram = (ISpRecoGrammar *) g; 
  SPSTATEHANDLE *node = (SPSTATEHANDLE *) n;
  SPSTATEHANDLE prev = *node;

  gram->CreateNewState(prev, node);
  gram->AddRuleTransition(prev, *node, SPRULETRANS_WILDCARD, 1.0, NULL);
}


//= Create a free transition between two nodes in a clause (e.g. optional).
// advance starting node to end node

void jhcSpRecoMS::add_jump (void *n0, void *n1)
{
  ISpRecoGrammar *gram = (ISpRecoGrammar *) g; 
  SPSTATEHANDLE *start = (SPSTATEHANDLE *) n0, *end = (SPSTATEHANDLE *) n1;

  gram->AddWordTransition(*start, *end, NULL, L" ", SPWT_LEXICAL, 1.0, NULL);
  *start = *end;
}


//= Create a free transition from last node to end state of rule.
// leaves node unchanged

void jhcSpRecoMS::end_phrase (void *n)
{
  ISpRecoGrammar *gram = (ISpRecoGrammar *) g; 
  SPSTATEHANDLE *node = (SPSTATEHANDLE *) n;

  gram->AddWordTransition(*node, NULL, NULL, L" ", SPWT_LEXICAL, 1.0, NULL);
}





