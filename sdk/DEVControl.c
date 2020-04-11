#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#ifndef FEATURE_QMI_ANDROID
#include <syslog.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <mtd/mtd-user.h>
#include <sys/ioctl.h>
#include <linux/reboot.h>
#include <syscall.h>
#include <pthread.h>
#include <stdbool.h>

static void do_enter_recovery_reset(void)
{
	system("sync");
	sleep(2);
	syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, 
		LINUX_REBOOT_CMD_RESTART2, "recovery");
}


static int enter_recovery_mode_delay(void)
{
	pthread_t id;
	int ret;
	ret = pthread_create(&id, NULL, (void *)do_enter_recovery_reset, NULL);
	return 0;
}


int exec_cdelta_cmd(const char *path)
{
	FILE *fp = NULL;

	if(path == NULL)
	{
		#if defined (BUILD_VERSION_7500A)
		fp = fopen("/data/dme/start_DM_session", "w+");
		if(fp == NULL)
		{
			return -1;
		}	
		else
		{
			fclose(fp);
			system("sync");
		}
		#endif	
		system("echo off > /sys/power/autosleep");
		(void)enter_recovery_mode_delay();
	}
	else
	{
		if((strlen(path) > 240) || (strlen(path) == 0))
		{
			return -1;
		}
		fp = fopen("/cache/package_path","w+");
		if(fp == NULL)
		{
			return -1;
		}
		fwrite(path, strlen(path),1,fp);
		fclose(fp);
		system("sync");
		system("echo off > /sys/power/autosleep");    //fix ap sleep when at cmd from uart
		(void)enter_recovery_mode_delay();
	}

	return 0;
}


int exec_cusbadb_cmd(bool value)
{
	char aa1[2];
	char command[36];
	if(value == 1)
	{
		strcpy(aa1,"n");

	}
	else if(value == 0)
	{
		strcpy(aa1, "y");
	}
	else
	{
		return -1;
	}
	
	snprintf(command, 36, "delete_adb_mass_storage_functions %s",aa1);
	system(command);
	system("sync");
	return 0;
}

