/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <ntdll.h>

/*
 * @implemented
 */
int
atoi(const char *str)
{
  return (int)strtol(str, 0, 10);
}
