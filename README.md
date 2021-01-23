# ALIA
## Architecture for Linguistic Interaction and Augmentation

The secret to AI is that there is no secret -- it is all programming, but the programming is done by other members of your culture! To this end ALIA provides an end-to-end symbolic cognitive system that is able to learn from natural language instructions via text or speech. 

Some examples of ALIA's use (and related papers) can be found at:

* Advice taking [video](https://youtu.be/EjzdjWy3SKM) and [paper](https://arxiv.org/abs/1911.09782)

* Guided perception [video](https://youtu.be/jZT1muSBjoc) and [paper](https://arxiv.org/abs/1911.11620)

To try out the system, first download the latest release of [MensEt](https://github.com/jconnell11/ALIA/releases/download/v3.80/MensEt_v420.zip) and unzip it to some folder. You will also need the Visual C++ 2019 [runtime](https://aka.ms/vs/16/release/vc_redist.x64.exe). After this start up the program MensEt.exe, select menu option "Demo / Local Text File", and pick something like [1-dance.tst](robot/MensEt/test/1-dance.tst). Hit ENTER to go on to the next sentence. You can also free-form type to the system using menu opton "Demo / > Interact Local". If ALIA does not understand you, it is likely you need to add some particular word to the [vocabulary.sgm](robot/MensEt/language/vocabulary.sgm) text file.

If you want to get fancier, you can switch over to speech mode. Use menu selection "Demo / Demo Options" and set "Read output always" to 1 to enable text-to-speech (native to Windows). You can also set "Speech (none, local, web)" to either 1 or 2. Option 1 uses Windows' built-in speech recognition (old and not very accurate). Option 2 ties into Microsoft's Azure online speech recognition, but you must first [set up an account](https://ms.portal.azure.com/#create/Microsoft.CognitiveServicesSpeechServices) then save your credentials in text file [sp_reco_web.key](robot/MensEt/sp_reco_web.key). For some guidance on what to say, see [this sheet](robot/MensEt/Robot_Dialog.pdf).

The system is implemented in [Visual C++ 2019](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&rel=16) (free) and should compile cleanly with the files here. However, to actually run the system you will need a slew of DLLs and some data files. These are all included in the release zips. Also available is the lastest release of the [Banzai](https://github.com/jconnell11/ALIA/releases/download/v3.80/MensEt_v420.zip) program. This uses the same NLU and reasoning engines, but controls a larger mobile manipulator, [ELI](robot/Banzai/ELI_robot.jpg). It is included here because it demonstrates how to create a "grounding" for a different physical robot via classes such as [jhcBallistic.cpp](robot/common/Grounding/jhcBallistic.cpp).

---

January 2021 - Jonathan Connell - jconnell@alum.mit.edu


