
// jhcAliaCore.cpp : top-level coordinator of components in ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
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

#include <conio.h>
#include <stdarg.h>

#include "Interface/jms_x.h"           // common video

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
  ver = 2.90;
  vol = 1;                   // enable free will reactions
  noisy = 1;

  // add literal text output and stack crawler to function repertoire
  talk.Bind(&(net.mf));
  kern.AddFcns(&talk);
  kern.AddFcns(&why);
  ndll = 0;

  // connect language to network converter
  net.Bind(this);
                      
  // clear state
  *rob = '\0';
  *cfile = '\0';
  log = NULL;
  Reset(0, NULL, 0);
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
    return jprintf(1, rpt, ">>> Could not read baseline knowledge file: %s !\n", list);
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

    // read various types of input (1 = extras level)
    cnt += add_info(dir, line, rpt, 1);        
  } 

  // clean up
  fclose(in);
  jprintf(1, rpt, " TOTAL = %d operators, %d rules\n\n", 
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
  if (readable(fname, 200, "%s%s_v.rules", dir, base))
    if (amem.Load(fname, 1, rpt, level) > 0)      
      cnt++;
  return cnt;
}


//= Assemble a file name and see if it is readable.

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

    // add associated operators and rules then link things up
    add_info(dir, line, rpt, 0);
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


//= Add in new rule or operator suggested by user (typcially only one or the other).
// acceptance moved into jhcAliaDir to allow rejection if user disliked for some reason
// should NULL pointers in caller afterward if successful (amem or pmem will delete at end)
// returns 1 if successful, 0 or negative for problem (consider deleting explicitly)

int jhcAliaCore::Accept (jhcAliaRule *r, jhcAliaOp *p)
{
  int ans = 1;

  if ((r == NULL) && (p == NULL))
    return -2;
  if (r != NULL)
  {
    if ((ans = amem.AddRule(r, 1)) > 0)
      mood.Infer();
  }
  if (p != NULL)
  {
    if ((ans = pmem.AddOperator(p, 1)) > 0)
      mood.React();
  }
  return ans;
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
    atree.AddProp(atree.Robot(), "name", rname, 0, -1.0);

    // possibly add just first name also ("Eli")
    strcpy_s(first, rname);
    if ((sep = strchr(first, ' ')) != NULL)
    {
      *sep = '\0';
      gr.ExtendRule("ATTN", first); 
      atree.AddProp(atree.Robot(), "name", first, 0, -1.0);
    }
  }
  gr.MarkRule(top);
  return 1;
}


//= Clear out all focal items.
// possibly starts up input conversion log file also

