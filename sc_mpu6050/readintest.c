
#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>
#include<pthread.h>

#include "sysfs_helper.h" //Identifies chips, helps out in general
#include "types.h"        // 

const char* y_dir = "/sys/bus/iio/devices/iio:device1/in_accel_y_raw";	//Y accelerometer location
void main(void)
{
	long num;
	char* line;
	FILE* file = fopen(y_dir);
	fscanf(*file, line);
	num = atol(line);
	printf("%ld\n",line);
}
