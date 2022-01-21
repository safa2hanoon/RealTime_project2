#ifndef __LOCAL_H_
#define __LOCAL_H_

/*
 * Common header file
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

// Keys to create shared memories
#define KEY 0x1000
#define BUS_KEY 0x2000
#define EXIT_KEY 0x3000
#define HALL_KEY 0x4000
#define DENIED_KEY 0x5000

// Queue types for Palestinians, Jordanians and Foreigners
#define QUEUE_P 3
#define QUEUE_J 2
#define QUEUE_F 1

// Passenger passport processing status
#define WAITING 1
#define PROCESSING 2
#define PROCESSED 3
#define DENIED 4

// Bus status
#define BUS_LOADING 1
#define BUS_TRIP 2

// Config file name
#define CONFIG_FILE "config.txt"

#endif
