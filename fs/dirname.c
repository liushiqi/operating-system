#include <stddef.h>
#include <string.h>

char *dirname(char *path) {
  static const char dot[] = ".";
  char *last_slash;
  /* Find last '/'.  */
  last_slash = path != NULL ? strrchr(path, '/') : NULL;
  if (last_slash != NULL && last_slash != path && last_slash[1] == '\0') {
    /* Determine whether all remaining characters are slashes.  */
    char *runp;
    for (runp = last_slash; runp != path; --runp)
      if (runp[-1] != '/')
        break;
    /* The '/' is the last character, we have to look further.  */
    if (runp != path)
      last_slash = memrchr(path, '/', runp - path);
  }
  if (last_slash != NULL) {
    /* Determine whether all remaining characters are slashes.  */
    char *runp;
    for (runp = last_slash; runp != path; --runp)
      if (runp[-1] != '/')
        break;
    /* Terminate the path.  */
    if (runp == path) {
      /* The last slash is the first character in the string.  We have to
         return "/".  As a special case we have to return "//" if there
         are exactly two slashes at the beginning of the string.  See
         XBD 4.10 Path Name Resolution for more information.  */
      if (last_slash == path + 1)
        ++last_slash;
      else
        last_slash = path + 1;
    } else
      last_slash = runp;
    last_slash[0] = '\0';
  } else
    /* This assignment is ill-designed but the XPG specs require to
       return a string containing "." in any case no directory part is
       found and so a static and constant string is required.  */
    path = (char *)dot;
  return path;
}

char *basename(char *filename) {
  char *p;

  if (filename == NULL || filename[0] == '\0')
    /* We return a pointer to a static string containing ".".  */
    p = (char *)".";
  else {
    p = strrchr(filename, '/');

    if (p == NULL)
      /* There is no slash in the filename.  Return the whole string.  */
      p = filename;
    else {
      if (p[1] == '\0') {
        /* We must remove trailing '/'.  */
        while (p > filename && p[-1] == '/')
          --p;

        /* Now we can be in two situations:
     a) the string only contains '/' characters, so we return
        '/'
     b) p points past the last component, but we have to remove
        the trailing slash.  */
        if (p > filename) {
          *p-- = '\0';
          while (p > filename && p[-1] != '/')
            --p;
        } else
          /* The last slash we already found is the right position
             to return.  */
          while (p[1] != '\0')
            ++p;
      } else
        /* Go to the first character of the name.  */
        ++p;
    }
  }

  return p;
}
