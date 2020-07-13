// jhcGroup_Merge.cpp : part of connected components analysis
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2005 IBM Corporation
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

#include "Processing/jhcGroup.h"


// NOTE: temporarily moved to this file for more accurate profiling

///////////////////////////////////////////////////////////////////////////

#ifdef JHC_TIME

//= Merges two classes to yield one name and a combined area measure.
// Leaves a forwarding "pointer" by negating canonical index.    
// Returns the final canonical index.

int jhcGroup::merge_labels (int now, int old)
{
  int size, cnt = 0, base = old;
  
  // look up canonical name of old region (now is already canonical)
  // chase pointers to find base class, if several pointer then compress path
  while ((size = areas.ARef(base)) < 0)
  {
    base = -size;
    cnt++;
  }
  if (cnt > 1)
    areas.ASet(old, -base);

  // see if current pixel has no class or same class as old
  if (now == base)
    return base;
  if (now == 0)
  {
    areas.AInc(base, 1);
    return base;
  }

  // else merge areas into lowest numbered class, leave forwarding pointer
  if (now < base)
  {
    areas.AInc(now, areas.ARef(base));
    areas.ASet(base, -now);
    return now;
  }
  areas.AInc(base, areas.ARef(now));
  areas.ASet(now, -base);
  return base;
}

#endif

