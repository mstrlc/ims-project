#include <iostream>
#include <string>
#include <simlib.h>
#include <string.h>
#include <iomanip>

using namespace std;

class SkierGenerator : public Event {
    void Behavior() {
        Skier *skier = new Skier;
        skier->Activate();
    }
};

class Skier : public Process {
    void Behavior() {

    }
};