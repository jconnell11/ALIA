// jhcJSON.cpp : builds common web JSON datastructures
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015 IBM Corporation
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

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include "Comm/jhcJSON.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcJSON::jhcJSON ()
{
  init();
}


//= Make element with boolean type.

jhcJSON::jhcJSON (bool v)
{
  init();
  Set(v);
}


//= Make element with integer type (double internally).

jhcJSON::jhcJSON (int v)
{
  init();
  Set(v);
}


//= Make element with numeric type.

jhcJSON::jhcJSON (double v)
{
  init();
  Set(v);
}


//= Make element with string type.

jhcJSON::jhcJSON (const char *v)
{
  init();
  Set(v);
}


//= Clear all basic data.

void jhcJSON::init ()
{
  kind = -1;             // null
  num  = 0.0;
  tag  = NULL;
  head = NULL;
  tail = NULL;
}


//= Default destructor does necessary cleanup.
// destroy everything connect to this element (use trees only!)

jhcJSON::~jhcJSON ()
{
  Clr();
}


///////////////////////////////////////////////////////////////////////////
//                              Type Functions                           //
///////////////////////////////////////////////////////////////////////////
 
//= Tells whether the element is an atom (unstructured).

bool jhcJSON::Atom () const
{
  return(((kind >= -1) && (kind <= 3)) ? true : false);
}


//= Tells whether the element is a null.

bool jhcJSON::Null () const
{
  return((kind == -1) ? true : false);
}


//= Tells whether the element is a boolean value.

bool jhcJSON::Boolean () const
{
  return(((kind == 0) || (kind == 1)) ? true : false);
}


//= Tells whether the element is a boolean false value.

bool jhcJSON::False () const
{
  return((kind == 0) ? true : false);
}


//= Tells whether the element is a boolean true value.

bool jhcJSON::True () const
{
  return((kind == 1) ? true : false);
}


//= Tells whether the element is an integer.

bool jhcJSON::Integer () const
{
  double n;

  return(((kind == 2) && (modf(num, &n) == 0.0)) ? true : false);
}


//= Tells whether the element is a number.

bool jhcJSON::Number () const
{
  return((kind == 2) ? true : false);
}


//= Tells whether the element is a character string.

bool jhcJSON::String () const
{
  return((kind == 3) ? true : false);
}


//= Tells whether the element is an ordered array.

bool jhcJSON::Array () const
{
  return((kind == 4) ? true : false);
}


//= Tells whether the element is an associative list.

bool jhcJSON::Map () const
{
  return((kind == 5) ? true : false);
}


///////////////////////////////////////////////////////////////////////////
//                                  Atoms                                //
///////////////////////////////////////////////////////////////////////////

//= Set value to 'null'.
// destroys all other values and changes type if needed

void jhcJSON::Clr ()
{
  if (tag != NULL)
    delete [] tag;
  if (head != NULL)
    delete head;
  if (tail != NULL)
    delete tail;
  init();
}


//= Set value to the given boolean.
// destroys all other values and changes type if needed

void jhcJSON::Set (bool v)
{
  Clr();
  kind = ((v) ? 1 : 0);
}


//= Set value to the given integer.
// destroys all other values and changes type if needed

void jhcJSON::Set (int v)
{
  Clr();
  kind = 2;
  num = (double) v;
}


//= Set value to the given number.
// destroys all other values and changes type if needed

void jhcJSON::Set (double v)
{
  Clr();
  kind = 2;
  num = v;
}


//= Set value to the given string.
// destroys all other values and changes type if needed

void jhcJSON::Set (const char *v)
{
  int n = (int) strlen(v) + 1;

  Clr();
  kind = 3;
  tag = new char [n];
  strcpy_s(tag, n, v);
}
  

//= Retrieve the boolean value of the item.
// returns false if item was not a boolean to begin with

bool jhcJSON::BoolVal () const
{
  return(Boolean() ? true : false);
}


//= Retrieve the numeric value of the item and coerce to integer.
// returns 0 if the item was not a number to begin with 

int jhcJSON::IntVal () const
{
  return(Integer() ? (int) num : 0);
}


//= Retrieve the numeric value of the item.
// returns 0 if the item was not a number to begin with 

double jhcJSON::NumVal () const
{
  return(Number() ? num : 0.0);
}


//= Retrieve a character string as the value of the item. 
// returns NULL if the item was not a number to begin with 

const char *jhcJSON::StrVal () const
{
  return(String() ? tag : NULL);
}


///////////////////////////////////////////////////////////////////////////
//                                 Arrays                                //
///////////////////////////////////////////////////////////////////////////

