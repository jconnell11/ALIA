# ALIA
## Architecture for Linguistic Interaction and Augmentation

This is an end-to-end symbolic cognitive system that is able to learn from natural language instructions via text or speech. Some examples of its use (and related papers) on a small robot can be found at:

* Advice taking [video](https://youtu.be/EjzdjWy3SKM) and [paper](https://arxiv.org/abs/1911.09782)

* Guided perception [video](https://youtu.be/jZT1muSBjoc) and [paper](https://arxiv.org/abs/1911.11620)

The system is implemented in C++ for Windows because Microsoft's OS has embedded (non-network) real-time speech recognition. The code normally compiles under VS2010, but newer versions should work as well. A precompiled executable, "MensEt.exe", can be found in directory "robot/MensEt". To run this you will need the DLLs as well as the "KB", "KB2", "language" and "test" directories. You will also need the VC++ [2010](https://download.microsoft.com/download/3/2/2/3224B87F-CFA0-4E70-BDA3-3DE650EFEBA5/vcredist_x64.exe) and [2019](https://aka.ms/vs/16/release/vc_redist.x64.exe) runtimes. To see the sequences from the video use menu option "Demo / Local Text File" with [test/1-dance.tst](robot/MensEt/test/1-dance.tst) or [test/2-tiger.tst](robot/MensEt/test/2-tiger.tst). Hit ENTER to accept each line as it appears in the Conversation window.

Alternatively you can try running with Microsoft Azure speech recognition. The accuracy is generally better, but you need an active network connection and an [Azure account](https://ms.portal.azure.com/#create/Microsoft.CognitiveServicesSpeechServices). Enter the credentials you obtain into text file "sp_reco_web.key" in the executable directory. Then, for both MensEt and Banzai, select menu "Demo / Demo Options" and set "Speech" to 2. If you are interested in the NLU, you can see how ALIA transforms inputs by looking at the "log/cvt_xxxx.txt" file generated at the end of a run (cf. [NOTE_forms.cvt](robot/MensEt/test/NOTE_forms.cvt)).

---

#### Experimenting with the MensEt Program

The basic demo involves a physical robot and uses language structured as described in [this sheet](robot/MensEt/test/Robot_Dialog.pdf).
Note that the demos are meant to be used with a physical robot. However, one of the things the code can do by itself is play sound effects from the "sfx" subdirectory. The test sequence [test/ken_dance.tst](robot/MensEt/test/ken_dance.tst) shows how these are incorporated along with movements. 
You can also directly interact with the code by using menu option "Demo / > Interact Local" and typing into the the "Input" field of the Conversation window. Alternatively, you can try speech by first selecting "Demo / Demo Options" and setting "Speech input" to 1 and "Read output always" also to 1. 

Unfortunately, at this time the system does not have a full dictionary which means you will likely have to add words to the [language/vocabulary.sgm](robot/MensEt/language/vocabulary.sgm) file. Nouns go under the "AKO" list and verbs under the "ACT" list. You should also add the proper morphological variants (e.g. noun plural, verb progressive, etc.) to the "=[XXX-morph]" section at the end. The MensEt menu option "Utilities / Chk Grammar" can help with this. 

Even without the robot, you can try some of the vision functions. First, select menu option "Demo / Demo Options" and change "Camera avaliable" to 1. Next, do "File / Camera" or "File / Camera Adjust" to select some RGB camera connected to your computer (set for 640 pixels wide). You can then maximize the application window and try menu option "Vision / Object Seeds" to see some simple color-based segmentation of objects. The system expects a camera that is about 4 inches high looking obliquely across a uniformly colored table. The details of segmentation can be seen with "Vision / Ground Plane" and properties of the closest object with "Vision / Features". This is how it reacts to the user-supplied advice "If you see a yellow object, dance" found in "test/ken_dance.tst".

To understand the structure of the underlying code load "robot/MensEt/MensEt.sln" in Visual Studio and look at the calls in the tree below:

<sub>

```
  CMensEtDoc::OnDemoInteract                          // Demo / > Interact Local
    jhcMensCoord::Respond
      jhcAliaSpeech::UpdateSpeech                     // get speech recognition
        jhcSpeechX::Update
      jhcBackgRWI::Update                             // update sensors from robot
        jhcManusRWI::body_update
          jhcStackSeg::Analyze                        // do vision processing
      jhcAliaSpeech::Respond                        
        jhcAliaSpeech::xfer_input                     // -- main NLU call
          jhcAliaCore::Interpret
            jhcGramExec::Parse                        // generate CFG tree
            jhcGramExec::AssocList                    // extract association list
              jhcGramExec::tree_slots
            jhcGraphizer::Convert                     // pre-pend speech acts
              jhcNetBuild::Assemble                   // make core semantic net
        jhcAliaCore::RunAll                           // -- main reasoning call
          jhcAliaChain::Status 
            jhcAliaDir::Status                        // look for applicable operator
              jhcAliaDir::next_method
                jhcAliaDir::pick_method
                  jhcAliaDir::match_ops
                    jhcProcMem::FindOps       
                      jhcAliaOp::FindMatches
                        jhcAliaOp::try_mate
                          jhcSituation::MatchGraph    // subgraph matcher
```

</sub>

---

#### Grounding and the Banzai Program

This uses the same NLU and Reasoning engines, but controls a larger mobile manipulator, [ELI](robot/Banzai/ELI_robot.jpg). It is included here because it demonstrates how to create a "grounding" for a different physical robot. In particular look at the file [jhcBallistic.cpp](robot/common/Grounding/jhcBallistic.cpp) in conjunction with [Ballistic.ops](robot/Banzai/KB/Ballistic.ops) for motion primitives. Similarly [jhcSocial.cpp](robot/common/Grounding/jhcSocial.cpp) along with [Social.ops](robot/Banzai/KB/Social.ops) shows how person detection with a Kinect 360 depth sensor was integrated.

For comparison, the Manus robot used with the MensEt program has main grounding classes [jhcBasicAct.cpp](robot/common/Grounding/jhcBasicAct.cpp) and [jhcTargetVis.cpp](robot/common/Grounding/jhcTargetVis.cpp) and corresponding ALIA linkages of [BasicAct.ops](robot/MensEt/KB/BasicAct.ops) and [TargetVis.ops](robot/MensEt/KB/TargetVis.ops). Notice that in the "KB" subdirectory there are also ".sgm" files associated with these to provide appropriate vocabulary, and ".rules" files in some cases to provide useful inferences.

Finally, grounding classes often "volunteer" information when specific events occur, such as the "power_state" function in [jhcBasicAct.cpp](robot/common/Grounding/jhcBasicAct.cpp). For the Manus robot some of these are handled by [status.ops](robot/MensEt/KB2/status.ops). For the bigger ELI robot there are a number of events handled by [people.ops](robot/Banzai/KB2/people.ops), such as what to do if you see a person on the VIP face recognition list (cf. "vip_seen" in [jhcSocial.cpp](robot/common/Grounding/jhcSocial.cpp)).

As with MensEt, you can run Banzai without a physical robot, or with just a Kinect 360, by adjusting the settings in the "Demo / Demo Options" menu. This is useful, for instance, if you want to see the kind of spatial occupancy maps it builds (try menu "Environ / Height Map"). You can also try depth-based person finding (menu "Innate / Track Head") or greeting VIPs with "Demo / Interactive" (add people with "People / Enroll Live").

---

July 2020 - Jonathan Connell - jconnell@alum.mit.edu


