# Light controller Application

## Overview
Light controller application runs on Ci40 borad. One MikroE board acts as awalwm2m client to simulate motion. Controller application acts as awalwm2m server and observes any motion event on MikroE board, and whenever there is a motion event, controller gets a notification for the same, and change led state to ON for 5 seconds. If further notifications are observed within the time out period then the time out is restarted.

Light controller application serves the purpose of:
- It acts as Awalwm2m server to communicate with Awalwm2m client that is running on a constrained device.

| Object Name               | Object ID      | Resource Name | Resource ID |
| :----                     | :--------------| :-------------| :-----------|
| "MotionSensorDevice" | 3302           | "SensorValue"      | 5501        |


## Prerequisites
Prior to running Light controller application, make sure that:
- Awalwm2m client daemon(awa_clientd) is running.
- Awalwm2m bootstrap daemon(awa_bootstrapd) is running.

**NOTE:** Please do "ps" on console to see "specific" process is running or not.

## Running Application on Ci40 board
Light controller application is getting started as a daemon. Although we could also start it from the command line as :

$ light_controller_appd

Output looks something similar to this :

Light Controller Application
```
------------------------
```

client session established


server session established



Waiting for constrained device 'MotionSensorDevice' to be up

Constrained device MotionSensorDevice registered


Value observed : 68

Turn ON led on Ci40 board


Turn OFF led on Ci40 board


Value observed : 69

Turn ON led on Ci40 board


Value observed : 70

Turn ON led on Ci40 board


Value observed : 71

Turn ON led on Ci40 board


Value observed : 72

Turn ON led on Ci40 board


Turn OFF led on Ci40 board
