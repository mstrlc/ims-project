import sys
import os
import subprocess
import time

cars_per_day = 200
buses_per_day = 4
tbar_power = 75
chairlift_power = 85
parking_lot_capacity = 300

if len(sys.argv) >= 2:
    for i in range(1, len(sys.argv)):
        opt = sys.argv[i]
        if opt == "--cars":
            cars_per_day = int(sys.argv[i + 1])
        elif opt == "--buses":
            buses_per_day = int(sys.argv[i + 1])
        elif opt == "--tbarpow":
            tbar_power = int(sys.argv[i + 1])
        elif opt == "--chairpow":
            chairlift_power = int(sys.argv[i + 1])
        elif opt == "--parking":
            parking_lot_capacity = int(sys.argv[i + 1])

print("Running simulation 14 times with the following parameters:")
print("Cars per day:            {}".format(cars_per_day))
print("Buses per day:           {}".format(buses_per_day))
print("T-bar power:             {}%".format(tbar_power))
print("Chairlift power:         {}%".format(chairlift_power))
print("Parking lot capacity:    {}".format(parking_lot_capacity))

total_skiers = 0
most_runs = 0
avg_runs_per_skier = 0
avg_waiting_queue_per_skier = 0
avg_going_up_per_skier = 0
avg_going_down_per_skier = 0
ratio_ski_wait = 0

runs = 14

for i in range(1, runs+1):
    try:
        output = subprocess.check_output(
            "./main --cars {} --buses {} --tbarpow {} --chairpow {} --parking {}".format(
                cars_per_day, buses_per_day, tbar_power, chairlift_power, parking_lot_capacity
            ),
            shell=True
        )
    except subprocess.CalledProcessError as e:
        runs -= 1
    print(".", end="", flush=True)
    output = output.decode("utf-8")
    lines = output.split("\n")
    for line in lines:
        if line.startswith("Total number of skiers:"):
            total_skiers += int(line.split(": ")[1])
        elif line.startswith("Skier with most runs:"):
            most_runs += int(line.split(": ")[1])
        elif line.startswith("Average runs per skier:"):
            avg_runs_per_skier += float(line.split(": ")[1])
        elif line.startswith("Average waiting in queue per skier:"):
            avg_waiting_queue_per_skier += float(line.split(": ")[1].split(" ")[0])
        elif line.startswith("Average going up lift per skier:"):
            avg_going_up_per_skier += float(line.split(": ")[1].split(" ")[0])
        elif line.startswith("Average going downhill per skier:"):
            avg_going_down_per_skier += float(line.split(": ")[1].split(" ")[0])
        elif line.startswith("Ratio of skiing to waiting (>1 == skiing > waiting):"):
            ratio_ski_wait += float(line.split(": ")[1])
    time.sleep(0.5)

print("")
print("Total number of skiers:              {:.2f}".format(total_skiers / runs))
print("Skier with most runs:                {:.2f}".format(most_runs / runs))
print("Average runs per skier:              {:.2f}".format(avg_runs_per_skier / runs))
print("Average waiting in queue per skier:  {:.2f}".format(avg_waiting_queue_per_skier / runs))
print("Average going up lift per skier:     {:.2f}".format(avg_going_up_per_skier / runs))
print("Average going downhill per skier:    {:.2f}".format(avg_going_down_per_skier / runs))
print("Ratio of skiing to waiting:          {:.2f}".format(ratio_ski_wait / runs))

