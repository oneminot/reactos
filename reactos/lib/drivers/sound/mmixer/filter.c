/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            lib/drivers/sound/mmixer/filter.c
 * PURPOSE:         Mixer Filter Functions
 * PROGRAMMER:      Johannes Anderwald
 */



#include "priv.h"

ULONG
MMixerGetFilterPinCount(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hMixer)
{
    KSPROPERTY Pin;
    MIXER_STATUS Status;
    ULONG NumPins, BytesReturned;

    // setup property request
    Pin.Flags = KSPROPERTY_TYPE_GET;
    Pin.Set = KSPROPSETID_Pin;
    Pin.Id = KSPROPERTY_PIN_CTYPES;

    // query pin count
    Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSPROPERTY), (PVOID)&NumPins, sizeof(ULONG), (PULONG)&BytesReturned);

    // check for success
    if (Status != MM_STATUS_SUCCESS)
        return 0;

    return NumPins;
}

MIXER_STATUS
MMixerGetFilterTopologyProperty(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hMixer,
    IN ULONG PropertyId,
    OUT PKSMULTIPLE_ITEM * OutMultipleItem)
{
    KSPROPERTY Property;
    PKSMULTIPLE_ITEM MultipleItem;
    MIXER_STATUS Status;
    ULONG BytesReturned;

    // setup property request
    Property.Id = PropertyId;
    Property.Flags = KSPROPERTY_TYPE_GET;
    Property.Set = KSPROPSETID_Topology;

    // query for the size
    Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), NULL, 0, &BytesReturned);

    if (Status != MM_STATUS_MORE_ENTRIES)
        return Status;

    // allocate an result buffer
    MultipleItem = (PKSMULTIPLE_ITEM)MixerContext->Alloc(BytesReturned);

    if (!MultipleItem)
    {
        // not enough memory
        return MM_STATUS_NO_MEMORY;
    }

    // query again with allocated buffer
    Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)MultipleItem, BytesReturned, &BytesReturned);

    if (Status != MM_STATUS_SUCCESS)
    {
        // failed
        MixerContext->Free((PVOID)MultipleItem);
        return Status;
    }

    // store result
    *OutMultipleItem = MultipleItem;

    // done
    return Status;
}

MIXER_STATUS
MMixerGetPhysicalConnection(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hMixer,
    IN ULONG PinId,
    OUT PKSPIN_PHYSICALCONNECTION *OutConnection)
{
    KSP_PIN Pin;
    MIXER_STATUS Status;
    ULONG BytesReturned;
    PKSPIN_PHYSICALCONNECTION Connection;

    /* setup the request */
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.Property.Id = KSPROPERTY_PIN_PHYSICALCONNECTION;
    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.PinId = PinId;

    /* query the pin for the physical connection */
    Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), NULL, 0, &BytesReturned);

    if (Status == MM_STATUS_UNSUCCESSFUL)
    {
        // pin does not have a physical connection
        return Status;
    }

    Connection = (PKSPIN_PHYSICALCONNECTION)MixerContext->Alloc(BytesReturned);
    if (!Connection)
    {
        // not enough memory
        return MM_STATUS_NO_MEMORY;
    }

    // query the pin for the physical connection
    Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)Connection, BytesReturned, &BytesReturned);
    if (Status != MM_STATUS_SUCCESS)
    {
        // failed to query the physical connection
        MixerContext->Free(Connection);
        return Status;
    }

    // store connection
    *OutConnection = Connection;
    return Status;
}

ULONG
MMixerGetControlTypeFromTopologyNode(
    IN LPGUID NodeType)
{
    if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_AGC))
    {
        // automatic gain control
        return MIXERCONTROL_CONTROLTYPE_ONOFF;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_LOUDNESS))
    {
        // loudness control
        return MIXERCONTROL_CONTROLTYPE_LOUDNESS;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_MUTE ))
    {
        // mute control
        return MIXERCONTROL_CONTROLTYPE_MUTE;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_TONE))
    {
        // tpne control
        //FIXME
        // MIXERCONTROL_CONTROLTYPE_ONOFF if KSPROPERTY_AUDIO_BASS_BOOST is supported
        // MIXERCONTROL_CONTROLTYPE_BASS if KSPROPERTY_AUDIO_BASS is supported
        // MIXERCONTROL_CONTROLTYPE_TREBLE if KSPROPERTY_AUDIO_TREBLE is supported
        UNIMPLEMENTED;
        return MIXERCONTROL_CONTROLTYPE_ONOFF;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_VOLUME))
    {
        // volume control
        return MIXERCONTROL_CONTROLTYPE_VOLUME;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_PEAKMETER))
    {
        // peakmeter control
        return MIXERCONTROL_CONTROLTYPE_PEAKMETER;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_MUX))
    {
        // mux control
        return MIXERCONTROL_CONTROLTYPE_MUX;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_MUX))
    {
        // mux control
        return MIXERCONTROL_CONTROLTYPE_MUX;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_STEREO_WIDE))
    {
        // stero wide control
        return MIXERCONTROL_CONTROLTYPE_FADER;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_CHORUS))
    {
        // chorus control
        return MIXERCONTROL_CONTROLTYPE_FADER;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_REVERB))
    {
        // reverb control
        return MIXERCONTROL_CONTROLTYPE_FADER;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_SUPERMIX))
    {
        // supermix control
        // MIXERCONTROL_CONTROLTYPE_MUTE if KSPROPERTY_AUDIO_MUTE is supported 
        UNIMPLEMENTED;
        return MIXERCONTROL_CONTROLTYPE_VOLUME;
    }
    UNIMPLEMENTED
    return 0;
}
