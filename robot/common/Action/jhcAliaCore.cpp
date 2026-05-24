// jhcAliaCore.cpp : top-level coordinator of components in ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
// Copyright 2020-2026 Etaoin Systems
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

#include <stdarg.h>
#include <ctype.h>

#include "Interface/jtimer.h"          // common video - for profiling

#include "Action/jhcAliaCore.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcAliaCore::~jhcAliaCore ()
{
  jhcAliaKernel *k2, *k = &kern;

  stop_all();
  while (k != NULL)
  {
    k2 = k->NextPool();
    if (k->CleanUp() > 0)    // jhcAliaDLL kernels
      delete k;
    k = k2;
  }
}


//= Default constructor initializes certain values.

jhcAliaCore::jhcAliaCore ()
{
  // global variables
  ver = 6.00;                // reflected in GUI
  gnd = 0;                   // no grounding DLLs yet

  // connect up required resources for components
  net.Bind(this);
  talk.Bind(this);        
  dmem.Bind(atree);
  mood.Bind(atree);
  ltm.Bind(dmem);
  fb.BindMood(mood);
  emo.BindMood(mood);

  // add various common grounding kernels to list
  kern.AddFcns(talk);
  kern.AddFcns(ltm);
  kern.AddFcns(why);
  kern.AddFcns(fb);
  kern.AddFcns(emo);
  kern.AddFcns(tim);

  // clear state
  *wdir   = '\0';
  *cfile  = '\0';
  *formal = '\0';
  *myself = '\0';
  netlog = NULL;
  Defaults();
  init_state();
}


//= Clear out all focal items and working memory.
// possibly starts up input conversion log file also

