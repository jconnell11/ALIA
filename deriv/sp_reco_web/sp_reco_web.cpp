// sp_reco_web.cpp : speech recognition using Microsoft Azure web service
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 IBM Corporation
// Copyright 2020-2024 Etaoin Systems
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

// NOTE: must install NuGet package "Microsoft.CognitiveServices.Speech" as described at
// https://learn.microsoft.com/en-us/azure/ai-services/speech-service/quickstarts/setup-platform

#include <combaseapi.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <speechapi_cxx.h>

#include "sp_reco_web.h"

using namespace Microsoft::CognitiveServices::Speech;


// Local function prototypes

BOOL init ();
BOOL shutdown ();
void next_chunk ();


///////////////////////////////////////////////////////////////////////////
//                          Global Variables                             //
///////////////////////////////////////////////////////////////////////////

//= Version information string.

static char info[80] = "Microsoft Azure Speech Recognition 2.10";


//= Microsoft Azure speech recognizer instance.

static std::shared_ptr<Microsoft::CognitiveServices::Speech::SpeechRecognizer> svc = NULL;


//= Special phrase list for adding people's names.

static std::shared_ptr<Microsoft::CognitiveServices::Speech::PhraseListGrammar> vocab = NULL;


//= Interface for determining if internet connection is working.

static std::shared_ptr<Microsoft::CognitiveServices::Speech::Connection> net = NULL;


//= COM object for controlling microphone muting.

static IAudioEndpointVolume *mic = NULL;


//= Most recent status from recognizer.

static int reco = 0;


//= Most recent recognition result.

static char heard[500] = "";


//= Most recent partial recognition.

static char part[500] = "";


//= Result duration plus endpoint silence (ms);

static int delay = 0;


//= Run-on utterance chunking variables.

static char blob[500] = "";
static const char *read = blob;


///////////////////////////////////////////////////////////////////////////
//                      Initialization and Locking                       //
///////////////////////////////////////////////////////////////////////////

//= DLL entry point.

BOOL APIENTRY DllMain (HANDLE hModule,
                       DWORD ul_reason_for_call, 
                       LPVOID lpReserved)
{
  if (ul_reason_for_call == DLL_PROCESS_DETACH)
    return shutdown();
  if (ul_reason_for_call != DLL_PROCESS_ATTACH)
    return TRUE;
  return init();
}


//= Do all system initializations.

BOOL init ()
{
  IMMDeviceEnumerator *list;
  IMMDevice *dev;

  CoInitialize(NULL);

  // get microphone control point
  CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *) &list);
  if (list != NULL)
  {
    list->GetDefaultAudioEndpoint(eCapture, eConsole, &dev);
    list->Release();
    if (dev != NULL)
    {
      dev->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *) &mic);
      dev->Release();
    }
  }
  return TRUE;
}


//= Do all clean up activities.

BOOL shutdown ()
{
  reco_cleanup();
  if (mic != NULL)
    mic->Release();

  CoUninitialize();
  return TRUE;
}


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                       //
///////////////////////////////////////////////////////////////////////////

//= Gives string with version number of DLL.

extern "C" DEXP const char *reco_version ()
{
  return info;
}


//= Configure system to process speech from default input source (microphone).
// reads required key and area from file:    config/sp_reco_web.key
// optionally reads special names from file: config/all_names.txt
// "reco" variable only changes on events, reset to zero after reco_status returns 2
// returns 1 + names if successful, 0 if cannot connect, neg for bad credentials

extern "C" DEXP int reco_setup ()
{
  std::shared_ptr<Microsoft::CognitiveServices::Speech::SpeechConfig> cfg;
  char name[80], key[80] = "", reg[40] = "";
  FILE *in;
  int i, n, cnt = 0;

  // remove any old service in case credentials change
  reco_cleanup();

  // read Azure web credentials from file and check format
  if (fopen_s(&in, "config/sp_reco_web.key", "r") != 0)
    return -3;                                                 // bad file
  fscanf_s(in, "%s %s", key, 80, reg, 40);
  fclose(in);
  if (((int) strlen(key) != 32) || (*reg == '\0'))             
    return -2;                                                 // bad format
  for (i = 0; i < 32; i++)
    if (isxdigit(key[i]) == 0)
      return -2;                                               // bad format

  // create instance of recognizer based on credentials
  if ((cfg = SpeechConfig::FromSubscription(key, reg)) == NULL)
    return -1;                                                 // invalid credentials
  cfg->SetProfanity(ProfanityOption::Raw);
  cfg->SetProperty(PropertyId::Speech_SegmentationSilenceTimeoutMs, "500");
  if ((svc = SpeechRecognizer::FromConfig(cfg)) == NULL)
    return 0;                                                  // no internet?

  // add proper spellings of names from file: config/all_names.txt
  vocab = PhraseListGrammar::FromRecognizer(svc);       
  if (fopen_s(&in, "config/all_names.txt", "r") == 0)
  {
    while (fgets(name, 80, in) != NULL)
      if ((n = (int) strlen(name)) > 0)
      {
        name[n - 1] = '\0';
        vocab->AddPhrase(name);                        // limit of 1024
        cnt++;
      }
    fclose(in);
  }

  // ---------------------------------------------------------------------------
  // CALLBACK: for partial result
  svc->Recognizing.Connect([] (const SpeechRecognitionEventArgs& e)
  {
    strcpy_s(part, (e.Result->Text).data());
    reco = 1;                                                  // speaking
  });

  // ---------------------------------------------------------------------------
  // CALLBACK: for final result
  svc->Recognized.Connect([] (const SpeechRecognitionEventArgs& e)
  {
    *part = '\0';
    if (e.Result->Reason == ResultReason::RecognizedSpeech)
    {
      const char *res = (e.Result->Text).data(); 
      if ((*res != '\0') && (strcmp(res, "Hey, Cortana.") != 0))      // quirk
      {
        delay = (int)(0.0001 * e.Result->Duration());
        strcpy_s(blob, res); 
        read = blob;
        reco = 2;
      }
    }
    else if (e.Result->Reason == ResultReason::NoMatch)
      reco = -1;                                               // unintelligible
  });

  // ---------------------------------------------------------------------------
  // CALLBACK: for network monitoring
  if ((net = Connection::FromRecognizer(svc)) != NULL)
    net->Disconnected.Connect([] (const ConnectionEventArgs& e)
    {
      reco = -2;                                               // lost network
    });

  // tell number of names
  return(cnt + 1);
}


