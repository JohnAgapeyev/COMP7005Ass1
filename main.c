#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
    bool isClient = false;
    bool isServer = false;

    int option;

    while ((option = getopt(argc, argv, "cs")) != -1) {
        switch (option) {
            case 'c':
                isClient = true;
                break;
            case 's':
                isServer = true;
                break;
        }
    }
    if (isClient == isServer) {
        puts("This program must be run with either the -c or -s flag, but not both.");
        puts("Please re-run this program with one of the above flags.");
        puts("-c represents client mode, -s represents server mode");
        return EXIT_SUCCESS;
    }
    return EXIT_SUCCESS;
}
