/* 
 * tiwlan driver loader - utility to load firmware and calibration data
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#ifdef ANDROID
#include <cutils/properties.h>
#include <cutils/log.h>
#include <hardware_legacy/power.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG 	"wlan_loader"
#define PROGRAM_NAME    "wlan_loader"
#endif
	
int check_and_set_property(char *prop_name, char *prop_val)
{
	char prop_status[PROPERTY_VALUE_MAX];
    	int count;

    	//In my environment, count need 10. so 100 should be enough.
    	for(count=200;( count != 0 );count--) 
    	{
        	property_set(prop_name, prop_val);
        	if(property_get(prop_name, prop_status, NULL) &&
            	   (strcmp(prop_status, prop_val) == 0) )
	    		break;
    	}

    	if( count ) 
	{
        	LOGD("count=%d Set property %s = %s - Ok\n", count, prop_name, prop_val);
    	}
    	else 
	{
        	LOGD("Set property %s = %s - Fail\n", prop_name, prop_val);
    	}
    	return count;
}

int main(void)
{
	int timeout = 16; //seconds
	char line[1024], *ptr;
	FILE *fp;

	acquire_wake_lock(PARTIAL_WAKE_LOCK, PROGRAM_NAME);

	LOGD("Checking Firmware status.......\n");

    	//First, make sure driver and firmware is working
	while (timeout > 0)
	{
		fp = fopen("/proc/net/wireless", "r");
		if (fp == NULL)
		{
			LOGD("Couldn't open /proc/net/wireless\n");
			timeout = 0;
			break;
		}
		fgets(line, 1024, fp);
		fgets(line, 1024, fp);
		ptr = fgets(line, 1024, fp);
		fclose(fp);
		if ((ptr != NULL) && (strstr(line, "wlan0:") != NULL))
			break;
		LOGD("Waiting for WIFI driver to be ready .... timeout=%d\n", timeout);
		timeout--;
		sleep(1);
	}
	if (timeout <= 0)
	{
        	LOGD("Timeout to wait for Wifi interface within 16 seconds.\n");
    		check_and_set_property("wlan.driver.status", "failed");
	}
	else
	{
        	LOGD("Wifi driver is ready for now. timeout=%d\n", timeout);
    		check_and_set_property("wlan.driver.status", "ok");
	}

	release_wake_lock(PROGRAM_NAME);

	return 0;
}

