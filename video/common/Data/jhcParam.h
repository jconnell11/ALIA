// jhcParam.h : useful structure for processing parameters
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2019 IBM Corporation
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

#ifndef _JHCPARAM_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCPARAM_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include <stdlib.h>


//= Useful structure for processing parameters.
// Implemented as a list of parameters since single entry is uncommon.
// Holds pointer to real value, default to use, and name of parameter.
// Note: Originally took 2 byte integer, 4 byte integer, and floating values.

class jhcParam 
{
// PRIVATE MEMBER VARIABLES
private:
  int total, next;
  int lbad;
  double fbad;
  int *locks;
  int *ldefs;
  double *fdefs;
  int **lvals;
  double **fvals;
  char *names;
  char tag[40];
  char title[80];


// PUBLIC MEMBER FUNCTIONS
public:
  // construction and destruction
  ~jhcParam ();
  jhcParam ();
  jhcParam (int n);
  void SetSize (int n);
  
  // list functions
  void CopyAll (const jhcParam& src);
  void ClearAll ();
  void RevertAll ();

  // file operations
  void SetTag (const char *token, int no_clr =1);
  void SetTitle (const char *label, ...);
  const char *GetTitle () const {return title;};
  int SaveVals (const char *file_name, const char *alt_tag =NULL, int prefix =0) const;
  int LoadDefs (const char *file_name, const char *alt_tag =NULL, int prefix =0);
  int SetDefsTxt (char *line);
  int RemVals (const char *file_name, const char *alt_tag =NULL, int prefix = 0) const;

  // entry loading
  void Rewind () {next = 0;}         /** Set insertion point to first parameter. */
  void Skip (int n =1) {next += n;}  /** Advance insertion point several slots.  */
  void SetSpec4 (int i, int *loc, int init, const char *descr =NULL);
  void SetSpecF (int i, double *loc, double init, const char *descr =NULL);
  void NextSpec4 (int *loc, int init, const char *descr =NULL);
  void NextSpecF (double *loc, double init, const char *descr =NULL);

  void SetSpec4 (int i, int *loc, const char *descr =NULL)    {SetSpec4(i, loc, *loc, descr);};
  void SetSpecF (int i, double *loc, const char *descr =NULL) {SetSpecF(i, loc, *loc, descr);};
  void NextSpec4 (int *loc, const char *descr =NULL)    {NextSpec4(loc, *loc, descr);};
  void NextSpecF (double *loc, const char *descr =NULL) {NextSpecF(loc, *loc, descr);};

  // altering entries
  int SetMatch (int *loc, int v);
  int SetMatch (double *loc, double v);
  int ScaleMatch (int *loc, double f, int odd =0);
  int ScaleMatch (double *loc, double f);
  int LockMatch (int *loc, int lockit =1);
  int LockMatch (double *loc, int lockit =1);

  // basic array access
  int Size () const {return total;}  /** Total number of parameters stored. */
  const char *Text (int i) const;
  int Ltype (int i) const;
  int Ftype (int i) const;
  int Lval (int i) const;
  double Fval (int i) const;
  int Ldef (int i) const;
  double Fdef (int i) const;
  int Lset (int i, int val);
  int Fset (int i, double val);

  // for backwards compatibility
  void NextSpec2 (int *loc, int init, const char *descr =NULL)
    {NextSpec4(loc, init, descr);};

  // auxilliary functions
  int LoadText (char *text, const char *fname, const char *tag, const char *def, int ssz) const;
  int SaveText (const char *fname, const char *tag, const char *text) const;

  // auxilliary functions - convenience
  template <size_t ssz>
    int LoadText (char (&text)[ssz], const char *fname, const char *tag, const char *def =NULL) const
      {return LoadText(text, fname, tag, def, ssz);}


// PRIVATE MEMBER FUNCTIONS
private:
  // construction and destruction
  void init_param ();
  void dealloc_param ();
  char *strcpy0 (char *dest, const char *src, int dest_size) const;

  // file operations
  char *next_token (char **src, const char *delim) const;
  int find_line (char *line, const char *fname, const char *tag) const;
  int excise_line (const char *fname, const char *tag) const;
  int add_line (const char *fname, const char *tag, const char *text) const;

};

#endif




