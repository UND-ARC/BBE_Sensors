
#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>
#include<pthread.h>

#include "sysfs_helper.h" //Identifies chips, helps out in general
#include "types.h"        // 

const char* iio_dir = "/sys/bus/iio/devices/";
