#include "main.h"

using namespace std;

#include <string>

args_t args;

int main(int argc, char* argv[]) {
    args = getArgs(argc, argv);

    Init(6 * HR, 18 * HR);
    RandomSeed(time(nullptr));

    chairliftQueue.SetName("Lanovka sestisedackova 6 osob");
    tbarQueue.SetName("Lanovka kotva 2 osoby");

    for (int i = 0; i < args.cars_per_day; i++) {
        (new CarGenerator())->Activate();
    }

    for (int i = 0; i < args.buses_per_day; i++) {
        (new BusGenerator())->Activate();
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