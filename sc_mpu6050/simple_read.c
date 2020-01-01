//Simple read function for the accel/gyro on the BeagleBone Enhanced
//(Most) comments added by ARC at the University of North Dakota
//Original file can be found on the SanCloud GitHub repo BBE_Sensors


//The BBE reads its sensors into files found in /sys/bus/iio/devices/iio:device1
//Sysfs refers to the system's filesystem, which basically means that all info (whether a pin is active or inactive, etc) is found in a file somewhere on the BagelBone.

/*THINGS TO TRY:
*Remove len from read_sysfs_posint -- DONE
*Remove len from read_sysfs_string
*Remove err from main
**Make your own code that reads some data in
*/



#include <stdio.h> 	    //This line was originally #include "usr/include/stdio.h"
#include <stdlib.h>	    //.
#include <unistd.h>	    //.
#include <sys/types.h>      //.
#include <sys/stat.h>       //.
#include <fcntl.h>	    //.
#include <sys/select.h>	    //.
#include <pthread.h>	    //.
#include <string.h>	    //.
#include <ctype.h>	    //.

#include "types.h"          //
#include "sysfs_helper.h"   //Identifies chip

static pthread_t agread_id = NULL;		//

//statics
static char *dev_dir_name, *buf_dir_name;	//Device directory name, buffer directory name (?)
static bool reader_runnning = false;		//Used to determine if the thread in main has finished
static bool ag_pass = false;		        //Test to see if accel/gyro test passes, return error if it doesn't
static int ax, ay, az;		           	//Accel x, y, z
static int gx, gy, gz;		        	//Gyro x, y, z
static float fin_anglvel_scale, fin_accel_scale;//final(?) accel/velocity scale
static float fx, fy, fz;	                //final(?) x, y, z
static float fgx, fgy, fgz;          		//final(?) gyro(?) x, y, z

//constants
const char *iio_dir = "/sys/bus/iio/devices/";	//iio directory location

//normal
int ret, c, i;			        //Arbitrary variables (counting, etc)
int fp;				        //File pointer
char *trigger_name = NULL;		//Wow, they actually used a char* for a name.
int datardytrigger = 1;		  	//Data ready trigger(?)
char *data;		             	//One of the only self-documenting variables in this entire program
int dev_num, trig_num;	   		//Device number/trigger number
char chip_name[10];	      		//Self-documenting. Why isn't this a char*?
char device_name[10];	    		//Used to decide which iio:device we are using. Should this be a char*?
char sysfs[100];	         	//sysfs file path(?). Again, this should probably be a char*.


//read system filesystem position integer
int read_sysfs_posint(char *filename, char *basedir, int* val) {
    int ret = 0;	//Return (originally declared globally)
    int len = 0;	//length(?) (originally global)
    FILE *sysfsfp;	//sysfs filepath(?)
    char buff[20] = {0};//Buffer of size 20 initialized to 0
    char *endptr;	//End pointer
    int filedesc;	//File descriptor (a small, non-negative integer for use in subsequent system calls)
    long newval = 0;	//A new value that's not exactly self-documenting

    if(val != NULL){

		//Try to allocate memory, return error if we can't
		char *temp = malloc(strlen(basedir) + strlen(filename));
		if (temp == NULL) {
			DEBUG_MSG("Memory allocation failed");
			return -ENOMEM;		//Error: No memory
		}

		//Print temp to a string
		sprintf(temp, "%s/%s", basedir, filename);
		sysfsfp = fopen(temp, "r");	//open the file temp
		if (sysfsfp == NULL) {
			ret = -errno;
			goto error_free;
		}

		filedesc = open( temp, O_RDONLY );	//Open the file descriptor for read only
		len = 20;				//Initialize length of filedesc
		read( filedesc, buff, len );	 	//read from filedesc into the buffer
		read( filedesc, buff, len ); 		//read from filedesc into the buffer
		close(filedesc);			//Self-documenting

		newval = strtol(buff, &endptr, 10);	//Read the string buff to a long newval, base 10. Store the 								address of the first invalid character in endptr
		*val = (int)newval;			//Why not just use atoi?
		fclose(sysfsfp);			//Self-documenting
		error_free: free(temp);			//Segmentation fault (core dumped)
    }
    return ret;
}

