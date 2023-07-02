/* *************************************************************************************************** 	*/
/* *************************************************************************************************** 	*/
/* Telechips Virtural CD-ROM Device Driver Interface 							*/
/* *************************************************************************************************** 	*/
/* *************************************************************************************************** 	*/

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/bio.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/cdrom.h>
#include <linux/interrupt.h>
#include <linux/blkdev.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include <asm/uaccess.h>
#include <linux/scatterlist.h>
#include <asm/memory.h>

#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>

#include "cdrom.h"
#include "cdif_proc.h"
#include "cdif_api.h"
#include "knetlink.h"


#define TCR_DISKS	256

#define TCR_CAPABILITIES \
	(CDC_CLOSE_TRAY|CDC_OPEN_TRAY|CDC_LOCK|CDC_SELECT_SPEED| \
	 CDC_SELECT_DISC|CDC_MULTI_SESSION|CDC_MCN|CDC_MEDIA_CHANGED| \
	 CDC_RESET|CDC_DRIVE_STATUS| \
	 CDC_CD_R|CDC_DVD_R|CDC_GENERIC_PACKET)

struct platform_device 	*cdrom_dev;
tcr_priv_data_t	tcr_thread_data;
struct tcr_device 	*tdev ;
struct gendisk 		*disk;

static DEFINE_SPINLOCK(tcr_index_lock);
static DEFINE_MUTEX(tcr_mutex);
static DEFINE_MUTEX(tcr_ref_mutex);

static DEFINE_MUTEX(tcr_umount);
static int state_eject;


static struct platform_driver tcc_cdrom_driver;
static unsigned long tcr_index_bits[TCR_DISKS / BITS_PER_LONG];
static int tcr_open(struct cdrom_device_info *, int);
static void tcr_release(struct cdrom_device_info *);
static unsigned int tcr_check_events(struct cdrom_device_info *cdi,
		unsigned int clearing, int slot);
static int tcr_packet(struct cdrom_device_info *, struct packet_command *);

static struct cdrom_device_ops tcr_dops = {
	.open			= tcr_open,
	.release		= tcr_release,
	.drive_status		= tcr_drive_status,
	.check_events		= tcr_check_events,
	.tray_move		= tcr_tray_move, /* It may be NULL */
	.lock_door		= tcr_lock_door,
	.select_speed		= tcr_select_speed,
	.get_last_session	= tcr_get_last_session,
	.get_mcn		= tcr_get_mcn,
	.reset			= tcr_reset,
	.audio_ioctl		= tcr_audio_ioctl,
	.capability		= TCR_CAPABILITIES,
	.generic_packet		= tcr_packet,
};	

static void tcr_kref_release(struct kref *kref);

static unsigned int tcr_check_events(struct cdrom_device_info *cdi, unsigned int clearing, int slot)
{

	return 0;
}

static inline struct tcc_cd *tcc_cd(struct gendisk *disk)
{
	return container_of(disk->private_data, struct tcc_cd, driver);
}

static inline struct tcc_cd *tcr_cd_get(struct gendisk *disk)
{
	struct tcc_cd *cd = NULL;

	mutex_lock(&tcr_ref_mutex);

	if(disk->private_data == NULL)
		goto out;
	cd = tcc_cd(disk);
	kref_get(&cd->kref);

	get_device(&cdrom_dev->dev);

	if(cd->device == NULL)
		goto out_put;

	goto out;


out_put:
	kref_put(&cd->kref, tcr_kref_release);
	cd = NULL;
out:
	mutex_unlock(&tcr_ref_mutex);
	return cd;

}

static void tcr_cd_put(struct tcc_cd *cd)
{
	mutex_lock(&tcr_ref_mutex);
	kref_put(&cd->kref, tcr_kref_release);
	put_device(&cdrom_dev->dev);
	mutex_unlock(&tcr_ref_mutex);
}




static int tcr_block_open(struct block_device *bdev, fmode_t mode)		
{
	struct tcc_cd *cd;
	int ret = -ENXIO;

	mutex_lock(&tcr_mutex);
	//mode = mode ^ FMODE_WRITE;
	mode &= ~(FMODE_WRITE);
	
	cd = tcr_cd_get(bdev->bd_disk);
	if(cd){
		ret = cdrom_open(&cd->cdi, bdev, mode);
		if(ret) tcr_cd_put(cd);
	}

	mutex_unlock(&tcr_mutex);

	return ret;
}

