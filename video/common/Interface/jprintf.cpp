// jprintf.cpp : replacement printf function which also saves output
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2019 IBM Corporation
// Copyright 2020 Etaoin Systems
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
#include <stdarg.h>
#include <direct.h>
#include <conio.h>
#include <share.h>

#include "Interface/jms_x.h"

#include "Interface/jprintf.h"


//= Persistent global file pointer for log file output.

static FILE *log_file = NULL;


//= Persistent global name of log file.

static char log_name[500] = "";


//= Persistent flag used to suppress console copy of output.

static int only_log = 0;


///////////////////////////////////////////////////////////////////////////

//= Set up new log file (and finish old one, if any).
// if no name is given then uses current directory and "log"
// unless full > 0 then tacks on "_MMDDYY_HHMM.txt"
// returns 1 if succeeds, 0 for no log file

int jprintf_open (const char *fname, int full)
{
  char date[80];

  // create new log file name based on current time
  if ((full > 0) && (fname != NULL))
    strcpy_s(log_name, fname);
  else
  {
    // possibly create directory
    if (_chdir("log") == 0)
      _chdir("..");
    else
      _mkdir("log");

    // create name based on time
    sprintf_s(log_name, "log/%s_%s.txt", ((fname != NULL) ? fname : "log"), jms_date(date));
  }

  // attempt to open (allow shared reading for "tail")
  jprintf_close();
  if ((log_file = _fsopen(log_name, "w", _SH_DENYWR)) != NULL)
    return 1;
  *log_name = '\0';
  return 0;
}


//= Tell whether to only send output to log file (and not console).
// returns the name used for opening the log file (if any yet)

const char *jprintf_log (int only)
{
  only_log = only;
  return log_name;
}


//= Force recent output to be written to file (e.g. for snooping).
// can use system("start powershell get-content foobar.log -wait") to view
// NOTE: need to link with "commode.obj" to truly flush to file

void jprintf_sync ()
{
  if (log_file != NULL)
    fflush(log_file);
}


//= Close any existing log (needed at end of program).
// always returns 0 for convenience

int jprintf_close ()
{
  if (log_file == NULL)
    return 0;
  fclose(log_file);
  log_file = NULL;
  *log_name = '\0';
  return 0;
}


//= Close any existing log and print message to user.
// waits for some key to be hit before returning
// common ending pattern for console programs
// always returns 1 for convenience

int jprintf_end (const char *msg, ...)
{
  va_list args;
  char val[1000];

  // print exit message (if any)
  if (msg != NULL)
  {
    va_start(args, msg); 
    vsprintf_s(val, msg, args);
    jprintf(val);
  }

  // close log file (if any) and await user confirmation
  jprintf_close();
  printf("\n\nDone.\n");
  printf("Press any key to continue . . .\n");
  _getch();
  return 1;
}


///////////////////////////////////////////////////////////////////////////

//= Send formatted output to console and log file.
// if open_log has not been called then just prints
// always returns 0 for convenience (used to return 1)

int jprintf (const char *msg, ...)
{
  va_list args;
  char val[1000];

  va_start(args, msg); 
  vsprintf_s(val, msg, args);
  return jprint(val);
}


//= Send output to console and log file if detail level okay.
// can also use with local member variable jprintf(1, noisy, "foo")
// if open_log has not been called then just prints
// always returns 0 for convenience (used to return 1)
// NOTE: always evals arguments so better not to do anything complicated

int jprintf (int th, int lvl, const char *msg, ...)
{
  va_list args;
  char val[1000];

  if (lvl < th)
    return 0;
  va_start(args, msg); 
  vsprintf_s(val, msg, args);
  return jprint(val);
}


//= Just print literal string to screen and log file.
// Note: printf has problems with "%" in string so use fwrite
// always returns 0 for convenience (used to return 1)

int jprint (const char *txt)
{
  int n = (int) strlen(txt);

  if (only_log <= 0)
    fwrite(txt, sizeof(char), n, stdout);
  if (log_file != NULL)
    fwrite(txt, sizeof(char), n, log_file);
  return 0;
}


//= Print a backspace character to screen and file.
// always returns 1 for convenience

int jprint_back ()
{
  if (only_log <= 0)
    fwrite("\b \b", sizeof(char), 3, stdout);
  if (log_file != NULL)
  {
    fseek(log_file, -1, SEEK_CUR);
    fwrite(" ", sizeof(char), 1, log_file);
    fseek(log_file, -1, SEEK_CUR);
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////

//= Captures "fprintf" to log file (if any) when "stdout" is the stream.
// returns number of characters written (not including end terminator)

int jfprintf (FILE *out, const char *msg, ...)
{
  va_list args;
  char val[1000];
  int n, csz = (int) sizeof(char);

  va_start(args, msg); 
  n = vsprintf_s(val, msg, args);
  if (out == stdout)
  {
    if (log_file != NULL)
      fwrite(val, csz, n, log_file);
    if (only_log > 0)
      return 0;
  }
  return((int) fwrite(val, csz, n, out));
}    


//= Captures "fputs" to log file (if any) when "stdout" is the stream.
// returns number of characters written (not including end terminator)

int jfputs (const char *msg, FILE *out)
{
  if (out == stdout)
  {
    if (log_file != NULL)
      fputs(msg, log_file);
    if (only_log > 0)
      return 0;
  }
  return fputs(msg, out);
}    
  

