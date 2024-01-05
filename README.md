The publication that resulted from the data acquired using this code can be found in the _Cell Reports Methods_ article:

[**Opto-electronic feedback control of membrane potential for real-time control of action potentials**](https://doi.org/10.1016/j.crmeth.2023.100671)

Balázs Ördög*, Tim De Coster*, Sven O. Dekker, Cindy I. Bart, Juan Zhang, Gerard J.J. Boink, Wilhelmina H. Bax, Shanliang Deng, Bram L. den Ouden, Antoine A.F. de Vries, Daniël A. Pijnappels

# APqr_final

RTXI code that was used to produce opto-electronic dynamic patch-clamping data for real-time (re)shaping of cardiac action potentials.

This repository consists of four main software versions. Each of them describes an [RTXI](http://rtxi.org/) module that has a different use case. The four modules are:
1) APqr7
2) APqr8
3) APqrPID3
4) APqrPIDLTLP4

## Requirements

A PC running RT-Linux is necessary to run RTXI. All four modules should be able to run with RTXI, starting from RTXI version 2.2 upwards. For details on computer specifications, please check the [RTXI website](http://rtxi.org/). Our specific set-up on which the code was tested and implemented was:
* Ubuntu 16.04 LTS
* 32GB RAM
* Intel Core i7-4790 CPU
* Intel Corporation Xeon E3-1200 v3/4th Gen Core Processor Integrated Graphics Controller
* National Instruments PCI-6221 DAQ card
* RTXI 2.2 software

## Modules

### APqr7 (Code to acquire data for Fig. 2)

This RTXI module implements an adaptive P-controller (proportional to the error) where AP correction is carried out through current injection (re- and depolarizing).

### APqr8 (Code to acquire data for Fig. 3)

This RTXI module relies on the same principle as APqr7 (adaptive P-controller) but instead of current injection makes use of LEDs and optogenetics to create AP correction. It should be noted that only one way of correction is possible, either depolarizing or repolarizing correction.

### APqrPID3 (Code to acquire data for Figs. 4-5)

This RTXI module implements a PID controller (systems control technique making use of a proportional, integral, and derivative term). This version is capable of correcting the AP in both directions (re- and depolarizing) and once again relies on optogenetics.

### APqrPIDLTLP4 (Code to acquire data for Figs. 6-7)

This RTXI module can imprint any AP-shape on a cardiac cell. It provides upstroke pulses and AP-control all with the use of light (re- and depolarizing).
