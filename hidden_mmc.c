#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/vmalloc.h>
#include <linux/blkdev.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include <linux/slab.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/memory.h>
#include <asm/segment.h>

//#include "hidden_api.h"
#include "hidden_mmc.h"
#define DBG printk
#if 1

static loff_t get_size(void){

	struct file* filp=NULL;
	mm_segment_t oldfs;

	unsigned char* data = kzalloc(sizeof(unsigned long long), GFP_KERNEL);
	loff_t mmc_size = 0;

	int err=-1, ret;
	loff_t pos = 0;

	oldfs = get_fs();
	set_fs(get_ds());

	filp = filp_open(SIZE_PATH, O_RDONLY ,GFP_KERNEL);

	if(IS_ERR(filp)){
		err = PTR_ERR(filp);
		goto err;
	}

	ret =  vfs_read(filp, data, sizeof(unsigned long long), &pos);

	if(ret < 0){
		DBG(KERN_INFO "Get MMC Size Fail !!\n");
		goto out;
	}
	
	ret = strict_strtoull(data, 0, &mmc_size);

	filp_close(filp, NULL);
	set_fs(oldfs);
	kfree(data);

	return mmc_size;

err:
	set_fs(oldfs);
	kfree(data);
	return err;
out:
	filp_close(filp, NULL);
	set_fs(oldfs);
	kfree(data);
	return err;
}

static loff_t MMC_GetHiddenBase(void){

	loff_t ulCardSize;
	loff_t ulBootStart;

	ulCardSize = get_size();

	ulBootStart = ulCardSize - (MMC_BOOT_PAGESIZE);

	return ulBootStart;
}

static void MMC_Hidden_Info(pHidden_Size_T pHiddenSector){

	int idx;
	loff_t ulHiddenBase;

	pHiddenSector->SectorSize[0] = MMC_HIDDEN0_PAGESIZE;
	pHiddenSector->SectorSize[1] = MMC_HIDDEN1_PAGESIZE;
	pHiddenSector->SectorSize[2] = MMC_HIDDEN2_PAGESIZE;
	pHiddenSector->SectorSize[3] = MMC_HIDDEN3_PAGESIZE;
	pHiddenSector->SectorSize[4] = MMC_HIDDEN4_PAGESIZE;
	pHiddenSector->SectorSize[5] = MMC_HIDDEN5_PAGESIZE;
	pHiddenSector->SectorSize[6] = MMC_HIDDEN6_PAGESIZE;
	pHiddenSector->SectorSize[7] = MMC_HIDDEN7_PAGESIZE;
	pHiddenSector->SectorSize[8] = MMC_HIDDEN8_PAGESIZE;
	pHiddenSector->SectorSize[9] = MMC_HIDDEN9_PAGESIZE;

	ulHiddenBase = MMC_GetHiddenBase();

	for(idx=0; idx < MMC_HIDDEN_MAX_COUNT; idx++){
		pHiddenSector->HiddenStart[idx]=
			((idx)? pHiddenSector->HiddenStart[idx-1] : ulHiddenBase) - pHiddenSector->SectorSize[idx];
	}

}


int MMC_ReadHidden(char* buf, size_t len, size_t offset, int hidden_idx){

	struct file* filp = NULL;
	mm_segment_t oldfs;
	int ret, err=-1, retry;

	loff_t hiddenStart, limitedSize;
	sHidden_Size_T stHidden;
	MMC_Hidden_Info(&stHidden);
	limitedSize = stHidden.SectorSize[hidden_idx] << 9;

	if(offset < limitedSize)
		hiddenStart = (stHidden.HiddenStart[hidden_idx]  + offset) << 9;
	else{
		DBG(KERN_INFO "[%s	] invaild access offset 0x%x Hidden Area 0x%x limitted size 0x%x\n",
			       	__func__, offset, hidden_idx, limitedSize);
		goto out;
	}
	#if 0
	printk(" stHidden.HiddenStart[hidden_idx] : 0x%08x\n", stHidden.HiddenStart[hidden_idx]);
	printk(" offset : %d\n", offset);
	printk("[%s] hiddenStart : 0x%08x\n", __func__, hiddenStart);
	#endif

	oldfs = get_fs();
	set_fs(get_ds());

	filp = filp_open(MMC_PATH, O_RDONLY, GFP_KERNEL);
	if(IS_ERR(filp)){
		DBG(KERN_INFO "MMC Hidden Open Fail !!\n");
		err = PTR_ERR(filp);
		goto err;
	}

	ret = vfs_read(filp, buf, len, &hiddenStart);
	if(ret < 0){
		for(retry=0; retry < 5; retry++){
			mdelay(10);
			ret = vfs_read(filp, buf, len, &hiddenStart);
			DBG(KERN_INFO "retry count : %d\n", retry);
			if(ret >0) break;
		}
		if(ret < 0){
			DBG(KERN_INFO "MMC Hidden Read Fail !!\n");
			goto err;
		}
	}
#if 0
	printk("[%s] hiddenStart : 0x%08x\n", __func__, hiddenStart);
printk("[%s] buf[0] : %d\n", __func__, (unsigned int)buf[0]);
	printk("[%s] buf[1] : %d\n", __func__, (unsigned int)buf[1]);
	printk("[%s] buf[2] : %d\n", __func__, (unsigned int)buf[2]);
	printk("[%s] buf[3] : %d\n", __func__, (unsigned int)buf[3]);
#endif
	filp_close(filp, NULL);
	set_fs(oldfs);
	return 0;

err:
	filp_close(filp, NULL);
	set_fs(oldfs);
	
	return err;

out:
	return -1;
}

int MMC_WriteHidden(char* buf, size_t len , size_t offset ,  int hidden_idx){

	struct file* filp = NULL;
	mm_segment_t oldfs;
	int ret, err, retry;

	loff_t hiddenStart, limitedSize;
	sHidden_Size_T stHidden;
	MMC_Hidden_Info(&stHidden);
	limitedSize = stHidden.SectorSize[hidden_idx] << 9;

	if(offset < limitedSize)
		hiddenStart = (stHidden.HiddenStart[hidden_idx] << 9) + offset;
	else{
		DBG(KERN_INFO "[%s	] invaild access offset 0x%X Hidden Area 0x%X limitted size 0x%X \n",
			       	__func__, offset, hidden_idx, limitedSize);
		goto out;
	}

	oldfs = get_fs();
	set_fs(get_ds());

	filp = filp_open(MMC_PATH, O_WRONLY, GFP_KERNEL);
	if(IS_ERR(filp)){
		DBG(KERN_INFO "MMC Hidden Open Fail !!\n");
		err = PTR_ERR(filp);
		goto err;
	}

	ret = vfs_write(filp, buf, len, &hiddenStart);
	if(ret < 0){

		for(retry = 0; retry < 5; retry++){
			mdelay(10);
			ret = vfs_write(filp, buf, len, &hiddenStart);
			DBG(KERN_INFO "retry count : %d\n", retry);
			if(ret>0) break;
		}
		if(ret <0){
			DBG(KERN_INFO "MMC Hidden Write Fail !!\n");
			goto out;
		}
	}

	filp_close(filp, NULL);
	set_fs(oldfs);
	return 0;

err:
	filp_close(filp, NULL);
	set_fs(oldfs);
	return err;

out:
	return -1;
}

#endif
