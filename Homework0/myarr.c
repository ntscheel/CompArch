#include <stdio.h>
void main(){
    int fiblen = 20;
    int fib [fiblen];
    fib[0] = 0;
    fib[1] = 1;
    for(int i = 2; i < fiblen; i++){
        fib[i] = fib[i-1] + fib[i-2];
    }
    for(int j = 0; j < fiblen; j++){
        printf("%d ", fib[j]);
    }
    printf("\n");
}