//= Set element as an array with given length (all values set to 'null').

void jhcJSON::MakeArr (int n, int wipe)
{
  jhcJSON *add, *item = this;
  int i;

  // make a blank array
  if ((kind == 4) && (wipe <= 0))
    return;
  Clr();
  kind = 4;

   // possibly point first entry to a null
  if (n <= 0)
    return;
  head = new jhcJSON;

  // make a sequences of other array entries
  for (i = 1; i < n; i++)
  {
    // create a new list entry pointing at a null
    add = new jhcJSON;
    add->kind = 4;
    add->head = new jhcJSON;

    // tack it onto the end of the list
    item->tail = add;
    item = add;
  }
}


//= Set the value of indexed element to be 'null'.
// forces item to become an array

int jhcJSON::SetVal (int index)
{
  return((index >= 0) ? SetVal(index, new jhcJSON) : 0);
}


//= Set the value of indexed element to be the given boolean.
// forces item to become an array

int jhcJSON::SetVal (int index, bool v)
{
  return((index >= 0) ? SetVal(index, new jhcJSON(v)) : 0);
}


//= Set the value of indexed element to be the given integer.
// forces item to become an array

int jhcJSON::SetVal (int index, int v)
{
  return((index >= 0) ? SetVal(index, new jhcJSON(v)) : 0);
}


//= Set the value of indexed element to be the given number.
// forces item to become an array

int jhcJSON::SetVal (int index, double v)
{
  return((index >= 0) ? SetVal(index, new jhcJSON(v)) : 0);
}


//= Set the value of indexed element to be the given string.
// forces item to become an array

int jhcJSON::SetVal (int index, const char *v)
{
  return((index >= 0) ? SetVal(index, new jhcJSON(v)) : 0);
}


//= Set the value of indexed element.
// binds the given pre-existing chunk of JSON structure
// NOTE: binds actual element and can destroy it later!
// forces item to become an array
// returns 1 if successful, 0 if not an array or beyond bounds

int jhcJSON::SetVal (int index, jhcJSON *v)
{
  jhcJSON *item = this;
  int i;

  // check array bound (0 is smallest allowed) then force type
  if (index < 0)
    return 0;
  MakeArr(0, 0);

  // walk down the list to find correct entry
  for (i = 0; i < index; i++)
    if ((item = item->tail) == NULL)
      return 0;

  // destroy any old entry and bind new one
  if (item->head != NULL)
    delete item->head;
  item->head = v;
  return 1;
}


//= Add a 'null' element to end of array (increases length).
// forces item to become an array
// returns a pointer to new element for later modification

jhcJSON *jhcJSON::NewVal ()
{
  return Add(new jhcJSON);
}


//= Add a boolean element to end of array (increases length).
// forces item to become an array

void jhcJSON::Add (bool v)
{
  Add(new jhcJSON(v));
}


//= Add an integer element to end of array (increases length).
// forces item to become an array

void jhcJSON::Add (int v)
{
  Add(new jhcJSON(v));
}


//= Add a numeric element to end of array (increases length).
// forces item to become an array

void jhcJSON::Add (double v)
{
  Add(new jhcJSON(v));
}


//= Add a string element to end of array (increases length).
// forces item to become an array

void jhcJSON::Add (const char *v)
{
  Add(new jhcJSON(v));
}


//= Add a new element to end of array (increases length).
// forces item to become an array
// returns pointer to element for convenience

jhcJSON *jhcJSON::Add (jhcJSON *v)
{
  jhcJSON *add, *item = this;

  // force type
  MakeArr(0, 0);

  // if array is empty, add as first real element
  if ((tail == NULL) && (head == NULL))
  {
    head = v;
    return v;
  }

  // make new entry pointing to this item
  add = new jhcJSON;
  add->kind = 4;
  add->head = v;

  // attach to last entry in list
  while (item->tail != NULL)
    item = item->tail;
  item->tail = add;
  return v;
}


//= Terminate array at this position.
// returns 1 if successful, 0 if not an array

int jhcJSON::Truncate ()
{
  if (!Array())
    return 0;
  if (tail != NULL)
    delete tail;
  tail = NULL;
  return 1;
}


//= Get the length of the array.
// returns 0 if not an array

int jhcJSON::Len () const
{
  const jhcJSON *item = this;
  int n = 1;
  
  if (!Array())
    return 0;
  while ((item = item->tail) != NULL)
    n++;
  return n;
}


//= Get the value of an indexed element.
// returns NULL if not an array or beyond bounds

