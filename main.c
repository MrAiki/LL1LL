#include "compile.h"

int main(void)
{

  if (openSource("test.ll")) {
    initSource();
    while (1) {
      nextToken();
    }
  }

  closeSource();
}

