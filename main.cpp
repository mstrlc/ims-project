#include "main.h"

using namespace std;

#include <string>

args_t args;

args_t getArgs(int argc, char* argv[]) {
    args_t a;

    a.cars_per_day = 200;
    a.buses_per_day = 4;
    a.people_per_day = 50;
    a.tbar_power = 75;
    a.chairlift_power = 85;
    a.parking_lot_capacity = 300;

    if (argc >= 2) {
        for (int i = 1; i < argc; i++) {
            string opt = string(argv[i]);
            if (opt == "--cars") {
                a.cars_per_day = atoi(argv[i + 1]);
            } else if (opt == "--buses") {
                a.buses_per_day = atoi(argv[i + 1]);
            } else if (opt == "--people") {
                a.people_per_day = atoi(argv[i + 1]);
            } else if (opt == "--tbarpow") {
                a.tbar_power = atoi(argv[i + 1]);
            } else if (opt == "--chairpow") {
                a.chairlift_power = atoi(argv[i + 1]);
            } else if (opt == "--parking") {
                args.parking_lot_capacity = atoi(argv[i + 1]);
            }
        }
    }

    return a;
}

int main(int argc, char* argv[]) {
    args = getArgs(argc, argv);

    Init(6 * HR, 18 * HR);
    RandomSeed(time(nullptr));

    chairliftQueue.SetName("Lanovka sestisedackova 6 osob");
    tbarQueue.SetName("Lanovka kotva 2 osoby");

    ParkingLot.SetCapacity(args.parking_lot_capacity);

    for (int i = 0; i < args.cars_per_day; i++) {
        (new CarGenerator())->Activate();
    }

    for (int i = 0; i < args.buses_per_day; i++) {
        (new BusGenerator())->Activate();
    }

    for (int i = 0; i < args.people_per_day; i++) {
        (new PedestrianGenerator())->Activate();
    }

    Chairlift* chairlift = new Chairlift();
    Tbar* tbar = new Tbar();

    chairlift->Activate(OPEN);
    tbar->Activate(OPEN);

    Run();

    chairliftQueue.Output();
    tbarQueue.Output();
    ParkingLot.Output();
    float average = (float)totalRuns / (float)totalSkiers;
    printf("Total number of skiers: %d\n", totalSkiers);
    printf("Average runs per skier: %f\n", average);
    printf("Skier with most runs: %d\n", maxRuns);
    return 0;
}