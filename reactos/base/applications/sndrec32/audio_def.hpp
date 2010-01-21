#ifndef _AUDIO_DEF__H_
#define _AUDIO_DEF__H_


#include <iostream>

//
// Defaults
//

#define _AUDIO_DEFAULT_FORMAT A44100_16BIT_STEREO

#define _AUDIO_DEFAULT_WAVEINBUFFERS 8
#define _AUDIO_DEFAULT_WAVEINBUFSECS 0.1f

#define _AUDIO_DEFAULT_WAVEOUTBUFFERS 2
#define _AUDIO_DEFAULT_WAVEOUTBUFSECS 1.0f

#define _AUDIO_DEFAULT_BUFSECS 1.0f


//
// Namespace stuff
//

#define _AUDIO_NAMESPACE_START_ namespace snd {
#define _AUDIO_NAMESPACE_END_ };

//
// Platform depend stuff
//

#include <windows.h>
#include <mmsystem.h> //Windows MultiMedia (WINMM) audio apis
#include <mmreg.h> //codecs stuff
#include <msacm.h> //codecs stuff

//#pragma comment(lib, "winmm.lib")
//#pragma comment(lib, "msacm32.lib")

#endif //ifdef _AUDIO_DEF__H_
