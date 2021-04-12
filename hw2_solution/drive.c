#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    unsigned int seed;
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <seed>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    seed = atoi(argv[1]);
    srand(seed);

    for (int i = 0; i < 200; i++)
    {
        usleep((1+ rand() % 3000)*1000);
        //printf("pkill -SIGUSR1 control1\n");
        system("pkill -SIGUSR1 control1");
    }
    printf("Completed sending 200 SIGUSR1.\n");
    return 0;
}