int read_sysfs_float(char *filename, char *basedir, float *val) {
    int ret = 0;//Also declared globally
    FILE *sysfsfp;	//Some kind of file

    //Allocate memory for temp
    char *temp = malloc(strlen(basedir) + strlen(filename) + 2);

    //Make sure we allocated memory properly
    if (temp == NULL) {
        DEBUG_MSG("Memory allocation failed");
        return -ENOMEM;
    }

    sprintf(temp, "%s/%s", basedir, filename);
    sysfsfp = fopen(temp, "r");
    if (sysfsfp == NULL) {
        ret = -errno;
        goto error_free;
    }
    fscanf(sysfsfp, "%f\n", val);
    fclose(sysfsfp);	//Close the file sysfsfp
    error_free: free(temp);
    return ret;
}

int read_sysfs_string(char *filename, char *basedir, char* val) {
    int ret = 0;//Again, redeclared from global
    int len = 0;//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

    FILE *sysfsfp;

    if(val != NULL){
		//Allocate some memory and verify that it was allocated properly
		char *temp = malloc(strlen(basedir) + strlen(filename) + 2);
		if (temp == NULL) {
			DEBUG_MSG("Memory allocation failed");
			return -ENOMEM;
		}


		sprintf(temp, "%s/%s", basedir, filename);
		sysfsfp = fopen(temp, "r");
		if (sysfsfp == NULL) {
			ret = -errno;
			goto error_free;
		}
		len = fscanf(sysfsfp, "%s\n", val);
	    	len=len;
		fclose(sysfsfp);
		error_free: free(temp);
    }
    return ret;
}

/**************************************************************************
 * Function Name  : ag_read_thread (comment changed from read_thread)
 * Parameter      : none
 * Description    : Thread to supply gyro and accel data over a socket
 * Return         : None
 * Scope          : GLOBAL
 * Comment        : -
 **************************************************************************/
void* ag_read_thread(void* arg) {
	int loop_count = 0;
	read_sysfs_float("in_anglvel_scale", dev_dir_name, &fin_anglvel_scale);
    	read_sysfs_float("in_accel_scale", dev_dir_name, &fin_accel_scale);

    {
        //Put the column headers up...
        DEBUG_MSG( "  gz    gx    gy      fgz     fgx     fgy         az    ax    ay       faz     fax     fay\n");

        while ( (loop_count < 20) && (reader_runnning) ) {
            //for (j = 0; j < 10000; j++) {
        	//We need to detect a failure so set to pass to start
        	ag_pass = true;
        	if (read_sysfs_posint("in_accel_x_raw", dev_dir_name, &ax) != 0 )
        		ag_pass = false;
        	if (read_sysfs_posint("in_accel_y_raw", dev_dir_name, &ay) != 0 )
        		ag_pass = false;
        	if (read_sysfs_posint("in_accel_z_raw", dev_dir_name, &az) != 0 )
        		ag_pass = false;

        	if (read_sysfs_posint("in_anglvel_x_raw", dev_dir_name, &gx) != 0 )
        		ag_pass = false;
        	if (read_sysfs_posint("in_anglvel_y_raw", dev_dir_name, &gy) != 0 )
        		ag_pass = false;
        	if (read_sysfs_posint("in_anglvel_z_raw", dev_dir_name, &gz) != 0 )
        		ag_pass = false;

    		//We need to exit out and fail.. no point continuing
        	if(ag_pass == false)
        		goto error_out;

            //It looks like this chunk of code calibrates the gyros if it's tilted more than 10 units in a direction.
            if (abs(gx) < 10) {
                gx = 0;
            }

            if (abs(gy) < 10) {
                gy = 0;
            }

            if (abs(gz) < 10) {
                gz = 0;
            }


            //Likewise, this chunk calibrates the accelerometers
            if (abs(ax) < 10) {
                ax = 0;
            }

            if (abs(ay) < 10) {
                ay = 0;
            }

            if (abs(az) < 10) {
                az = 0;
            }

            fx = ax * fin_accel_scale;
            fy = ay * fin_accel_scale;
            fz = az * fin_accel_scale;
            fgx = gx * fin_anglvel_scale;
            fgy = gy * fin_anglvel_scale;
            fgz = gz * fin_anglvel_scale;

            //Convert from radians to degrees
            fgx *= 57.29577951307855f;
            fgy *= 57.29577951307855f;
            fgz *= 57.29577951307855f;

            	DEBUG_MSG( "%05d %05d %05d  %7.2f %7.2f %7.2f      %05d %05d %05d  %7.2f %7.2f %7.2f\n", gz, gx, gy, fgz, fgx, fgy, az, ax, ay, fz, fx, fy);

 			//Scale as the result is deg/s and our timebase is 100ms (10 times a second)
		    fgy /= 100.0f;
		    fgx /= 100.0f;
		    fgz /= 100.0f;

		    loop_count++;
            //DEBUG_MSG("%7.2f, %7.2f, %7.2f\n", fz, fx, fy);

            usleep(100000);//10ms sample rate
        }
    }
error_out:
    reader_runnning = false;

    pthread_exit(NULL);
}




