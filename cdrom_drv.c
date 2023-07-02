#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/cdrom.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include "cdrom.h"
#include "cdif_drv.h"

#include "knetlink.h"

#include <plat/nand.h>
extern int MMC_ReadHidden(char* buf, size_t len, size_t offset, int hidden_idx);
extern int tcc_nand_read_sector( unsigned int uiDrvNum, unsigned int uiAreaIndex, unsigned int uiSectorAddress, unsigned int uiSectorCount, void *pvBuffer );

/* TOC Interface cdrom                                                    */


int read_toc(unsigned long* buffer, unsigned int size)
{	
	return CD_read_TOC(buffer, size);
//	return 0;
}



static int tcr_read_tochdr(struct cdrom_device_info *cdi,  struct cdrom_tochdr *tochdr)
{
printk("tcr_read_tochdr called \n");
/*
	int result;
	unsigned char* buffer;

//	buffer = kzalloc(512, GFP_KERNEL);
	buffer = kzalloc(4, GFP_KERNEL);

	if(!buffer) 
	{
		kfree(buffer);
		return -ENOMEM;
	}

//	result = read_toc_form_file(buffer, 512);
//	result = read_toc(buffer, 512);
	result = read_toc(buffer, 4);

	tochdr->cdth_trk0 = buffer[2];
	tochdr->cdth_trk1 = buffer[3];

	kfree(buffer);
	return result;
*/
	set_State_eject(0);		// unmount state initialize

	tochdr->cdth_trk0 = 1;
	tochdr->cdth_trk1 = 1;
	return 0;
}

static int tcr_read_tocentry(struct cdrom_device_info *cdi,  struct cdrom_tocentry *tocentry)
{

	int result;
//	unsigned char* buffer;
	unsigned long* buffer;
	unsigned char msf_buffer[3];
	printk("tcr_read_tocentry called \n");
//	buffer = kzalloc(32, GFP_KERNEL);
	buffer = kzalloc(sizeof(unsigned long)*4, GFP_KERNEL);

	if(!buffer)
	{
		kfree(buffer);
		return -ENOMEM;
	}

//	result = read_toc_form_file(buffer, 32);
//	result = read_toc(buffer, 32);
	result = read_toc(buffer, 3);
/*
	tocentry->cdte_ctrl = buffer[5] & 0xf;

	tocentry->cdte_adr = buffer[5] >> 4;
	tocentry->cdte_datamode = (tocentry->cdte_ctrl & 0x04) ? 1 : 0;
*/
	tocentry->cdte_ctrl = 0x04;
	tocentry->cdte_datamode = (tocentry->cdte_ctrl & 0x04) ? 1 : 0;

	msf_buffer[0] = buffer[0] & 0xff;
	msf_buffer[1] = buffer[1] & 0xff;
	msf_buffer[2] = buffer[2] & 0xff;

printk("msf_buffer 1, 2, 3 : %d, %d, %d \n", msf_buffer[0], msf_buffer[1], msf_buffer[2]);

	if (tocentry->cdte_format == CDROM_MSF) {
		tocentry->cdte_addr.msf.minute = msf_buffer[0];
		tocentry->cdte_addr.msf.second = msf_buffer[1];
		tocentry->cdte_addr.msf.frame = msf_buffer[2];
//		tocentry->cdte_addr.msf.minute = buffer[12];
//		tocentry->cdte_addr.msf.second = buffer[13];
//		tocentry->cdte_addr.msf.frame = buffer[14];
//printk("cdrom_drv : cdrom_msf mode \n");
	} else
	{
//printk("cdrom_drv : addr lba mode \n");	
		tocentry->cdte_addr.lba = (((((msf_buffer[0] << 8) + msf_buffer[1]) << 8)
					+ msf_buffer[2]) << 8);		

//		tocentry->cdte_addr.lba = (((((buffer[12] << 8) + buffer[13]) << 8)
//					+ buffer[14]) << 8);		
	}
	kfree(buffer);
	return result;

}

int tcr_read_data(unsigned int start, unsigned int nsector, unsigned char* buff)
{
	//	return read_data_form_file(buff, start, nsector);		//read from emmc or nand
		return CD_ReadSector(start, nsector, buff);	//read from cd deck
}


/* ---------------------------------------------------------------------- */
/* interface to cdrom.c                                                   */

int tcr_tray_move (struct cdrom_device_info *cdi, int pos)
{
	return 0;
}

int tcr_lock_door (struct cdrom_device_info *cdi, int lock)
{
	return 0;
}

int tcr_drive_status (struct cdrom_device_info *cdi, int slot)
{
	return 4;
}

int tcr_disk_status (struct cdrom_device_info *cdi)
{
	return 0;
}

int tcr_get_last_session (struct cdrom_device_info *cdi, struct cdrom_multisession *ms_info)
{
	TCC_CD *cd = cdi->handle;

	ms_info->addr.lba = cd->ms_offset;
	ms_info->xa_flag = cd->xa_flag || cd->ms_offset > 0;

	return 0;
}

int tcr_get_mcn (struct cdrom_device_info *cdi , struct cdrom_mcn *mcn)
{
	return 0;
}

int tcr_reset (struct cdrom_device_info *cdi)
{
	return 0;
}	

int tcr_select_speed (struct cdrom_device_info *cdi, int speed)
{
	return 0;
}	

/* ----------------------------------------------------------------------- */
/* this is called by the generic cdrom driver. arg is a _kernel_ pointer,  */
/* because the generic cdrom driver does the user access stuff for us.     */
/* only cdromreadtochdr and cdromreadtocentry are left - for use with the  */
/* sr_disk_status interface for the generic cdrom driver.                  */

int tcr_audio_ioctl(struct cdrom_device_info *cdi, unsigned int cmd, void *arg)
{
	switch (cmd) {
	case CDROMREADTOCHDR:
		return tcr_read_tochdr(cdi, arg);
	case CDROMREADTOCENTRY:
		return tcr_read_tocentry(cdi,arg);

	case CDROMPLAYTRKIND:

	default:
		return -EINVAL;
	}
}


