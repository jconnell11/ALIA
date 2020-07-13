// jhcTaisRemote.cpp : interface to allow local GUI to run ALIA remotely
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#include "Body/jhcTaisRemote.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcTaisRemote::~jhcTaisRemote ()
{
  Shutdown();
}


//= Default constructor initializes certain values.

jhcTaisRemote::jhcTaisRemote ()
{
  build_cvt();
  tx.SetBuf(200000);         // for images
  *(rx.host) = '\0';         // PULL connection 
  Defaults();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters for interacting with remote ALIA brain.

int jhcTaisRemote::tais_params (const char *fname)
{
  jhcParam *ps = &tps;
  int ok;

  ps->SetTag("tais_port", 0);
  ps->NextSpec4( &(tx.port), 4815, "Outgoing publish port");
  ps->NextSpec4( &(rx.port), 4816, "Incoming pull port");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcTaisRemote::Defaults (const char *fname)
{
  int ok = 1;

  ok &= tais_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcTaisRemote::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= tps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Set up server for language and robot interaction and initialize robot.
// returns 2 if success + robot, 1 if success but no robot, 0 or negative for problem

int jhcTaisRemote::Init (int id, int noisy)
{
  // clear state
  esc = 0;

  // check communication channels
  tx.Reset();
  if (tx.ZChk() <= 0)
  {
    jprintf(1, noisy, ">>> Cannot transmit on port %d !\n", tx.port);
    return -2;
  }
  rx.Reset();
  if (rx.ZChk() <= 0)
  {
    jprintf(1, noisy, ">>> Cannot receive on port %d !\n", rx.port);
    return -1;
  }

  // try to connect to robot
  if (body == NULL)
    return 0;
  if (body->Reset(noisy, "config", id) <= 0)
    return 1;
  return 2;
}


//= Inject text (and quit indication) from the user into remote brain.
// returns 1 if successful, 0 or negative for problem
// NOTE: need to call this in order to clear NewInput()

int jhcTaisRemote::Accept (const char *txt, int done)
{
  int ans = 1;

  *utxt = '\0';
  if ((txt != NULL) && (*txt != '\0'))
  {
    strcpy_s(utxt, txt);
    ans = send_listen(utxt);
  }
  if (done > 0)
    Shutdown();
  return ans;
}


//= Provide remote brain with robot sensors and get back robot commands and text.
// returns 1 if okay, 0 to quit, negative for problem

int jhcTaisRemote::Respond (jhcGenIO *io)
{
  // get new sensor values (text injected separately)
  if (body == NULL)
    return -4;
  body->Update();

  // communicate with remote brain
  if (send_sensor() <= 0)
    return -3;
  if (body->NewFrame())
    if (send_image(body->View()) <= 0)
      return -2;
  if (get_response() <= 0)
    return -1;

  // pass along text repsonse and motor commands
  if ((io != NULL) && (*btxt != '\0'))
  {
    io->ShutUp();
    io->Say(btxt);
  }
  body->Issue();
  return((esc > 0) ? 0 : 1);
}


//= Turn off remote brain by sending special shutdown message.

void jhcTaisRemote::Shutdown ()
{
  // check communications then send channel marker
  if (esc++ > 0)
    return;
  if (tx.ZChk() <= 0)
    return;
  tx.ZPrintf("from_user\n");

  // send stop command
  tx.ZPrintf("{\n");
  tx.ZPrintf("  \"message\": \"exit\"\n");
  tx.ZPrintf("  }\n");
  tx.ZEnd();
}


///////////////////////////////////////////////////////////////////////////
//                           Outgoing Data                               //
///////////////////////////////////////////////////////////////////////////

//= Transfer input from user to remote brain.

int jhcTaisRemote::send_listen (const char *txt)
{
  // check communications then send channel marker
  if (tx.ZChk() <= 0)
    return 0;
  tx.ZPrintf("from_user\n");

  // send string
  tx.ZPrintf("{\n");
  tx.ZPrintf("  \"message\": \"listen\",\n");
  tx.ZPrintf("  \"payload\": {\n");
  tx.ZPrintf("    \"text\": \"%s\"\n", txt);
  tx.ZPrintf("  }}\n");
  tx.ZEnd();
  return 1;
}


//= Transmit various readings from robot base to remote brain.

int jhcTaisRemote::send_sensor ()
{
  // check communications then send channel marker
  if ((tx.ZChk() <= 0) || (body == NULL))
    return 0;
  tx.ZPrintf("from_body\n");
 
  // generate packet with readings  
  tx.ZPrintf("{\n");
  tx.ZPrintf("  \"message\": \"sensor\",\n");
  tx.ZPrintf("  \"payload\": {\n");
  tx.ZPrintf("    \"xpos\": %3.1f,\n",      body->X());
  tx.ZPrintf("    \"ypos\": %3.1f,\n",      body->Y());
  tx.ZPrintf("    \"aim\": %3.1f,\n",       body->Heading());
  tx.ZPrintf("    \"width\": %3.1f,\n",     body->Width());
//  tx.ZPrintf("    \"force\": %3.1f,\n",     body->Squeeze());
  tx.ZPrintf("    \"height\": %3.1f,\n",    body->Height());
  tx.ZPrintf("    \"distance\": %3.1f,\n",  body->Distance());
  tx.ZPrintf("  }}\n");
  tx.ZEnd();
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Incoming Data                               //
///////////////////////////////////////////////////////////////////////////

//= Look for any commands or text generated by remote brain.

int jhcTaisRemote::get_response ()
{
  char kind[80];
  const char *tail;

  // make sure communications are working
  *btxt = '\0';
  if (rx.ZChk() <= 0)
    return 0;

  // read all waiting packets (keeps only last report)
  while (rx.ZRead() > 0)
  {
    if ((tail = find_tag("message", rx.Message())) == NULL)
      continue;
    if (trim_txt(kind, tail, 80) == NULL)
      continue;
    if (report_msg(kind, tail) > 0)
      continue;
    if (cmd_msg(kind, tail) > 0)
      continue;
  }
  return 1;
}


//= Find part of packet after given tag.
// returns pointer to remainder of packet, NULL if tag not found

const char *jhcTaisRemote::find_tag (const char *tag, const char *msg) const
{
  const char *start;

  if ((start = strstr(msg, tag)) == NULL)
    return NULL;
  if ((start = strchr(start, ':')) == NULL)
    return NULL;
  return(start + 1);
}


//= Get a string from message but strip off bounding quotation marks.
// returns pointer to string, NULL if fauled

const char *jhcTaisRemote::trim_txt (char *txt, const char *msg, int ssz) const
{
  const char *start, *end;

  if ((start = strchr(msg, '\"')) == NULL)
    return NULL;
  if ((end = strchr(start + 1, '\"')) == NULL)
    return NULL;
  strncpy_s(txt, ssz, start + 1, end - start - 1);     // always terminates
  return txt;
}


//= Check for and handle report message (if any).
// returns 1 if found, 0 otherwise

int jhcTaisRemote::report_msg (const char *kind, const char *data)
{
  const char *tail;

  if (strcmp(kind, "report") != 0)
    return 0;
  if ((tail = find_tag("payload", data)) != NULL)
    if ((tail = find_tag("text", tail)) != NULL)
      trim_txt(btxt, tail, 200);
  return 1;
}


//= Check for and handle command message (if any).
// returns 1 if found, 0 otherwise

int jhcTaisRemote::cmd_msg (const char *kind, const char *data)
{
  const char *tail;
  double val;

  if (strcmp(kind, "cmd") != 0)
    return 0;
  if ((tail = find_tag("payload", data)) != NULL)
    if (body != NULL)
    {
      // parse packet of values
      if (pull_float(val, "move", tail) > 0)
        body->MoveVel(val);
      if (pull_float(val, "turn", tail) > 0)
        body->TurnVel(val);
      if (pull_float(val, "lift", tail) > 0)
        body->LiftVel(val);
      if (pull_float(val, "grab", tail) > 0)
        body->Grab((int) val);
    }
  return 1;
}


//= Extract floating point value after given tag.
// returns 1 if successful, 0 if not found (value left unaltered)

int jhcTaisRemote::pull_float (double &val, const char *tag, const char *msg) const
{
  const char *start;

  if ((start = strstr(msg, tag)) != NULL)
    if ((start = strchr(start, ':')) != NULL)
      if (sscanf_s(start + 1, "%lf", &val) == 1)
        return 1;
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                           Image Transmission                          //
///////////////////////////////////////////////////////////////////////////

//= Send base64 encoding of JPEG form of image over ZeroMQ channel.
// returns 1 if okay, 0 or negative for error

int jhcTaisRemote::send_image (const jhcImg *img)
{
  char fname[] = "jhc_temp.jpg";
  FILE *in;
  UL32 val;
  int n;

  // check communications then save image to JPEG file
  if (tx.ZChk() <= 0)
    return -2;
  if (jio.Save(fname, *img) <= 0)
    return -1;
  if (fopen_s(&in, fname, "rb") != 0)
    return 0;

  // send channel marker and ZeroMQ packet header
  tx.ZPrintf("from_camera\n");
  tx.ZPrintf("{\n");
  tx.ZPrintf("  \"message\": \"camera\",\n");
  tx.ZPrintf("  \"payload\": \"");

  // send base64 encoding of JPEG
  while ((n = get24(val, in)) > 0)
    put24(val, n);

  // finish packet and cleanup
  tx.ZPrintf("\"\n");
  tx.ZPrintf("  }\n");
  tx.ZEnd();
  fclose(in);
  return 1;
}


//= Get up to three consecutive bytes from file.
// for sequence A,B,C outputs val = A:B:C (or A,B -> A:B:0)
// returns number of valid bytes read (0 if EOF)

int jhcTaisRemote::get24 (UL32& val, FILE *in) const
{
  int sh, v8, n = 0;

  val = 0;
  for (sh = 16; sh >= 0; sh -=8)
  {
    if ((v8 = fgetc(in)) == EOF)
      break;
    val |= ((v8 & 0xFF) << sh);
    n++;
  }
  return n;
}


//= Send given number of bytes (up to 3) as ASCII over ZeroMQ link.
// for val = a:b:c:d outputs sequence a,b,c,d
// <pre>
//   N1(sh>=12)  N2(sh>=6)   N3(sh>=0)
//   222211 11   1111 1100   00 000000
//   321098 76   5432 1098   76 543210
//   aaaaaa:bb - bbbb:cccc - cc:dddddd 
// </pre>

void jhcTaisRemote::put24 (UL32 val, int n)
{
  int sh, v6, stop = 6 * (3 - n);

  for (sh = 18; sh >= stop; sh -= 6)
  {
    v6 = (val >> sh) & 0x3F;
    tx.ZSend(cvt[v6]);
  }
}


//= Build lookup table for converting 6 bit chunks to ASCII characters.
// this is standard base64 coding (62,63 = +,/) with no padding

void jhcTaisRemote::build_cvt ()
{
  int i;

  for (i = 0; i < 26; i++)
    cvt[i] = (UC8)('A' + i);
  for (i = 0; i < 26; i++)
    cvt[i + 26] = (UC8)('a' + i);
  for (i = 0; i < 10; i++)
    cvt[i + 52] = (UC8)('0' + i);
  cvt[62] = (UC8) '+';
  cvt[63] = (UC8) '/';
}
