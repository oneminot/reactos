/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         Generic Table Implementation
 * FILE:            lib/rtl/genertictbl.c
 * PROGRAMMERS:     
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
* @unimplemented
*/
BOOLEAN
STDCALL
RtlDeleteElementGenericTable (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
* @unimplemented
*/
BOOLEAN
STDCALL
RtlDeleteElementGenericTableAvl (
	PRTL_AVL_TABLE Table,
	PVOID Buffer
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlEnumerateGenericTable (
	PRTL_GENERIC_TABLE Table,
	BOOLEAN Restart
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlEnumerateGenericTableAvl (
	PRTL_AVL_TABLE Table,
	BOOLEAN Restart
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlEnumerateGenericTableLikeADirectory (
	IN PRTL_AVL_TABLE Table,
	IN PRTL_AVL_MATCH_FUNCTION MatchFunction,
	IN PVOID MatchData,
	IN ULONG NextFlag,
	IN OUT PVOID *RestartKey,
	IN OUT PULONG DeleteCount,
	IN OUT PVOID Buffer
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlEnumerateGenericTableWithoutSplaying (
	PRTL_GENERIC_TABLE Table,
	PVOID *RestartKey
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlEnumerateGenericTableWithoutSplayingAvl (
	PRTL_AVL_TABLE Table,
	PVOID *RestartKey
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlGetElementGenericTable(
	PRTL_GENERIC_TABLE Table,
	ULONG I
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlGetElementGenericTableAvl (
	PRTL_AVL_TABLE Table,
	ULONG I
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
VOID
STDCALL
RtlInitializeGenericTable (
	PRTL_GENERIC_TABLE Table,
	PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine,
	PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine,
	PRTL_GENERIC_FREE_ROUTINE FreeRoutine,
	PVOID TableContext
	)
{
	UNIMPLEMENTED;
}


/*
 * @implemented
 */
VOID STDCALL
RtlInitializeGenericTableAvl(IN OUT PRTL_AVL_TABLE Table,
                             IN PRTL_AVL_COMPARE_ROUTINE CompareRoutine,
                             IN PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine,
                             IN PRTL_AVL_FREE_ROUTINE FreeRoutine,
                             IN PVOID TableContext)
{
  RtlZeroMemory(Table,
                sizeof(RTL_AVL_TABLE));
  Table->BalancedRoot.Parent = &Table->BalancedRoot;

  Table->CompareRoutine = CompareRoutine;
  Table->AllocateRoutine = AllocateRoutine;
  Table->FreeRoutine = FreeRoutine;
  Table->TableContext = TableContext;
}


/*
* @unimplemented
*/
PVOID
STDCALL
RtlInsertElementGenericTable (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer,
	ULONG BufferSize,
	PBOOLEAN NewElement OPTIONAL
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlInsertElementGenericTableAvl (
	PRTL_AVL_TABLE Table,
	PVOID Buffer,
	ULONG BufferSize,
	PBOOLEAN NewElement OPTIONAL
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlInsertElementGenericTableFull (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer,
	ULONG BufferSize,
	PBOOLEAN NewElement OPTIONAL,
	PVOID NodeOrParent,
	TABLE_SEARCH_RESULT SearchResult
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlInsertElementGenericTableFullAvl (
	PRTL_AVL_TABLE Table,
	PVOID Buffer,
	ULONG BufferSize,
	PBOOLEAN NewElement OPTIONAL,
	PVOID NodeOrParent,
	TABLE_SEARCH_RESULT SearchResult
	)
{
	UNIMPLEMENTED;
	return 0;
}


/*
* @unimplemented
*/
BOOLEAN
STDCALL
RtlIsGenericTableEmpty (
	PRTL_GENERIC_TABLE Table
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
* @unimplemented
*/
BOOLEAN
STDCALL
RtlIsGenericTableEmptyAvl (
	PRTL_AVL_TABLE Table
	)
{
	UNIMPLEMENTED;
	return FALSE;
}


/*
* @unimplemented
*/
PVOID
STDCALL
RtlLookupElementGenericTable (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlLookupElementGenericTableAvl (
	PRTL_AVL_TABLE Table,
	PVOID Buffer
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlLookupElementGenericTableFull (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer,
	OUT PVOID *NodeOrParent,
	OUT TABLE_SEARCH_RESULT *SearchResult
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlLookupElementGenericTableFullAvl (
	PRTL_AVL_TABLE Table,
	PVOID Buffer,
	OUT PVOID *NodeOrParent,
	OUT TABLE_SEARCH_RESULT *SearchResult
	)
{
	UNIMPLEMENTED;
	return 0;
}


/*
 * @implemented
 */
ULONG STDCALL
RtlNumberGenericTableElements(IN PRTL_GENERIC_TABLE Table)
{
  return Table->NumberGenericTableElements;
}


/*
 * @implemented
 */
ULONG STDCALL
RtlNumberGenericTableElementsAvl(IN PRTL_AVL_TABLE Table)
{
  return Table->NumberGenericTableElements;
}

/* EOF */
