#include "compile.h"

int main(void)
{

  if (openSource("test_easy.ll")) {
    compile();
    /* printCodeList(); */
    execute();
  }

  closeSource();
}

