/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrti.h>


#undef putchar
int
puts(const char *s)
{
	
  int c;
  while ((c = *s++))
    putchar(c);
  return putchar('\n');

}

int
_putws(const wchar_t *s)
{
	
  wint_t c;
  while ((c = *s++))
    putwchar(c);
  return putwchar(L'\n');

}
