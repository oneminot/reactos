#ifndef _DMusProp_
#define _DMusProp_

//#include "dmusbuff.h"

#define STATIC_KSDATAFORMAT_SUBTYPE_DIRECTMUSIC\
    0x1a82f8bc,  0x3f8b, 0x11d2, {0xb7, 0x74, 0x00, 0x60, 0x08, 0x33, 0x16, 0xc1}
DEFINE_GUIDSTRUCT("1a82f8bc-3f8b-11d2-b774-0060083316c1", KSDATAFORMAT_SUBTYPE_DIRECTMUSIC);
#define KSDATAFORMAT_SUBTYPE_DIRECTMUSIC DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_DIRECTMUSIC)

#define STATIC_KSNODETYPE_DMSYNTH\
    0x94824f88, 0x6183, 0x11d2, {0x8f, 0x7a, 0x0, 0xc0, 0x4f, 0xbf, 0x8f, 0xef}
DEFINE_GUIDSTRUCT("94824F88-6183-11d2-8F7A-00C04FBF8FEF", KSNODETYPE_DMSYNTH);
#define KSNODETYPE_DMSYNTH DEFINE_GUIDNAMED(KSNODETYPE_DMSYNTH)

#define STATIC_KSNODETYPE_DMSYNTH_CAPS\
    0xbca2a2f1, 0x93c6, 0x11d2, {0xba, 0x1d, 0x0, 0x0, 0xf8, 0x75, 0xac, 0x12}
DEFINE_GUIDSTRUCT("bca2a2f1-93c6-11d2-ba1d-0000f875ac12", KSNODETYPE_DMSYNTH_CAPS);
#define KSNODETYPE_DMSYNTH_CAPS DEFINE_GUIDNAMED(KSNODETYPE_DMSYNTH_CAPS)

#define STATIC_KSPROPSETID_Synth_Dls\
    0xd523fa2c, 0xdee3, 0x11d1, {0xa7, 0x89, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12}
DEFINE_GUIDSTRUCT("d523fa2c-dee3-11d1-a789-0000f875ac12", KSPROPSETID_Synth_Dls);
#define KSPROPSETID_Synth_Dls DEFINE_GUIDNAMED(KSPROPSETID_Synth_Dls)

typedef enum
{
    KSPROPERTY_SYNTH_DLS_DOWNLOAD = 0,
    KSPROPERTY_SYNTH_DLS_UNLOAD,
    KSPROPERTY_SYNTH_DLS_COMPACT,
    KSPROPERTY_SYNTH_DLS_APPEND,
    KSPROPERTY_SYNTH_DLS_WAVEFORMAT
} KSPROPERTY_SYNTH_DLS;

typedef struct _SYNTH_BUFFER
{
    ULONG   BufferSize;
    PVOID   BufferAddress;
} SYNTH_BUFFER, *PSYNTH_BUFFER;

typedef struct _SYNTHDOWNLOAD
{
    HANDLE  DownloadHandle;
    BOOL    Free;
} SYNTHDOWNLOAD, *PSYNTHDOWNLOAD;

#define STATIC_KSPROPSETID_Synth\
    0xfedfae25L, 0xe46e, 0x11d1, {0xaa, 0xce, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12}
DEFINE_GUIDSTRUCT("fedfae25-e46e-11d1-aace-0000f875ac12", KSPROPSETID_Synth);
#define KSPROPSETID_Synth DEFINE_GUIDNAMED(KSPROPSETID_Synth)

typedef enum
{
    KSPROPERTY_SYNTH_VOLUME = 0,
    KSPROPERTY_SYNTH_VOLUMEBOOST,
    KSPROPERTY_SYNTH_CAPS,
    KSPROPERTY_SYNTH_PORTPARAMETERS,
    KSPROPERTY_SYNTH_CHANNELGROUPS,
    KSPROPERTY_SYNTH_VOICEPRIORITY,
    KSPROPERTY_SYNTH_LATENCYCLOCK,
    KSPROPERTY_SYNTH_RUNNINGSTATS
} KSPROPERTY_SYNTH;

