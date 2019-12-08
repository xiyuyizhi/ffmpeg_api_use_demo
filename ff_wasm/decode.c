#include "structs.h"
#include "base.h"

#define MAIN_RUN
#define USE_THREAD

#ifdef MAIN_RUN
int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    printf("Usage: ./demo filePath xxx! \n");
    return 0;
  }
}
#endif