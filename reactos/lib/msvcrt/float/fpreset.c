#include <msvcrt/float.h>


/*
 * @unimplemented
 */
void _fpreset(void)
{
  /* FIXME: This causes an exception */
//	__asm__ __volatile__("fninit\n\t");
  return;
}