#define SYNTH_PC_DLS                (0x00000001)
#define SYNTH_PC_EXTERNAL           (0x00000002)
#define SYNTH_PC_SOFTWARESYNTH      (0x00000004)
#define SYNTH_PC_MEMORYSIZEFIXED    (0x00000008)
#define SYNTH_PC_GMINHARDWARE       (0x00000010)
#define SYNTH_PC_GSINHARDWARE       (0x00000020)
#define SYNTH_PC_SYSTEMMEMORY       (0x7fffffff)

#if (NTDDI_VERSION < NTDDI_WINXP)
#define SYNTH_PC_REVERB             (0x00000040)
#elif (NTDDI_VERSION >= NTDDI_WINXP)
#define SYNTH_PC_XGINHARDWARE       (0x00000040)
#define SYNTH_PC_DLS2               (0x00000200)
#define SYNTH_PC_REVERB             (0x40000000)
#endif

typedef struct _SYNTHCAPS
{
    GUID    Guid;
    DWORD   Flags;
    DWORD   MemorySize;
    DWORD   MaxChannelGroups;
    DWORD   MaxVoices;
    DWORD   MaxAudioChannels;
    DWORD   EffectFlags;
    WCHAR   Description[128];
} SYNTHCAPS, *PSYNTHCAPS;


typedef struct _SYNTH_PORTPARAMS
{
    DWORD   ValidParams;
    DWORD   Voices;
    DWORD   ChannelGroups;
    DWORD   AudioChannels;
    DWORD   SampleRate;
    DWORD   EffectsFlags;
    DWORD   Share;
} SYNTH_PORTPARAMS, *PSYNTH_PORTPARAMS;

#define SYNTH_PORTPARAMS_VOICES           0x00000001
#define SYNTH_PORTPARAMS_CHANNELGROUPS    0x00000002
#define SYNTH_PORTPARAMS_AUDIOCHANNELS    0x00000004
#define SYNTH_PORTPARAMS_SAMPLERATE       0x00000008
#define SYNTH_PORTPARAMS_EFFECTS          0x00000020
#define SYNTH_PORTPARAMS_SHARE            0x00000040

#define SYNTH_EFFECT_NONE             0x00000000
#define SYNTH_EFFECT_REVERB           0x00000001
#define SYNTH_EFFECT_CHORUS           0x00000002
#define SYNTH_EFFECT_DELAY            0x00000004

typedef struct _SYNTHVOICEPRIORITY_INSTANCE
{
    DWORD   ChannelGroup;
    DWORD   Channel;
} SYNTHVOICEPRIORITY_INSTANCE, *PSYNTHVOICEPRIORITY_INSTANCE;

typedef struct _SYNTH_STATS
{
    DWORD   ValidStats;
    DWORD   Voices;
    DWORD   TotalCPU;
    DWORD   CPUPerVoice;
    DWORD   LostNotes;
    DWORD   FreeMemory;
    LONG    PeakVolume;
} SYNTH_STATS, *PSYNTH_STATS;


#define SYNTH_STATS_VOICES          (1 << 0)
#define SYNTH_STATS_TOTAL_CPU       (1 << 1)
#define SYNTH_STATS_CPU_PER_VOICE   (1 << 2)
#define SYNTH_STATS_LOST_NOTES      (1 << 3)
#define SYNTH_STATS_PEAK_VOLUME     (1 << 4)
#define SYNTH_STATS_FREE_MEMORY     (1 << 5)

#ifndef _DIRECTAUDIO_PRIORITIES_DEFINED_
#define _DIRECTAUDIO_PRIORITIES_DEFINED_