jhcJSON *jhcJSON::GetVal (int index) const
{
  const jhcJSON *item = this;
  int i;

  if (!Array() || (index < 0))
    return NULL;
  for (i = 0; i < index; i++)
    if ((item = item->tail) == NULL)
      return NULL;
  return item->head;
}


//= Check if string value is already in array (for sets vs. bags).

bool jhcJSON::HasVal (const char *txt) const
{
  const jhcJSON *hd, *item = this;
  const char *v;

  if (!Array() || (txt == NULL))
    return false;
  while (item != NULL)
  {
    if ((hd = item->head) != NULL)
      if ((v = hd->StrVal()) != NULL)
        if (_stricmp(v, txt) == 0)
          return true;
    item = item->tail;
  }
  return false;
}


///////////////////////////////////////////////////////////////////////////
//                                  Maps                                 //
///////////////////////////////////////////////////////////////////////////

//= Set self to be an associative map.

void jhcJSON::MakeMap (int wipe)
{
  if ((kind == 5) && (wipe <= 0))
    return;
  Clr();
  kind = 5;
}


//= Create a new key with initial value 'null'.
// forces item to become a map
// returns pointer to new value for later alteration

jhcJSON *jhcJSON::NewKey (const char *key)
{
  return SetKey(key, new jhcJSON);
}


//= Create a key with value being the given boolean.
// forces item to become a map

void jhcJSON::SetKey (const char *key, bool v)
{
  SetKey(key, new jhcJSON(v));
}


//= Create a key with value being the given integer.
// forces item to become a map

void jhcJSON::SetKey (const char *key, int v)
{
  SetKey(key, new jhcJSON(v));
}


//= Create a key with value being the given number.
// forces item to become a map

void jhcJSON::SetKey (const char *key, double v)
{
  SetKey(key, new jhcJSON(v));
}


//= Create a key with value being the given string.
// forces item to become a map

void jhcJSON::SetKey (const char *key, const char *v)
{
  SetKey(key, new jhcJSON(v));
}


//= Create or change the value for given key (unique).
// binds the given pre-existing chunk of JSON structure
// NOTE: binds actual element and can destroy it later!
// forces item to become a map
// returns pointer to value for convenience

jhcJSON *jhcJSON::SetKey (const char *key, jhcJSON *v)
{
  jhcJSON *prev, *item = this;
  int n;

  // force item to be a map
  MakeMap(0);

  // look for matching entry
  while ((item->tag == NULL) || (strcmp(key, item->tag) != 0))
  {
    prev = item;
    if ((item = item->tail) == NULL)
      break;
  }

  // if nothing found then create a new entry
  if (item == NULL)
  {
    // possibly reuse first entry if blank
    if (tag == NULL)
      item = this;
    else
    {
      // add new entry to end of list 
      item = new jhcJSON;
      item->kind = 5;
      prev->tail = item;
    }
    
    // copy desired key
    n = (int) strlen(key) + 1;
    item->tag = new char [n];
    strcpy_s(item->tag, n, key);
  }

  // destroy any old value and bind new one
  if (item->head != NULL)
    delete item->head;
  item->head = v;
  return v;
}


//= Remove the entry for a given key.
// returns 1 if successful, 0 if not a map

int jhcJSON::RemKey (const char *key)
{
  jhcJSON *prev, *item = this;

  // check type
  if (!Map())
    return 0;

  // look for matching entry (if any)
  while ((item->tag == NULL) || (strcmp(key, item->tag) != 0))
  {
    prev = item;
    if ((item = item->tail) == NULL)
      break;
  }

  // either erase key (if first in list) or splice out of list
  if (item == this)
  {
    delete tag;
    tag = NULL;
  }
  else if (item != NULL)
  {
    prev->tail = item->tail;
    delete item;
  }
  return 1;
}


//= Returns key associated with current entry (NULL if not a map).

const char *jhcJSON::Key () const
{
  return((Map()) ? tag : NULL);
}


//= Look for value associated with given key (unique).
// returns node if successful, NULL if not found or not a map

jhcJSON *jhcJSON::GetKey (const char *key) const
{
  const jhcJSON *item = this;

  if (!Map())
    return NULL;
  while ((item->tag == NULL) || (_stricmp(key, item->tag) != 0))
    if ((item = item->tail) == NULL)
      return NULL;
  return item->head;
}


//= Look for value associated with key or create a new null one.
// forces item to become a map
// returns node found or made

