// jhcSpTextMS.cpp : text-to-speech functions using Microsoft SAPI
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2017 IBM Corporation
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
#include <tchar.h>
#include <atlbase.h>

#if WINVER >= 0x0600
  #include <sapi.h>
#else
  #include "Acoustic/sapi_xp.h"
#endif

#include "Interface/jhcString.h"      // common video

#include "Acoustic/jhcSpTextMS.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcSpTextMS::jhcSpTextMS ()
{
  CoInitialize(NULL);  
  *tfile = '\0';
  *vname = '\0';
  v = NULL; 
  pitch = 0;      // 10 sounds good with Microsoft Mike, -5 for Anna
  t_ok = -1;              
}


//= Default destructor does necessary cleanup.

jhcSpTextMS::~jhcSpTextMS ()
{
  tts_cleanup();
  CoUninitialize();  
}


//= Open some named DLL at run-time and try to bind speaking functions in it.
// returns 1 if completely successful, 0 or negative for failure

int jhcSpTextMS::BindTTS (const char *fname, const char *cfg, int start)
{
  // ignores any file name
  if (start <= 0)
    return 1;

  // try starting up bound subsystems
  if (tts_setup(cfg) > 0)
    if (tts_start() > 0)
      return 1;
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                      Text-to-Speech Configuration                     //
///////////////////////////////////////////////////////////////////////////

//= Fills string with version number of DLL and possibly other information.
// returns pointer to input string for convenience

const char *jhcSpTextMS::tts_version (char *spec, int ssz) const
{
  strcpy_s(spec, ssz, "1.30 Microsoft");
  return spec;
}


//= Loads all voice and output device parameters based on the file given.
// the single configuration file can point to other files as needed (REQUIRED)
// returns 1 if successful, 0 or negative for some error

int jhcSpTextMS::tts_setup (const char *cfg_file)
{
  ISpVoice *voice;

  // clear any previous setup
  tts_cleanup();

  // create a non-shared recognition engine and connect to audio input
  if (FAILED(CoCreateInstance(CLSID_SpVoice,
                              NULL, 
                              CLSCTX_ALL, 
                              IID_ISpVoice,          
                              (void **)(&voice))))
    return t_ok;
  v = (void *) voice;
  t_ok = 1;

  // try setting to requested voice
  tts_set_voice(vname);
  tts_voice(vname, 80);

  // change pitch for obnoxious Microsoft Anna (Windows 7)
//  if ((GetVersion() & 0xFF) >= 6)
//    pitch = -5;  
  return t_ok;
}


//= Fills string with description of voice being used for output.
// returns pointer to input string for convenience

const char *jhcSpTextMS::tts_voice (char *spec, int ssz) const
{
  jhcString rkey;
  ISpVoice *voice = (ISpVoice *) v;
  HKEY h;
  ISpObjectToken *info;
  WCHAR *w;
  char key[200];
  char *subkey;
  unsigned long len = 200;
  HRESULT hr;

  // see if system initialized yet
  if ((t_ok <= 0) || (spec == NULL))
    return NULL;
  *spec = '\0';

  // find registry key for voice
  if (FAILED(voice->GetVoice(&info)))
    return spec;
  hr = info->GetId(&w);
  info->Release();
  if (FAILED(hr))
    return spec;
  WideCharToMultiByte(CP_UTF8, 0, w, -1, key, 200, NULL, NULL);
  CoTaskMemFree(w);

  // get default value from registry
  if ((subkey = strchr(key, '\\')) == NULL)
    return spec;
  rkey.Set(subkey + 1);
  if (FAILED(RegOpenKeyEx(HKEY_LOCAL_MACHINE, rkey.Txt(), 0, KEY_READ, &h)))
    return spec;
  hr = RegQueryValueExA(h, NULL, NULL, NULL, (unsigned char *) key, &len);
  RegCloseKey(h);
  if (hr != ERROR_SUCCESS)
    return spec;
  key[len] = '\0';

  // combine base name with pitch
  strcpy_s(spec, ssz, key);
//  sprintf_s(spec, ssz, "%s %+d", key, pitch);    // pitch adjustment
  return spec;
}


//= Attempt to force TTS to use a certain voice.
// can also attempt to change standard volume as a percentage
// returns 1 if successful, 0 or negative for error

int jhcSpTextMS::tts_set_voice (const char *spec, int pct)
{
  jhcString rkey;
  ISpVoice *voice = (ISpVoice *) v;
  CComPtr<ISpObjectTokenCategory> cat0;
  ISpObjectTokenCategory *cat;
  IEnumSpObjectTokens *list;
  ISpObjectToken *info = NULL;
  HKEY h;
  WCHAR *w;
  char key[200];
  char *subkey;
  unsigned long i, nv, len;
  HRESULT hr;

  // see if system initialized yet
  if ((t_ok <= 0) || (spec == NULL) || (*spec == '\0'))
    return -3;

  // get number of available voices
  if (FAILED(cat0.CoCreateInstance(CLSID_SpObjectTokenCategory)))
    return -2;
  if (FAILED(cat0->SetId(SPCAT_VOICES, FALSE)))
    return -2;
  cat = cat0.Detach();
  if (FAILED(cat->EnumTokens(NULL, NULL, &list)))
    return -2;  
//  if (FAILED(SpEnumTokens(SPCAT_VOICES, NULL, NULL, &list)))
//    return -2;
  if (FAILED(list->GetCount(&nv)))
    return -2;

  for (i = 0; i < nv; i++)
  {
    // get next voice in list
    if (info != NULL)
      info->Release();
    if (FAILED(list->Next(1, &info, NULL)))
      break;

    // extract associated registry key
    if (FAILED(info->GetId(&w)))
      continue;
    WideCharToMultiByte(CP_UTF8, 0, w, -1, key, 200, NULL, NULL);
    CoTaskMemFree(w); 
    if ((subkey = strchr(key, '\\')) == NULL)
      continue;
    rkey.Set(subkey + 1);

    // get default value from registry     
    if (FAILED(RegOpenKeyEx(HKEY_LOCAL_MACHINE, rkey.Txt(), 0, KEY_READ, &h)))
      continue;
    len = 200;
    hr = RegQueryValueExA(h, NULL, NULL, NULL, (unsigned char *) key, &len); 
    RegCloseKey(h);
    if (hr != ERROR_SUCCESS)
      continue;
    key[len] = '\0';

    // compare to requested name
    if (strstr(key, spec) != NULL)
      break;
  }

  // try setting new voice
  if (i >= nv)
    return -1;
  hr = voice->SetVoice(info);
  info->Release();
  if (FAILED(hr))
    return 0;
  strcpy_s(vname, key);

  // set volume
  if ((pct <= 0) || (pct > 100)) 
    voice->SetVolume(100);
  else
    voice->SetVolume((USHORT) pct);
  return 1;
}  


//= Fills string with description of output device being used.
// returns pointer to input string for convenience

const char *jhcSpTextMS::tts_output (char *spec, int ssz) const
{
  jhcString cvt;
  ISpVoice *voice = (ISpVoice *) v;
  HKEY h;
  ISpObjectToken *info;
  WCHAR *w;
  char key[200];
  char *subkey;
  unsigned long len = 200;
  int ans = 1;

  // see if system initialized yet
  if ((t_ok <= 0) || (spec == NULL))
    return NULL;
  *spec = '\0';

  // find registry key for audio output device
  if (FAILED(voice->GetOutputObjectToken(&info)))
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


//= Start the text-to-speech system running and awaits input.
// takes a debugging level specification and log file designation
// use this to initially fire up the system (REQUIRED)
// returns 1 if successful, 0 or negative for some error

int jhcSpTextMS::tts_start (int level, const char *log_file)
{
  return t_ok;
}


//= Stop all speech output and clean up all objects and files.
// only call this at the end of a processing session (REQUIRED)

void jhcSpTextMS::tts_cleanup ()
{
  ISpVoice *voice = (ISpVoice *) v;

  // terminate current utterance (if any)
  tts_shutup();

  // release reco engine 
  if (voice != NULL)
    voice->Release();
  v = NULL;

  // mark as dead
  t_ok = -1;
}


///////////////////////////////////////////////////////////////////////////
//                          Speaking Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Speak some message after filling in fields (like printf).
// queues utterance if already speaking, does not wait for completion
// returns 1 if successful, 0 or negative for some error

int jhcSpTextMS::tts_say (const char *msg)
{
  ISpVoice *voice = (ISpVoice *) v;
  WCHAR wide[200];
  ULONG q;
  int n = 0;

  // check for problems
  if (t_ok <= 0)
    return -1;
  if (*msg == '\0')
    return 0;

  // add in pitch value as XML tag then start speaking
//  n = swprintf_s(wide, L"<pitch absmiddle=\"%d\"> ", pitch);
  MultiByteToWideChar(CP_UTF8, 0, msg, -1, wide + n, 200 - n);
  if (FAILED(voice->Speak(wide, SPF_ASYNC, &q)))
    return 0;

  // record message and header length for progress monitoring
  if (q <= 10)
  {
    hlen[q - 1] = n;
    strcpy_s(buf[q - 1], msg);
  }
  return 1;
}


//= Tells if system has completed emitting utterance yet.
// will fill in given string (if any) with remainder of words to be spoken
// returns viseme+1 if talking, 0 if queue is empty, negative for some error

int jhcSpTextMS::tts_status (char *rest, int ssz)
{
  ISpVoice *voice = (ISpVoice *) v;
  SPVOICESTATUS info;
  int q;

  // set up defaults then see if status can be ready
  if (rest != NULL)
    *rest = '\0';
  if (t_ok <= 0)
    return -1;
  if (FAILED(voice->GetStatus(&info, NULL)))
    return -1;

  // interpret status results
  if (info.dwRunningState == SPRS_DONE)
    return 0;
  if ((rest != NULL) && (info.ulInputSentLen > 0))
  {
    // strip off added pitch header from saved message 
    q = info.ulCurrentStream - 1;
    if ((q >= 0) && (q < 10))
      strcpy_s(rest, ssz, buf[q] + info.ulInputWordPos - hlen[q]);
  }
  return(info.VisemeId + 1);
}


//= Wait until system finishes speaking (BLOCKS).
// returns 1 if successful, 0 or negative for some error

int jhcSpTextMS::tts_wait ()
{
  ISpVoice *voice = (ISpVoice *) v;

  if (t_ok <= 0)
    return -1;
  if (voice->WaitUntilDone(INFINITE) != S_OK)
    return 0;
  return 1;
}


//= Immediately terminate whatever is being said and anything queued.
// returns 1 if successful, 0 or negative for some error

int jhcSpTextMS::tts_shutup ()
{
  ISpVoice *voice = (ISpVoice *) v;

  if (t_ok <= 0)
    return -1;
  if (FAILED(voice->Speak(NULL, SPF_PURGEBEFORESPEAK, NULL)))
    return 0;
  return 1;
}

