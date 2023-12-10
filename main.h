#include <simlib.h>
#include <stdio.h>
#include <string.h>

#include <iomanip>
#include <iostream>
#include <string>

using namespace std;

#define SEC 1.0
#define MIN 60 * SEC
#define HR 60 * MIN
#define DAY 24 * HR

#define OPEN 8 * HR    // Opening at 8:00
#define CLOSE 16 * HR  // Closing at 16:00

// Ski logic

#define CHAIRLIFT_MAX_CAPACITY_PER_HOUR 2800
#define CHAIRLIFT_LENGTH 1200  // m
#define CHAIRLIFT_SPEED 5      // m/s
#define CHAIRLIFT_DURATION CHAIRLIFT_LENGTH / CHAIRLIFT_SPEED

#define TBAR_MAX_CAPACITY_PER_HOUR 850
#define TBAR_LENGTH 1000  // m
#define TBAR_SPEED 3.4    // m/s
#define TBAR_DURATION TBAR_LENGTH / TBAR_SPEED

#define SLOPE_BLUE 0
#define SLOPE_RED 1
#define SLOPE_BLACK 2

#define LIFT_CHAIRLIFT 0
#define LIFT_TBAR 1

struct args_t {
    int cars_per_day;
    int buses_per_day;
    int people_per_day;

    int parking_lot_capacity;

    int tbar_power;
    int chairlift_power;
};

extern args_t args;

Queue chairliftQueue;
Queue tbarQueue;

Queue readyToLeave;

int totalRuns = 0;
int maxRuns = 0;
int totalSkiers = 0;

float totalTimeOnLift = 0;
float totalSkiingTime = 0;
float totalTimeInQueue = 0;

class Skier : public Process {
   public:
    bool hadRefreshment = false;
    bool uphill = false;
    int numOfRuns = 0;
    int finishTime;

    int getSkipassTime() {
        int res = Random() * 100;
        if (res < 15) {
            return 2*HR;
        }
        if (res < 50) {
            return 4*HR;
        } else {
            return 8*HR;
        }
    }

    void GoUpLift(int duration) {
        totalTimeOnLift += duration;
        Wait(duration);
        this->uphill = true;
        this->Activate();
    };

    int SelectLift() {
        int res = Random() * 100;
        int chosen;
        if (res < 80) {  // Chair
            chosen = LIFT_CHAIRLIFT;
        } else {  // Tbar
            chosen = LIFT_TBAR;
        }
        return chosen;
    }

    int SelectSlope() {
        int res = Random() * 100;
        if (res < 50)  // Red 1200m
        {
            return SLOPE_RED;
        } else if (res < 85)  // Blue 1500m
        {
            return SLOPE_BLUE;
        } else  // Black 1000m
        {
            return SLOPE_BLACK;
        }
    }

    void Behavior() {
        totalSkiers++;
        this->finishTime = Time + getSkipassTime();  // Get time of skipass finish
        Wait(Normal(10 * MIN, 2.5 * MIN));           // Get ready
    start:
        // Downhill station
        if (Time <= OPEN) {
            Wait(Uniform(2 * MIN, 5 * MIN));
            goto start;
        } else if (Time >= CLOSE || Time >= this->finishTime) {
            goto leave;
        } else {
            // Decide if to have refreshment or go ski
            if (!this->hadRefreshment) {
                if (Random() * 100 < 10) {
                    if (Time >= 11 * HR && Time <= 13 * HR) {  // Lunchtime
                        Wait(Normal(45 * MIN, 10 * MIN));
                    } else {  // Only quick snack
                        Wait(Normal(15 * MIN, 3 * MIN));
                    }
                    hadRefreshment = true;
                }
            }
            int lift = SelectLift();  // Decide which lift
            Wait(Normal(2*MIN, 15*SEC));// Get to the queue
            double queueTime = Time;
            if (lift == LIFT_TBAR) {
                Into(tbarQueue);
            } else {
                Into(chairliftQueue);
            }

            Passivate();  // Waiting in the queue
            queueTime = Time - queueTime;
            totalTimeInQueue += queueTime;

            if (!uphill) {  // Got kicked out of the queue
                goto leave;
            }

            // Uphill station
            int slope = SelectSlope();  // Decide which slope
            double skiingTime = 0;
            if (slope == SLOPE_RED) {   // Red
                skiingTime = (Normal(7.3 * MIN, 30 * SEC));
            } else if (slope == SLOPE_BLUE) {  // Blue
                skiingTime = (Normal(8.5 * MIN, 60 * SEC));
            } else {  // Black
                skiingTime = (Normal(6.1 * MIN, 40 * SEC));
            }
            Wait(skiingTime);
            totalSkiingTime += skiingTime;
            this->uphill = false;
            this->numOfRuns++;
            if(this->numOfRuns >= maxRuns) {
                maxRuns = this->numOfRuns;
            }
            goto start;
        }
    leave:
        totalRuns += numOfRuns;
        Into(readyToLeave);
        Passivate();
    };
};

class SkierGenerator : public Event {
    void Behavior() {
        Skier *skier = new Skier;
        skier->Activate();
    }
};

