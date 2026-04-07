#include "mycalc.h"
#include <stdio.h>
#include <stdlib.h>

int mycalc(int argc, char **argv) {
  // argv[0] = "mycalc", argv[1] = num1, argv[2] = op, argv[3] = num2
  if (argc != 4) {
    fprintf(stderr, "Usage: mycalc <num1> <+|-|x|/> <num2>\n");
    return -1;
  }

  int a = atoi(argv[1]);
  int b = atoi(argv[3]);
  char op = argv[2][0];
  int result;

  if (op == '+') {
    result = a + b;
  } else if (op == '-') {
    result = a - b;
  } else if (op == 'x') {
    result = a * b;
  } else if (op == '/') {
    if (b == 0) {
      fprintf(stderr, "Error: division by zero\n");
      return -1;
    }
    result = a / b;
  } else {
    fprintf(stderr, "Error: unknown operator\n");
    return -1;
  }

  // print result
  printf("Operation: %d %c %d = %d\n", a, op == 'x' ? '*' : op, b, result);
  return 0;
}