# ALIA
## Architecture for Linguistic Interaction and Augmentation

The secret to AI is that there is no secret! It is all just programming, but the programming is done by other members of your culture. To this end ALIA provides an end-to-end symbolic cognitive system that is able to learn from natural language instructions via text or speech. It can acquire specific likes and dislikes, generic problem solving skills, and socially appropriate reactions, as shown in this [video](https://youtu.be/Yoq7n6lGhYo). 

![Stealing stuff](red_handed.jpg)

Building an agent by incrementally uploading (as opposed to DNN batch training) allows many opportunities for debugging so that the finally product is not alien, but more like a probabitionary member of society. You can read about the basic system design [here](https://arxiv.org/abs/1911.09782), or try out a text-only version by downloading for [Windows](https://github.com/jconnell11/ALIA/releases/download/v5.40/txt_loop.zip) (x64) or [Linux](https://github.com/jconnell11/ALIA/releases/download/v5.40/txt_loop_ix.zip) (ARM64).

## Capabilities

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

## Development GUI

The main development tool for ALIA is the [Banzai](robot/Banzai) program, which runs under Windows. It was built to control the [ELI](robot/Banzai/ELI_robot.jpg) class of homebrew robots, yet it can be run standalone as well. It is useful in this capacity since it shows many aspects of the internal state and lets you adjust numerous parameters. The system can be compiled with  [Visual C++ 2022](https://visualstudio.microsoft.com/downloads/?cid=learn-onpage-download-install-visual-studio-page-cta) Community (free), but there is also a pre-compiled executable. To run the program download the [exe](robot/Banzai/Banzai.exe) then unzip the contents of the [auxilliary files](releases/download/v5.40/Banzai_extras.zip) and [DLL bundle](releases/download/v5.40/Banzai_DLL.zip) (mostly face recognition support) to the same directory. You will also need to install the Visual C++ 2022 [runtime](https://aka.ms/vs/17/release/vc_redist.x64.exe)

Try out the system by starting up the program Banzai.exe, select menu option "Demo / Text File", and pick some input file such as [dont_grab.tst](robot/Banzai/test/dont_grab.tst). Hit ENTER to go on to the next sentence. For a fancier demo, use menu selection "File / Open Video" and select [environ/blocks_t512.bmp](robot/Banzai/environ/blocks_t512.bmp), then do "Demo / Text File" with [blocks_demo.tst](robot/Banzai/test/blocks_demo.tst). You can also free-form type to the system using menu opton "Demo / > Interactive". If ALIA does not understand you, it is likely you need to add some particular word to the [vocabulary.sgm](robot/Banzai/language/vocabulary.sgm) text file. 

In addition to the text mode, the system is designed to be used with Microsoft's Azure online speech recognition
This is essentially __free__ for low intensity usage, but you will need credentials to access this on-line service. Start by signing up [here](https://portal.azure.com/#create/Microsoft.CognitiveServicesSpeechServices) (possibly making a Microsoft account first) then select "Speech Services" and "+ Create". Finally, click "Manage keys" to find valid "Key" and "Location" strings. For the Windows version of things, these strings should be substituted in the text file [config/sp_reco_web.key](robot/Banzai/config/sp_reco_web.key). 

By default speech recognition is off, but you can use "Demo / Demo Options" to set "Speech" to 2 (web). In noisy environments it sometimes helps to require an initial acoustic wake-up word by setting "Wake" to 2 (front). In this case you must get the system's attention by starting with the word "robot" or "Banzai". Finally, you can also enable text-to-speech by setting "Read output always" to 1 (no Azure required).

A secondary GUI program, [MensEt](robot/MensEt), was built for a small forklift vehicle. It operates in a similar fashion to Banzai but has some fun sound effects. Try "Demo / Text File" with [ken_dance.tst](robot/MensEt/test/ken_dance.tst). Of course this is better with a physical robot ...

## Embedded Library

If you want to try integrating ALIA with your own robot consider using the [alia_act](audio/common/API/alia_act.h) library. This interfaces to an external robot through a pile of variables, but only does language and motion right now (perception is coming ...). The __related projects__ [Wansui](https://github.com/jconnell11/Wansui) (ROS) and [Ganbei](https://github.com/jconnell11/Ganbei) (Python) show how to use the [Linux version](deriv/alia_act/libalia_act.so) on two affordable commercial robots.

The Linux shared library can be rebuilt with Visual Studio and the solution file [alia_act_ix.sln](deriv/alia_act/alia_act_ix.sln). But to properly cross-compile you must first install the Windows Subsystem for Linux by opening a command line and typing:

    wsl --install -d Ubuntu-18.04

You then need to start the local copy of Ubuntu and install the gcc/g++ compiler for ARM64 using the command:

    sudo apt-get install g++-aarch64-linux-gnu

The pure Windows [DLL](deriv/alia_act/alia_act.dll), by contrast, only needs Visual Studio and the solution file [alia_act.sln](deriv/alia_act/alia_act.sln). 

---

May 2024 - Jonathan Connell - jconnell@alum.mit.edu


