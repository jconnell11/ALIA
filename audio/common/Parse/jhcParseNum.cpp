// jhcParseNum.cpp : converts speech string into floating point number
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "Parse/jhcParseNum.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcParseNum::jhcParseNum ()
{
  char z[5][40] = {"oh", "nought", "naught", "ought", "aught"};
  char d[20][40] = {"zero", "one", "two", "three", "four", 
                    "five", "six", "seven", "eight", "nine", 
                    "ten", "eleven", "twelve", "thirteen", "fourteen", 
                    "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"};
  char t[8][40] = {"twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};
  int i;

  // set up various digit names in member arrays
  for (i = 0; i < 5; i++)
    strcpy_s(zero[i], z[i]);  
  for (i = 0; i < 20; i++)
    strcpy_s(digit[i], d[i]);  
  for (i = 0; i < 8; i++)
    strcpy_s(tens[i], t[i]);  
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Translate string form of number to floating point value.
// returns 1 if successful, 0 if fails (leaves "res" unchanged)

int jhcParseNum::ConvertNum (double& res, const char *txt) 
{
  char token[80];
  double m, v = 0.0;
  int whole; 

  // check for simple text form
  if (txt_cvt(res, txt) > 0)
    return 1;

  // get integer quantity word by word
  SetSource(txt);
  whole = build_int(token);

  // get fractional part (if any)
  if ((_stricmp(token, "point") == 0) ||
      (_stricmp(token, "dot") == 0))
    v = build_frac(token);

  // assemble value, possibly with final modifier like "K"
  v += whole;
  if ((m = get_mult(token)) > 0.0)
  {
    v *= m;
    ReadWord(token);
  }

  // check for extraneous words at end
  if (*token != '\0')
    return 0;
  res = v;
  return 1;
}


//= Check if already nice text form like "1,937.2M".
// returns 1 if successful, 0 if fails (leaves "res" unchanged)

int jhcParseNum::txt_cvt (double& res, const char *txt) const
{
  char token[80];
  char *d = token;
  const char *s = txt;
  double v, m = 1000.0;

  // copy digits and decimal point but ignore commas
  while (*s != '\0')
  {
    if (isdigit(*s) || (*s == '.'))
      *d++ = *s;
    else if (*s != ',')
      break;
    s++;
  }

  // translate known numeric part
  *d = '\0';
  if (sscanf_s(token, "%lf", &v) != 1)
    return 0;

  // check for trailing scientific unit (strip spaces)
  while (*s != '\0')
  {
    if (*s != ' ')
      break;
    s++;
  }
  if ((m = get_mult(s)) > 0.0)
    v *= m;

  // successful conversion
  res = v;
  return 1;
}


//= Get integer portion of number.
// returns assembled number (first bad word in "token")

int jhcParseNum::build_int (char *token)
{
  int v0 = 0, v = 0, more = 0, first = 1;

  while (ReadWord(token, 0) > 0)
  {
    // special introductory "a" allowed
    if ((first > 0) && (_stricmp(token, "a") == 0))
    {
      v = 1;
      first = 0;
      continue;
    }

    // handle valid number words
    if (get_lo(more, v, token) > 0)            // 0-9
      continue;
    if (get_hi(more, v, token) > 0)            // 10-19
      continue;
    if (get_x10(more, v, token) > 0)           // 20-90
      continue;
    if (get_100(more, v, token) > 0)           // 100 is special
      continue;
    if (get_place(more, v, token) > 0)         // 1000, etc.
    {
      v0 += v;                                 // save chunk and restart
      v = 0;
      continue;
    }

    // abort if any other words found
    if (_stricmp(token, "and") != 0)
      break;
  }
  return(v0 + v);
}


//= Get fractional part of value word by word.
// handles single and double digits only
// returns value 0.0 to 1.0 (first bad word in "token")

double jhcParseNum::build_frac (char *token)
{
  double den = 1.0;
  int v = 0, more = 0;

  while (ReadWord(token, 0) > 0)
  {
    if (get_lo(more, v, token) > 0)            // 0-9
      den *= 10.0;
    else if (get_hi(more, v, token) > 0)       // 10-19
      den *= 100.0;
    else if (get_x10(more, v, token) > 0)      // 20-90
      den *= 10;
    else
      break;
  }
  return(v / den);
}


//= Translate digit word into an amount to add to the running sum.
// returns 1 if matched, 0 otherwise

int jhcParseNum::get_lo (int& more, int& v, const char *token) const
{
  int i, dv = -1;

  // check for some alternate form of zero
  for (i = 0; i < 5; i++)
    if (_stricmp(token, zero[i]) == 0)
    {
      dv = 0;
      break;
    }

  // else check for single digit (0-9)
  if (dv < 0.0)
    for (i = 0; i <= 9; i++)
      if (_stricmp(token, digit[i]) == 0)
      {
        dv = i;
        break;
      }
  
  // alter value as needed
  if (dv < 0.0)
    return 0;
  if (more <= 0)                               // shift to add a digit
    v *= 10;
  v += dv;
  more = 0;                                    // no compounds allowed
  return 1;
}


//= Translate teens word into an amount to add to the running sum.
// returns 1 if matched, 0 otherwise

int jhcParseNum::get_hi (int& more, int& v, const char *token) const
{
  int i, dv = -1;

  // check for some number in the teens (10-19)
  for (i = 10; i <= 19; i++)
    if (_stricmp(token, digit[i]) == 0)
    {
      dv = i;
      break;
    }

  // alter value as needed
  if (dv < 0)
    return 0;
  if (more <= 1)                               // shift to add two digits
    v *= 100;
  v += dv;
  more = 0;                                    // no compounds allowed
  return 1;
}


//= Translate multiple of 10 into an amount to add to the running sum.
// returns 1 if matched, 0 otherwise

int jhcParseNum::get_x10 (int& more, int& v, const char *token) const
{
  int i, dv = -1;

  // check for some multiple of ten (20-90)
  for (i = 0; i < 8; i++)
    if (_stricmp(token, tens[i]) == 0)
    {
      dv = 10 * (i + 2);
      break;
    }

  // alter value as needed
  if (dv < 0)
    return 0;
  if (more <= 0)                               // shift to add two digits
    v *= 100;
  v += dv;
  more = 1;                                    // can tack on one digit
  return 1;
}


//= Translate "hundred" into a multiplier for the running sum.
// returns 1 if matched, 0 otherwise

int jhcParseNum::get_100 (int& more, int& v, const char *token) const
{
  if (_stricmp(token, "hundred") != 0)      
    return 0;
  v *= 100;                                    // shift to add two digits
  more = 2;                                    // can tack on two digits 
  return 1;
}


//= Translate word into some 3 digit place group.
// returns 1 if matched, 0 otherwise

int jhcParseNum::get_place (int& more, int& v, const char *token) const
{
  double m;

  if ((m = get_mult(token)) <= 0.0)
    return 0;
  v = (int)(v * m);
  more = 0;                                    // no compounds allowed
  return 1;
}


//= Translate word into an amount to multiply the running sum by.
// returns 0.0 if unknown multiplier

double jhcParseNum::get_mult (const char *token) const
{
  double m = 1000.0;

  if ((_stricmp(token, "thousand") == 0) || 
      (_stricmp(token, "K") == 0))             // "grand" also?
    return m;
  m *= 1000.0;
  if ((_stricmp(token, "million") == 0) || 
      (_stricmp(token, "M") == 0))
    return m;
  m *= 1000.0;
  if ((_stricmp(token, "billion") == 0) || 
      (_stricmp(token, "G") == 0) || 
      (_stricmp(token, "B") == 0))
    return m;
  m *= 1000.0;
  if ((_stricmp(token, "trillion") == 0) || 
      (_stricmp(token, "T") == 0))
    return m;
  return 0.0;
}


///////////////////////////////////////////////////////////////////////////
//                                Debugging                              //
///////////////////////////////////////////////////////////////////////////

//= Check a bunch of examples.

void jhcParseNum::Test () 
{
  char src[][80] = {"0.03K",
                    "93.9m", 
                    "sixteen",
                    "twenty three",
                    "a hundred",
                    "one hundred thousand",
                    "three million",
                    "three hundred and twenty three",
                    "nine hundred sixteen thousand",
                    "nine hundred sixteen thousand two hundred and twenty two",
                    "three hundred thirty seven thousand",
                    "two million eighteen thousand five hundred eighty seven",
                    "sixty two million five hundred nineteen thousand six hundred seventy five",
                    "four hundred and twelve thousand six hundred and thirty two", 
                    "ten thousand",
                    "twenty million",
                    "one six seven zero",
                    "twenty fifteen",
                    "twenty three sixty two",
                    "one fifty six oh seven",
                    "ninety nine hundred",
                    "three point one four sixteen",
                    "three hundred twelve point one two nine",
                    "seventeen dot ought ninety nine",
                    "eleven point nine million",
                    "seven hundred fifty six dot eighty five k",
                    "zero point one nine zero",
                    "6 thousand"};
  double ans[] = {30.0,
                  93900000.0,
                  16.0,
                  23.0,
                  100.0,
                  100000.0,
                  3000000.0,
                  323.0,
                  916000.0,
                  916222.0,
                  337000.0,
                  2018587.0,
                  62519675.0,
                  412632.0,
                  10000.0,
                  20000000.0,
                  1670.0,
                  2015.0,
                  2362.0, 
                  15607.0,
                  9900.0,
                  3.1416,
                  312.129,
                  17.099,
                  11900000.0,
                  756850.0,
                  0.19,
                  6000.0};
  double v;
  int i, rc;

  for (i = 0; i < 28; i++)
  {
    v = 0.0;
    rc = ConvertNum(v, src[i]);
    if ((v - (int) v) == 0.0)
      jprintf("%c %d: <%s> --> %d\n", ((v != ans[i]) ? '*' : ' '), rc, src[i], (int) v);
    else  
      jprintf("%c %d: <%s> --> %f\n", ((v != ans[i]) ? '*' : ' '), rc, src[i], v);
  }
}