jhcJSON *jhcJSON::FindKey (const char *key) 
{
  jhcJSON *item = this;

  // force item to be a map
  MakeMap(0);

  // see if already in list of key-value pairs
  while ((item->tag == NULL) || (_stricmp(key, item->tag) != 0))
    if ((item = item->tail) == NULL)
    {
      item = new jhcJSON;
      SetKey(key, item);         // searches again
      return item;
    }
  return item->head;
}


//= Get the value at the current position in the array or map.
// returns NULL if not an array or map

jhcJSON *jhcJSON::First () const
{
  if (!Array() && !Map())
    return NULL;
  return head;
}


//= Get the remainder of the array or map after this element.
// returns NULL if not an array or map

jhcJSON *jhcJSON::Rest () const
{
  if (!Array() && !Map())
    return NULL;
  return tail;
}


//= Check if the entry for given key is the same as the given string value.

bool jhcJSON::MatchStr (const char *key, const char *txt) const
{
  const jhcJSON *v;
  const char *val;

  if ((v = GetKey(key)) != NULL)
    if ((val = v->StrVal()) != NULL)
      if (_stricmp(val, txt) == 0)
        return true;
  return false;
}


///////////////////////////////////////////////////////////////////////////
//                             Serialization                             //
///////////////////////////////////////////////////////////////////////////

//= Pretty print whole structure.

void jhcJSON::Print () const
{
  char msg[4000];

  Dump(msg, 0, 4000);
  jprintf(msg);
}


//= Pretty print whole structure to a string.
// returns number of characters used

int jhcJSON::Dump (char *dest, int lvl, int ssz) const
{
  int n = 0;

  if (Atom())
    n = print_atom(dest, ssz);
  else if (Array())
    n = print_arr(dest, lvl, ssz);
  else if (Map())
    n = print_map(dest, lvl, ssz);
  if (lvl == 0)
    n += sprintf_s(dest + n, ssz - n, "\n");
  return n;
}


//= Print simple atomic forms (indentation already done).
// returns number of characters used

int jhcJSON::print_atom (char *dest, int ssz) const
{
  int n = 0;

  if (Null())
    n = sprintf_s(dest, ssz, "null");
  else if (False())
    n = sprintf_s(dest, ssz, "false");
  else if (True())
    n = sprintf_s(dest, ssz, "true");
  else if (Integer())
    n = sprintf_s(dest, ssz, "%.0f", NumVal());
  else if (Number())
    n = sprintf_s(dest, ssz, "%f", NumVal());
  else if (String())
    n = sprintf_s(dest, ssz, "\"%s\"", StrVal());
  return n;
}


//= Print simple array of values.
// does indentation for each element
// returns number of characters used

int jhcJSON::print_arr (char *dest, int lvl, int ssz) const
{
  char lead[80] = "";
  const jhcJSON *hd, *item = this;
  int i, n = 0;

  // check if array is empty
  if (First() == NULL)
    return sprintf_s(dest, ssz, "[]");

  // print opening bracket on current line
  n = sprintf_s(dest, ssz, "[\n");
  for (i = 0; i < lvl; i++)
    strcat_s(lead, 80, "  ");

  // print all elements on separate lines
  while (item != NULL)
  {
    // print array element value after indent
    n += sprintf_s(dest + n, ssz - n, "%s  ", lead);
    hd = item->First();
    n += hd->Dump(dest + n, lvl + 1, ssz - n);
    item = item->Rest();

    // possibly add comma at end
    if (item != NULL)
      n += sprintf_s(dest + n, ssz - n, ",");
    n += sprintf_s(dest + n, ssz - n, "\n");
  }

  // closing bracket on own indented line
  n += sprintf_s(dest + n, ssz - n, "%s]", lead);    
  return n;
}


//= Print association list of keys and values.
// does indentation for each element
// returns number of characters used

int jhcJSON::print_map (char *dest, int lvl, int ssz) const
{
  char lead[80] = "";
  const jhcJSON *hd, *item = this;
  int i, n = 0;

  // check if list is empty
  if ((Rest() == NULL) && (Key() == NULL))
    return sprintf_s(dest, ssz, "{}");

  // print opening brace on current line
  n = sprintf_s(dest, ssz, "{\n");
  for (i = 0; i < lvl; i++)
    strcat_s(lead, 80, "  ");

  // print all elements on separate lines
  while (item != NULL)
  {
    // make sure key is valid
    if (item->Key() == NULL)
    {
      item = item->Rest();
      continue;
    }

    // print key and value
    n += sprintf_s(dest + n, ssz - n, "%s  \"%s\" : ", lead, item->Key());
    if ((hd = item->First()) == NULL)
      n += sprintf_s(dest + n, ssz - n, "null");
    else
      n += hd->Dump(dest + n, lvl + 1, ssz - n);
    item = item->Rest();

    // possibly add comma at end
    if (item != NULL)
      n += sprintf_s(dest + n, ssz - n, ",");
    n += sprintf_s(dest + n, ssz - n, "\n");
  }

  // closing brace on own indented line
  n += sprintf_s(dest + n, ssz - n, "%s}", lead);
  return n;    
}


