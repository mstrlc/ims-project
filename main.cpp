#include "main.h"

#define CARS_PER_DAY 500

using namespace std;

int main(int argc, char* argv[]) {
    Init(6*HR, 18*HR);
    RandomSeed(time(nullptr));

    chairliftQueue.SetName("Lanovka sestisedackova 6 osob");
    tbarQueue.SetName("Lanovka kotva 2 osoby");

    for (int i = 0; i < CARS_PER_DAY; i++) {
        (new CarGenerator())->Activate();
    }

    Chairlift* chairlift = new Chairlift();
    Tbar* tbar = new Tbar();

    chairlift->Activate(OPEN);
    tbar->Activate(OPEN);

    Run();

    chairliftQueue.Output();
    tbarQueue.Output();
    ParkingLot.Output();

    return 0;
}