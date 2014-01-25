/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            timer.c
 * PURPOSE:         Programmable Interval Timer emulation -
 *                  i82C54/8254 compatible
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "io.h"
#include "timer.h"
#include "pic.h"

/* PRIVATE VARIABLES **********************************************************/

static PIT_CHANNEL PitChannels[PIT_CHANNELS];
PPIT_CHANNEL PitChannel2 = &PitChannels[2];

/* PRIVATE FUNCTIONS **********************************************************/

static VOID PitLatchChannelStatus(BYTE Channel)
{
    if (Channel >= PIT_CHANNELS) return;

    /*
     * A given counter can be latched only one time until it gets unlatched.
     * If the counter is latched and then is latched again later before the
     * value is read, then this last latch command is ignored and the value
     * will be the value at the time the first command was issued.
     */
    if (PitChannels[Channel].LatchStatusSet == FALSE)
    {
        BYTE StatusLatch = 0;
        /* HACK!! */BYTE NullCount = 0;/* HACK!! */

        StatusLatch = PitChannels[Channel].Out << 7 | NullCount << 6;
        StatusLatch |= (PitChannels[Channel].ReadWriteMode & 0x03) << 4;
        StatusLatch |= (PitChannels[Channel].Mode & 0x07) << 1;
        StatusLatch |= (PitChannels[Channel].Bcd  & 0x01);

        PitChannels[Channel].LatchStatusSet = TRUE;
        PitChannels[Channel].StatusLatch    = StatusLatch;
    }
}

static VOID PitLatchChannelCount(BYTE Channel)
{
    if (Channel >= PIT_CHANNELS) return;

    /*
     * A given counter can be latched only one time until it gets unlatched.
     * If the counter is latched and then is latched again later before the
     * value is read, then this last latch command is ignored and the value
     * will be the value at the time the first command was issued.
     */
    if (PitChannels[Channel].ReadStatus == 0x00)
    {
        PitChannels[Channel].ReadStatus  = PitChannels[Channel].ReadWriteMode;

        /* Convert the current value to BCD if needed */
        PitChannels[Channel].OutputLatch = READ_PIT_VALUE(PitChannels[Channel],
                                                          PitChannels[Channel].CurrentValue);
    }
}

static VOID PitSetOut(PPIT_CHANNEL Channel, BOOLEAN State)
{
    if (State == Channel->Out) return;

    /* Set the new state of the OUT pin */
    Channel->Out = State;

    // /* Call the callback */
    // if (Channel->OutFunction) Channel->OutFunction(Channel->OutParam, State);
}

static VOID PitInitCounter(PPIT_CHANNEL Channel)
{
    switch (Channel->Mode)
    {
        case PIT_MODE_INT_ON_TERMINAL_COUNT:
            PitSetOut(Channel, FALSE);
            break;

        case PIT_MODE_HARDWARE_ONE_SHOT:
        case PIT_MODE_RATE_GENERATOR:
        case PIT_MODE_SQUARE_WAVE:
        case PIT_MODE_SOFTWARE_STROBE:
        case PIT_MODE_HARDWARE_STROBE:
            PitSetOut(Channel, TRUE);
            break;
    }
}

static VOID PitWriteCommand(BYTE Value)
{
    BYTE Channel       = (Value >> 6) & 0x03;
    BYTE ReadWriteMode = (Value >> 4) & 0x03;
    BYTE Mode     = (Value >> 1) & 0x07;
    BOOLEAN IsBcd = Value & 0x01;

    /*
     * Check for valid PIT channel - Possible values: 0, 1, 2.
     * A value of 3 is for Read-Back Command.
     */
    if (Channel > PIT_CHANNELS) return;

    /* Read-Back Command */
    if (Channel == PIT_CHANNELS)
    {
        if ((Value & 0x20) == 0) // Bit 5 (Count) == 0: We latch multiple counters' counts
        {
            if (Value & 0x02) PitLatchChannelCount(0);
            if (Value & 0x04) PitLatchChannelCount(1);
            if (Value & 0x08) PitLatchChannelCount(2);
        }
        if ((Value & 0x10) == 0) // Bit 4 (Status) == 0: We latch multiple counters' statuses
        {
            if (Value & 0x02) PitLatchChannelStatus(0);
            if (Value & 0x04) PitLatchChannelStatus(1);
            if (Value & 0x08) PitLatchChannelStatus(2);
        }
        return;
    }

    /* Check if this is a counter latch command... */
    if (ReadWriteMode == 0)
    {
        PitLatchChannelCount(Channel);
        return;
    }

    /* ... otherwise, set the modes and reset flip-flops */
    PitChannels[Channel].ReadWriteMode = ReadWriteMode;

    PitChannels[Channel].LatchStatusSet = FALSE;
    PitChannels[Channel].StatusLatch    = 0x00;

    PitChannels[Channel].ReadStatus  = 0x00;
    PitChannels[Channel].WriteStatus = 0x00;

    PitChannels[Channel].CountRegister = 0x00;
    PitChannels[Channel].OutputLatch   = 0x00;

    PitChannels[Channel].Pulsed = FALSE;


    // PitChannels[Channel].Out = FALSE; // <-- unneeded, see the PitInitCounter call below.

    /* Fix the current value if we switch to BCD counting */
    PitChannels[Channel].Bcd = IsBcd;
    if (IsBcd && PitChannels[Channel].CurrentValue > 9999)
        PitChannels[Channel].CurrentValue = 9999;

    switch (Mode)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        {
            PitChannels[Channel].Mode = Mode;
            break;
        }

        case 6:
        case 7:
        {
            /*
             * Modes 6 and 7 become PIT_MODE_RATE_GENERATOR
             * and PIT_MODE_SQUARE_WAVE respectively.
             */
            PitChannels[Channel].Mode = Mode - 4;
            break;
        }
    }

    PitInitCounter(&PitChannels[Channel]);
}

