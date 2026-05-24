// jhcProcMem.cpp : procedural memory for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2019 IBM Corporation
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

#include <stdio.h>

#include "Interface/jhcMessage.h"   // common video
#include "Interface/jms_x.h"
#include "Interface/jprintf.h"

#include "Action/jhcAliaPlay.h"     // common robot

#include "Reasoning/jhcProcMem.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcProcMem::~jhcProcMem ()
{
  clear();
}


//= Get rid of all loaded operators.
// always returns 0 for convenience

int jhcProcMem::clear ()
{
  jhcAliaOp *p0, *p = ops;

  while (p != NULL)
  {
    p0 = p;
    p = p->next;
    delete p0;
  }
  ops = NULL;
  np = 0;
  return 0;
}


//= Default constructor initializes certain values.

jhcProcMem::jhcProcMem ()
{
  *formal = '\0';
  ops = NULL;
  np = 0;
  noisy = 2;                 // defaulted from jhcAliaCore
  detail = 0;
//detail = 44;               // show detailed matching for some operator 
}


///////////////////////////////////////////////////////////////////////////
//                             List Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Add item onto tail of list after possibly checking for duplicates (dup > 0).
// returns 1 if added, 0 if duplicate, neg for problem (consider deleting)

int jhcProcMem::AddOperator (jhcAliaOp *p, int ann, int usr, int dup)
{
  jhcAliaOp *p0 = ops;

  // check for likely duplication or other format problems
  if (p == NULL)
    return -1;
  if (dup > 0)
    while (p0 != NULL)
    {
      if (p->Identical(*p0))
      {
        if (usr > 0)
        {
          // possibly revise old operator instead of adding
          jprintf(1, ann, "  ... KNOWN: set old operator %d preference = %4.2f\n", 
                  p0->OpNum(), p->pref);
          p0->pref = p->pref;
          if ((ann >= 2) && (noisy >= 1))
          {
            jprintf("\n.................................\n");
            p0->Print();
            jprintf(".................................\n\n");
          }
          delete p;                    // clean up since not saved
          return 1;
        }
        jprintf(1, ann, "  ... DUPLICATE: identical to old operator %d\n", p0->OpNum());
        return 0;
      }
      p0 = p0->next;
    }

  // add to end of operator list
  if (ops == NULL)
    ops = p;
  else
  {
    p0 = ops;
    while (p0->next != NULL)
      p0 = p0->next;
    p0->next = p;
  }

  // assign operator ID number
  p->next = NULL;
  p->id = ++np;

  // possibly announce formation
  if ((ann > 0) && (noisy >= 1))
  {
    jprintf("\n.................................\n");
    p->Print();
    jprintf(".................................\n\n");
  }
  return 1;
}


//= Create a copy of last operator used by "src" but replace action "mark" by "seq".
// "map" translates from variables in "seq" to vars in actually invoked "mark"
// returns 1 if succesful, 0 or negative for some problem

int jhcProcMem::AddVariant (const jhcAliaOp& op0, const jhcNetNode& main, const jhcBindings& s2o, 
                            jhcAliaChain *seq, int ann)
{
  jhcBindings o2c, s2c;
  jhcAliaOp *op;
  jhcAliaChain **entry;
  jhcAliaChain *tail, *seq2;
  const jhcAliaDir *dir;

  // simplest case (no edit required)
  if (seq == NULL)
    return 1;

  // remove equivalent of marked step from copy of original operator
  op = op_copy(o2c, op0);
  op->pref = op0.pref;
  entry = (op->meth)->StepEntry(o2c.LookUp(&main), &(op->meth));
  if (entry == NULL)
  {
    delete op;
    return 0;
  }
  tail = disconnect(**entry);
  delete *entry;                                 // might leave unused nodes

  // splice in equivalent replacement (unless no-op) using correct vars 
  dir = seq->GetDir();
  if ((dir != NULL) && (dir->Kind() == JDIR_DO) && (dir->key).Empty() &&
      ((tail != NULL) || (entry != &(op->meth))))
    *entry = tail;
  else
  {
    s2c.CopyReplace(s2o, o2c);
    seq2 = seq->Instantiate(*op, s2c);           // never NULL
    *entry = seq2->Append(tail);
  }

  // possibly announce new operator formation then add to list
  if (noisy >= 1)
  {
    jprintf("\nREVISE: OP %d method", op0.OpNum());
    if (dir != NULL)
      jprintf(" with alternate %s[ %s ]", dir->KindTag(), dir->KeyTag());
    jprintf("\n");
  }
  return AddOperator(op, ann);
}


