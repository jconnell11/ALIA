// jhcSpeechDLL.h : wrapper for speech-related DLL functions
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

#ifndef _JHCSPEECHDLL_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSPEECHDLL_
/* CPPDOC_END_EXCLUDE */

#include <windows.h>

#include "jhcGlobal.h"


//= Wrapper for speech-related DLL functions.

class jhcSpeechDLL
{
// PRIVATE MEMBER VARIABLES
private:
  // dll names
  char r_path[200], p_path[200], t_path[200];  

  // dll binding variables
  HINSTANCE r_dll, p_dll, t_dll;

  // recognition function variables
  FARPROC r_version, r_setup, r_input, r_engine, r_start, r_cleanup;
  FARPROC r_uloc, r_uadd, r_udel, r_uclr, r_ulist, r_madd, r_mdel, r_mclr, r_mlist;
  FARPROC r_listen, r_status, r_heard, r_phone, r_speaker;

  // parsing function variables
  FARPROC p_version, p_setup, p_start, p_cleanup;
  FARPROC p_load, p_clear, p_enable, p_disable, p_extend;
  FARPROC p_analyze, p_focus, p_span, p_top, p_next, p_down, p_up;

  // text to speech function variables
  FARPROC t_version, t_setup, t_voice, t_output, t_start, t_cleanup;
  FARPROC t_say, t_status, t_wait, t_shutup;


// PROTECTED MEMBER VARIABLES
protected:
  int r_ok, p_ok, t_ok;                     // system status


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and destruction
  jhcSpeechDLL ();
  ~jhcSpeechDLL ();
  int BindReco (const char *fname, const char *cfg =NULL);
  int BindParse (const char *fname, const char *cfg =NULL);
  int BindTTS (const char *fname, const char *cfg =NULL);

  // low level sp_reco DLL functions
  const char *reco_version (char *detail);
  int reco_setup (const char *cfg_file =NULL);
  const char *reco_input (char *detail);
  const char *reco_engine (char *detail);
  int reco_start (int level =0, char *log_file =NULL);
  void reco_cleanup ();
  void reco_loc_user (int azim, int elev =0, int dist =36);
  int reco_add_user (const char *name);
  void reco_del_user (const char *name);
  void reco_clr_users ();
  int reco_list_users (char *list);
  int reco_add_model (const char *topic);
  void reco_del_model (const char *topic);
  void reco_clr_models ();
  int reco_list_models (char *list);
  void reco_listen (int doit =1);
  int reco_status ();
  int reco_heard (char *text, char *conf, int choice =0);
  void reco_phonetic (char *pseq, int choice =0);
  int reco_speaker (char *name);

  // low level sp_parse DLL functions
  const char *parse_version (char *detail);
  int parse_setup (const char *cfg_file =NULL);
  int parse_start (int level =0, char *log_file =NULL);
  void parse_cleanup ();
  int parse_load (const char *grammar);
  void parse_clear ();
  int parse_enable (const char *rule);
  int parse_disable (const char *rule =NULL);
  int parse_extend (const char *rule, const char *option);
  int parse_analyze (const char *text, const char *conf =NULL);
  int parse_focus (char *token =NULL);
  int parse_span (int *first =NULL, int *last =NULL);
  int parse_top ();
  int parse_next ();
  int parse_down ();
  int parse_up ();

  // low level sp_tts DLL functions
  const char *tts_version (char *detail);
  int tts_setup (const char *cfg_file =NULL);
  const char *tts_voice (char *detail);
  const char *tts_output (char *detail);
  int tts_start (int level =0, char *log_file =NULL);
  void tts_cleanup ();
  int tts_say (const char *msg);
  int tts_status ();
  int tts_wait ();
  int tts_shutup ();


// PRIVATE MEMBER FUNCTIONS
private:
  int r_bind (const char *dll_name);
  int p_bind (const char *dll_name);
  int t_bind (const char *dll_name);
  int r_release ();
  int p_release ();
  int t_release ();

};


#endif  // once