static int tcr_block_release(struct gendisk *disk, fmode_t fmode)
{
	struct tcc_cd *cd = tcc_cd(disk);
	mutex_lock(&tcr_mutex);
	cdrom_release(&cd->cdi, fmode);
	tcr_cd_put(cd);
	mutex_unlock(&tcr_mutex);

	return 0;
}

static int tcr_block_ioctl(struct block_device *bdev, fmode_t fmode, unsigned cmd,
		unsigned long arg)
{
	struct tcc_cd *cd = tcc_cd(bdev->bd_disk);
	int ret;
//printk("[dw_debug] - tcr_block_ioctl called fmode: %d, cmd : %d \n", fmode, cmd);
	mutex_lock(&tcr_mutex);
#if 0
	ret = cdif_ioctl(cmd, arg);

	if(ret != -ENOSYS)
		goto out;
#endif

	ret = cdrom_ioctl(&cd->cdi, bdev, fmode, cmd, arg);	
	if (ret != -ENOSYS)
		goto out;

out:

	mutex_unlock(&tcr_mutex);
	return ret;

	return 0;
}

static unsigned int tcr_block_check_events (struct gendisk *disk , unsigned int clearing)
{
	return 0;
}

static int tcr_block_revalidate_disk(struct gendisk *disk)
{
	return 0;
}

static const struct block_device_operations tcr_bdops =
{
	.owner		= THIS_MODULE,
	.open		= tcr_block_open,
	.release	= tcr_block_release,
	.ioctl		= tcr_block_ioctl,
	.check_events	= tcr_block_check_events,
	.revalidate_disk = tcr_block_revalidate_disk,
	/* 
	 * No compat_ioctl for now because sr_block_ioctl never
	 * seems to pass arbitrary ioctls down to host drivers.
	 */
};


static int tcr_open(struct cdrom_device_info* cdi, int purpose)
{
	return 0;
}

static void tcr_release(struct cdrom_device_info *cdi)
{

}

static int tcr_packet(struct cdrom_device_info *cdi , struct packet_command *cgc)
{
	return 0;
}

#define KERNEL_THREAD
#if defined(KERNEL_THREAD)
static void tcr_request(struct request_queue *rq)
{
	tcr_priv_data_t *tcrq = (tcr_priv_data_t*)rq->queuedata;
	wake_up_process(tcrq->thread);
}

