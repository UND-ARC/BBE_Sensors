//Simple interial navigation system using the BeagleBone Enhanced onboard sensors
//Designed by ARC at the University of North Dakota

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

float GPS(const char dimension)
{
	//Our position in one dimension, should be accurate to within 10 meters
	//We'll need to know how to read from the GPS in order to fill this section in.
}

float INS(float a, float po, float vo, int t)
{
	return po + ((vo + (a*t))*t);
}

void SEND(float t, float x, float y, float z)
{
	//TEMPORARY CODE UNTIL TELEMETRY IS FINISHED
	printf("t: %.2f x:%.2f y:%.2f z:%.2f\n", time, x, y, z);
}

void main(void)
{
	//Units: Meters, seconds
	float x, y, z;			//Position (calculated, x = xo + vt)
	float xo, yo, zo = 0;		//Initial position (previous position)
	float vox, voy, voz = 0;	//Initial velocity (previous velocity)
	float vx, vy, vz;		//Velocity (calculated, v = vo + at)
	float ax, ay, az;		//Acceleration (measured)
	int to, t;			//Initial time, end time

	while(1)
	{
		//
		//IF we have gps
		if(y < 16000 && vy < 450)
		{
			//get x position from gps
			x = GPS("x");
			//Get z position from gps
			z = GPS("z");
		}

		else
		{
			//Get x from INS
			x = INS(ax, xo, vox, t);
			//Get z from INS
			z = INS(az, zo, voz, t);
		}

		//IF we have large changes in barometer (ie. If we're below 50k km
		if(y < 50000)
		{
		  //THEN get y from barometer
			y = BAROMETER();
		}
		
		//Otherwise, get y from INS
		else
		{
			//get y acceleration
			ay = ACCEL();
		 	//get y from ins
			y = INS(ay, yo, voy, t);
		}

		//Send x, y, z to ground
		SEND(t, x, y, z);	//As a wise man once said: send it.
	}
}
