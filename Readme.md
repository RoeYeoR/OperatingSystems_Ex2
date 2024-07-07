
This README provides instructions on how to run various programs included in this project. Each section below details specific command line usages for different scenarios.

## Prerequisites

Ensure all required software and dependencies are installed before running these programs.

## Installation

Follow the project's installation guide to set up necessary components.

## Usage Instructions

### Program 1: Simple Execution

bash
./ttt 123456789 # Replace "123456789" with any sequence of numbers


### Program 2: Using `mync` for Network Communication

bash
./mync -e "./ttt 123456789" # Replace "123456789" with any sequence of numbers


### Program 3: Multiple Networking Modes

#### Run 1:
bash
./mync -e "./ttt 123456789" -i TCPS4050
nc localhost 4050


#### Run 2:
bash
./mync -e "./ttt 123456789" -b TCPS4050
nc localhost 4050 # Output will also be shown on client


#### Run 3:
bash
nc -l 4455 # Terminal 1
./mync -e "./ttt 123456789" -i TCPS4050 -o TCPClocalhost,4455 # Terminal 2
nc localhost 4050 # Terminal 3


### Program 3.5: Different Terminal Interactions

#### Run 1:
bash
./mync -o TCPS4050 # Terminal 1
nc localhost 4050 # Terminal 2, input from Terminal 1 shown here


#### Run 2:
bash
./mync -i TCPS4050 # Terminal 1
nc localhost 4050 # Terminal 2, input from Terminal 2 shown in Terminal 1


#### Run 3:
bash
nc -l 4050 # Terminal 1
./mync -o TCPClocalhost,4050 # Terminal 2, interactions shown in Terminal 1


#### Run 4:
bash
./mync -b TCPS4050 # Terminal 1
nc localhost 4050 # Terminal 2, output shown on the same terminal


### Program 4: UDP and TCP Communication

#### Run 1:
bash
./mync -e "./ttt 123456789" -i UDPS4050 -t 10 # Terminal 1
nc -u localhost 4050


#### Run 2:
bash
nc -l 4455 # Terminal 1
./mync -e "./ttt 123456789" -i UDPS4050 -o TCPClocalhost,4455 # Terminal 2
nc -u localhost 4050 # Terminal 3


### Program 6: Unix Domain Socket Communication

#### Run 1:
bash
./mync -e "./ttt 123456789" -i UDSSS
nc -U "/tmp/new_server"


#### Run 2:
bash
./mync -e "./ttt 123456789" -i UDSSD 
nc -Uu "/tmp/new_server"


#### Run 3:
bash
nc -lU "/tmp/new_server"
./mync -e "./ttt 123456789" -o UDSCS