#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define LOG_TAG "WLAN_MAC"
#include "cutils/log.h"
#include "cutils/misc.h"

#define WLAN_MAC_PROC_FILE "/data/misc/wifi/wlan_mac"
#define WLAN_MAC_FILE "/data/misc/wifi/wlan_mac"
#define WLAN_MAC_SYS_FILE "/sys/class/net/wlan0/address"
#define DRIVER_MODULE_PATH  "/system/lib/modules/wlan.ko"
#define DRIVER_MODULE_NAME "wlan"

extern int init_module(void *, unsigned long, const char *);
extern int delete_module(const char *, unsigned int);

int store_wlan_mac(void)
{
	FILE *mac = NULL;
	char real_mac[32];
	int count = 50; //10 seconds

again:
	mac = fopen(WLAN_MAC_SYS_FILE, "r");
	if (mac == NULL)
	{
		LOGD("open %s failed.", WLAN_MAC_SYS_FILE);
		if (count-- > 0)
		{
			usleep(200000);
			goto again;
		}
		return -1;
	}

	fgets(real_mac, 32, mac);
	LOGD("Real mac: %s", real_mac);

	fclose(mac);

	mac = fopen(WLAN_MAC_FILE, "w+");
	if (mac == NULL)
	{
		LOGD("open %s failed.", WLAN_MAC_FILE);
		return -1;
	}
	fputs(real_mac, mac);
	fclose(mac);

	return 0;
}

int check_wlan_mac(void)
{
	int fd;

	fd = open(WLAN_MAC_FILE, O_RDONLY);
	if (fd < 0)
	{
		LOGD("Open [%s] failed.", WLAN_MAC_FILE);
		return -1;
	}
	close(fd);

	return 0;
}

static int insmod(const char *filename)
{
    void *module;
    unsigned int size;
    int ret;

    module = load_file(filename, &size);
    if (!module)
        return -1;

    ret = init_module(module, size, "");

    free(module);

    return ret;
}

static int rmmod(const char *modname)
{
    int ret = -1;
    int maxtry = 10;

    while (maxtry-- > 0) 
	{
        ret = delete_module(modname, O_NONBLOCK | O_EXCL);
        if (ret < 0 && errno == EAGAIN)
            usleep(500000);
        else
            break;
    }

    if (ret != 0)
        LOGD("Unable to unload driver module \"%s\": %s\n",
             modname, strerror(errno));
    return ret;
}

int main(void)
{
	int ret;

	LOGD("WLAN MAC checker is working......");

	ret = check_wlan_mac();
	if (ret == 0)
		return 0;

	/* Or we try to get MAC via WiFi driver. */
	ret = insmod(DRIVER_MODULE_PATH);	
	if (ret < 0)
	{
		LOGD("Install wifi driver failed.");
		rmmod(DRIVER_MODULE_NAME);
		return -1;
	}

	system("busybox ifconfig wlan0 up");

	/* We can try to store MAC to file. */
	store_wlan_mac();

	rmmod(DRIVER_MODULE_NAME);

	return 0;
}