void jhcAliaCore::Reset (int forget, const char *rname, int spact)
{
  char date[80], fname[80];

  // clear action tree
  StopAll();
  atree.ClrFoci(1, rname);
  kern.Reset(&atree);
  stat.Reset();
  mood.Reset();
  topval = 0;

  // possibly forget all rules and operators
  if (forget > 0)
  {
    amem.ClearRules();
    pmem.ClearOps();
  }

  // remember robot name (if any)
  *rob = '\0';
  if (rname != NULL)
    strcpy_s(rob, rname);

  // reset affective modulation
  atree.InitSkep(0.5);
  pess  = 0.5;
  wild  = 0.5;
  det   = 1.0;
  argh  = 1.0;               // secs
  waver = 2.0;               // secs

  // communicate debugging level
  atree.noisy = noisy;
  pmem.noisy = noisy;

  // record starting time and make conversion log file
  t0 = jms_now();
  if (spact > 0)
  {
    CloseCvt();
    if (*cfile != '\0')
      strcpy_s(fname, cfile);
    else
      sprintf_s(fname, "log/log_%s.cvt", jms_date(date));
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
  int nt, spact, attn = 0;

  // check if name mentioned and get parse results
  attn = gr.NameSaid(sent, amode);
  if ((nt = gr.Parse(sent, 1)) > 0)
    gr.AssocList(alist, 1);
  if ((awake == 0) && (attn <= 0))
    return 0;

  // show parsing steps then generate semantic nets 
  gr.PrintInput();
  if (nt > 0)
  {
    mood.Hear((int) strlen(input));      // reduces "lonely" (even if not understood)
    gr.PrintResult(3, 1);
  }
  spact = net.Convert(alist, sent);      // nt = 0 gives huh? response
  net.Summarize(log, sent, nt, spact);
  return((attn > 0) ? 2 : 1);
}


//= Run all focal elements in priority order.
// must mark all seed nodes to retain before calling with gc > 0
// member variable "svc" set to current focus and "bid" to appropriate value
// tells how many foci were processed on this cycle

int jhcAliaCore::RunAll (int gc)
{
  char time[40];
  jhcAliaChain *s;
  int res, cnt = 0;

  // get any observations, check expired attentional foci, and recompute halo
  jprintf(3, noisy, "\nSTEP %d ----------------------------------------------------\n\n", atree.Version());
  kern.Volunteer();
  if (atree.Update(gc) > 0)                      // also if bth or node blfs change?
    amem.RefreshHalo(atree, noisy - 1);
  if (gc > 0)
  {
    mood.Update(atree);                          // status NOTEs
    gather_stats();                              // build graphs
  }
  if (atree.Active() > 0)
    jprintf(2, noisy, "============================= %s =============================\n\n", jms_offset(time, t0));


  // go through the foci from newest to oldest
  while ((svc = atree.NextFocus()) >= 0)
  {
    jprintf(2, noisy, "-- servicing focus %d\n\n", svc);
    s = atree.FocusN(svc);
    bid = atree.BaseBid(svc);
    if (atree.NeverRun(svc))
      res = s->Start(this, 0);
    else
      res = s->Status();
    atree.SetActive(s, ((res == 0) ? 1 : 0));    // replacement might have occurred
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


//= Accumulate data of various sorts for potential graphing later.
// data for robot-dependent peripherals entered previously by coordination class

void jhcAliaCore::gather_stats ()
{
  stat.Thought(atree);                        
  stat.Shift();
}


//= Stop all running activities (order is arbitrary).

void jhcAliaCore::StopAll ()
{
  jhcAliaChain *s;
  int i, nf = atree.NumFoci();

  for (i = 0; i < nf; i++)
  {
    s = atree.FocusN(i);
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
//                         Directive Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Converts any halo facts into wmem facts and posts NOTEs about them.
// updates bindings "b" to point to equivalent wmem nodes instead of halo
// generates new rule if a two step inference was used for some halo fact
// returns number of NOTEs generated (better not to notice explicitly?)

int jhcAliaCore::MainMemOnly (jhcBindings& b, int note) 
{
  jhcBindings b2;
  int n, r;

  b2.Copy(b);
  n = atree.ReifyRules(b, note);       // any NOTEs come first in log
  r = amem.Consolidate(b2);            // new rules come last in log
  mood.Infer(r);
  return n;
}


//= Look for all in-progress activities matching graph and cause them to fail.
// return 1 if no match or all stopped, -2 if cannot stop some

int jhcAliaCore::HaltActive (jhcGraphlet& desc)
{
  jhcNetNode *main = desc.Main();
  jhcAliaChain *ch;
  int i, n = atree.NumFoci(), ans = 1;

  // scan through each focus (excluding the curent one)
  main->SetNeg(0);
  for (i = 0; i < n; i++)
    if (i != svc)
      if ((ch = atree.FocusN(i)) != NULL)
      {
        // stop if current focus has high enough priority
        if (bid >= atree.BaseBid(i))
          ch->FindActive(desc, 1);
        else if (ch->FindActive(desc, 0) > 0)
          ans = -2;
      }

  // restore negative and mark as done
  main->SetNeg(1);
  main->SetBelief(1.0);
  return ans;
}


///////////////////////////////////////////////////////////////////////////
//                             Halo Control                              //
///////////////////////////////////////////////////////////////////////////

//= Assign all nodes from this NOTE a unique source marker.

int jhcAliaCore::Percolate (const jhcAliaDir& dir) 
{
  jhcNetNode *n;
  const jhcGraphlet *key = &(dir.key);
  int i, tval, ni = key->NumItems();

  // only assign new id if needed
  if (dir.own > 0)
    return dir.own;
  tval = ++topval;

  // mark all nodes in graphlet with special NOTE id
  // ignore objects becaue they carry no intrinsic semantic value
  for (i = 0; i < ni; i++)
    if ((n = key->Item(i)) != NULL)
      if (!n->ObjNode() && (n->top < tval))
      {
        n->top = tval;
        atree.Dirty();        // queue recomputation of halo
      }
  return tval;
}


//= Deselect nodes in NOTE and re-derive halo without them.
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
// "bid" member variable set automatically when servicing related focus
// returns instance number (zero okay), negative for problem

int jhcAliaCore::FcnStart (const jhcNetNode *fcn)
{
  const char *fname = fcn->Lex();

  jprintf(2, noisy, "F-START %s \"%s\" @ %d\n\n", fcn->Nick(), fname, bid);
  return kern.Start(fcn, bid);
}


//= Check if kernel procedure done yet.
// translates to/from kernel Call function
// returns 1 if successful, 0 if still working, -2 for fail

int jhcAliaCore::FcnStatus (const jhcNetNode *fcn, int inst) 
{
  const char *fname = fcn->Lex();
  int res = -2;

  jprintf(2, noisy, "\nF-STATUS %s \"%s\"\n", fcn->Nick(), fname);
  if (inst >= 0)
    res = kern.Status(fcn, inst);
  jprintf(2, noisy, "  -> FCN %s\n\n", ((res > 0) ? "success !" : ((res < 0) ? "FAIL" : "continue ...")));
  return((res < 0) ? -2 : res);
}


//= Tell some instance of a kernel function to stop.
// returns -1 always for convenience

int jhcAliaCore::FcnStop (const jhcNetNode *fcn, int inst)
{
  const char *fname = fcn->Lex();

  jprintf(2, noisy, "\nF-STOP %s \"%s\"\n\n", fcn->Nick(), fname);
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
//  dg.Generate(g, atree);
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

//= Load all rules and operators beyond baseline and kernels.
// always loads whatever is in the "learned" version of files

void jhcAliaCore::LoadLearned ()
{
  jprintf(1, noisy, "Reloading learned knowledge:\n");
  pmem.Load("KB/learned.ops",   1, noisy + 1, 2);          // 2 = accumulated level
  amem.Load("KB/learned.rules", 1, noisy + 1, 2);         
  pmem.Overrides("KB/learned.pref");
  amem.Overrides("KB/learned.conf");
  jprintf(1, noisy, "\n");
}


//= Save all rules and operators beyond baseline and kernels.
// saves a copy with time and date stamp as well as "learned" version

void jhcAliaCore::DumpLearned () const
{
  char base[80] = "KB/kb_";
  int nop, nr;

  // save rules and operators  
  jprintf(1, noisy, "\nSaving learned knowledge:\n");
  jms_date(base + 6, 0, 74);
  nr = amem.Save(base, 2);                                 // 2 = accumulated level
  nop = pmem.Save(base, 2);                               
  amem.Alterations(base);                 
  pmem.Alterations(base);

  // make copies as generic database
  copy_file("KB/learned.rules", base);
  copy_file("KB/learned.ops",   base);
  copy_file("KB/learned.conf",  base);
  copy_file("KB/learned.pref",  base);
  jprintf(1, noisy, " TOTAL = %d operators, %d rules\n", nop, nr);
}


//= Completely copy one file to another (irregardless of operating system).

void jhcAliaCore::copy_file (const char *dest, const char *base) const
{
  char buf[8192];
  char src[80];
  const char *ext;
  FILE *in, *out;
  size_t sz;

  // copy extension from destination to create full source name
  strcpy_s(src, base);
  if ((ext = strrchr(dest, '.')) != NULL)
    strcat_s(src, ext);

  // try opening files
  if (fopen_s(&in, src, "rb") != 0)
    return;
  if (fopen_s(&out, dest, "wb") != 0)
  {
    fclose(in);
    return;
  }

  // copy chunk by chunk then clean up
  while ((sz = fread(buf, 1, 8192, in)) > 0) 
    fwrite(buf, 1, sz, out);
  fclose(out);
  fclose(in);
}


//= Save all rules and operators learned during this session.

void jhcAliaCore::DumpSession () const
{
  amem.Save("session.rules", 3);
  pmem.Save("session.ops", 3);
//  atree.Save("session.wmem");
}


//= Save all rules and operators from any source.

void jhcAliaCore::DumpAll () const
{
  amem.Save("all.rules", 0);
  pmem.Save("all.ops", 0);
}
