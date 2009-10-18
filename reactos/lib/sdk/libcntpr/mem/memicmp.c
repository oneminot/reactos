#include <string.h>
#include <ctype.h>

/*
 * @implemented
 */
int
_memicmp(const void *s1, const void *s2, size_t n)
{
  if (n != 0)
  {
    const unsigned char *p1 = s1, *p2 = s2;

    do {
      if (toupper(*p1) != toupper(*p2))
	return (*p1 - *p2);
      p1++;
      p2++;
    } while (--n != 0);
  }
  return 0;
}
