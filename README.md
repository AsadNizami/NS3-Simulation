# NS3-Simulation
Understanding the impact of MAC scheduling algorithms, numerology, and mobility in 5G NR

## Group 1

- Asad Nizami 19/11/EC/13
- Kumar Gaurav 19/11/EC/17

## Installation 

NS3

1. Use the following shell script: https://github.com/AsadNizami/Shell-script-for-NS3-installation/blob/main/run_install.sh
2. Mark the file executable and run ~$ ./run_install

## How to run:

1. Place the main.cc file inside the scratch directory.
2. Run ~$ ./waf --run "scratch/main.cc --numerology=<numerology> --seed=<seed> --udpFullBuffer=<0/1> --simTime=<time> --schedulerOpt=<algorithm>"


*The main.cc file is heavily inspired from 5G-LENA example- https://github.com/QiuYukang/5G-LENA/blob/master/examples/cttc-3gpp-channel-nums.cc*

