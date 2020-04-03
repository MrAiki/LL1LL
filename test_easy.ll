func fib(x) {
  switch (x) {
    case 1,2: return x;
    default: return fib(x-1) + fib(x-2);
  }
}

func frac(n) {
  switch (n) {
    case 1: return 1;
    default: return n * frac(n-1);
  }
}

func msg(arg) {
  return "message : " + arg + "\n";
}

const test = "test";

var n
for (n = 1;n <= 15;n++) {
  stdout << msg(fib(n));
}