//= Copy original operator exactly and record new_var -> old_var translations.

jhcAliaOp *jhcProcMem::op_copy (jhcBindings& b, const jhcAliaOp& op0) const
{
  jhcAliaOp *op = new jhcAliaOp(op0.kind);

  op->BuildCond();
  op->Assert(*(op0.Pattern()), b);
  op->BuildIn(NULL);  
  op->meth = (op0.meth)->Instantiate(*op, b);
  return op;
}


//= Remove all transition pointers from procedure step so they don't get deleted later.
// returns original continuation from step (assumes single directive)

jhcAliaChain *jhcProcMem::disconnect (jhcAliaChain& step) const
{
  jhcAliaChain *tail = step.cont;               

  step.cont = NULL;              
  step.alt  = NULL;
  step.fail = NULL;
  return tail;
}


//= Remove an operator from the list and permanently delete it.
// must make sure original rem pointer is set to NULL in caller
// used by jhcAliaDir to clean up incomplete ADD operator

void jhcProcMem::Remove (const jhcAliaOp *rem)
{
  jhcAliaOp *prev = NULL, *p = ops;

  // look for operator in list
  if (rem == NULL)
   return;
  while (p != NULL)
  {
    // possibly splice out of list
    if (p == rem)
    {
      if (prev != NULL)
        prev->next = p->next;
      else
        ops = p->next;
      break;
    }

    // move on to next list entry
    prev = p;
    p = p->next;
  }

  // delete given item even if it was not in list
  delete rem;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Find applicable operators that match trigger directive.
// operators and bindings are stored inside directive itself
// returns total number of bindings found, negative for error

int jhcProcMem::FindOps (jhcAliaDir *dir, jhcWorkMem& wmem, double pth, double mth) 
{
  jhcAliaOp *p = ops;
  int i, k, n, mc0, mmax;

  // get operator type by examining directive kind
  if (dir == NULL)
    return -2;
  k = dir->kind;
  if ((k < 0) || (k >= JDIR_MAX))
    return -1;
  if ((k == JDIR_BIND) || (k == JDIR_ALL))
    k = JDIR_FIND;

  // set up to get up to bmax bindings using halo as needed
  mmax = dir->MaxOps();
  dir->mc = mmax;
  wmem.MaxBand(3);

  // try matching all operators above the preference threshold
  // ignore preference until at least some match has been found
  dir->anyops = 0;
  while (p != NULL)
  {
    if ((p->kind == k) && ((dir->anyops <= 0) || (p->pref >= pth)))
    {
      // get all bindings that result in matches to this operator
      // set "any" flag to indicate that some match has been found
      mc0 = dir->mc;
      p->dbg = ((p->id == detail) ? 3 : 0);
      if (p->FindMatches(*dir, wmem, mth, 0) < 0)
        break;

      // ignore matches if operator preference was below threshold      
      // otherwise save operator associated with this group of bindings
      if (p->pref < pth)
        dir->mc = mc0;
      else
        for (i = mc0 - 1; i >= dir->mc; i--)
          dir->op[i] = p;                         
    }
    p = p->next;
  }

  // possibly report summary of what was found
  n = mmax - dir->mc;
  if (noisy >= 2)
  {
    jprintf("Got %d matches", n);
    if (n > 0)
      jprintf(": OPS = ");
    for (i = mmax - 1; i >= dir->mc; i--)
      jprintf("%d ", (dir->op[i])->id);
    jprintf("\n");
  }
  return n;
}


//= Create action-initiating operator based on trace and triggered by given context.
// returns 1 if added, 0 if rejected for some reason

int jhcProcMem::BuildSpur (const jhcGraphlet& ctx, jhcAliaChain *trace, double pref)
{
  char date[40];
  jhcGraphlet pat;
  jhcBindings mt;
  jhcAliaOp *op = new jhcAliaOp(JDIR_NOTE);
  int rc;

  // announce entry and augment trigger specification
  jprintf("\nSPECULATE: new operator based on user command\n");
  pat.IncludeArgs(ctx);

  // create trigger using local nodes and copy procedure minus skolem 
  op->BuildCond();
  op->Assert(pat, mt);
  op->BuildIn(NULL);  
  op->meth = trace->Instantiate(*op, mt, NULL, 1);

  // set overall features of rule and try adding to collection
  sprintf_s(op->prov, "speculative <- %s at %s", formal, jms_date(date));
  op->SetPref(pref);
  if ((rc = AddOperator(op, 1)) > 0)
    return 1;
  delete op;                 // delete if problem (e.g. duplicate)
  return rc;
}


///////////////////////////////////////////////////////////////////////////
//                           Operator Tests                              //
///////////////////////////////////////////////////////////////////////////

//= Determine if some other operator perfectly matches this one.
// only guards against EXACT duplicate, ignores preference differences

bool jhcAliaOp::Identical (const jhcAliaOp& ref) 
{
  const jhcAliaChain *seen[100];
  jhcSituation sit;                    // to avoid jhcAliaOp::match_found()
  jhcBindings b;
  int nst = 0;

  if (ref.kind != kind)
    return false;
  if (!sit.Isomorphic(cond, ref.cond, b))
    return false;
  return iso_method(meth, ref.meth, b, seen, nst);
}


//= Determine if two procedures are identical by comparing all paths.
// takes initial variable mapping "b" from trigger or prior steps
// "seen" array used to detect loops, filled from 0 to n-1

bool jhcAliaOp::iso_method (const jhcAliaChain *step, const jhcAliaChain *step2, 
                            jhcBindings& b, const jhcAliaChain *seen[], int& nst) const
{
  jhcSituation sit;                    // to avoid jhcAliaOp::match_found()
  const jhcAliaDir *d, *d2;
  const jhcAliaPlay *p, *p2;
  int i, nr, ns;

  // sanity check then detect any loopback
  if ((step == NULL) || (step2 == NULL) || (seen == NULL))
    return true;                       // blocks most follow-ons
  for (i = 0; i < nst; i++)
    if (step == seen[i])
      return true;                     // all prior steps matched

  // check for matching outgoing transitions
  if ((step->HasCont() != step2->HasCont()) || 
      (step->HasAlt()  != step2->HasAlt()) || 
      (step->HasFail() != step2->HasFail()))
    return false;

  // register step as started
  if (nst >= 100)
    return false;                      // default to non-match
  seen[nst++] = step;

  // examine payload
  if (((d = step->GetDir()) != NULL) && ((d2 = step2->GetDir()) != NULL))
  {
    // compare kind and key of directive
    if (d->kind != d2->kind)
      return false;
    if (!sit.Isomorphic(d->key, d2->key, b))
      return false;
  }
  else if (((p = step->GetPlay()) != NULL) && ((p2 = step2->GetPlay()) != NULL))
  {
    // compare all activities of play (assumes listed in same order!)
    if (((nr = p->NumReq())   != p2->NumReq()) ||
        ((ns = p->NumSimul()) != p2->NumSimul()))
      return false;
    for (i = 0; i < nr; i++)
      if (!iso_method(p->ReqN(i), p2->ReqN(i), b, seen, nst))
        return false;
    for (i = 0; i < ns; i++)
      if (!iso_method(p->SimulN(i), p2->SimulN(i), b, seen, nst))
        return false;
  }
  else
    return false;                      // different payload types

  // make sure remainder of procedure also matches
  if (step->cont != NULL)
    if (!iso_method(step->cont, step2->cont, b, seen, nst))
      return false;
  if (step->alt != NULL) 
    if (!iso_method(step->alt, step2->alt, b, seen, nst))
      return false;
  if (step->fail != NULL) 
    if (!iso_method(step->fail, step2->fail, b, seen, nst))
      return false;
  return true;
}


///////////////////////////////////////////////////////////////////////////
//                            File Functions                             //
///////////////////////////////////////////////////////////////////////////

//= Read a list of procedures from a file.
// appends to existing advice unless add <= 0
// level: 0 = kernel, 1 = extras, 2 = previous accumulation
// typically give base file name like "KB/Nemo_072721_1038", fcn adds ".ops"
// returns number of operators read, 0 or negative for problem

int jhcProcMem::Load (const char *base, int add, int rpt, int level)
{
  jhcTxtLine in;
  char full[200], src[80] = "";
  const char *fname = base;
  jhcAliaOp *p;
  char *end;
  int ans = 1, n = 0;

  // possibly clear old stuff then try to open file
  if (add <= 0)
    clear();
  if (strchr(base, '.') == NULL)
  {
    sprintf_s(full, "%s.ops", base);
    fname = full;
  }
  if (!in.Open(fname))
  {
//    jprintf("  >>> Could not read operator file: %s !\n", fname);
    return -1;
  }

  // determine provenance string to use
  if (level <= 1)
  {
    strcpy_s(src, fname);
    if ((end = strrchr(src, '.')) != NULL)
      *end = '\0';
  } 

  // try reading operators from file  
  while (ans >= 0)
  {
    // make and load a new operator
    p = new jhcAliaOp;                    
    if ((ans = p->Load(in)) <= 0)
    {
      // delete and purge input if parse error 
      if (!in.End())
        jprintf(">>> Bad syntax at line %d in: %s\n", in.Last(), fname);
      delete p;
      if (in.NextBlank() == NULL)
        break;
    }
    else
    {
      // successful addition
      p->lvl = level;
      if (level < 2)
        sprintf_s(p->prov, "operator %d from %s", p->pnum, src);
      if (AddOperator(p, 0, 0, 0) <= 0)
        delete p;
      n++;
    }
  }

  // possibly announce result
  jprintf(1, rpt, "  %3d action operators  from: %s\n", n, fname);
  return n;
}


//= Save all current operators at or above some level to a file.
// level: 0 = kernel, 1 = extras, 2 = previous accumulation, 3 = newly added
// typically give base file name like "KB/Nemo_072721_1038", fcn adds ".ops"
// returns number of operators saved, negative for problem

int jhcProcMem::Save (const char *base, int level) const
{
  char full[200];
  const char *fname = base;
  FILE *out;
  int cnt;

  if (strchr(base, '.') == NULL)
  {
    sprintf_s(full, "%s.ops", base);
    fname = full;
  }
  if (fopen_s(&out, fname, "w") != 0)
  {
    jprintf("  >>> Could not write operator file: %s !\n", fname);
    return -1;
  }
  if (level >= 2)
  {
    fprintf(out, "// newly learned operators not in KB0 or KB2\n");
    fprintf(out, "// ==========================================\n\n");
  }
  cnt = save_ops(out, level);
  fclose(out);
  return cnt;
}


//= Save all operators in order irrespective of category.
// returns number saved

int jhcProcMem::save_ops (FILE *out, int level) const
{
  jhcAliaOp *p = ops;
  int cnt = 0;

  while (p != NULL)
  {
    if (p->lvl >= level)
      if (p->Save(out, 1) > 0)
      {
        fprintf(out, "\n\n");
        cnt++;
      }
    p = p->next;
  }
  return cnt;
}


//= Store alterations of preference values relative to KB0 and KB2 operators.
// typically give base file name like "KB/Nemo_072721_1038", fcn adds ".pref"
// returns number of exceptions stored (writes file)

int jhcProcMem::Alterations (const char *base) const
{
  char full[200];
  const char *sf, *fname = base;
  FILE *out;
  const jhcAliaOp *p = ops;
  int na = 0;

  // try opening file and writing header
  if (strchr(base, '.') == NULL)
  {
    sprintf_s(full, "%s.pref", base);
    fname = full;
  }
  if (fopen_s(&out, fname, "w") != 0)
  {
    jprintf("  >>> Could not write preference file: %s !\n", fname);
    return -1;
  }
  fprintf(out, "// learned changes to default operator preferences and durations\n\n");

  // scan through operators for altered values
  while (p != NULL)
  {
    sf = strrchr(p->prov, ' ');
    if ((sf != NULL) && ((p->pref != p->pref0) || (p->Budget() != p->time0)))
    {
      fprintf(out, "%s %d = %4.2f", sf + 1, p->pnum, p->pref);
      if (p->Budget() != p->time0)
        fprintf(out, " : %3.1f + %3.1f", p->tavg, p->tstd);
      fprintf(out, "\n");
      na++; 
    }  
    p = p->next;
  }

  // clean up
  fclose(out);
  return na;
}


//= Change default preference values of KB0 and KB2 operators based on learning.
// typically give base file name like "KB/Nemo_072721_1038", fcn adds ".pref"
// returns number of operators altered (reads file)

int jhcProcMem::Overrides (const char *base)
{
  jhcTxtLine in;
  char full[200], src[40];
  const char *item, *sf, *fname = base;
  jhcAliaOp *p;
  double pf, ta, ts;
  int n, dur, na = 0;

  // try opening file
  if (strchr(base, '.') == NULL)
  {
    sprintf_s(full, "%s.pref", base);
    fname = full;
  }
  if (!in.Open(fname))
  {
//    jprintf("  >>> Could not read preference file: %s !\n", fname);
    return -1;
  }

  // read and parse each line
  while (in.NextContent() != NULL)
  {
    // extract provenance file and original number
    if ((item = in.Token()) == NULL)
      break;
    strcpy_s(src, item);
    if ((item = in.Token()) == NULL)
      break;
    if (sscanf_s(item, "%d", &n) != 1)
      break;

    // extract updated confidence value (required)
    if ((item = in.Token()) == NULL)
      break;
    if (strcmp(item, "=") != 0)
      break;
    if ((item = in.Token()) == NULL)
      break;
    if (sscanf_s(item, "%lf", &pf) != 1)
      break;

    // see if any timing information (optional)
    dur = 0;
    if ((item = in.Token()) != NULL)
    {
      if (strcmp(item, ":") != 0)
        break;
      if ((item = in.Token()) == NULL)
        break;
      if (sscanf_s(item, "%lf", &ta) != 1)
        break;
      if ((item = in.Token()) == NULL)
        break;
      if (strcmp(item, "+") != 0)
        break;
      if ((item = in.Token()) == NULL)
        break;
      if (sscanf_s(item, "%lf", &ts) != 1)
        break;
      dur = 1;
    }

    // find matching operator (if any)
    p = ops;
    while (p != NULL)
    {
      sf = strrchr(p->prov, ' ');      // trim "operator 2 from KB0/Foo"
      if ((sf != NULL) && (p->pnum == n) && (strcmp(sf + 1, src) == 0))
      {
        // update preference value and possibly duration
        p->pref = pf;
        if (dur > 0)
        {
          p->tavg = ta;
          p->tstd = ts;
        }
        na++;
        break;
      }
      p = p->next;
    }
  }
  return na;
}
