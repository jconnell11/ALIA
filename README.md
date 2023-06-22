# ALIA
## Architecture for Linguistic Interaction and Augmentation

The secret to AI is that there is no secret! It is all just programming, but the programming is done by other members of your culture. To this end ALIA provides an end-to-end symbolic cognitive system that is able to learn from natural language instructions via text or speech. It can acquire specific likes and dislikes, generic problem solving skills, and socially appropriate reactions, as shown in this [video](https://youtu.be/Yoq7n6lGhYo). Building an agent by incrementally uploading (as opposed to DNN batch training) allows many opportunities for debugging so that the finally product is not alien, but more like a probabitionary member of society. You can read about the basic system design [here](https://arxiv.org/abs/1911.09782), or try out a Windows text-only version yourself by downloading [this](https://github.com/jconnell11/ALIA/releases/download/v5.10/alia_txt_v510.zip).

![Stealing stuff](red_handed.jpg)

## Demos

ALIA is good at learning sequential procedures, but it can also understand action prohibitions and even ignore certain users, as shown in this 
[video](https://youtu.be/EjzdjWy3SKM). Yet often the information needed to accomplish some task is not readily apparent, but instead must be sought out. Fortunately, ALIA can [take advice](https://arxiv.org/abs/1911.11620) about what sensing actions to perform, as illustrated in this [demo](https://youtu.be/jZT1muSBjoc) of guided perception.

![MensEt advice taking](grab_Mary.jpg) ![MensEt guided perception](tiger.jpg)

ALIA can also perform visual question answering as shown in the interaction below. This involves active information gathering, spatial inference, and hypothesis testing. To appreciate the complexity of the reasoning process, you can download the Banzai program (see below) and try some of your own questions while watching the log console.

![Banzai blocks demo](blocks_demo.bmp)

But it is not just about questions, ALIA can also orchestrate moderately sophisticated actions like stacking objects. In this [video](https://youtu.be/9sdTyfvoMPg) the user issues a basic verbal command then the robot automatically coordinates various visual analysis, trajectory planning, and body motion activties to accomplish the task. 

![Banzai manipulation](stacking_sm.jpg)

ALIA has even more capabilities beyond the ones shown here.
You can craft full behaviors conversationally, including iterative constructs like "all" and parallel constructs like "while". And there is a rudimentary editing system so you can say things like "No, don't grab it, point at it instead". There is also limited vocabulary (POS) inference and the text input panel has simple typo correction.
Finally, long term memory has recently been added. You can tell the system something like "Joe's wife is Jill" then "Remember that". Since all facts, rules, and operators get stored in the KB subdirectory, the next time you start the program you can ask "Who is Joe's wife" and get the proper answer. 

## Code

ALIA is built to run under Windows 10, and the files in this repository are meant to be compiled under [Visual C++ 2019](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&rel=16) (free). However the system is written in fairly vanilla C/C++ so it should be possible to port the core of the system to Linux. Also, although there is a text mode, the system is designed to be used with Microsoft's Azure online speech recognition. To set this up you must first [make an account](https://ms.portal.azure.com/#create/Microsoft.CognitiveServicesSpeechServices) then save your credentials in text file [sp_reco_web.key](robot/MensEt/sp_reco_web.key).

To try out the system, first download the latest release of [MensEt](https://github.com/jconnell11/ALIA/releases/download/v5.10/MensEt_v510.zip) and unzip it to some folder. You will also need a bunch of [DLLs](https://github.com/jconnell11/ALIA/releases/download/v5.10/MensEt_DLL.zip) and the Visual C++ 2019 [runtime](https://aka.ms/vs/16/release/vc_redist.x64.exe). After this, start up the program MensEt.exe, select menu option "Demo / Local Text File", and pick something like [1-dance.tst](robot/MensEt/test/1-dance.tst). Hit ENTER to go on to the next sentence. You can also free-form type to the system using menu opton "Demo / > Interact Local". If ALIA does not understand you, it is likely you need to add some particular word to the [vocabulary.sgm](robot/MensEt/language/vocabulary.sgm) text file.

If you want to get fancier, you can switch over to speech mode. Use menu selection "Demo / Demo Options" and set "Read output always" to 1 to enable text-to-speech (native to Windows). You can also set "Speech (none, local, web)" to either 1 or 2 for input. Option 1 uses Windows' built-in speech recognition (old and not very accurate) while Option 2 ties into Microsoft's Azure online speech recognition. For some guidance on what to say, see [this sheet](robot/MensEt/Robot_Dialog.pdf).

Also available is the lastest release of the [Banzai](https://github.com/jconnell11/ALIA/releases/download/v5.10/Banzai_v510.zip) program and its [DLLs](https://github.com/jconnell11/ALIA/releases/download/v5.10/Banzai_DLL.zip). This uses the same NLU and reasoning engines, but controls a larger mobile manipulator, [ELI](robot/Banzai/ELI_robot.jpg). It is included here because it demonstrates how to create a "grounding" for a different physical robot via classes such as [jhcBallistic.cpp](robot/common/Grounding/jhcBallistic.cpp). 
To try this out (without a robot) use menu selection "File / Open Video" and select [environ/blocks_t512.bmp](robot/Banzai/environ/blocks_t512.bmp). Then do "Demo / Text File" with [blocks_demo.tst](robot/Banzai/test/blocks_demo.tst). Keep hitting ENTER to go on to the next sentence. 

Finally, if you want to try integrating ALIA with you our robot consider using 
[alia_txt.h](audio/common/API/alia_txt.h) or [alia_sp.h](audio/common/API/alia_sp.h) with the corresponding DLL (under [Releases](https://github.com/jconnell11/ALIA/releases)). There are two very basic shell programs included as examples of how to do this: [txt_loop.cpp](deriv/alia_txt/txt_loop.cpp) and [sp_loop.cpp](deriv/alia_sp/sp_loop.cpp) (needs a valid sp_reco_web.key file). Of course for a physical robot you would also need to write your own grounding DLL like [basic_act.cpp](deriv/basic_act/basic_act.cpp) to interface to the robot hardware. Once you create this put it in the GND directory, add it to GND/kernels.lst, and supply a corresponding operator file in KB0. There is a add-on [sound_fcn](deriv/sound_fcn) DLL you can build and try plugging into any of the example programs using this method to see how it's done (copy both [SoundFcn](robot/MensEt/KB0) files).

---

June 2023 - Jonathan Connell - jconnell@alum.mit.edu