//unsigned long lastFailMsf;
static int tcr_issue_rq(void *d)
{
	tcr_priv_data_t *tcr_prive = (tcr_priv_data_t*)d;
	struct request_queue *rq = tcr_prive->req;
	struct request *req = NULL;

	int ret = -1;
	unsigned long msf=0;

	do {
		if(req == NULL){
			set_current_state(TASK_INTERRUPTIBLE);
			spin_lock(rq->queue_lock);
			req = blk_fetch_request(rq);
			spin_unlock(rq->queue_lock);
		}
		
		if(!req){
			if(kthread_should_stop()){
				set_current_state(TASK_RUNNING);
				break;
			}
			schedule();
			continue;
		}

		set_current_state(TASK_RUNNING);
		if(req->cmd_type == REQ_TYPE_FS){
			switch(rq_data_dir(req)){

				case READ:

					// handle remained read requests after cd deck eject(unmount)
					mutex_lock(&tcr_umount);
					if(state_eject == 1)
					{
						//printk("[dw_debug] umount handling \n");

						memset(req->buffer, 0, blk_rq_cur_bytes(req));
						spin_lock(rq->queue_lock);
						__blk_end_request_all(req, -EIO);
						req = NULL;
						spin_unlock(rq->queue_lock);
						mutex_unlock(&tcr_umount);
						break;
					}
					mutex_unlock(&tcr_umount);

					msf = ((unsigned long)blk_rq_pos(req) >> 2) + 150;		//modefied msf for cd deck 					

/*
if(msf == lastFailMsf)
{
	spin_lock(rq->queue_lock);
	__blk_end_request_all(rq, -EIO);
    req = NULL;
    spin_unlock(rq->queue_lock);
    mutex_unlock(&tcr_umount);
	break;
}
*/
printk("[tcc-debug] cdrom.c : request[msf:%d] \n", msf);
					ret = tcr_read_data(msf, blk_rq_cur_sectors(req) >> 2 , tdev->buf);
					
					if(ret >=0)
					{
						printk("[dw_debug] data read success[msf:%d]! \n", msf);
						memcpy(req->buffer, tdev->buf, blk_rq_cur_bytes(req));
						spin_lock(rq->queue_lock);
						if(__blk_end_request(req, 0, blk_rq_cur_bytes(req)) == false)
							req = NULL;
						spin_unlock(rq->queue_lock);
					}
					else
					{
						printk("[dw_debug] data read fail[msf:%d]! \n",msf);
						spin_lock(rq->queue_lock);
						__blk_end_request_all(req, -EIO);
						req = NULL;
						spin_unlock(rq->queue_lock);
						//lastFailMsf = msf;
					}
					break;
				default:

					spin_lock(rq->queue_lock);
					__blk_end_request_all(req, -EIO);
					req = NULL;
					spin_unlock(rq->queue_lock);
					break;
			}

		}else{
			spin_lock(rq->queue_lock);
			__blk_end_request_all(req, -EIO);
			req = NULL;
			spin_unlock(rq->queue_lock);
		}

		schedule();

	}while(1);

	return ret;


}
#else
/*
static void tcr_request(struct request_queue *rq)
{
	struct request *req = NULL;
	struct tcc_cd *cd;

	req = blk_fetch_request(rq);

	if(req->cmd_type == REQ_TYPE_BLOCK_PC){
		tcr_read_data(blk_rq_pos(req), blk_rq_sectors(req), req->buffer);
	}else if(req->cmd_type != REQ_TYPE_FS)
		goto out;

	cd = tcc_cd(req->rq_disk);

	if(rq_data_dir(req) == WRITE){
		goto out;
	}else if (rq_data_dir(req) == READ){
		tcr_read_data(blk_rq_pos(req), blk_rq_sectors(req), req->buffer);
	}else blk_dump_rq_flags(req, "unknown command \n");

	__blk_end_request(req, 0, blk_rq_cur_bytes(req));

	return 0;

out:
	__blk_end_request(req, -EIO, blk_rq_cur_bytes(req));

	return 0;
}
*/
#endif


void set_State_eject(int status)
{
	printk("set_state_eject : %d\n", status);

	mutex_lock(&tcr_umount);
	state_eject = status;
	mutex_unlock(&tcr_umount);
	CD_SetDevForceStop(status);
}

static int tcr_initial(struct platform_device *pdev)
{
	struct tcc_cd		*cd;

	int minor, error = -ENOMEM;

//	cd = kzalloc(sizeof(*cd), GFP_KERNEL);
	cd = kzalloc(sizeof(struct tcc_cd), GFP_KERNEL);
	if(!cd) goto fail;

	tdev = kzalloc(sizeof(struct tcr_device) , GFP_KERNEL);
	tdev->data = kmalloc(sizeof(struct tcr_device), GFP_KERNEL);
	tdev->buf = kmalloc(128*512, GFP_KERNEL);

	kref_init(&cd->kref);

	disk = alloc_disk(1);
	if(!disk) goto fail_free;

	spin_lock(&tcr_index_lock);
	minor = find_first_zero_bit(tcr_index_bits, TCR_DISKS);
	if(minor == TCR_DISKS){
		spin_unlock(&tcr_index_lock);
		error = -EBUSY;
		goto fail_put;
	}
	__set_bit(minor, tcr_index_bits);
	spin_unlock(&tcr_index_lock);

	tdev->rq = blk_init_queue(tcr_request, &(tdev->lock));
	blk_queue_max_hw_sectors (tdev->rq, 128);
	blk_queue_logical_block_size(tdev->rq, 2048);

	disk->major = TCC_CDROM_MAJOR;
	disk->first_minor = minor;
	sprintf(disk->disk_name, "tcr%d", minor);
	disk->fops = &tcr_bdops;
	disk->flags = GENHD_FL_CD | GENHD_FL_BLOCK_EVENTS_ON_EXCL_WRITE;
	disk->events = DISK_EVENT_MEDIA_CHANGE | DISK_EVENT_EJECT_REQUEST;

	blk_queue_rq_timeout(tdev->rq, TCR_TIMEOUT);

	cd->device = tdev;
	cd->disk = disk;
	//cd->driver = &tcc_cdrom_driver;
	cd->driver = NULL;
//	cd->disk = disk;
	cd->capacity = 0x1fffff;
	cd->device->changed = 1;	/* force recheck CD type */
	cd->media_present = 1;
	cd->use = 1;
	cd->readcd_known = 0;
	cd->readcd_cdda = 0;

	cd->cdi.ops = &tcr_dops;
	cd->cdi.handle = cd;
	cd->cdi.mask = 0;
	cd->cdi.capacity = 1;
	sprintf(cd->cdi.name, "tcr%d", minor);

	tdev->sector_size = 2048;	/* A guess, just in case */

	disk->driverfs_dev = &pdev->dev;
	set_capacity(disk, cd->capacity);
	disk->private_data = &cd->driver;
	disk->queue = tdev->rq;
	cd->cdi.disk = disk;

	if (register_cdrom(&cd->cdi))
		goto fail_put;

	dev_set_drvdata(&pdev->dev, cd);
	disk->flags |= GENHD_FL_REMOVABLE;

#if defined(KERNEL_THREAD)
	
	tcr_thread_data.req = tdev->rq;
	tdev->rq->queuedata = &(tcr_thread_data);
	tcr_thread_data.sg = kmalloc(sizeof(struct scatterlist) * 128 , GFP_KERNEL);
	sg_init_table(tcr_thread_data.sg, 128);
	sema_init(&tcr_thread_data.thread_sem, 1);

	tcr_thread_data.thread = kthread_run(tcr_issue_rq, &tcr_thread_data, disk->disk_name);

#endif
	add_disk(disk);
	printk( "Attached Telechips CD-ROM Interface dirver %s\n", cd->cdi.name);

	return 0;

fail_put:
	put_disk(disk);

fail_free:
	kfree(cd);

fail:
	return error;
	
}



