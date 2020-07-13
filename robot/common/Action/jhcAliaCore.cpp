// jhcAliaCore.cpp : top-level coordinator of components in ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
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

#include <conio.h>
#include <stdarg.h>

#include "Interface/jms_x.h"

#include "Action/jhcAliaCore.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcAliaCore::~jhcAliaCore ()
{
  StopAll();
  CloseCvt();
}


//= Default constructor initializes certain values.

jhcAliaCore::jhcAliaCore ()
{
  // global variables
  ver = 1.60;
  noisy = 1;

  // add literal text output to function repertoire
  kern.AddFcns(&talk);
  ndll = 0;

  // connect language to network converter
  net.Bind(this);
                      
  // clear state
  log = NULL;
  Reset();
}


///////////////////////////////////////////////////////////////////////////
//                                Extensions                             //
///////////////////////////////////////////////////////////////////////////

//= Loads grammars, rules, and operators associated with current kernels.
// each kernel has a BaseTag like "Social" and then files *.sgm, *.ops, and *.rules
// grammar for speech altered separately (jhcAliaSpeech::kern_gram)

void jhcAliaCore::KernExtras (const char *kdir)
{
  const jhcAliaKernel *k = &kern;
  const char *tag; 
  int nr0 = amem.NumRules(), nop0 = pmem.NumOperators();

  jprintf(1, noisy, "Loading kernel rules and operators:\n");
  while (k != NULL)
  {
    // read files based on tags in each kernel class (0 = kernel level)
    tag = k->BaseTag();
    if (*tag != '\0')
      add_info(kdir, tag, noisy + 1, 0);
    k = k->NextPool();
  }
  jprintf(1, noisy, " TOTAL = %d operators, %d rules\n\n", 
          pmem.NumOperators() - nop0, amem.NumRules() - nr0);
}


//= Loads up a bunch of rules and operators as listed in file.
// new ones are added with level = 0 to separate from customized stuff
// sample "KB2/baseline.lst" file contains the text:
//   animals
//   colors
//   acknowledge
//   investigate
// for each base name there can be files *.sgm, *.ops, and *.rules
// all the named files should be in this same directory
// grammar for speech altered separately (jhcAliaSpeech::base_gram)
// returns number of individual files read

int jhcAliaCore::Baseline (const char *list, int add, int rpt)
{
  char dir[80], line[80];
  FILE *in;
  char *end;
  int n, r0 = amem.NumRules(), op0 = pmem.NumOperators(), cnt = 0;

  // possibly clear old stuff then try to open file
  if (add <= 0)
  {
    r0 = amem.ClearRules();
    op0 = pmem.ClearOps();
  }
  if (fopen_s(&in, list, "r") != 0)
    return jprintf(1, rpt, ">>> Could not open baseline knowledge file: %s !\n", list);
  jprintf(1, rpt, "Adding baseline knowledge from: %s\n", list);

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
    // ignore commented out lines and blank ones
    if (strncmp(line, "//", 2) == 0)
      continue;
    if ((n = (int) strlen(line)) <= 0)
      continue;

    // read various types of input (1 = extras level)
    if (line[n - 1] == '\n')
      line[n - 1] = '\0';
    cnt += add_info(dir, line, rpt, 1);        
  } 

  // clean up
  fclose(in);
  jprintf(1, rpt, "  TOTAL = %d operators, %d rules\n\n", 
          pmem.NumOperators() - op0, amem.NumRules() - r0);
  return cnt;
}


//= Read in lexical terms, operators, and rules associated with base string.
// "rpt" is for messages, "level" is marking for additions: 0 = kernel, 1 = extras
// returns total number of files read

int jhcAliaCore::add_info (const char *dir, const char *base, int rpt, int level)
{
  char fname[200];
  int cnt = 0;

  if (readable(fname, 200, "%s%s.sgm", dir, base))
    if ((net.mf).AddVocab(&gr, fname) > 0)
      cnt++;
  if (readable(fname, 200, "%s%s.ops", dir, base))
    if (pmem.Load(fname, 1, rpt, level) > 0)      
      cnt++;
  if (readable(fname, 200, "%s%s.rules", dir, base))
    if (amem.Load(fname, 1, rpt, level) > 0)      
      cnt++;
  return cnt;
}


//= Assemble a file name and see if it is readable.
// leaves assembled name in variable "fname"

bool jhcAliaCore::readable (char *fname, int ssz, const char *msg, ...) const
{
  va_list args;
  FILE *in;

  if (msg == NULL)
    return false;
  va_start(args, msg); 
  vsprintf_s(fname, ssz, msg, args);
  if (fopen_s(&in, fname, "r") != 0)
    return false;
  fclose(in);
  return true;
}


