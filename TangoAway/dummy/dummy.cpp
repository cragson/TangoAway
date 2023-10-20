#include <iostream>

#include <Windows.h>

static int counter = 0;

int main()
{
    SetConsoleTitleA("TangoAway: Dummy");

    printf("[+] Dummy says hello :^)\n");

    while (counter != 10000)
    {
        printf("[%p] Counter: %d\n", &counter, counter);

        counter++;

        Sleep(1500);
    }
} 