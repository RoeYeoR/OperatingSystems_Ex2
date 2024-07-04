HOW TO RUN:


prog3: 

#Run1:
./mync -e "ttt 123456789" -i TCPS4050
nc localhost 4050

#Run2:
./mync -e "ttt 123456789" -b TCPS4050 
nc localhost 4050 // output also will be shown on client

#Run3
nc -l 4455 //terminal 1
./mync -e "ttt 123456789" -i TCPS4050 -o TCPClocalhost,4455 //terminal 2
nc localhost 4050 //terminal 3

prog3.5: 

#Run1:
./mync -o TCPS4050 // terminal 1
nc localhost 4050 // terminal 2. every input from terminal 1 will be shown in terminal 2

#Run2:
./mync -i TCPS4050 // terminal 1
nc localhost 4050 // terminal 2. every input from termina2  will be shown in terminal 1

#Run3:
nc -l 4050 // terminal 1
./mync -o TCPClocalhost,4050 // terminal 2. every client who connected to port 4050 can write and it will be shown in terminal 1.

#Run4:
./mync -b TCPS4050 // terminal 1
nc localhost 4050 // terminal 2. every output will be shown on the same terminal(2).




