CSE438-Embedded Systems Programming

Assignment 3 Accessing kernel hash tables via a device file interface
Name: Neeraja Lahudva Kuppuswamy
ASU ID: 1224187432

########################################################

Overview
********

This is an application which demonstrates creation or device driver
which has a hash table data structure. The user is given two test 
script files from which based on read write commands, the use will
call device driver function to read from and write to the key buckets
of respective hashtable. Data read from hashtable is logged to output
files. Two threads are used to handle two input and output log files.

Requirements
************

Raspberry Pi Model 3B+ or higher
GCC compiler
make
Raspberry Pi Kernel Headers

Building and Running
********************

To Build:
: Open the RPi's terminal and navigate to project folder
: Make sure raspberry pi kernel headers are installed. If not, run command 'sudo apt-get install raspberrypi-kernel-headers' to install it and reboot the system
: Use command 'make'. This will compile assignment3.c file and create an executable assignment3 along with kernel object files
: Use command 'sudo insmod ht438_drv.ko' to load into the kernel module.
: You can two devices ht438_dev_0 and ht438_dev_1 in /dev directory.

To Run:
: Use command 'sudo ./assignment3 t1_0 t1_1' to run the program. Here t1_0 and t1_1 are input files.
: You can see the output in terms of data written and read from the device.
: The data read from device is logged to t1_out and t2_out files.
: You can open and check the logged data.
: After the execution use command 'sudo rmmod ht438_drv' to remove the kernel module.
: On terminal use comman 'dmesg' so see all the kernel printk.
: Comman 'make clean' cleans the project directory.
