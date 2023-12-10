#include <simlib.h>
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

Queue chairliftQueue;
Queue tbarQueue;

Queue readyToLeave;

class Skier : public Process {
   public:
    bool hadRefreshment = false;
    bool uphill = false;
    int numOfRuns = 0;
    int finishTime;

    int getSkipassTime() {
        int res = Random() * 100;
        if (res < 15) {
            return 2;
        }
        if (res < 50) {
            return 4;
        } else {
            return 8;
        }
    }

    void GoUpLift(int duration) {
        Wait(duration);
        this->uphill = true;
        this->Activate();
    };

    int SelectLift() {
        int res = Random() * 100;
        int chosen;
        if (res < 85) {  // Chair
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
        } else if (res < 80)  // Blue 1500m
        {
            return SLOPE_BLUE;
        } else  // Black 1000m
        {
            return SLOPE_BLACK;
        }
    }

    void Behavior() {
        Wait(Normal(10 * MIN, 2.5 * MIN));           // Get ready
        this->finishTime = Time + getSkipassTime();  // Get time of skipass finish
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
                        hadRefreshment = true;
                    } else {  // Only quick snack
                        Wait(Normal(15 * MIN, 3 * MIN));
                        hadRefreshment = true;
                    }
                }
            }
            int lift = SelectLift();  // Decide which lift
            if (lift == LIFT_TBAR) {
                // ! TODO time to queue
                Into(tbarQueue);
            } else {
                Into(chairliftQueue);
            }

            Passivate();  // Waiting in the queue

            if (!uphill) {  // Got kicked out of the queue
                goto leave;
            }

            // Uphill station
            int slope = SelectSlope();  // Decide which slope
            if (slope == SLOPE_RED) {   // Red
                Wait(Normal(5.3 * MIN, 30 * SEC));
            } else if (slope == SLOPE_BLUE) {  // Blue
                Wait(Normal(6.5 * MIN, 60 * SEC));
            } else {  // Black
                Wait(Normal(4.1 * MIN, 40 * SEC));
            }
            this->uphill = false;
            numOfRuns++;
            goto start;
        }
    leave:
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
    void Behavior() {
    start:
        if (Time >= CLOSE) {
            goto stop;
        }

        Wait(15 * SEC);  // ! TODO  jak rychle kotvy prijizdej
        if (!tbarQueue.Empty()) {
            int limit = (tbarQueue.Length() < 2) ? tbarQueue.Length() : 2;
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
    void Behavior() {
    start:
        if (Time >= CLOSE) {
            goto stop;
        }

        Wait(10 * SEC);  // ! TODO  cas branky
        int i;
        if (!chairliftQueue.Empty()) {
            int limit = (chairliftQueue.Length() < 6) ? chairliftQueue.Length() : 6;
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

#define PARKING_CAR_CAPACITY 300  // cars

Store ParkingLot("Parkoviste", PARKING_CAR_CAPACITY);

int carsCouldntPark = 0;

class Car : public Process {
   public:
    int numberOfPeople;

    int getNumberOfPeople() {
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
        for (int i = 0; i < this->numberOfPeople; i++) {
            (new SkierGenerator())->Activate();
        }
    wait:
        Wait(2 * HR);
        // Wait until skiers are done skiing
        if (readyToLeave.Length() >= this->numberOfPeople) {
            for (int i = 0; i < this->numberOfPeople; i++) {
                Skier *skier = (Skier *)readyToLeave.GetFirst();
                skier->Terminate();
            }
        } else {
            goto wait;
        }
        Leave(ParkingLot);
    }
};

class CarGenerator : public Event {
    void Behavior() {
        Car *car = new Car;
        car->Activate();
    }
};