static BYTE PitReadData(BYTE Channel)
{
    LPBYTE ReadWriteMode = NULL;
    LPWORD CurrentValue  = NULL;

    /*
     * If the status was latched, the first read operation
     * will return the latched status, whichever the count
     * value or the status was latched first.
     */
    if (PitChannels[Channel].LatchStatusSet)
    {
        PitChannels[Channel].LatchStatusSet = FALSE;
        return PitChannels[Channel].StatusLatch;
    }

    /* To be able to read the count asynchronously, latch it first if needed */
    if (PitChannels[Channel].ReadStatus == 0) PitLatchChannelCount(Channel);

    /* The count is now latched */
    ASSERT(PitChannels[Channel].ReadStatus != 0);

    ReadWriteMode = &PitChannels[Channel].ReadStatus ;
    CurrentValue  = &PitChannels[Channel].OutputLatch;

    if (*ReadWriteMode & 1)
    {
        /* Read LSB */
        *ReadWriteMode &= ~1;
        return LOBYTE(*CurrentValue);
    }

    if (*ReadWriteMode & 2)
    {
        /* Read MSB */
        *ReadWriteMode &= ~2;
        return HIBYTE(*CurrentValue);
    }

    /* Shouldn't get here */
    ASSERT(FALSE);
    return 0;
}

static VOID PitWriteData(BYTE Channel, BYTE Value)
{
    LPBYTE ReadWriteMode = NULL;

    if (PitChannels[Channel].WriteStatus == 0x00)
    {
        PitChannels[Channel].WriteStatus = PitChannels[Channel].ReadWriteMode;
    }

    ReadWriteMode = &PitChannels[Channel].WriteStatus;

    if (*ReadWriteMode & 1)
    {
        /* Write LSB */
        *ReadWriteMode &= ~1;
        PitChannels[Channel].CountRegister &= 0xFF00;
        PitChannels[Channel].CountRegister |= Value;
    }
    else if (*ReadWriteMode & 2)
    {
        /* Write MSB */
        *ReadWriteMode &= ~2;
        PitChannels[Channel].CountRegister &= 0x00FF;
        PitChannels[Channel].CountRegister |= Value << 8;
    }

    /* ReadWriteMode went to zero: we are going to load the new count */
    if (*ReadWriteMode == 0x00)
    {
        if (PitChannels[Channel].CountRegister == 0x0000)
        {
            if (PitChannels[Channel].Bcd)
                PitChannels[Channel].CountRegister = 9999;
            else
                PitChannels[Channel].CountRegister = 0xFFFF; // 0x10000; // 65536
        }

        /* Convert the current value from BCD if needed */
        PitChannels[Channel].CountRegister = WRITE_PIT_VALUE(PitChannels[Channel],
                                                             PitChannels[Channel].CountRegister);
        PitChannels[Channel].ReloadValue = PitChannels[Channel].CountRegister;
    }
}

static BYTE WINAPI PitReadPort(ULONG Port)
{
    switch (Port)
    {
        case PIT_DATA_PORT(0):
        case PIT_DATA_PORT(1):
        case PIT_DATA_PORT(2):
        {
            return PitReadData(Port - PIT_DATA_PORT(0));
        }
    }

    return 0;
}

static VOID WINAPI PitWritePort(ULONG Port, BYTE Data)
{
    switch (Port)
    {
        case PIT_COMMAND_PORT:
        {
            PitWriteCommand(Data);
            break;
        }

        case PIT_DATA_PORT(0):
        case PIT_DATA_PORT(1):
        case PIT_DATA_PORT(2):
        {
            PitWriteData(Port - PIT_DATA_PORT(0), Data);
            break;
        }
    }
}

