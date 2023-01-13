// jhcDeclMem.h : long term factual memory for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
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

#ifndef _JHCDECLMEM_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCDECLMEM_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Semantic/jhcNodePool.h"


//= Long term factual memory for ALIA system.
// does approximate partial matching of nodes using proximal info
// considers all unary properties of base node and their unary modifiers 
// also considers n-ary relations of bases node (only) with modifiers
// and unary properties of the relation's object arguments (brackets) 
// does not link shared arguments (largely treated as separate items)
// <pre>
// 
//   obj-1 <-name- Ken
//         <-ako-- main
//         <-hq--- ferocious <-deg- very
//         <-wrt-- car -ako-> [obj-2] <-hq- blue
//         <-wrt-- wife -ako-> [obj-3] <-name- Gwen
//         <-ako-- husband -wrt-> [obj-3] <-name- Gwen
//         <-agt-- cause -obj-> flee <-mod- quickly     (args omitted)
//
// </pre>

class jhcDeclMem : public jhcNodePool
{
// PRIVATE MEMBER VARIABLES
private:
  class jhcActionTree *atree;          // working memory
  int first[4];                        // classes of facts 
  double bth;                          // cached belief threshold


// PUBLIC MEMBER VARIABLES
public:
  int noisy, gh, enc, ret, detail;     // specific debugging                

  // matching weights and thresholds
  jhcParam wps;
  double nwt, kwt, fmod, ath, farg, rth;
  int alts;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcDeclMem ();
  jhcDeclMem ();
  void Bind (class jhcActionTree *w) {atree = w;}

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // explicit formation
  int Remember (jhcNetNode *top);

  // familiarity
  int DejaVu ();
  jhcNetNode *Recognize (jhcNetNode *locus, double qth =0.5) const;

  // spotlight
  void GhostFacts () const;

  // file functions
  int LoadFacts (const char *base, int add =0, int rpt =0, int level =1);
  int SaveFacts (const char *base, int level =1) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int wt_params (const char *fname);

  // explicit formation
  jhcNetNode *jhcDeclMem::ltm_node (const jhcNetNode *n, const jhcBindings& xfer);
  double add_node (jhcGraphlet& desc, jhcNetNode *n, const jhcNetNode *src, int rels) const;
  double add_pred (jhcGraphlet& desc, jhcNetNode *pred, const jhcNetNode *src, int rels) const;
  double elab_obj (jhcGraphlet& desc2, jhcNetNode *obj, const jhcNetNode *src, int rels) const;
  int obj_prop (double& sc, int& best, jhcGraphlet& desc, const jhcNetNode *obj, const jhcNetNode *src, const char *role) const;
  int num_tied (const jhcGraphlet& desc) const;
  bool equiv_nodes (const jhcNetNode *focus, const jhcNetNode *mate, const jhcNetNode *obj) const;
  bool equiv_props (const jhcNetNode *focus, const jhcNetNode *mate, const jhcGraphlet *desc) const; 
  bool lex_equiv (const jhcNetNode *focus, const jhcNetNode *mate) const
    {return(focus->LexSame(mate) || (focus->LexMatch("you") || (mate->Lex() == NULL)));}

  // familiarity
  int tether (jhcNetNode *focus, jhcNetNode *win);
  double score_nodes (const jhcNetNode *focus, const jhcNetNode *mate) const;
  double score_unary (const jhcNetNode *focus, const jhcNetNode *mate, double qth, int mode) const;
  double score_rels (const jhcNetNode *focus, const jhcNetNode *mate, double qth) const;
  double score_args (const jhcNetNode *focus, const jhcNetNode *mate, const jhcNetNode *obj, double qth) const;

  // spotlight
  void buoy_preds (jhcNetNode *n, int tval, int rels, int lvl) const;


};


#endif  // once