static int __devinit tcc_cdrom_probe(struct platform_device *pdev)
{
	int rc;
	rc = register_blkdev(TCC_CDROM_MAJOR, "tcr");


	if(rc) return rc;

	rc = tcr_initial(pdev);


	if(rc<0){
		unregister_blkdev(TCC_CDROM_MAJOR, "tcr");
		return rc;
	}

#if 1
	cdif_init();
	cdif_proc_start();
	nl_init();
#endif

	cdrom_dev = pdev;

	return rc;
}

static void tcr_kref_release(struct kref *kref)
{
	struct tcc_cd *cd = container_of(kref, struct tcc_cd, kref);
	struct gendisk *disk = cd->disk;

	spin_lock(&tcr_index_lock);
	clear_bit(MINOR(disk_devt(disk)), tcr_index_bits);
	spin_unlock(&tcr_index_lock);

	unregister_cdrom(&cd->cdi);

	disk->private_data = NULL;

	put_disk(disk);

	kfree(cd);

}


static int tcr_remove(struct device *dev)
{
	struct tcc_cd *cd = dev_get_drvdata(dev);

	del_gendisk(cd->disk);

	mutex_lock(&tcr_ref_mutex);
	kref_put(&cd->kref, tcr_kref_release);
	mutex_unlock(&tcr_ref_mutex);

	return 0;
}

static int __devexit tcc_cdrom_remove(struct platform_device *pdev)
{
//printk("---- %s ----\n", __func__);
	kthread_stop(tcr_thread_data.thread);
	blk_cleanup_queue(tdev->rq);
	tcr_remove(&pdev->dev);
	unregister_blkdev(TCC_CDROM_MAJOR, "tcr");
#if 1
	cdif_proc_stop();
	cdif_release();
	nl_exit();
#endif

	kfree(tdev->data);
	kfree(tdev->buf);

	kfree(tdev);
	return 0;
}

static int tcc_cdrom_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int tcc_cdrom_resume (struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver tcc_cdrom_driver = {
	.probe		= tcc_cdrom_probe,
	.remove		= __devexit_p(tcc_cdrom_remove),
	.suspend	= tcc_cdrom_suspend,
	.resume		= tcc_cdrom_resume,
	.driver		= {
		.name		= "tcc_cdrom",
	},
};

static int __init init_tcr(void)
{
	return platform_driver_register(&tcc_cdrom_driver);
}

static void __exit exit_tcr(void)
{
	return platform_driver_unregister(&tcc_cdrom_driver);
}

module_init(init_tcr);
module_exit(exit_tcr);

MODULE_DESCRIPTION("TCC cdrom (tcr) driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS_BLOCKDEV_MAJOR(TCC_CDROM_MAJOR);