//Discover the device
int discover (void)
{
	int ret = 0;	 //Return variable, also declared globally
	int asret = 0; 	 //Allocated string return

//    DEBUG_MSG("Getting sysfs path\n");
    if (sc_get_sysfs_path(sysfs) != SC_SUCCESS) {
        DEBUG_MSG("Failed to get sysfs path\n");
        ret = -ENODEV;	//Error: No device
        goto error_ret;
    }

//    DEBUG_MSG("sss:::%s\n", sysfs);
    if (sc_get_chip_name(chip_name) != SC_SUCCESS) {
        DEBUG_MSG("Failed to get chip name\n");
        ret = -ENODEV;	//Error: No device
        goto error_ret;
    }
    DEBUG_MSG("chip_name=%s\n", chip_name);

    for (i = 0; i < strlen(chip_name); i++) {
        device_name[i] = tolower(chip_name[i]);
    }
//    device_name[strlen(chip_name)] = '\0';
//    DEBUG_MSG("device name: %s\n", device_name);

    /* Find the device requested */
    dev_num = find_type_by_name(device_name, "iio:device");//find_type_by_name() can be found in sysfs_helper.c
    if (dev_num < 0) {
        DEBUG_MSG("Failed to find %s\n", device_name);
        ret = -ENODEV;	//Error: No device
        goto error_ret;
    }

//    DEBUG_MSG("iio device number being used is %d\n", dev_num);
    asprintf(&dev_dir_name, "%siio:device%d", iio_dir, dev_num);
    if (trigger_name == NULL) {


        /*
         * Build the trigger name. If it is device associated its
         * name is <device_name>_dev[n] where n matches the device
         * number found above
         */
    	asret = asprintf(&trigger_name, "%s-dev%d", device_name, dev_num);
      //Check this if statement - I think asret is null if memory is empty
        if (asret < 0) {
            ret = -ENOMEM;	//ENOMEM = Error: NO MEMory
            goto error_ret;
        }
    }

    /* Verify the trigger exists */
    trig_num = find_type_by_name(trigger_name, "trigger");
    if (trig_num < 0) {
        DEBUG_MSG("Failed to find the trigger %s\n", trigger_name);
        ret = -ENODEV;	//ENODEV = Error: NO DEVice
        goto error_ret;
    }
//    DEBUG_MSG("iio trigger number being used is %d\n", trig_num);
//    DEBUG_MSG("return val %d\n", ret);

    error_ret: return ret;
}



/*Things to check (posted 21/03/2019)
-Is err needed?
-Is reader_running needed?
-What does ag stand for?
*/
int main(void)
{
  int err;	//This can probably be deleted - see pthread_join
  char *b;	//This can probably be a void** - see pthread_join

  ret = discover();	//Discover the device, return null if valid.

  if( ret )
	  goto error_exit;
	
  //Create a thread that runs the function ag_read_thread and check to make sure it's running every 20ms
  err = pthread_create(&agread_id, NULL, &ag_read_thread, NULL);
  err = err;		//If you comment this out, you get a compiler warning. Otherwise, see reddit.com/r/badcode

  reader_runnning = true;

  while(reader_runnning){
	  usleep(200000);
  }

  //Join the thread
  if (agread_id != 0) {
      pthread_join(agread_id, (void**) &b);
  }

 error_exit:
  close(fp);  //Close filepath

  //Return a message to tell us whether the accel gyro test passed
  if(ag_pass){
  	ret = 0;
      DEBUG_MSG("Test SUCCESS\n");
  }
  else{
      DEBUG_MSG("Test error %d\n", ret);
  }

  //Free memory
  free(buf_dir_name);
  if (datardytrigger)
	free(trigger_name);
  return ret;
}
