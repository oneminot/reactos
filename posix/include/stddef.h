/*
 * stddef.h
 *
 * standard type definitions. Based on the Single UNIX(r) Specification,
 * Version 2
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by KJK::Hyperion <noog@libero.it>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef __STDDEF_H_INCLUDED__
#define __STDDEF_H_INCLUDED__

/* INCLUDES */
#include <sys/types.h>

/* OBJECTS */

/* TYPES */
typedef signed long int ptrdiff_t;
typedef unsigned short int wchar_t;

/* CONSTANTS */
#ifndef NULL
#define NULL ((void *)(0)) /* Null pointer constant. */
#endif

/* PROTOTYPES */

/* MACROS */
#define offsetof(t,m) ((size_t) &((t *)0)->m)

#endif /* __STDDEF_H_INCLUDED__ */

/* EOF */