#define DAUD_CRITICAL_VOICE_PRIORITY    (0xF0000000)
#define DAUD_HIGH_VOICE_PRIORITY        (0xC0000000)
#define DAUD_STANDARD_VOICE_PRIORITY    (0x80000000)
#define DAUD_LOW_VOICE_PRIORITY         (0x40000000)
#define DAUD_PERSIST_VOICE_PRIORITY     (0x10000000)

#define DAUD_CHAN1_VOICE_PRIORITY_OFFSET    (0x0000000E)
#define DAUD_CHAN2_VOICE_PRIORITY_OFFSET    (0x0000000D)
#define DAUD_CHAN3_VOICE_PRIORITY_OFFSET    (0x0000000C)
#define DAUD_CHAN4_VOICE_PRIORITY_OFFSET    (0x0000000B)
#define DAUD_CHAN5_VOICE_PRIORITY_OFFSET    (0x0000000A)
#define DAUD_CHAN6_VOICE_PRIORITY_OFFSET    (0x00000009)
#define DAUD_CHAN7_VOICE_PRIORITY_OFFSET    (0x00000008)
#define DAUD_CHAN8_VOICE_PRIORITY_OFFSET    (0x00000007)
#define DAUD_CHAN9_VOICE_PRIORITY_OFFSET    (0x00000006)
#define DAUD_CHAN10_VOICE_PRIORITY_OFFSET   (0x0000000F)
#define DAUD_CHAN11_VOICE_PRIORITY_OFFSET   (0x00000005)
#define DAUD_CHAN12_VOICE_PRIORITY_OFFSET   (0x00000004)
#define DAUD_CHAN13_VOICE_PRIORITY_OFFSET   (0x00000003)
#define DAUD_CHAN14_VOICE_PRIORITY_OFFSET   (0x00000002)
#define DAUD_CHAN15_VOICE_PRIORITY_OFFSET   (0x00000001)
#define DAUD_CHAN16_VOICE_PRIORITY_OFFSET   (0x00000000)

#define DAUD_CHAN1_DEF_VOICE_PRIORITY   (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN1_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN2_DEF_VOICE_PRIORITY   (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN2_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN3_DEF_VOICE_PRIORITY   (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN3_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN4_DEF_VOICE_PRIORITY   (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN4_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN5_DEF_VOICE_PRIORITY   (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN5_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN6_DEF_VOICE_PRIORITY   (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN6_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN7_DEF_VOICE_PRIORITY   (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN7_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN8_DEF_VOICE_PRIORITY   (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN8_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN9_DEF_VOICE_PRIORITY   (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN9_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN10_DEF_VOICE_PRIORITY  (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN10_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN11_DEF_VOICE_PRIORITY  (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN11_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN12_DEF_VOICE_PRIORITY  (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN12_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN13_DEF_VOICE_PRIORITY  (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN13_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN14_DEF_VOICE_PRIORITY  (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN14_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN15_DEF_VOICE_PRIORITY  (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN15_VOICE_PRIORITY_OFFSET)
#define DAUD_CHAN16_DEF_VOICE_PRIORITY  (DAUD_STANDARD_VOICE_PRIORITY | DAUD_CHAN16_VOICE_PRIORITY_OFFSET)
#endif

typedef struct _SYNTH_REVERB_PARAMS
{
    float   fInGain;
    float   fReverbMix;
    float   fReverbTime;
    float   fHighFreqRTRatio;
} SYNTH_REVERB_PARAMS, *PSYNTH_REVERB_PARAMS;


#define STATIC_KSPROPSETID_SynthClock \
    0xfedfae26L, 0xe46e, 0x11d1, 0xaa, 0xce, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12
DEFINE_GUIDSTRUCT("fedfae26-e46e-11d1-aace-0000f875ac12", KSPROPSETID_SynthClock);
#define KSPROPSETID_SynthClock DEFINE_GUIDNAMED(KSPROPSETID_SynthClock)

typedef enum
{
    KSPROPERTY_SYNTH_MASTERCLOCK
} KSPROPERTY_SYNTHCLOCK;
#endif
