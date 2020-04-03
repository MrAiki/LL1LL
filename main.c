#include "compile.h"
#include <stdio.h>

int main(int argc, char** argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s program \n", argv[0]);
    return 1;
  }

  if (openSource(argv[1])) {
    compile();
    /* printCodeList(); */
    execute();
    closeSource();
  } else {
    return 1;
  }

  return 0;
}

