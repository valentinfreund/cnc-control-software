# simple-cnc-software
CNC Software that provides basic motor control functionality for 3 axis, accompanied by a Graphical User Interface. 

The current Host Software can be tested without the STM32 and is currently configured to be used without

```bash
# Debian/Ubuntu systems
cd cnc-software-host_V1.2
make
./CNC_Control
```
Loads the blank.nc file into the editor on start-up. (has to be in the same directory as the bin)

### Split into two parts: 
-  Directory containing the Core files to upload on an STM32H723VGT6. 
-  Trajectory Planner with GUI and a GCode-Editor which runs under Linux (tested on Ubuntu 20.04) on a Host PC or RaspberryPi5

DISCLAIMER: Project is ongoing and still missing essential features which will be added in the future

### Pictures taken while testing:
![Alt Text](images/status_bereit.png)

![Alt Text](images/status_laufend.png)

![Alt Text](images/status_pause.png)
