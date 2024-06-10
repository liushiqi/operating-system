#include <string.h>

char *strchrnul(const char *s, int c) {
  c = (unsigned char)c;
  if (!c)
    return (char *)s + strlen(s);

  for (; *s && *(unsigned char *)s != c; s++)
    ;
  return (char *)s;
}
