#include <time.h>
#include <stdio.h>

int main(){
    clock_t start, end;
    double time;

    float div = 10000.0;
    float div_res;

    start = clock();
    for(float i=0.0;i<10000000;i++){
        div_res = div / i;
    }
    end = clock();
    printf("Time used for division: %f\n",((double) (end - start)) / CLOCKS_PER_SEC);

    start = clock();
    for(float i=0.0;i<10000000;i++){
        div_res = div + i;
    }
    end = clock();
    printf("Time used for addition: %f\n",((double) (end - start)) / CLOCKS_PER_SEC);

}