class Tbar : public Process {
    int getActualOccupancy() {
        int res = Random() * 100;
        if (res < 65) {
            return 2;
        } else if (res < 95) {
            return 1;
        } else {
            return 0;
        }
    };

    void Behavior() {
    start:
        int actual_per_hour = ((TBAR_MAX_CAPACITY_PER_HOUR / 2) * args.tbar_power / 100);
        double actual_per_second = actual_per_hour / (HR);
        int interval = (1 / actual_per_second);
        if (Time >= CLOSE) {
            goto stop;
        }
        Wait(interval * SEC);  // Interval between opening of gates
        if (!tbarQueue.Empty()) {
            int limit = (tbarQueue.Length() < 2) ? tbarQueue.Length() : this->getActualOccupancy();
            for (int i = 0; i < limit; i++) {
                Skier *skier = (Skier *)tbarQueue.GetFirst();
                skier->GoUpLift(TBAR_DURATION);
            }
        }
        goto start;
    stop:
        while (!tbarQueue.Empty()) {
            Skier *skier = (Skier *)tbarQueue.GetFirst();
            skier->Activate();
        }
        Passivate();
    }
};

class Chairlift : public Process {
    int getActualOccupancy() {
        int res = Random() * 100;
        if (res < 20) {
            return 6;
        } else if (res < 45)
            return 5;
        else if (res < 75) {
            return 4;
        } else if (res < 88) {
            return 3;
        } else if (res < 95) {
            return 2;
        } else if (res < 98) {
            return 1;
        } else {
            return 0;
        }
    };

    void Behavior() {
    start:
        int actual_per_hour = ((CHAIRLIFT_MAX_CAPACITY_PER_HOUR / 6) * args.chairlift_power / 100);
        double actual_per_second = actual_per_hour / (HR);
        int interval = (1 / actual_per_second);
        if (Time >= CLOSE) {
            goto stop;
        }
        Wait(interval * SEC);  // Interval between opening of gates
        if (!chairliftQueue.Empty()) {
            int limit = (chairliftQueue.Length() < 6) ? chairliftQueue.Length() : this->getActualOccupancy();
            for (int i = 0; i < limit; i++) {
                Skier *skier = (Skier *)chairliftQueue.GetFirst();
                skier->GoUpLift(CHAIRLIFT_DURATION);
            }
        }
        goto start;
    stop:
        while (!chairliftQueue.Empty()) {
            Skier *skier = (Skier *)chairliftQueue.GetFirst();
            skier->Activate();
        }
        Passivate();
    }
};

// Arrival logic

Store ParkingLot("Parkoviste", args.parking_lot_capacity);

int carsCouldntPark = 0;

class Car : public Process {
   public:
    unsigned int numberOfPeople;

    unsigned int getNumberOfPeople() {
        int res = Normal(3, 1);
        if (res < 1) {
            return 1;
        } else if (res < 2) {
            return 2;
        } else if (res < 3) {
            return 3;
        } else if (res < 4) {
            return 4;
        } else if (res < 5) {
            return 5;
        } else {
            return 6;
        }
    }

    void Behavior() {
        Wait(Normal(3.5 * HR, 0.7 * HR));  // Car waiting to arrive between cca 7:45 and 12:00
        this->numberOfPeople = getNumberOfPeople();

        if (ParkingLot.Full()) {
            carsCouldntPark++;
            Terminate();  // Car leaves because no parking
        }
        Enter(ParkingLot);
        // Generate skiers
        for (unsigned int i = 0; i < this->numberOfPeople; i++) {
            (new SkierGenerator())->Activate();
        }
    wait:
        Wait(1 * HR);
        // Wait until skiers are done skiing
        if (readyToLeave.Length() >= this->numberOfPeople) {
            for (unsigned int i = 0; i < this->numberOfPeople; i++) {
                Skier *skier = (Skier *)readyToLeave.GetFirst();
                skier->Terminate();
            }
        } else {
            goto wait;
        }
        Leave(ParkingLot);
    }
};

class Bus : public Process {
   public:
    unsigned int numberOfPeople;

    unsigned int getNumberOfPeople() {
        return Normal(40, 10);
    }

    void Behavior() {
        Wait(Uniform(1 * HR, 8 * HR));  // Buses arrive between 7:00 and 14:00
        this->numberOfPeople = getNumberOfPeople();
        for (unsigned int i = 0; i < this->numberOfPeople; i++) {
            (new SkierGenerator())->Activate();
        }
    wait:
        Wait(2 * HR);
        // Wait until skiers are done skiing
        if (readyToLeave.Length() >= this->numberOfPeople) {
            for (unsigned int i = 0; i < this->numberOfPeople; i++) {
                Skier *skier = (Skier *)readyToLeave.GetFirst();
                skier->Terminate();
            }
        } else {
            goto wait;
        }
    }
};

class CarGenerator : public Event {
    void Behavior() {
        Car *car = new Car;
        car->Activate();
    }
};

class BusGenerator : public Event {
    void Behavior() {
        Bus *bus = new Bus;
        bus->Activate();
    }
};