//= Load grounding DLLs and associated operators from a list of names.
// all the routines must be associated with the same "body" instance
// new operators are added with level = 0 to separate from customized stuff
// sample "add_on/fcn_pools.lst" file contains the text:
//   man_drv
//   man_blocks
// so man_drv.dll and man_drv.ops should be in this same directory
// returns number of additional DLLs loaded, negative for problem

int jhcAliaCore::AddOn (const char *fname, void *body, int rpt)
{
  char dir[80], line[80], name[200];
  FILE *in;
  char *end;
  int n, cnt = 0;

  // try to open file 
  if (fopen_s(&in, fname, "r") != 0)
    return jprintf(1, rpt, ">>> Could not open groundings file: %s !\n", fname);
  jprintf(1, rpt, "Adding groundings from: %s\n", fname);

  // save directory part of path
  strcpy_s(dir, fname);
  if ((end = strrchr(dir, '/')) == NULL)
    end = strrchr(dir, '\\');
  if (end != NULL)
    *(end + 1) = '\0';
  else
    *dir = '\0';

  // read list of names
  while (fgets(line, 80, in) != NULL)
  {
    // make sure this DLL can be remembered (for later closing)
    if (ndll >= dmax)
    {
      jprintf(">>> More than %d DLLs in jhcAliaCore::AddOn !\n", dmax);
      break;
    }

    // strip off any carriage return
    n = (int) strlen(line);
    if (line[n - 1] == '\n')
      line[n - 1] = '\0';

    // attempt to bind DLL 
    sprintf_s(name, "%s%s.dll", dir, line);
    if (gnd[ndll].Load(name) <= 0)
    {
      jprintf(1, rpt, "  -- could not add: %s.dll\n", line);
      continue;
    }

    // add associated operators
    sprintf_s(name, "%s%s.ops", dir, line);
    if (pmem.Load(name, 1, 2, 0) <= 0)
      continue;

    // success so link things up
    gnd[ndll].Bind(body);
    kern.AddFcns(gnd + ndll);
    ndll++;
    cnt++;
  }

  // clean up
  fclose(in);
  jprintf(1, rpt, "\n");
  return cnt;
}


//= Add in new rule or operator suggested by user.
// two step process allows rejection if user disliked for some reason
// returns 1 if successful, -2 for problem

