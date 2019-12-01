#include <stdio.h>
#include "./util.h"

char *concat_strarray(char *sarr[], int len)
{
  char *ret = NULL;
  int size = 0;

  for (int i = 0; i < len; i++)
  {
    size += strlen(sarr[i]);
  }
  ret = malloc(size + 1);
  *ret = '\0';
  for (int i = 0; i < len; i++)
  {
    strcat(ret, sarr[i]);
  }
  return ret;
}