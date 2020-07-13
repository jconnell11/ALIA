// sp_parse.h : encapsulated functions for context-free parsing
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2014 IBM Corporation
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

// The simple speech recognition interface automatically processes audio 
// (usually from a microphone) and generates best guesses as to the text.
// The system can be dynamically reconfigured while running for different 
// users and different expected topics. A typical program would look like:
//
//   parse_setup();
//   parse_load("topic_3.txt")
//   parse_enable("top_level");
//   parse_start();
//   while (1)
//   {
//     ...
//     parse_analyze(text);
//     parse_focus(node);
//     printf("Node <%s> covers <%s>\n", node, text);
//   }
//   parse_cleanup();

///////////////////////////////////////////////////////////////////////////

#ifndef _SP_PARSE_
#define _SP_PARSE_


#include <stdlib.h>            // needed for NULL


// function declarations (possibly combined with other header files)

#ifndef DEXP
  #ifndef STATIC_LIB
    #ifdef SP_PARSE_EXPORTS
      #define DEXP __declspec(dllexport)
    #else
      #define DEXP __declspec(dllimport)
    #endif
  #else
    #define DEXP
  #endif
#endif


/*
// link to library stub (but not if combined with sp_reco)

#ifndef SP_PARSE_EXPORTS
  #ifdef _DEBUG
    #pragma comment(lib, "sp_parse_d.lib")
  #else
    #pragma comment(lib, "sp_parse.lib")
  #endif
#endif
*/


///////////////////////////////////////////////////////////////////////////
//                        Parsing Configuration                          //
///////////////////////////////////////////////////////////////////////////

//= Fills string with version number of DLL and possibly other information.
// returns pointer to input string for convenience

extern "C" DEXP const char *parse_version (char *spec);


//= Loads all common grammar and parsing parameters based on the file given.
// the single configuration file can point to other files as needed (REQUIRED)
// returns 1 if successful, 0 or negative for some error

extern "C" DEXP int parse_setup (const char *cfg_file =NULL);


//= Start accepting utterances to parse according to some grammar(s).
// takes a debugging level specification and log file designation
// use this to initially fire up the system (REQUIRED)
// returns 1 if successful, 0 or negative for some error

extern "C" DEXP int parse_start (int level =0, const char *log_file =NULL);


//= Stop accepting utterances and clean up all objects and files.
// only call this at the end of a processing session (REQUIRED)

extern "C" DEXP void parse_cleanup ();


///////////////////////////////////////////////////////////////////////////
//                   Run-Time Parsing Modifications                      //
///////////////////////////////////////////////////////////////////////////

//= Load a certain (or additional) grammar from a file.
// this can be done without explicitly pausing the system
// if only one grammar is allowed, this overwrites previous
// initially all rules are disabled (call parse_enable)
// returns 2 if appended, 1 if exclusive, 0 or negative for some error


extern "C" DEXP int parse_load (const char *grammar);


//= Remove all grammars that may have been loaded.

extern "C" DEXP void parse_clear ();


//= Enable some top-level (i.e. sentence) rule within the grammar.
// returns 1 if successful, 0 if not found, negative for some error

extern "C" DEXP int parse_enable (const char *rule);


//= Disable some top-level (i.e. sentence) rule within the grammar.
// a NULL rule name serves to disable ALL top level rules
// returns 1 if successful, 0 if not found, negative for some error

extern "C" DEXP int parse_disable (const char *rule =NULL);


//= Add a new expansion to some existing rule in the grammar.
// alters internal graph and attempts to change original grammar file also
// returns 2 if ok, 1 if only run-time changed, 0 or negative for error

extern "C" DEXP int parse_extend (const char *rule, const char *option);


///////////////////////////////////////////////////////////////////////////
//                          Parsing Results                              //
///////////////////////////////////////////////////////////////////////////

//= Accept an utterance for parsing by currently active grammar(s).
// can optionally take list of confidences (0-100) for each word
// automatically sets focus to top of parse tree (if found)
// returns number of interpretations, 0 if no valid parse, negative if error

extern "C" DEXP int parse_analyze (const char *text, const char *conf =NULL);


//= Returns the name or string associated with the current focus node.
// returns 1 if successful, 0 or negative for error

extern "C" DEXP int parse_focus (char *token =NULL);


//= Returns the range of surface words covered by the current focus node.
// word 0 is the initial word in the utterance
// returns total number of words in utterance, 0 or negative for error

extern "C" DEXP int parse_span (int *first, int *last);


//= Reset the focus to the top most node of the parse tree.
// can select particular interpretation if more than one
// returns 1 if successful, 0 or negative for error

extern "C" DEXP int parse_top (int n =0);


//= Move focus to next non-terminal to the right in the current expansion.
// returns 1 if successful, 0 if focus unchanged, negative for error

extern "C" DEXP int parse_next ();


//= Move focus down one level (i.e. expand a non-terminal node).
// automatically sets focus to leftmost branch of parse tree
// returns 1 if successful, 0 if focus unchanged, negative for error

extern "C" DEXP int parse_down ();


//= Move focus up one level (i.e. restore it to location before call to down). 
// returns 1 if successful, 0 if focus unchanged, negative for error

extern "C" DEXP int parse_up ();


///////////////////////////////////////////////////////////////////////////

#endif  // once
