// jhcAliaCore.cpp : top-level coordinator of components in ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
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

#include <stdarg.h>

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
  ver = 5.10;                // reflected in GUI
  vol = 1;                   // enable free will reactions
  acc = 0;                   // forget rules/ops

  // connect up required resources for components
  net.Bind(this);
  talk.Bind(this);        
  dmem.Bind(atree);
  mood.Bind(atree);
  ltm.Bind(dmem);
  fb.BindMood(mood);

  // add various common grounding kernels to list
  kern.AddFcns(talk);
  kern.AddFcns(ltm);
  kern.AddFcns(why);
  kern.AddFcns(fb);
  kern.AddFcns(tim);
  add_dlls("GND/kernels.lst");

  // standard cycle timing
  thz = 80.0;
  shz = 30.0;

  // clear state
  *cfile = '\0';
  log = NULL;
  Defaults();
  init_state(NULL);
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
    end = trim + (int) strlen(trim) - 1;
    if (*end == '\n')
      *end = '\0';
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


//= Clear out all focal items and working memory.
// possibly starts up input conversion log file also

void jhcAliaCore::init_state (const char *rname)
{
  // clear action tree
  stop_all();
  atree.ResetFoci(rname);    // adds -name-> prop
  kern.Reset(atree);
  stat.Reset();
  mood.Reset();
  topval = 0;
  spact = 0;

  // possibly forget all rules and operators
  amem.ClearRules();
  pmem.ClearOps();

  // reset affective modulation
  det   = 1.0;
  argh  = 1.0;               // secs
  waver = 5.0;               // secs
  deep  = 20;

  // communicate debugging level
  atree.noisy = noisy;
  pmem.noisy = noisy;
  amem.noisy = noisy;
  dmem.noisy = noisy;
  mood.noisy = noisy;

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

//= Parameters for standard sensing and thinking rates.

int jhcAliaCore::rate_params (const char *fname)
{
  jhcParam *ps = &rps;
  int ok;

  ps->SetTag("core_rate", 0);
  ps->NextSpecF( &thz, 80.0, "Thought cycle rate (Hz)");   
  ps->NextSpecF( &shz, 30.0, "Default body rate (Hz)");    
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for selecting which console messages are displayed.

int jhcAliaCore::msg_params (const char *fname)
{
  jhcParam *ps = &mps;
  int ok;

  ps->SetTag("core_msg", 0);
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


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcAliaCore::Defaults (const char *fname)
{
  int ok = 1;

  ok &= rate_params(fname);
  ok &= msg_params(fname);
  ok &= atree.LoadCfg(fname);
  ok &= mood.LoadCfg(fname);
  ok &= dmem.Defaults(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcAliaCore::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= rps.SaveVals(fname);
  ok &= mps.SaveVals(fname);
  ok &= atree.SaveCfg(fname);
  ok &= mood.SaveCfg(fname);
  ok &= dmem.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                                Extensions                             //
///////////////////////////////////////////////////////////////////////////

//= Add the name of a particular person to parsing and speech grammars.
// make sure to call Listen(1) afterward to re-engage speech recognition

int jhcAliaCore::AddName (const char *name, int bot)
{
  char first[80];
  char *sep;

  // get first name from full name
  if ((name == NULL) || (*name == '\0'))
    return 0;
  strcpy_s(first, name);
  if ((sep = strchr(first, ' ')) != NULL)
    *sep = '\0';

  // update parsing grammar and possibly speech front-end also 
  sp_listen(0);
  if (bot > 0)
    gram_add("ATTN", name, 0);
  gram_add("NAME", name, 0);
  gram_add("NAME-P", (net.mf).SurfWord(name, JTAG_NAMEP), 0);
  if (sep != NULL)
  {
    if (bot > 0)
      gram_add("ATTN", first, 0);
    gram_add("NAME", first, 0);
    gram_add("NAME-P", (net.mf).SurfWord(first, JTAG_NAMEP), 0);
  }
  return 1;
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
    if ((ans = amem.AddRule(r, 2, 1)) > 0)       
      mood.Infer();
  }
  if (p != NULL)
  {
    if ((ans = pmem.AddOperator(p, 1)) > 0)
      mood.React();
  }
  return ans;
}

/*
//= Generate a new version of operator used by "src" in which action "mark" is replaced by "subst".

int jhcAliaCore::OpEdit (const jhcAliaDir *src, const jhcAliaDir *mark, const jhcBindings& map, const jhcAliaDir *subst)
{
  if ((src == NULL) || (mark == NULL) || (subst == NULL))
    return -1;

jhcBindings w2o;
const jhcBindings *inst = src->LastVars();
const jhcBindings *scope = (mark->step)->Scope();
const jhcNetNode *k0, *sub, *var, *k;
int i, nb = map.NumPairs();

jprintf("build wmem -> op bindings:\n");
for (i = 0; i < nb; i++)
{
  // undo any FINDs (invert scope)
  k0 = map.GetKey(i);
  sub = map.GetSub(i);

  if ((var = scope->FindKey(sub)) == NULL)       // THESE
    var = sub;                                   // COULD BE A SINGLE 
  k = inst->FindKey(var);  // but this is const  // FORWARD MAP

//src->LastVars(inst, dir);
//AddVar(op, mark, subst, 

// could build map
  
  jprintf("  %s -map-> %s -inv_guess-> %s -inv_method-> %s\n", k0->Nick(), sub->Nick(), var->Nick(), k->Nick());
// needs last step of [old-op(k) to new-op(k2)]
}

return 0;

  return pmem.AddVariant(src->LastOp(), mark, subst, &w2o, 1);    
//  return pmem.AddVariant(src->LastOp(), mark, subst, src->LastVars(), 1);    
}
*/

///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Load up all operators, rules, and grammar fragments for next run.

void jhcAliaCore::Reset (const char *rname, int cvt)
{
  // set basic grammar and clear state
  jprintf("Initializing ALIA core %4.2f\n\n", Version());
  gr.ClearGrammar();
  gr.LoadGram("language/alia_top.sgm", -1);
  AddName(rname, 1);
  gr.MarkRule("toplevel");
  (net.mf).AddVocab(gr, "language/vocabulary.sgm", 0, -1);
  init_state(rname);

  // possibly some test LTM facts then support for groundings 
  if (dmem.LoadFacts("test.facts", 0, 3, 0) >= 0)
    jprintf("\n"); 
  kern_extras("KB0/");                           // operators and rules

  // load main operators and rules (and words)
  baseline("KB2/baseline.lst", 1, 2);            // includes graphizer.sgm
  if (vol > 0)
    baseline("KB2/volition.lst", 1, 2);
  if (acc >= 1)
    LoadLearned();                               // includes KB/learned.sgm

  // catalog known words and start graphizer log
  vc.GetWords(gr.Expansions());
  if (cvt > 0)
    open_cvt();
}


//= Loads grammars, rules, and operators associated with current kernels.
// each kernel has a BaseTag like "Social" and then files *.sgm, *.ops, and *.rules
// grammar for speech altered separately (jhcAliaSpeech::kern_gram)

void jhcAliaCore::kern_extras (const char *kdir)
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


//= Read in lexical terms, operators, and rules associated with base string.
// "rpt" is for messages, "lvl" is marking for additions: 0 = kernel, 1 = extras
// returns total number of files read

int jhcAliaCore::add_info (const char *dir, const char *base, int rpt, int lvl)
{
  char fname[200];
  int cnt = 0;

  if (readable(fname, 200, "%s%s.sgm", dir, base))
    if ((net.mf).AddVocab(gr, fname, 0, lvl) > 0)
      cnt++;
  if (readable(fname, 200, "%s%s.ops", dir, base))
    if (pmem.Load(fname, 1, rpt, lvl) > 0)      
      cnt++;
  if (readable(fname, 200, "%s%s.rules", dir, base))
    if (amem.Load(fname, 1, rpt, lvl) > 0)      
      cnt++;
  if (readable(fname, 200, "%s%s_v.rules", dir, base))
    if (amem.Load(fname, 1, rpt, lvl) > 0)      
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


//= Make sentence conversion log file.

void jhcAliaCore::open_cvt ()
{
  char date[80], fname[80];

  // pick output file name
  if (*cfile != '\0')
    strcpy_s(fname, cfile);
  else
    sprintf_s(fname, "log/log_%s.cvt", jms_date(date));

  // try opening it
  if (log != NULL)
    fclose(log);
  log = NULL;
  if (fopen_s(&log, fname, "w") != 0)
    log = NULL;
}


//= Process input sentence from some source.
// if awake == 0 then needs to hear attention word to process sentence
// returns 2 if attention found, 1 if understood, 0 if unintelligible

int jhcAliaCore::Interpret (const char *input, int awake, int amode)
{
  char alist[1000] = "";
  const char *fix, *sent = ((input != NULL) ? input : alist);
  int nt = 0, attn = 0;

  // check if name mentioned (will trigger on unparsable "robot fizzboom")
//  attn = gr.NameSaid(sent, amode);
//  if ((awake == 0) && (attn <= 0))
//    return 0;

  // parse input string to get association list
  if (input != NULL)
  {
    sent = gr.Expand(sent, 1);                   // undo contractions   
    nt = gr.Parse(sent, 0);                      
    if (nt <= 0) 
      if ((fix = vc.FixTypos(sent)) != NULL)     // correct typing errors
      {
        sent = fix;
        nt = gr.Parse(sent, 0);
        if (nt > 0)
          jprintf(1, noisy, " { Fixed typos in original: \"%s\" }\n", gr.NoContract());
      }
    if (nt <= 0)
      if (guess_cats(sent) > 0)                  // handle unknown words
        nt = gr.Parse(sent, 0);
    if (nt > 0)
      gr.AssocList(alist, 1);
  }

  // check if name mentioned (will NOT trigger on unparsable "robot fizzboom")
  hear0 = 0;
  attn = net.NameSaid(alist, amode);
  if ((awake == 0) && (attn <= 0))
    return 0;

  // show parsing steps and reduce "lonely" (even if not understood)
  gr.PrintInput(NULL, __min(noisy, 1));
  if (nt > 0)
  {
    mood.Hear((int) strlen(input));     
    gr.PrintResult(pshow, 1);
  }

  // generate semantic nets (nt = 0 gives huh? response)
  spact = net.Convert(alist, sent);     
  net.Summarize(log, sent, nt, spact);
  hear0 = ((attn > 0) ? 2 : 1);
  return hear0;
}


//= Try to identifying unknonw open-class words from morphology and context.
// returns number fixed 

int jhcAliaCore::guess_cats (const char *sent)
{
  char wd[40];
  const char *txt = sent;
  int cat, cnt = 0;

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
      gram_add_hq(wd);
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
      gram_add(vc.Category(), vc.Mystery(), 3);                      // should not happen
  }
  if (cnt > 0)
    sp_listen(1);
  return cnt;
}


//= For an adjective add base form plus assumed comparative and superlative.
// Note: does NOT add adverbial form ("MOD")

void jhcAliaCore::gram_add_hq (const char *wd)
{
  gram_add("HQ", wd, 3);
  gram_add("HQ-ER",  (net.mf).SurfWord(wd, JTAG_ACOMP), 3);          // comparative
  gram_add("HQ-EST", (net.mf).SurfWord(wd, JTAG_ASUP), 3);           // superlative
}


//= Consider next best parse tree to generate a new bulk sequence for TRY directive.
// returns NULL if no other parses with same speech act as original

jhcAliaChain *jhcAliaCore::Reinterpret ()
{
  char alist[1000] = "";

  if ((spact >= 1) && (spact <= 3))              // fact, command, or question
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

  // get any observations, check expired attentional foci, and recompute halo
  jprintf(4, noisy, "\nSTEP %d ----------------------------------------------------\n\n", atree.Version());
  kern.Volunteer();
  if (atree.Update(gc) > 0)                      // also if bth or node blfs change?
  {
    dmem.DejaVu();                               // (re-)tether objects to LTM
    atree.ClearHalo();
    dmem.GhostFacts();                           // add in proximal LTM facts
    amem.RefreshHalo(atree, noisy - 1);          // apply all rules
  }
  if (gc > 0)
  {
    now = jms_now();
    mood.Emit(Talking());
    stat.Motion(mood);      
    stat.Thought(this);                        
    mood.Update();                               // status NOTEs
  }
  if (atree.Active() > 0)
    jprintf(3, noisy, "============================= %s =============================\n\n", jms_offset(time, t0, 1));

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
  double frac = 1.0;
  int cyc, n = 1, ms = ROUND(1000.0 / shz);      // standard sensing time

jtimer(17, "DayDream");
  // determine how many total thought cycles to run right now
  if (start == 0)
    start = now;
  else 
  {
    frac = thz * jms_secs(now, last) + rem;
    n = ROUND(frac);
  }
  last = now;

  // possibly catch up on thinking (commands will be ignored)
  for (cyc = 1; cyc < n; cyc++)
    if (jms_diff(jms_now(), last) < ms)
      RunAll(0);
  rem = frac - cyc;          // how many thought cycles NOT completed
  think += cyc;
  sense++;
jtimer_x(17);
}


//= Current run is done so shut things down smoothly.

void jhcAliaCore::Done (int save)
{
  // stop all running activities
  stop_all();

  // close any open graphizer log
  if (log != NULL)
    fclose(log);
  log = NULL;

  // possibly save all operators and rules in KB files
  if ((save > 0) && (acc >= 2))
    DumpLearned();

  // report final memory contents
  jprintf("\n==========================================================\n");
  ShowMem();
  jprintf("DONE - Think %3.1f Hz, Sense %3.1f Hz\n", Thinking(), Sensing()); 
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
    mood.BumpMinPref(-1);         
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


//= Load all rules and operators beyond baseline and kernels.
// always loads whatever is in the "learned" version of files

void jhcAliaCore::LoadLearned ()
{
  jprintf(1, noisy, "Reloading learned knowledge:\n");
  pmem.Load("KB/learned.ops",   1, noisy + 1, 2);          // 2 = accumulated level
  pmem.Overrides("KB/learned.pref");
  amem.Load("KB/learned.rules", 1, noisy + 1, 2);         
  amem.Overrides("KB/learned.conf");
  dmem.LoadFacts("KB/learned.facts", 1, noisy + 1, 2);  
  (net.mf).AddVocab(gr, "KB/learned.sgm", 0, 2);
  jprintf(1, noisy, "\n");
}


//= Save all rules and operators beyond baseline and kernels.
// saves a copy with time and date stamp as well as "learned" version

void jhcAliaCore::DumpLearned () const
{
  char base[80] = "KB/kb_";
  int nop, nr, nf, nw;

  // save rules and operators  
  jprintf(1, noisy, "\nSaving learned knowledge:\n");
  jms_date(base + 6, 0, 74);
  nop = pmem.Save(base, 2);                               // 2 = accumulated level
  pmem.Alterations(base);
  nr = amem.Save(base, 2);                                 
  amem.Alterations(base);   
  nf = dmem.SaveFacts(base, 2);
  nw = gr.SaveCats(base, 2, net.mf);

  // make copies as generic database
  copy_file("KB/learned.ops",   base);
  copy_file("KB/learned.pref",  base);
  copy_file("KB/learned.rules", base);
  copy_file("KB/learned.conf",  base);
  copy_file("KB/learned.facts", base);
  copy_file("KB/learned.sgm",   base);
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

void jhcAliaCore::DumpSession () 
{
  pmem.Save("session.ops", 3);
  amem.Save("session.rules", 3);
  dmem.SaveFacts("session.facts", 3);
  gr.SaveCats("session.sgm", 3, net.mf);
//atree.Save("session.wmem");
}


//= Save all rules and operators from any source.

void jhcAliaCore::DumpAll () const
{
  pmem.Save("all.ops", 0);
  amem.Save("all.rules", 0);
  dmem.SaveFacts("all.facts", 0);
  gr.SaveCats("all.sgm", -1, net.mf);
}