///////////////////////////////////////////////////////////////////////////
//                           De-serialization                            //
///////////////////////////////////////////////////////////////////////////

//= Create a structure from a serialized string.
// returns empty string if valid format, NULL for problem

const char *jhcJSON::Ingest (const char *src)
{
  const char *s = src;

  // skip leading whitespace
  while (1)
  {
    if (*s == '\0') 
      return s;
    if (strchr(",: \t\r\n", *s) == NULL)
      break;
    s++;
  }

  // try to start some particular element type
  if (*s == '[')
    return build_arr(s);
  if (*s == '{')
    return build_map(s);
  return build_atom(s);
}


//= Keep adding elements to array until closing bracket found.
// returns remainder of string, NULL if bad format

const char *jhcJSON::build_arr (const char *src)
{
  jhcJSON *item;
  const char *s = src + 1;  // skip opening bracket

  // set this node as the head of an empty list
  MakeArr();
  while (*s != '\0')
  {
    // strip white space and look for end of array
    while (1)
    {
      if (*s == '\0')
        return NULL;
      if (*s == ']')
        return(s + 1);
      if (strchr(", \t\r\n", *s) == NULL)
        break;
      s++;
    }

    // try to add a new item at end of list
    item = new jhcJSON;
    if ((s = item->Ingest(s)) == NULL)
    {
      delete item;
      return NULL;
    }
    Add(item);
  }
  return NULL;
}


//= Keep adding key:value pairs to map until closing brace found.
// returns remainder of string, NULL if bad format

const char *jhcJSON::build_map (const char *src)
{
  char key[80];
  jhcJSON *item;
  const char *s = src + 1;  // skip opening brace

  // set this node as the head of an empty list
  MakeMap();
  while (*s != '\0')
  {
    // strip white space and look for end of map
    while (1)
    {
      if (*s == '\0')
        return NULL;
      if (*s == '}')
        return(s + 1);
      if (strchr(", \t\r\n", *s) == NULL)
        break;
      s++;
    }

    // try to get key name
    if (*s != '\"')
      return NULL;
    if ((s = get_token(key, 80, s + 1, "\"", 80)) == NULL)
      return NULL;

    // turn second part (value) into a new item
    item = new jhcJSON;
    if ((s = item->Ingest(s + 1)) == NULL)
    {
      delete item;
      return NULL;
    }
    SetKey(key, item);
  }
  return NULL;
}


//= Set self to be some sort of atom based on head of string.
// assumes node is initially cleared (null)
// returns rest of string if okay, NULL if fails 

const char *jhcJSON::build_atom (const char *src)
{
  char token[500];
  const char *s;
  double val;

  // check for string in quotes
  if (*src == '\"')
  {
    if ((s = get_token(token, 500, src + 1, "\"", 500)) == NULL)
      return NULL;
    Set(token);
    return(s + 1);
  }

  // check for null or boolean value
  if (strchr("tfn", tolower(*src)) != NULL)
  {
    if ((s = get_token(token, 10, src, "]}, \t\r\n", 500)) == NULL)
      return NULL;
    if (_stricmp(token, "false") == 0)
      Set(false);
    else if (_stricmp(token, "true") == 0)
      Set(true);
    else if (_stricmp(token, "null") != 0)
      return NULL;
    return s;
  }

  // try to interpret as a number
  if ((s = get_token(token, 20, src, "]}, \t\r\n", 500)) == NULL)
    return NULL;
  if (sscanf_s(token, "%lf", &val) != 1)
    return NULL;
  Set(val);
  return s;
}


//= Read until first character from given set found.
// returns rest of string if okay, NULL if too big

const char *jhcJSON::get_token (char *token, int n, const char *src, const char *stop, int ssz) const
{
  const char *end;
  int n2;

  // find end of token and check if length is okay
  if ((end = strpbrk(src, stop)) != NULL)
    n2 = (int)(end - src);
  else
    n2 = (int) strlen(src);
  if ((n2 < 0) || (n2 >= n))
    return NULL;

  // copy token portion of source string
  strncpy_s(token, ssz, src, n2);
  token[n2] = '\0';
  return(src + n2);  
}