//= Start processing speech right now.
// returns 1 if successful, 0 or negative for some error

extern "C" DEXP int reco_start ()
{
  reco = 0;
  *part = '\0';
  *heard = '\0';
  *blob = '\0';
  read = blob;
  if (svc == NULL)
    return -1;
  svc->StartContinuousRecognitionAsync().get();
  return 1;
}


//= Turn microphone on and off (e.g. to prevent TTS transcription).

extern "C" DEXP void reco_listen (int doit)
{
  mic->SetMute((doit <= 0), NULL);
}


//= Check to see if any utterances are ready for harvesting.
// return: 2 new result, 1 speaking, 0 silence, -1 unintelligible, -2 lost connection 

extern "C" DEXP int reco_status ()
{
  return reco;
}


//= Gives text string of ongoing recognition process.

extern "C" DEXP const char *reco_partial ()
{
  return part;
}


//= Gives text string of last full recognition result (changes status).

extern "C" DEXP const char *reco_heard ()
{
  next_chunk();
  return heard;
}


//= Gives approximate time (ms) that utterance started before notification.

extern "C" DEXP int reco_delay ()
{
  return delay;
}
 

//= Add a particular name to grammar to increase likelihood of correct spelling.
// can be called even when recognition is actively running

extern "C" DEXP int reco_name (const char *name)
{
  std::shared_ptr<Microsoft::CognitiveServices::Speech::PhraseListGrammar> vocab;

  if ((svc == NULL) || (vocab == NULL))
    return -1;
  if ((name == NULL) || (*name == '\0'))
    return 0;
  vocab->AddPhrase(name);                        // limit of 1024
  return 1;
}


//= Stop recognizing speech (can be restarted with reco_start).

extern "C" DEXP void reco_stop ()
{
  reco = 0;
  *part = '\0';
  *heard = '\0';
  *blob = '\0';
  read = blob;
  if (svc != NULL)
    svc->StopContinuousRecognitionAsync().get();
}


//= Stop recognizing speech and clean up all objects and files.

extern "C" DEXP void reco_cleanup ()
{
  reco_stop();
  vocab = NULL;              // smart pointer releases?
  net = NULL;                // smart pointer releases?
  svc = NULL;                // smart pointer releases?
}


///////////////////////////////////////////////////////////////////////////
//                           Response Chunking                           //
///////////////////////////////////////////////////////////////////////////

//= Use Inverse-Text-Normalized form to help break up long lexical results.
// blob = "I saw you in London. I then went to France."
// call 1 --> heard = "I saw you in London."   + reco = 2 (still)
// call 2 --> heard = "I then went to France." + reco = 0 (done)

void next_chunk ()
{
  char abbr[6][10] = {"Dr", "Mr", "Ms", "Mrs", "Prof", "St"};
  const char *end, *scan = read;
  int i, n, bk;

  // clear output then sanity check
  *heard = '\0';
  if (reco < 2)
    return;

  // look for terminal punctuation
  while ((end = strpbrk(scan, ".!?")) != NULL)
  {
    // ? and ! end immediately but ignore . inside word (e.g. "3.14")
    if (*end != '.')
      break;
    if (end[1] != ' ')  
    {
      scan = end + 1;
      continue;
    }
    n = (int)(end - read);

    // possibly expand "Dr" to "drive" (not if "Dr. Jones")
    if ((n >= 2) && (strncmp(end - 2, "Dr", 2) == 0))  
      if ((end[1] != ' ') || !isupper(end[2]) || (strchr("FB", end[2]) != NULL))              
      {
        // copy string without "Dr" then add replacement
        strncat_s(heard, read, n - 2);  
        strcat_s(heard, "drive");
        if (end[1] == '\0')             // end of blob
          strcat_s(heard, ".");         

        // keep scanning after any trailing space
        read = end + 1;
        scan = read;
        continue;
      }

    // check for allowable abbreviations
    for (i = 0; i < 6; i++)
      if ((bk = (int) strlen(abbr[i])) <= n)
        if (strncmp(end - bk, abbr[i], bk) == 0)
          break;
    if (i >= 6)                         // true end found
      break;
    scan = end + 1; 
  }

  // copy rest of allowed portion of string
  if (end != NULL)
    n = (int)(end - read) + 1;
  else
    n = (int) strlen(read);
  strncat_s(heard, read, n);

  // set up for remainder of blob (if any)
  read += n;
  if (read[0] != '\0')
    read++;                    // skip space
  else
    reco = 0;                  
}
