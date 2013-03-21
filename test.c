#include<stdio.h>

int main()
{
 int *i = malloc(5*sizeof(int));
  i[0] = 0;
  i[1] = 1;
  i[2] = 2;
  i[3] = 3;
  i[4] = 4;

  i += 3;
  printf("%d\n", i[0]);
}