static VOID PitDecrementCount(PPIT_CHANNEL Channel, DWORD Count)
{
    switch (Channel->Mode)
    {
        case PIT_MODE_INT_ON_TERMINAL_COUNT:
        {
            /* Decrement the value */
            if (Count > Channel->CurrentValue)
            {
                /* The value does not reload in this case */
                Channel->CurrentValue = 0;
            }
            else Channel->CurrentValue -= Count;

            /* Did it fall to the terminal count? */
            if (Channel->CurrentValue == 0 && !Channel->Pulsed)
            {
                /* Yes, raise the output line */
                if (Channel == &PitChannels[0]) PicInterruptRequest(0);
                Channel->Pulsed = TRUE;
            }
            break;
        }

        case PIT_MODE_RATE_GENERATOR:
        {
            BOOLEAN Reloaded = FALSE;

            while (Count)
            {
                if ((Count > Channel->CurrentValue)
                    && (Channel->CurrentValue != 0))
                {
                    /* Decrease the count */
                    Count -= Channel->CurrentValue;

                    /* Reload the value */
                    Channel->CurrentValue = Channel->ReloadValue;

                    /* Set the flag */
                    Reloaded = TRUE;
                }
                else
                {
                    /* Decrease the value */
                    Channel->CurrentValue -= Count;

                    /* Clear the count */
                    Count = 0;

                    /* Did it fall to zero? */
                    if (Channel->CurrentValue == 0)
                    {
                        Channel->CurrentValue = Channel->ReloadValue;
                        Reloaded = TRUE;
                    }
                }
            }

            /* If there was a reload on channel 0, raise IRQ 0 */
            if ((Channel == &PitChannels[0]) && Reloaded) PicInterruptRequest(0);

            break;
        }

        case PIT_MODE_SQUARE_WAVE:
        {
            INT ReloadCount = 0;
            WORD ReloadValue = Channel->ReloadValue;

            /* The reload value must be even */
            ReloadValue &= ~1;

            while (Count)
            {
                if (((Count * 2) > Channel->CurrentValue)
                    && (Channel->CurrentValue != 0))
                {
                    /* Decrease the count */
                    Count -= Channel->CurrentValue / 2;

                    /* Reload the value */
                    Channel->CurrentValue = ReloadValue;

                    /* Increment the reload count */
                    ReloadCount++;
                }
                else
                {
                    /* Decrease the value */
                    Channel->CurrentValue -= Count * 2;

                    /* Clear the count */
                    Count = 0;

                    /* Did it fall to zero? */
                    if (Channel->CurrentValue == 0)
                    {
                        /* Reload the value */
                        Channel->CurrentValue = ReloadValue;

                        /* Increment the reload count */
                        ReloadCount++;
                    }
                }
            }

            if (ReloadCount == 0) break;

            /* Toggle the flip-flop if the number of reloads was odd */
            if (ReloadCount & 1)
            {
                Channel->Out = !Channel->Out;
            }

            /* Was there any rising edge on channel 0 ? */
            if (((Channel->Out && (ReloadCount == 1))
                || (ReloadCount > 1))
                && (Channel == &PitChannels[0]))
            {
                /* Yes, IRQ 0 */
                PicInterruptRequest(0);
            }

            break;
        }

        case PIT_MODE_SOFTWARE_STROBE:
        {
            // TODO: NOT IMPLEMENTED
            break;
        }

        case PIT_MODE_HARDWARE_ONE_SHOT:
        case PIT_MODE_HARDWARE_STROBE:
        {
            /* These modes do not work on x86 PCs */
            break;
        }
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID PitClock(DWORD Count)
{
    UINT i;

    if (Count == 0) return;

    for (i = 0; i < PIT_CHANNELS; i++)
    {
        // if (!PitChannels[i].Couting) continue;
        PitDecrementCount(&PitChannels[i], Count);
    }
}

DWORD PitGetResolution(VOID)
{
    INT i;
    DWORD MinReloadValue = 65536;

    for (i = 0; i < PIT_CHANNELS; i++)
    {
        DWORD ReloadValue = PitChannels[i].ReloadValue;

        /* 0 means 65536 */
        if (ReloadValue == 0) ReloadValue = 65536;

        if (ReloadValue < MinReloadValue) MinReloadValue = ReloadValue;
    }

    /* Return the frequency resolution */
    return PIT_BASE_FREQUENCY / MinReloadValue;
}

VOID PitInitialize(VOID)
{
    /* Register the I/O Ports */
    RegisterIoPort(PIT_COMMAND_PORT, NULL       , PitWritePort);
    RegisterIoPort(PIT_DATA_PORT(0), PitReadPort, PitWritePort);
    RegisterIoPort(PIT_DATA_PORT(1), PitReadPort, PitWritePort);
    RegisterIoPort(PIT_DATA_PORT(2), PitReadPort, PitWritePort);
}

/* EOF */
