#include "main.h"

using namespace std;

int main(int argc, char *argv[]) {
    Init(0, 86400);
    RandomSeed(time(nullptr));
    Run();

    return 0;
}