void jhcAliaCore::init_state ()
{
  // clear action tree
  stop_all();
  atree.ResetFoci(formal);   // adds -name-> prop
  kern.Reset(atree);
  stat.Reset();
  mood.Reset();
  topval = 0;
  spact = JSP_NONE;

  // possibly forget all rules and operators
  amem.ClearRules();
  pmem.ClearOps();

  // reset affective modulation
  det   = 1.0;
  argh  = 1.0;               // secs
  waver = 30.0;              // secs
  deep  = 20;

  // communicate debugging level
  atree.noisy = noisy;
  pmem.noisy = noisy;
  amem.noisy = noisy;
  dmem.noisy = noisy;

  // reset loop timing
  t0 = jms_now();
  start = 0;
  rem = 0.0;
  sense = 0;
  think = 0;
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for selecting which console messages are displayed.

int jhcAliaCore::msg_params (const char *fname)
{
  jhcParam *ps = &mps;
  int ok;

  ps->SetTag("core_dbg", 0);
  ps->NextSpec4( &noisy,             1, "Directive calls (std = 1)");
  ps->NextSpec4( &pshow,             2, "Parsing details (std = 2)");
  ps->NextSpec4( &(net.dbg),         0, "Text interpretation (dbg = 3)");
  ps->NextSpec4( &((talk.dg).noisy), 0, "Output generation (dbg = 2)");
  ps->NextSpec4( &finder,            0, "FIND processing (dbg = 1)");
  ps->NextSpec4( &memhyp,            0, "Final wmem hyp (dbg = 1)");

  ps->NextSpec4( &(amem.detail),     0, "Matching of rule number");    
  ps->NextSpec4( &(pmem.detail),     0, "Matching of op number");   
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for standard sensing and thinking rates.

int jhcAliaCore::rate_params (const char *fname)
{
  jhcParam *ps = &rps;
  int ok;

  ps->SetTag("core_mind", 0);
  ps->NextSpec4( &acc,  2,   "Mind (KB0/KB2, extras, full)");
  ps->NextSpec4( &vol,  1,   "Load baseline volition (KB2)");
  ps->NextSpecF( &thz, 80.0, "Thought cycle rate (Hz)");   
  ps->NextSpecF( &shz, 30.0, "Default body rate (Hz)"); 
  ps->Skip();
  ps->NextSpec4( &spec, 1,   "Allow speculation (dbg = 2)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcAliaCore::Defaults (const char *fname)
{
  int ok = 1;

  ok &= msg_params(fname);
  ok &= rate_params(fname);
  ok &= atree.LoadCfg(fname);
  ok &= mood.LoadCfg(fname);
  ok &= emo.Defaults(fname);
  ok &= dmem.Defaults(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcAliaCore::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= mps.SaveVals(fname);
  ok &= rps.SaveVals(fname);
  ok &= atree.SaveCfg(fname);
  ok &= mood.SaveCfg(fname);
  ok &= emo.SaveVals(fname);
  ok &= dmem.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                                Extensions                             //
///////////////////////////////////////////////////////////////////////////

//= Add the name of a particular person to parsing and speech grammars.
// make sure to call Listen(1) afterward to re-engage speech recognition

int jhcAliaCore::GramName (const char *name, int bot)
{
  char first[80];
  char *sep;

  // get first name from full name
  if ((name == NULL) || (*name == '\0'))
    return 0;
  strcpy_s(first, name);
  if ((sep = strchr(first, ' ')) != NULL)
    *sep = '\0';

  // add full (or only) name to grammar and speech
  sp_listen(0);
  if (bot > 0)
    gram_add("ATTN", name, -1);        
  gram_add("NAME", name, -1);
  gram_add("NAME-P", (net.mf).SurfWord(name, JTAG_NAMEP), -1);

  // add just first name to grammar and speech
  if (sep == NULL)
    return 1;
  if (bot > 0)
    gram_add("ATTN", first, -1);      
  gram_add("NAME", first, -1);
  gram_add("NAME-P", (net.mf).SurfWord(first, JTAG_NAMEP), -1);
  return 1;
}


//= Add in new rule or operator suggested by user (typcially only one or the other).
// acceptance moved into jhcAliaDir to allow rejection if user disliked for some reason
// should NULL pointers in caller afterward if successful (amem or pmem will delete at end)
// returns 1 if successful, 0 or negative for problem (consider deleting explicitly)

int jhcAliaCore::Accept (jhcAliaRule *r, jhcAliaOp *p)
{
  char date[40];
  int ans = 1;

  if ((r == NULL) && (p == NULL))
    return -2;
  if (r != NULL)
  {
    sprintf_s(r->prov, "user -> %s at %s", formal, jms_date(date));
    if ((ans = amem.AddRule(r, 2, 1)) > 0)       
      mood.Infer();
  }
  if (p != NULL)
  {
    sprintf_s(p->prov, "user -> %s at %s", formal, jms_date(date));
    if ((ans = pmem.AddOperator(p, 1, 1)) > 0)
      mood.React();
  }
  return ans;
}


//= Create a low preference operator or low belief rule based on user command.
// gets most general context (most likely to run) that still constraints all refs
// returns 1 if successful, 0 or negative if not added for some reason

int jhcAliaCore::Speculate (jhcAliaChain *bulk, int spact)
{
  jhcGraphlet ctx, fact, refs, halt, refs2, used;
  int i, n, ok;

  // possibly announce entry
  if (spec <= 0)
    return -6;
  jprintf(2, spec, "\n==========================\n");
  jprintf(2, spec, "SPECULATE: \"%s\", gen = %d\n", net.SpeechAct(spact), atree.Version());

  // find items in rule condition (refs) and conclusion (fact) 
  // need to add "fact" into "halt" and remove "refs" from both
  bulk->CollectRefs(refs, halt, fact);
  fact.RemAll(refs);
  halt.RemAll(refs);
  halt.Append(fact);

  // for rule generation check "fact" and massage "refs" list
  if (spact == JSP_FACT)
  {
    if (fact.Empty())
      return -1;                       // barf if nothing to assert
    used.IncludeArgs(fact);
    refs2.Copy(refs);                  // because "refs" gets changed below
    n = refs2.NumItems();              
    for (i = 0; i < n; i++)
      if (!used.InList(refs2.Item(i)))
        refs.RemItem(refs2.Item(i));   // remove unused reference term
    if (refs.Empty())
      return -5;                       // barf if no triggering conditions 
  }

  // build thumbnail description of each item (all operators implicitly use self)
  if (refs.Empty()) 
  {
    jprintf(2, spec, "examining self ...\n");
    ok = gather_props(ctx, atree.Robot(), halt);
  }
  else
    ok = scour_facts(ctx, refs, halt);
  if (ok <= 0)           
  {
    jprintf(2, spec, "> some items have no description!\n");
    jprintf(2, spec, "==========================\n");
    return 0;                          
  }
  add_convo(ctx);                      // add "me" or "you" if needed

  // possibly show final rule/OP context
  if (spec >= 2)
  {
    jprintf("\n");
    ctx.ListAll("spec ctx");
    ctx.Print("situation");
    jprintf("==========================\n");
  }

  // try building a new rule or operator
  if (spact == JSP_FACT)
    ok = amem.BuildRule(ctx, fact);    // fact combines NOTEs
  else
    ok = pmem.BuildSpur(ctx, bulk);    // strip skolem from bulk
  return ok;
}


//= Search working memory for most recent fact(s) about "refs" and save in "ctx".
// returns 1 if all items have constraints, 0 if some item has no extra info 
// Note: erases any original contents of ctx

int jhcAliaCore::scour_facts (jhcGraphlet& ctx, const jhcGraphlet& refs, const jhcGraphlet& halt) const
{
  jhcGraphlet desc, xtra;
  jhcNetNode *item;
  int i, n = refs.NumItems();

  // go through each node in "refs" list
  ctx.Clear();
  for (i = 0; i < n; i++)
  {
    // get best unary and n-ary properties 
    item = refs.Item(i);
    jprintf(2, spec, "\nCONTEXT for %s ...\n", item->Nick());
    if (describe(desc, item, refs, halt) <= 0)
      gather_rels(xtra, item, refs, halt);

    // barf if no description for this item
    if (desc.Empty())
      return 0;
    ctx.Append(desc);
  }
  return 1;
}


//= Fill "spec" with a minimal description of "item".
// returns mru of newest part of spec if ok, 0 if predication with args in halt
// Note: erases any original contents of desc

int jhcAliaCore::describe (jhcGraphlet& desc, jhcNetNode *item, const jhcGraphlet& refs, const jhcGraphlet& halt) const
{
  jhcGraphlet xtra;
  jhcNetNode *arg;
  int i, mru, best, na = item->NumArgs();

  // for predication, quit if any argument invalid
  jprintf(2, spec, "  describe %s (%s): %d args\n", item->Nick(), item->LexStr(), na);
  desc.Clear();
  if (na > 0)
    for (i = 0; i < na; i++)
      if (halt.InList(item->Arg(i)))
      {
        jprintf(2, spec, "    arg in halt!\n");
        return 0;
      }
    
  // add best unary property (e.g. "ako" or "fcn") and possibly node itself
  best = gather_props(desc, item, halt);
  if ((na > 0) || (item->Lex() != NULL))
  {
    jprintf(2, spec, "    + add %s\n", item->Nick());
    desc.AddItem(item);
    best = __max(best, item->LastUsed());
  }

  // add decriptions of predication's arguments
  if (na > 0)
  {
    jprintf(2, spec, "  expand args of %s ...\n", item->Nick());
    for (i = 0; i < na; i++)
    {
      arg = item->Arg(i);
      if (!refs.InList(arg))           // check if redundant
        if ((mru = describe(xtra, arg, refs, halt)) > 0)
          desc.Append(xtra);
    }
  }
  jprintf(2, spec, "  ==> describe(%s) = %d\n", item->Nick(), best);
  return best;                         // zero if spec empty
}


//= Add valid unary properties of item to specification.
// skip property if on halt list, only keep if mru >= current best
// returns best mru if some non-empty description in spec, 0 otherwise
// Note: erases any original contents of desc
 
int jhcAliaCore::gather_props (jhcGraphlet& desc, const jhcNetNode *item, const jhcGraphlet& halt) const
{
  jhcGraphlet xtra;
  jhcNetNode *prop;
  int i, mru, n = item->NumProps(), best = 0;

  // find most recently used properties for item
  jprintf(2, spec, "    gather_props(%s)\n", item->Nick());
  desc.Clear();
  for (i = 0; i < n; i++)
  {
    // make sure valid and not older than best
    prop = item->Prop(i);
    if (prop->Hyp() || prop->Halo() || (prop->NumArgs() != 1) || halt.InList(prop) || !prop->Home(&atree))
      continue;
    jprintf(2, spec, "      consider prop %s (%s), mru = %d\n", prop->Nick(), prop->LexStr(), prop->LastUsed());
    if ((mru = prop->LastUsed()) < best)
      continue;

    // add property to item specification
    if (mru > best)
    {
      if (!desc.Empty())
        jprintf(2, spec, "        flush old spec\n");
      desc.Clear();                    
    }
    desc.AddItem(prop);
    jprintf(2, spec, "        + add %s\n", prop->Nick());
    best = mru;
  }

  // find most recently used properties of retained predicates (like "very")
  if ((n = desc.NumItems()) <= 0)
    return 0;                          // empty specification
  for (i = 0; i < n; i++)
    if (gather_props(xtra, desc.Item(i), halt) > 0)
      desc.Append(xtra);
  jprintf(2, spec, "    ==> props(%s) = %d\n", item->Nick(), best);
  return best;                         // non-zero because spec not empty
}


//= Add valid relation as well as its properties and arguments to specification.
// skip relation if on halt list, only keep if mru >= current best
// returns best mru if some non-empty description in spec, 0 otherwise
// Note: erases any original contents of desc

int jhcAliaCore::gather_rels (jhcGraphlet& desc, const jhcNetNode *item, const jhcGraphlet& refs, const jhcGraphlet& halt) const
{
  jhcGraphlet xtra;
  jhcNetNode *rel;
  int i, mru, n = item->NumProps(), best = 0;

  // find most recently used relations for item
  jprintf(2, spec, "gather_rels(%s)\n", item->Nick());
  desc.Clear();
  for (i = 0; i < n; i++)
  {
    // make sure relation is valid and not too old
    rel = item->Prop(i);
    if (rel->Hyp() || rel->Halo() || (rel->NumArgs() < 2) || halt.InList(rel) || !rel->Home(&atree))
      continue;
    jprintf(2, spec, " consider rel %s (%s), mru = %d\n", rel->Nick(), rel->LexStr(), rel->LastUsed());
    if ((mru = rel->LastUsed()) < best)
      continue;

    // build description and add to description
    if (describe(xtra, rel, refs, halt) <= 0)
      continue;                        // invalid arguments
    if (mru > best)
    {
      if (!desc.Empty())
        jprintf(2, spec, "        flush old spec\n");
      desc.Clear();                    // flush older relations
    }
    desc.Append(xtra);
    best = mru;
  }
  jprintf(2, spec, "==> rels(%s) = %d\n", item->Nick(), best);
  return best;                         // zero if specification empty
}


//= Make sure conversation participants (me, you) are fully specified.

void jhcAliaCore::add_convo (jhcGraphlet& ctx) const
{
  const jhcNetNode *me = atree.Robot(), *you = atree.Human();
  const jhcNetNode *item;
  jhcNetNode *arg;
  int i, j, na, n = ctx.NumItems();

  for (i = 0; i < n; i++)
  {
    item = ctx.Item(i);
    na = item->NumArgs();
    for (j = 0; j < na; j++)
    {
      arg = item->Arg(j);
      if ((arg == me) || (arg == you))
        ctx.AddItem(arg);                        // adds "-lex-"
    }
  }
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Set up directory to read configuration files from.
// makes sure there is a slash at end (use Dir() to get clean version) 

const char *jhcAliaCore::SetDir (const char *path)
{
  int n;

  *wdir = '\0';
  if (path != NULL)
  {
    strcpy_s(wdir, path);
    n = (int) strlen(path) - 1;
    if ((path[n] != '/') && (path[n] != '\\'))
      strcat_s(wdir, "/");
  }
  return wdir;
}


//= Prefix the given relative file name with the working directory.
// uses internal temporary string which might get re-used

const char *jhcAliaCore::wrt (const char *rel)
{
  sprintf_s(fname, "%s%s", wdir, rel);
  return fname;
}


//= Load up all operators, rules, and grammar fragments for next run.
// rname is robot name, prt controls printing, cvt saves parse trees
// prt: 0 = nothing, 1 = log file only, 2 = console only, 3 = log + console
// assumes base directory for configuration and log files already recorded

void jhcAliaCore::Reset (const char *rname, int prt, int cvt)
{
  char *sep;
  int i, n;

  // save full robot name (for learning provenance) and first name
  strcpy_s(formal, "Nemo Banzai");
  strcpy_s(myself, "Nemo");
  if ((rname != NULL) && (*rname != '\0'))
  {
    strcpy_s(formal, rname);
    strcpy_s(myself, rname);                  
    if ((sep = strchr(myself, ' ')) != NULL)
      *sep = '\0';
  }
  strcpy_s(amem.formal, formal);
  strcpy_s(pmem.formal, formal);

  // determine how to handle voluminous message stream
  log_opts(prt);
  *echo = '\0';                                  // canonicalized input

  // potentially add extra grounding functions (needs wdir)
  if (gnd <= 0)
  {
    add_dlls(wrt("GND/kernels.lst"));    
    gnd = 1;
  }

  // set basic grammar and closed-class words and clear state
  jprintf("Initializing ALIA core %4.2f\n\n", Version());
  gr.ClearGrammar();
  gr.LoadGram(wrt("language/alia_top.sgm"), -1);
  GramName(formal, 1);
  gr.SetBonus("ACT-2");                          // prefer these trees
  gr.MarkRule("toplevel");
  init_state();

  // possibly load consolidated OPS and rules
  if ((acc >= 2) && ExistKB())
    LoadKnowledge();
  else
    load_foundation();

  // add the names of important people (keeps "vip" list)
  n = vip.Load(wrt("config/VIPs.txt"), 0);
  for (i = 0; i < n; i++)
    GramName(vip.Full(i));
  jprintf("  %3d known user names  from: %s\n\n", n, wrt("config/VIPs.txt"));

  // catalog known words and start graphizer log
  vc.GetWords(gr.Expansions());
  if (cvt > 0)
    open_cvt(rname);

  // allow printf batching for speed (in case overridden)
  jprintf_fflush = 0;
  *stamp = '\0';
}


//= Control printing of messages to console and/or log file.
// prt: 0 = nothing, 1 = log file only, 2 = console only, 3 = log + console
// NOTE: "log" directory under working directory must already exist!

void jhcAliaCore::log_opts (int prt)
{
  char fname[200], date[80];

  // possibly suppress all console printout 
  jprintf_log((prt < 2) ? 1 : 0);
  
  // possibly open a log file using robot's first name and date/time
  if ((prt == 1) || (prt >= 3))
  {
    if (*myself != '\0')
      sprintf_s(fname, "%slog/%s_%s.txt", wdir, myself, jms_date(date));
    else
      sprintf_s(fname, "%slog/log_%s.txt", wdir, jms_date(date));
    if (jprintf_open(fname) <= 0)
      printf("  >>> Could not open main log file: %s !\n", fname + strlen(wdir));
  }
}


//= Load grounding DLLs and associated operators from a list of names.
// all the routines must be associated with the same "soma" (RWI) instance
// "GND/kernels.lst" file contains a list like:
//   sound_fcn
//   basic_act
// so sound_fcn.dll and basic_act.dll should be in this same directory 
// DLLs must supply functions listed in sample Action/alia_gnd.h
// returns number of additional DLLs loaded, negative for problem

int jhcAliaCore::add_dlls (const char *fname)
{
  char dir[80], line[80], name[200];
  jhcAliaDLL *gnd = NULL;
  FILE *in;
  char *trim, *end;
  int cnt = 0;

  // try to open file 
  if (fopen_s(&in, fname, "r") != 0)
    return 0;

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
    // see if valid line (minus any newline marker)
    trim = line;
    while (*trim == ' ')
      trim++;
    trim[strcspn(trim, "\n\r\x0A")] = '\0';
    if ((*trim == '\0') || (strncmp(trim, "//", 2) == 0))
      continue;

    // attempt to bind DLL 
    if (gnd == NULL)
      gnd = new jhcAliaDLL;
    sprintf_s(name, "%s%s.dll", dir, trim);
    if (gnd->Load(name) > 0)
    {
      // add associated operators and rules then link things up
      kern.AddFcns(*gnd);
      gnd = NULL;
      cnt++;
    }
  }

  // clean up
  fclose(in);
  delete gnd;                          // in case last one not used
  return cnt;
}


//= Load OPs and rules for grounding kernels plus those in KB2/baseline.lst file.

void jhcAliaCore::load_foundation ()
{   
  // add open-class words 
  (net.mf).AddVocab(gr, wrt("language/vocabulary.sgm"), 1, 0);

  // possibly load test LTM facts then support for groundings 
  if (dmem.LoadFacts(wrt("KB2/test.facts"), 0, 3, 0) >= 0)
    jprintf("\n"); 
  kern_extras(wrt("KB0/"));                    // operators and rules

  // load main operators and rules (and associated words)
  baseline(wrt("KB2/baseline.lst"), 1, 2);     // includes graphizer.sgm
  if (vol > 0)
    baseline(wrt("KB2/volition.lst"), 1, 2);
  if (acc >= 1)
    LoadLearned();                             // includes KB/extras.sgm
}


//= Loads grammars, rules, and operators associated with current kernels.
// each kernel has a BaseTag like "Social" and then files *.sgm, *.ops, and *.rules
// grammar for speech altered separately (jhcAliaSpeech::kern_gram)

void jhcAliaCore::kern_extras (const char *kdir)
{
  const jhcAliaKernel *k = &kern;
  const char *tag; 
  int nr0 = amem.NumRules(), nop0 = pmem.NumOperators(), nw0 = gr.OpenClass();

  jprintf(1, noisy, "Loading kernel rules and operators:\n");
  while (k != NULL)
  {
    // read files based on tags in each kernel class (0 = kernel level)
    tag = k->BaseTag();
    if (*tag != '\0')
      add_info(kdir, tag, noisy, 0);
    k = k->NextPool();
  }
  jprintf(1, noisy, " TOTAL = %d operators, %d rules, %d words\n\n", 
          pmem.NumOperators() - nop0, amem.NumRules() - nr0, gr.OpenClass() - nw0);
}


//= Read in lexical terms, operators, and rules associated with base string.
// "rpt" is for messages, "lvl" is marking for additions: 0 = kernel, 1 = extras
// returns total number of files read

int jhcAliaCore::add_info (const char *dir, const char *base, int rpt, int lvl)
{
  char fname[200];
  int cnt = 0;

  if (readable(fname, 200, "%s%s.ops", dir, base))
    if (pmem.Load(fname, 1, rpt, lvl) > 0)      
      cnt++;
  if (readable(fname, 200, "%s%s.rules", dir, base))
    if (amem.Load(fname, 1, rpt, lvl) > 0)      
      cnt++;
  if (readable(fname, 200, "%s%s_v.rules", dir, base))
    if (amem.Load(fname, 1, rpt, lvl) > 0)      
      cnt++;
  if (readable(fname, 200, "%s%s.sgm", dir, base))
    if ((net.mf).AddVocab(gr, fname, rpt, lvl) > 0)
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
  va_end(args);
  if (fopen_s(&in, fname, "r") != 0)
    return false;
  fclose(in);
  return true;
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

int jhcAliaCore::baseline (const char *list, int add, int rpt)
{
  char dir[80], line[80];
  FILE *in;
  char *end;
  int n, r0 = amem.NumRules(), op0 = pmem.NumOperators(), cnt = 0, nw0 = gr.OpenClass();

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
      if (strchr(" \t\n\r\x0A", line[n++]) == NULL)
        end = line + n;
    if (end == line)
      continue;
    *end = '\0';

    // read various types of input (1 = extras level)
    cnt += add_info(dir, line, rpt, 1);        
  } 

  // clean up
  fclose(in);
  jprintf(1, rpt, " TOTAL = %d operators, %d rules, %d words\n\n", 
          pmem.NumOperators() - op0, amem.NumRules() - r0, gr.OpenClass() - nw0);
  return cnt;
}


//= Make sentence to internal directives conversion log file.

void jhcAliaCore::open_cvt (const char *rname)
{
  char date[80], fname[80], first[80] = "log";
  char *sep;

  // pick output file name
  if (*cfile != '\0')
    strcpy_s(fname, cfile);
  else
  {
    if ((rname != NULL) && (*rname != '\0'))
    {
      strcpy_s(first, rname);                  
      if ((sep = strchr(first, ' ')) != NULL)
        *sep = '\0';
    }
    sprintf_s(fname, "%slog/%s_%s.cvt", wdir, first, jms_date(date)); 
  }

  // try opening it
  if (netlog != NULL)
    fclose(netlog);
  netlog = NULL;
  if (fopen_s(&netlog, fname, "w") == 0)
    return;
  netlog = NULL;
  printf("  >>> Could not open conversion log file: %s !\n", fname + strlen(wdir));
}


//= Process input sentence from some source.
// if gate == 0 then usually needs to hear attention word to process sentence
// amode: -1 text, 0 not needed, 1 front or back, 2 front, 3 by itself
// returns 2 if attention found, 1 if understood, 0 if unintelligible

int jhcAliaCore::Interpret (const char *input, int gate, int amode)
{
  char alist[1000] = "";
  const char *fix, *sent = alist;
  int wake = 0, nt = 0;

  // sanity check 
  if ((input == NULL) || (*input == '\0'))
    return 0;
  sent = gr.Expand(input, 1);          // undo contractions  
  hear0 = 0;

  // try to parse if reasonable
  wake = gr.NameSaid(sent, amode);     // will trigger on unparsable "robot fizzboom"
//  if ((gate == 0) && (wake <= 0))                          
//    return jprintf(1, noisy, " { Ignored input: \"%s\" }\n", input);
  if ((amode >= 0) && !syllables(sent, 2))         
    return jprintf(1, noisy, " { Too few syllables in: \"%s\" }\n", input);
  nt = gr.Parse(sent, 0);

  // try fixing typing errors for unparsable text inputs 
  if ((nt <= 0) && (amode < 0))
    if ((fix = vc.FixTypos(sent)) != NULL)     
    {
      sent = fix;
      nt = gr.Parse(sent, 0);
      if (nt > 0)
        jprintf(1, noisy, " { Fixed typos in original: \"%s\" }\n", gr.NoContract());
    }

  // try guessing unknown words for unparsable robot-directed inputs 
  if ((nt <= 0) && ((amode < 0) || (gate > 0) || (wake > 0)))          
    if (guess_cats(sent) > 0)          
      if ((nt = gr.Parse(sent, 0)) <= 0)
        gram_rollback();                         // remove any additions

  // possibly just ignore unparsable inputs
  if ((nt <= 0) && (gate <= 0) && (wake <= 0))
    return jprintf(1, noisy, " { Ignored input: \"%s\" }\n", input);

  // condense parse tree into association list
  if (nt > 0)
  {
    gr.AssocList(alist, 1);
//    wake = net.NameSaid(alist, amode);   // will NOT trigger on unparsable "robot fizzboom"
//    if ((gate == 0) && (wake <= 0))             
//      return jprintf(1, noisy, " { Ignored input: \"%s\" }\n", input);
  }

  // get canonicalized form of input for logs (incl. unparsable)
  if ((gate > 0) || (wake > 0) || (nt > 0))
  {
    if (nt > 0)
      strcpy_s(echo, gr.Clean());                // with typos fixed
    else 
      strcpy_s(echo, vc.Marked());               // with unknown words marked
    echo[0] = (char) toupper((int)(echo[0]));    // capitalize first letter
    if (input[strlen(input) - 1] == '?')
      if (echo[strlen(echo) - 1] != '?')
        strcat_s(echo, "?");                     // save question mark
  }

  // show parsing steps and reduce "lonely" 
  gr.PrintInput(NULL, echo, __min(noisy, 1));
  if (nt > 0)
  {
    mood.Hear((int) strlen(input));     
    gr.PrintResult(pshow, 1);
  }

  // generate semantic nets (nt = 0 gives huh? response)
jtimer(17, "Convert");
  spact = net.Convert(alist, sent);     
jtimer_x(17);
  net.Summarize(netlog, echo, nt, spact);
//  return((wake > 0) ? 2 : 1);
  return 2;                                      // valid input always wakes
}


//= Try to identifying unknown open-class words from morphology and context.
// returns number fixed 

int jhcAliaCore::guess_cats (const char *sent)
{
  char wd[40];
  const char *txt = sent;
  int cat, cnt = 0;

  // save current grammar state (with no speculative additions)  
  gr.SaveCats(wrt("KB/checkpoint.sgm"), 0, net.mf);

  // go through the input looking for unknown words
  vc.InitGuess();
  while ((txt = vc.NextGuess(txt)) != NULL)
  {
    // retrieve guess about the category of some word
    jprintf(1, noisy, " { Adding \"%s\" to grammar %s category }\n", vc.Mystery(), vc.Category());
    if (cnt++ <= 0)
      sp_listen(0);
    cat = (net.mf).GramBase(wd, vc.Mystery(), vc.Category());

    // explicitly add morphological variants for some categories
    if (cat == JTV_NAME)
    {
      gram_add("NAME", wd, 3);
      gram_add("NAME-P", (net.mf).SurfWord(wd, JTAG_NAMEP), 3);      // possessive
    }
    else if (cat == JTV_NSING)
    {
      gram_add("AKO", wd, 3);
      gram_add("AKO-S", (net.mf).SurfWord(wd, JTAG_NPL), 3);         // plural
      gram_add("AKO-P", (net.mf).SurfWord(wd, JTAG_NPOSS), 3);       // possessive
    }
    else if (cat == JTV_APROP)
    {
      gram_add("HQ", wd, 3);
      gram_add("HQ-ER",  (net.mf).SurfWord(wd, JTAG_ACOMP), 3);      // comparative
      gram_add("HQ-EST", (net.mf).SurfWord(wd, JTAG_ASUP), 3);       // superlative
    }
    else if (cat == JTV_VIMP)
    {
      gram_add("ACT", wd, 3);
      gram_add("ACT-S", (net.mf).SurfWord(wd, JTAG_VPRES), 3);       // present tense
      gram_add("ACT-D", (net.mf).SurfWord(wd, JTAG_VPAST), 3);       // past tense
      gram_add("ACT-G", (net.mf).SurfWord(wd, JTAG_VPROG), 3);       // progressive
    }
    else if (cat == JTV_ADV)
    {
      gram_add("MOD", wd, 3);
      gram_add("HQ", (net.mf).BaseWord(wd, wd, JTAG_ADV), 3);        // quickly -> quick (+ others?)                                        
    }
    else
      gram_add(vc.Category(), vc.Mystery(), 3);                      // possibly robot name (ATTN)
  }
  if (cnt > 0)
    sp_listen(1);
  return cnt;
}


//= Restore previous state of the grammar effectively removing any new words added.

void jhcAliaCore::gram_rollback ()
{
  // possibly announce
  jprintf(1, noisy, " { Rollback grammar additions! }\n");

  // erase everything then reload base grammar
  gr.ClearGrammar();
  gr.LoadGram(wrt("language/alia_top.sgm"), -1);
  GramName(formal, 1);
  gr.SetBonus("ACT-2");                          // should not be needed
  gr.MarkRule("toplevel");

  // restore original open-class words
  (net.mf).AddVocab(gr, wrt("KB/checkpoint.sgm"), 0, 2);
}


//= See if enough syllables heard as an aid to rejecting spurious noises (e.g. "spin").
// counts vowel clusters ("ia" = 2) but adjusts for word breaks, leading "y", and final "e"
// returns 1 if count meets threshold, 0 otherwise
// NOTE: problems with answers like "yes", interjections like "stop"?

int jhcAliaCore::syllables (const char *txt, int th) const
{
  const char *t = txt;
  char t0, v = '\0';
  int sp = 1, n = 0;

  // scan through lowercase string
  while (*t != '\0')
  {
    t0 = (char) tolower(*t++);
    if ((strchr("aiou", t0) != NULL) ||
        ((t0 == 'e') && (isalpha(*t) != 0)) ||
        ((t0 == 'y') && (sp <= 0)))
    {
      // vowel cluster (except leading "y" or final "e", split "ia")
      if ((v == '\0') || ((v == 'i') && (t0 == 'a')))
        if (++n >= th)
          return 1;
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
  return 0;
}


//= Consider next best parse tree to generate a new bulk sequence for TRY directive.
// returns NULL if no other parses with same speech act as original

jhcAliaChain *jhcAliaCore::Reinterpret ()
{
  char alist[1000] = "";

  // check for fact, command, or question
  if ((spact == JSP_FACT) || (spact == JSP_CMD) || (spact == JSP_YNQ) ||
      (spact == JSP_WHQ)  || (spact == JSP_EXQ) || (spact == JSP_FIND))
    while (gr.NextBest() >= 0)
      if (net.Assemble(gr.AssocList(alist, 1)) == spact)
      {   
        jprintf(1, noisy, "\n@@@ switch to parser Tree %d:\n\n", gr.Selected());
        jprintf(1, noisy, "  --> %s\n\n", gr.NoTabs(alist));
        return net.TrySeq();
      }
  return NULL;
}


//= Run all focal elements in priority order.
// must mark all seed nodes to retain before calling with gc > 0
// member variable "svc" set to current focus and "bid" to appropriate value
// tells how many foci were processed on this cycle

int jhcAliaCore::RunAll (int gc)
{
  jhcAliaChain *s;
  int res, cnt = 0;

  // possibly timestamp
  jprintf(4, noisy, "\nSTEP %d ----------------------------------------------------\n\n", atree.Version());  
  if ((gc > 0) && (strcmp(stamp, jms_time(time)) != 0))
  {
    strcpy_s(stamp, time);
    jprintf("\n[%s] --------------------------------------------------------------\n\n", stamp);
  }

  // get any observations, check expired attentional foci, and recompute halo
  kern.Volunteer();
  if (atree.Update(gc) > 0)                      // also if bth or node blfs change?
  {
jtimer(18, "DejaVu");
    dmem.DejaVu();                               // (re-)tether objects to LTM
jtimer_x(18);
    atree.ClearHalo();
    dmem.GhostFacts();                           // add in proximal LTM facts
jtimer(19, "RefreshHalo");
    amem.RefreshHalo(atree, noisy - 1);          // apply all rules
jtimer_x(19);
  }
  if (gc > 0)
  {
    now = jms_now();
    stat.Affect(mood);    
    stat.Thought(this);                        
    mood.Update();    
    atree.Niggle(mood.Focused());                          
  }
//  if (atree.Active() > 0)
//    jprintf(3, noisy, "============================= %s =============================\n\n", jms_offset(time, t0, 1));

  // go through the foci from newest to oldest
  while ((svc = atree.NextFocus()) >= 0)
  {
    jprintf(2, noisy, "\n-- servicing focus %d\n", svc);
    s = atree.FocusN(svc);
    bid = atree.BaseBid(svc);
    if (atree.NeverRun(svc))
      res = s->Start(this, 0);
    else
      res = s->Status();
    atree.SetActive(s, ((res == 0) ? 1 : 0));    // replacement might have occurred
    cnt++;
  }
  return cnt;
}


//= Perform several cycles of reasoning disconnected from sensors and actuators.
// quits early if some Run() takes too long to prevent missing sensor schedule

void jhcAliaCore::DayDream ()
{
  double budget = 0.9, turbo = 4.0, frac = 1.0;
  int ms = ROUND(1000.0 * budget / shz);         // time limit for this call
  int melt, cyc, n = 1; 

jtimer(20, "DayDream");
  // determine how many total thought cycles to run right now
  if (start == 0)
    start = now;                                 // for total run time stats
  else 
  {
    frac = thz * jms_secs(now, last) + rem;      // number needed to catch up
    n = ROUND(frac);                             // ideal number to do now
    melt = ROUND(turbo * thz / shz);             // cognition speed limit
    n = __min(n, melt);
  }
  last = now;                                    // time thinking call happened

  // possibly catch up on thinking (output commands will be ignored)
  for (cyc = 1; cyc < n; cyc++)                  // already did one
  {
    if (jms_diff(jms_now(), last) >= ms)         // quit early if too long
      break;
    RunAll(0);                                   // think some more
  }
  rem = frac - cyc;                              // cycles NOT completed
  think += cyc;
  sense++;                                       // original call had sensors

  // make sure printfs captured by setvbuf() get emitted
  fflush(stdout);                    
jtimer_x(20);
}


//= Current run is done so shut things down smoothly.
// if batt >= 0 then prints charge level message

void jhcAliaCore::Done (int save, int batt)
{
  // stop all running activities
  stop_all();

  // close any open graphizer tree log
  if (netlog != NULL)
    fclose(netlog);
  netlog = NULL;

  // possibly save all operators and rules in KB files
  if ((save > 0) && (acc > 0))
  {
    if (acc >= 2)
      DumpKnowledge();
    else 
      DumpLearned();
  }

  // report final memory contents (fflush for setvbuf in console)
  jprintf("\n==========================================================\n");
  ShowMem();
  jprintf("DONE - Think %3.1f Hz, Sense %3.1f Hz\n", Thinking(), Sensing()); 
  if (batt >= 0)
    jprintf("\nbattery = %d%%\n", batt);
  fflush(stdout);
  jprintf_close();
}


//= Stop all running activities (order is arbitrary).

void jhcAliaCore::stop_all ()
{
  jhcAliaChain *s;
  int i, nf = atree.NumFoci();

  for (i = 0; i < nf; i++)
  {
    s = atree.FocusN(i);
    s->Stop();
  }
  atree.ClrFoci();
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


//= Find all valid operators matches for given directive.
// returns number found, variable bindings recorded in directive itself

int jhcAliaCore::GetChoices (jhcAliaDir *d)
{
  int n;

  // basic work
  n = pmem.FindOps(d, atree, atree.MinPref(), atree.MinBlf());

  // possibly lower operator threshold
  if ((n <= 0) && (d->anyops > 0))
    mood.OpBelow();         
  return n;
}


///////////////////////////////////////////////////////////////////////////
//                             Halo Control                              //
///////////////////////////////////////////////////////////////////////////

//= Assign all nodes from this NOTE a unique source marker.
// marks all nodes in graphlet with special NOTE id (always increases)
// ignores objects because they carry no intrinsic semantic value

int jhcAliaCore::Percolate (const jhcGraphlet& dkey)
{
  jhcNetNode *n;
  int i, ni = dkey.NumItems(), tval = ++topval;

  for (i = 0; i < ni; i++)
    if ((n = dkey.Item(i)) != NULL)
      if (n->top < tval)      // need object marking for ghost facts
      {
        n->top = tval;
        atree.Dirty();        // queue recomputation of halo
      }
  return tval;
}


///////////////////////////////////////////////////////////////////////////
//                          External Grounding                           //
///////////////////////////////////////////////////////////////////////////

//= Start up some kernel function given by lex of fcn node.
// "bid" member variable set automatically when servicing related focus
// returns instance number (zero okay), negative for problem

int jhcAliaCore::GndStart (const jhcNetNode *fcn)
{
  if (fcn == NULL)
    return -1;
  jprintf(2, noisy, "G-START %s \"%s\" @ %d\n\n", fcn->Nick(), fcn->Lex(), bid);
  return kern.Start(*fcn, bid);
}


//= Check if kernel procedure done yet.
// translates to/from kernel Call function
// returns 1 if successful, 0 if still working, -2 for fail

int jhcAliaCore::GndStatus (const jhcNetNode *fcn, int inst) 
{
  int res = -2;

  if (fcn == NULL)
    return -2;
  jprintf(2, noisy, "G-STATUS %s \"%s\"\n", fcn->Nick(), fcn->Lex());
  if (inst >= 0)
    res = kern.Status(*fcn, inst);
  if (res == 0)
    jprintf(2, noisy, "  -> kernel continue ...\n");
//  jprintf(2, noisy, "  -> GND %s\n\n", ((res > 0) ? "success !" : ((res < 0) ? "FAIL" : "continue ...")));
  return((res < 0) ? -2 : res);
}


//= Tell some instance of a kernel function to stop.
// returns -1 always for convenience

int jhcAliaCore::GndStop (const jhcNetNode *fcn, int inst)
{
  if (fcn == NULL)
    return -1;
  jprintf(2, noisy, "\nG-STOP %s \"%s\"\n\n", fcn->Nick(), fcn->Lex());
  kern.Stop(*fcn, inst);
  return -1;
}


///////////////////////////////////////////////////////////////////////////
//                              Debugging                                //
///////////////////////////////////////////////////////////////////////////

//= Tell the name of all grounding kernels currently in system.

void jhcAliaCore::KernList () const
{
  const jhcAliaKernel *k = &kern;
  const char *tag;

  jprintf("Grounding kernels:\n");
  while (k != NULL)
  {
    tag = k->BaseTag();
    jprintf("  %s\n", ((*tag == '\0') ? "(EchoFcn)" : tag));
    k = k->NextPool();
  }
}


//= Test that all "KB/knowledge.*" files exist (may be empty).

bool jhcAliaCore::ExistKB () const
{
  char ext[4][10] = {"ops", "rules", "facts", "sgm"};
  char fname[80];
  FILE *in;
  int i;

  for (i = 0; i < 4; i++)
  {
    sprintf_s(fname, "%sKB/knowledge.%s", wdir, ext[i]);
    if (fopen_s(&in, fname, "r") != 0)
      return false;
    fclose(in);
  }
  return true;
}


//= Load ALL rules and operators.
// always loads whatever is in the "knowledge" version of files

void jhcAliaCore::LoadKnowledge ()
{
  jprintf(1, noisy, "Reloading consolidated knowledge:\n");
  pmem.Load(wrt("KB/knowledge.ops"), 0, noisy, 2);         // 2 = accumulated level
  amem.Load(wrt("KB/knowledge.rules"), 0, noisy, 2);   
  dmem.LoadFacts(wrt("KB/knowledge.facts"), 0, noisy, 2); 
  (net.mf).AddVocab(gr, wrt("KB/knowledge.sgm"), noisy, 2);
  jprintf(1, noisy, "\n");
}


//= Save ALL rules and operators.
// saves a copy with time and date stamp as well as "knowledge" version
// similar to DumpAll()

void jhcAliaCore::DumpKnowledge ()
{
  char base[80];
  int n, nop, nr, nf, nw;

  // build output file name
  jprintf(1, noisy, "\nSaving consolidated knowledge:\n");
  sprintf_s(base, "%sKB/%s_", wdir, myself);
  n = (int) strlen(base);
  jms_date(base + n, 0, 80 - n);

  // save rules and operators  
  nop = pmem.Save(base, 0);            // 0 = all sources
  nr = amem.Save(base, 0);                                 
  nf = dmem.SaveFacts(base, 0);
  nw = gr.SaveCats(base, 0, net.mf);

  // make copies as generic database
  copy_file(wrt("KB/knowledge.ops"),   base);
  copy_file(wrt("KB/knowledge.rules"), base);
  copy_file(wrt("KB/knowledge.facts"), base);
  copy_file(wrt("KB/knowledge.sgm"),   base);
  jprintf(1, noisy, " TOTAL = %d operators, %d rules, %d facts, %d words\n", nop, nr, nf, nw);
}


//= Load all rules and operators BEYOND baseline and kernels.
// always loads whatever is in the "extras" version of files

void jhcAliaCore::LoadLearned ()
{
  jprintf(1, noisy, "Reloading non-KB0/KB2 learned knowledge:\n");
  pmem.Load(wrt("KB/extras.ops"), 1, noisy, 2);            // 2 = accumulated level
  pmem.Overrides(wrt("KB/extras.pref"));
  amem.Load(wrt("KB/extras.rules"), 1, noisy, 2);         
  amem.Overrides(wrt("KB/extras.conf"));
  dmem.LoadFacts(wrt("KB/extras.facts"), 1, noisy, 2);  
  (net.mf).AddVocab(gr, wrt("KB/extras.sgm"), noisy, 2);
  jprintf(1, noisy, "\n");
}


//= Save all rules and operators BEYOND baseline and kernels.
// saves a copy with time and date stamp as well as "learned" version

void jhcAliaCore::DumpLearned ()
{
  char base[80];
  int n, nop, nr, nf, nw;

  // build output file name
  jprintf(1, noisy, "\nSaving non-KB0/KB2 learned knowledge:\n");
  sprintf_s(base, "%sKB/add_", wdir);
  n = (int) strlen(base);
  jms_date(base + n, 0, 80 - n);

  // save rules and operators  
  nop = pmem.Save(base, 2);            // 2 = accumulated level
  pmem.Alterations(base);
  nr = amem.Save(base, 2);                                 
  amem.Alterations(base);   
  nf = dmem.SaveFacts(base, 2);
  nw = gr.SaveCats(base, 2, net.mf);

  // make copies as generic database
  copy_file(wrt("KB/extras.ops"),   base);
  copy_file(wrt("KB/extras.pref"),  base);
  copy_file(wrt("KB/extras.rules"), base);
  copy_file(wrt("KB/extras.conf"),  base);
  copy_file(wrt("KB/extras.facts"), base);
  copy_file(wrt("KB/extras.sgm"),   base);
  jprintf(1, noisy, " TOTAL = %d operators, %d rules, %d facts, %d words\n", nop, nr, nf, nw);
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
// NOTE: directory "dump" must already exist!

void jhcAliaCore::DumpSession () 
{
  pmem.Save(wrt("dump/session.ops"), 3);
  amem.Save(wrt("dump/session.rules"), 3);
  dmem.SaveFacts(wrt("dump/session.facts"), 3);
  gr.SaveCats(wrt("dump/session.sgm"), 3, net.mf);
//atree.Save(wrt("dump/session.wmem"));
}


//= Save all rules and operators from any source.
// NOTE: directory "dump" must already exist!

void jhcAliaCore::DumpAll ()
{
  pmem.Save(wrt("dump/all.ops"), 0);
  amem.Save(wrt("dump/all.rules"), 0);
  dmem.SaveFacts(wrt("dump/all.facts"), 0);
  gr.SaveCats(wrt("dump/all.sgm"), -1, net.mf);
}