int jhcAliaCore::Accept ()
{
  if (net.rule != NULL)
  {
    amem.AddRule(net.rule, 1);
    net.rule = NULL;
    return 1;
  }
  if (net.oper != NULL)
  {
    pmem.AddOperator(net.oper, 1);
    net.oper = NULL;
    return 1;
  }
  return -2;                 // for jhcAliaDir::Status
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Set up basic parsing grammar and top level rule.
// can also set up robot name as attention word (speech grammar loaded separately)

int jhcAliaCore::MainGrammar (const char *gfile, const char *top, const char *rname)
{
  char first[80];
  char *sep;

  gr.ClearGrammar();
  if (gr.LoadGrammar(gfile) <= 0)
    return 0;
  if ((rname != NULL) && (*rname != '\0'))   
  {
    // add robot full name as attention word ("Eli Banzai")
    gr.ExtendRule("ATTN", rname); 
    attn.AddLex(attn.self, rname, 0, -1.0);

    // possibly add just first name also ("Eli")
    strcpy_s(first, rname);
    if ((sep = strchr(first, ' ')) != NULL)
    {
      *sep = '\0';
      gr.ExtendRule("ATTN", first); 
      attn.AddLex(attn.self, first, 0, -1.0);
    }
  }
  gr.MarkRule(top);
  return 1;
}


//= Clear out all focal items.
// possibly starts up input conversion log file also

void jhcAliaCore::Reset (int forget, const char *rname, int cvt)
{
  char date[80], fname[80];

  // clear action tree
  StopAll();
  attn.ClrFoci(1, rname);
  kern.Reset(&attn);
  topval = 0;

  // possibly forget all rules and operators
  if (forget > 0)
  {
    amem.ClearRules();
    pmem.ClearOps();
  }

  // reset affective modulation
  bth = 0.5;
  pth = 0.5;
  wild = 0.0;

  // communicate debugging level
  attn.noisy = noisy;
  pmem.noisy = noisy;

  // record starting time and make log file
  t0 = jms_now();
  if (cvt > 0)
  {
    CloseCvt();
    sprintf_s(fname, "log/cvt_%s.txt", jms_date(date));
    if (fopen_s(&log, fname, "w") != 0)
      log = NULL;
  }
}


//= Process input sentence from some source.
// if awake == 0 then needs to hear attention word to process sentence
// returns 2 if attention found, 1 if understood, 0 if unintelligible

int jhcAliaCore::Interpret (const char *input, int awake, int amode)
{
  char alist[1000] = "";
  const char *sent = ((input != NULL) ? input : alist);
  int nt, cvt, attn = 0;

  // check if name mentioned and get parse results
  attn = gr.NameSaid(sent, amode);
  if ((nt = gr.Parse(sent)) > 0)
    gr.AssocList(alist, 1);
  if ((awake == 0) && (attn <= 0))
    return 0;

  // show parsing steps then generate semantic nets
  if (nt > 0)
  {
    gr.PrintInput();
    gr.PrintResult(3, 1);
  }
  cvt = net.Convert(alist);            // if nt = 0 then gives huh? response

  // possibly log conversion of user input
  if (log != NULL)
    log_conversion(sent, nt, cvt);
  return((attn > 0) ? 2 : 1);
}


//= Create a log entry for debugging grammar and graphizer.

void jhcAliaCore::log_conversion (const char *sent, int nt, int cvt) 
{
  // record input and parsing exceptions
  fprintf(log, ".................................................\n\n");
  fprintf(log, "\"%s\"\n\n", sent);
  if (nt == 0)
    fprintf(log, "*** NO PARSE ***\n\n");
  else if (nt > 1)
    fprintf(log, "*** %d parses ***\n\n", nt);

  // record interpretation result
  if (cvt == 7)
    fprintf(log, "-- farewell --\n\n");
  else if (cvt == 6)
    fprintf(log, "-- greeting --\n\n");
  else if (cvt == 5)
    fprintf(log, "-- hail --\n\n");
  else if ((cvt == 4) && (net.oper != NULL))
    (net.oper)->Save(log);
  else if ((cvt == 3) && (net.rule != NULL))
    (net.rule)->Save(log);
  else if (((cvt == 2) || (cvt == 1)) && (net.bulk != NULL))
  {
    (net.bulk)->Save(log);
    fprintf(log, "\n");
  }
  else if (nt > 0)
    fprintf(log, "-- nothing --\n\n");
}


//= Run all focal elements in priority order.
// must mark all seed nodes to retain before calling with gc > 0
// tells how many foci were processed on this cycle

int jhcAliaCore::RunAll (int gc)
{
  char time[40];
  jhcAliaChain *s;
  int win, res, cnt = 0;

  // get any observations, check expired attentional foci, and recompute halo
  jprintf(3, noisy, "\nSTEP %d ----------------------------------------------------\n\n", attn.Version());
  kern.Volunteer();
  attn.Update(gc);
  if (attn.Active() > 0)
    jprintf(2, noisy, "============================= %s =============================\n\n", jms_elapsed(time, t0));

  // go through the foci from newest to oldest
  while ((win = attn.NextFocus()) >= 0)
  {
    jprintf(2, noisy, "-- servicing focus %d\n\n", win);
    s = attn.FocusN(win);
    bid = attn.BaseBid(win);
    if (attn.NeverRun(win))
      res = s->Start(*this);
    else
      res = s->Status();
    attn.SetActive(win, ((res == 0) ? 1 : 0));
    cnt++;
  }

  // possibly wait for user to view step results
  if (noisy >= 3)
  {
    jprintf("Hit any key to continue ...");
    _getch();
    jprintf("\n\n");
  }
  return cnt;
}


//= Stop all running activities (order is arbitrary).

void jhcAliaCore::StopAll ()
{
  jhcAliaChain *s;
  int i, nf = attn.NumFoci();

  for (i = 0; i < nf; i++)
  {
    s = attn.FocusN(i);
    s->Stop();
  }
}


//= Close the input conversion log file (if any).

void jhcAliaCore::CloseCvt ()
{
  if (log != NULL)
    fclose(log);
  log = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                             Halo Control                              //
///////////////////////////////////////////////////////////////////////////

//= Re-derive all halo inferences based on current working memory contents.

void jhcAliaCore::RecomputeHalo () 
{
  amem.RefreshHalo(attn, bth, noisy - 1);
}


//= Derive inferences from newly active NOTE directive.
// each inferred node is marked by a unique identifier

int jhcAliaCore::Percolate (const jhcAliaDir& dir) 
{
  jhcNetNode *n;
  const jhcGraphlet *key = &(dir.key);
  int i, tval, ni = key->NumItems(), chg = 0;

  // only assign new id if needed
  if (dir.own > 0)
    return dir.own;
  tval = ++topval;

  // mark all nodes in graphlet with special NOTE id
  for (i = 0; i < ni; i++)
    if ((n = key->Item(i)) != NULL)
      if (!n->ObjNode() && (n->top < tval))
      {
        n->top = tval;
        chg++;
      }

  // recompute relevance of halo facts to NOTEs
  if (chg > 0)
    RecomputeHalo();
  return tval;
}


//= Deselect nodes in NOTE and rederive halo without them.
// Note: derived nodes take max of ids (current id may have masked older one)
//       but each time lower priority focus tried, halo refreshed by Percolate

int jhcAliaCore::ZeroTop (const jhcAliaDir& dir) 
{
  jhcNetNode *n;
  const jhcGraphlet *key = &(dir.key);
  int i, ni = key->NumItems();

  for (i = 0; i < ni; i++)
    if ((n = key->Item(i)) != NULL)
      n->top = 0;
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                          External Grounding                           //
///////////////////////////////////////////////////////////////////////////

//= Start up some kernel function given by lex of fcn node.
// returns instance number (zero okay), negative for problem

int jhcAliaCore::FcnStart (const jhcNetNode *fcn)
{
  const char *fname = fcn->Word();

  jprintf(2, noisy, "F-START %s \"%s\" @ %d\n\n", fcn->Nick(), fname, bid);
  return kern.Start(fcn, bid);
}


//= Check if kernel procedure done yet.
// translates to/from kernel Call function
// returns 1 if successful, 0 if still working, -2 for fail

int jhcAliaCore::FcnStatus (const jhcNetNode *fcn, int inst) 
{
  const char *fname = fcn->Word();
  int res = -2;

  jprintf(2, noisy, "\nF-STATUS %s \"%s\"\n", fcn->Nick(), fname);
  if (inst >= 0)
    res = kern.Status(fcn, inst);
  jprintf(2, noisy, "  -> %s\n\n", ((res > 0) ? "success !" : ((res < 0) ? "FAIL" : "continue ...")));
  return((res < 0) ? -2 : res);
}


//= Tell some instance of a kernel function to stop.
// returns -1 always for convenience

int jhcAliaCore::FcnStop (const jhcNetNode *fcn, int inst)
{
  const char *fname = fcn->Word();

  jprintf(2, noisy, "F-STOP %s \"%s\"\n\n", fcn->Nick(), fname);
  kern.Stop(fcn, inst);
  return -1;
}


///////////////////////////////////////////////////////////////////////////
//                           Language Output                             //
///////////////////////////////////////////////////////////////////////////

//= Compose a message for user based on graphlet given.
// higher priority message interrupts lower which then fails
// returns instance number, zero or negative for problem

int jhcAliaCore::SayStart (const jhcGraphlet& g)
{
  dg.Generate(g, attn);
/*
bid is available as member var

  if anything queued
    if bid <= old
      then return fail
    insert pause (or ...)  <- interrupt action = ShutUp (add pause to acoustic, queue new message after) <-- Utter already does this (part of issue)
  old = bid;
  output = convert g      
*/
  return 1;
}


int jhcAliaCore::SayStatus (const jhcGraphlet& g, int inst)
{
  return 1;
}


int jhcAliaCore::SayStop (const jhcGraphlet& g, int inst)
{
  return -1;
}


///////////////////////////////////////////////////////////////////////////
//                              Debugging                                //
///////////////////////////////////////////////////////////////////////////

//= Print out contents of working memory.

void jhcAliaCore::PrintMem () const 
{
  jprintf("\nWMEM =");
  attn.Print(2);
  jprintf("\n");
}


//= Load all rules and operators beyond baseline and kernels.

void jhcAliaCore::LoadLearned ()
{
  jprintf(1, noisy, "Reloading learned knowledge:\n");
  pmem.Load("learned.ops",   1, noisy + 1, 2);         // 2 = accumulated level
  amem.Load("learned.rules", 1, noisy + 1, 2);         
}


//= Save all rules and operators beyond baseline and kernels.

void jhcAliaCore::DumpLearned () const
{
  int nop, nr;

  jprintf(1, noisy, "Saving learned knowledge:\n");
  nop = pmem.Save("learned.ops", 2);                   // 2 = accumulated level
  nr  = amem.Save("learned.rules", 2);                 
  jprintf(1, noisy, " TOTAL = %d operators, %d rules\n\n", nop, nr);
}


//= Save all rules and operators learned during this session.

void jhcAliaCore::DumpSession () const
{
  amem.Save("session.rules", 3);
  pmem.Save("session.ops", 3);
//  attn.Save("session.wmem");
}


//= Save all rules and operators from any source.

void jhcAliaCore::DumpAll () const
{
  pmem.Save("all.ops", 0);
  amem.Save("all.rules", 0);
}
