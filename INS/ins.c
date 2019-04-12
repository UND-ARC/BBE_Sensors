//Interial navigation system - Designed for ARC at the University of North Dakota

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

void main(void)
{
	int x, y, z;	//Position (calculated, x = xo + vt)
	int xo, yo, zo = 0;	//Initial position (previous position)
	int vox, voy, voz = 0;	//Initial velocity (previous velocity)
	int vx, vy, vz;	//Velocity (calculated, v = vo + at)
	int ax, ay, az;	//Acceleration (measured)

	while(1)
	{
		//IF we have gps
			//THEN
			//get x position from gps
			x = GPS("x");
			//Get z position from gps
			z = GPS("z");
		//Otherwise
			//Get x from INS
			//Get z from INS
		//IF we're below 50KM
		  //THEN get y from barometer
			y = BAROMETER();
		//Otherwise
		 	//get y from ins
		//Send x, y, z to ground
		SEND(x, y, z);	//As a wise man once said: send it.
	}
}
