/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    syscall.c

Abstract:

Environment:

    Kernel mode only

Author: 

    Klaus P. Gerlicher

Revision History:

    12-Nov-1999:	created
    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

////////////////////////////////////////////////////
// INCLUDES
////
#include "remods.h"
#include "precomp.h"

#include <linux/sched.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <linux/utsname.h>
#include <linux/sched.h>
#include <asm/unistd.h>

char syscallTemp[1024];

typedef struct _FRAME_SYSCALL
{
    ULONG eip;
    ULONG cs;
    ULONG eflags;
}FRAME_SYSCALL;

BOOLEAN bReportProcessEvents = TRUE;

ULONG OldSyscallHandler=0;

ULONG ulFreeModule=0;

struct module* pModJustFreed=NULL;
void (*old_cleanup_module)(void)=NULL;

void other_module_cleanup_module(void)
{
    DPRINT((0,"other_module_cleanup_module()\n"));

    if(old_cleanup_module)
    {
        DPRINT((0,"other_module_cleanup_module(): calling %x\n",(ULONG)old_cleanup_module));
        old_cleanup_module();
    }
    
    if(pModJustFreed)
    {
        DPRINT((0,"other_module_cleanup_module(): calling RevirtualizeBreakpointsForModule(%x)\n",(ULONG)pModJustFreed));
        RevirtualizeBreakpointsForModule(pModJustFreed);
    }
}

void CSyscallHandler(FRAME_SYSCALL* ptr,ULONG ulSysCall,ULONG ebx)
{
//	DPRINT((0,"CSyscallHandler(): %.4X:%.8X (syscall = %u)\n",ptr->cs,ptr->eip,ulSysCall));
    switch(ulSysCall)
    {
        case 1: // sys_exit
            DPRINT((0,"CSysCallHandler(): 1\n"));
			if(bReportProcessEvents)
			{
				PICE_sprintf(syscallTemp,"pICE: process destroyed \"%s\" PID=%.4X\n",current->comm,current->pid);
				AddToRingBuffer(syscallTemp);
			}
            break;
        case 11: // sys_execve
            DPRINT((0,"CSysCallHandler(): 11\n"));
			if(bReportProcessEvents)
			{
				if(PICE_strlen((char*)ebx))
					PICE_sprintf(syscallTemp,"pICE: process created \"%s\" PID=%.4X (parent \"%s\")\n",(char *)ebx,current->pid,current->comm);
				else
					PICE_sprintf(syscallTemp,"pICE: process created PID=%.4X (parent \"%s\")\n",current->pid,current->comm);
				AddToRingBuffer(syscallTemp);
			}
            break;
        case 128: // sys_init_module
            DPRINT((0,"CSysCallHandler(): 128\n"));
            if(PICE_strlen((char *)ebx))
            {
                if(pmodule_list)
                {
                    struct module* pMod = *pmodule_list;
                    do
                    {
                        if(PICE_strcmpi((char*)ebx,(LPSTR)pMod->name)==0)
                        {
                            ULONG ulInitAddress;
                            PICE_sprintf(syscallTemp,"pICE: module \"%s\" loaded (%x-%x init @ %x)\n",(char*)ebx,pMod,(ULONG)pMod+pMod->size,pMod->init);
                            if((ulInitAddress=FindFunctionInModuleByName("init_module",pMod)))
                            {
			                    DPRINT((0,"setting DR1=%.8x\n",ulInitAddress));

                                SetHardwareBreakPoint(ulInitAddress,1);
                            }
                        }
                    }while((pMod = pMod->next));
                }
                else
                {
                    PICE_sprintf(syscallTemp,"pICE: module loaded \"%s\"\n",(char *)ebx);
                }
            }
            else
                PICE_sprintf(syscallTemp,"pICE: module loaded\n");
            AddToRingBuffer(syscallTemp);
            break;
        case 129: // sys_delete_module
            DPRINT((0,"CSysCallHandler(): 129\n"));
            if(PICE_strlen((char *)ebx))
            {
                if(IsModuleLoaded((LPSTR)ebx)!=NULL && PICE_strcmpi((char*)ebx,"pice")!=0 )
                {
                    PICE_sprintf(syscallTemp,"pICE: module freed \"%s\"\n",(char *)ebx);
                    Print(OUTPUT_WINDOW,syscallTemp);
					if((pModJustFreed = FindModuleByName((char*)ebx)) )
					{
                        if(pModJustFreed->cleanup)
                        {
                            old_cleanup_module = pModJustFreed->cleanup;
                            pModJustFreed->cleanup = other_module_cleanup_module;
                        }
                        else
                        {
						    RevirtualizeBreakpointsForModule(pModJustFreed);
                        }
					}
                }
            }
            else
            {
                PICE_sprintf(syscallTemp,"pICE: module freed\n");
                AddToRingBuffer(syscallTemp);
            }
			break;
    }
}

__asm__ (" 
NewSyscallHandler:
		// save used regs
		pushfl
		cli
        cld
        pushal
	    pushl %ds 

        // push the syscall number
        pushl %ebx
        pushl %eax

        // frame ptr
        lea 48(%esp),%eax
        pushl %eax

	    // setup default data selectors 
	    movw %ss,%ax
	    movw %ax,%ds 

    	call CSyscallHandler

		// remove pushed params
        add $12,%esp

		// restore used regs
	    popl %ds 
        popal
		popfl

		// chain to old handler 
		.byte 0x2e
		jmp *OldSyscallHandler");

void InstallSyscallHook(void)
{
	ULONG LocalSyscallHandler;

	ENTER_FUNC();

	MaskIrqs();
	if(!OldSyscallHandler)
	{
		__asm__("mov $NewSyscallHandler,%0"
			:"=r" (LocalSyscallHandler)
			:
			:"eax");
		OldSyscallHandler=SetGlobalInt(0x80,(ULONG)LocalSyscallHandler);
	    
		ScanExports("free_module",(PULONG)&ulFreeModule);

		DPRINT((0,"InstallSyscallHook(): free_module @ %x\n",ulFreeModule));
	}
	UnmaskIrqs();

    LEAVE_FUNC();
}

void DeInstallSyscallHook(void)
{
	ENTER_FUNC();

	MaskIrqs();
	if(OldSyscallHandler)
	{
		SetGlobalInt(0x80,(ULONG)OldSyscallHandler);
        (ULONG)OldSyscallHandler=0;
	}
	UnmaskIrqs();

    LEAVE_FUNC();
}