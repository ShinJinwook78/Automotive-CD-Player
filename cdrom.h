/* *************************************************************************************************** 	*/
/* *************************************************************************************************** 	*/
/* Telechips Virtural CD-ROM Device Driver Interface 							*/
/* *************************************************************************************************** 	*/
/* *************************************************************************************************** 	*/

#include <linux/genhd.h>
#include <linux/kref.h>

#include <linux/device.h>

#define MAX_RETRIES	3
#define TCR_TIMEOUT	(30 * HZ)

struct module;
struct request;
struct request_queue;

typedef struct _tcr_priv_data_t {
	struct task_struct	*thread;
	struct request_queue	*req;
	struct scatterlist	*sg;
	struct semaphore	thread_sem;
} tcr_priv_data_t;

struct tcr_device {
	unsigned int		sector_size;
	unsigned char		*data;
	unsigned short		user;
	unsigned short		media_change;
	struct request_queue	*rq;
	spinlock_t		lock;
	struct gendisk		*ghd;
	unsigned char		*buf;
	struct device		gendev, tdev_dev;
	unsigned int 		changed;
};

typedef struct tcc_cd {

	struct pltform_driver *driver;
	unsigned capacity;	/* size in blocks                       */
	struct tcr_device *device;
	unsigned int vendor;	/* vendor code, see sr_vendor.c         */
	unsigned long ms_offset;	/* for reading multisession-CD's        */
	unsigned use:1;		/* is this device still supportable     */
	unsigned xa_flag:1;	/* CD has XA sectors ? */
	unsigned readcd_known:1;	/* drive supports READ_CD (0xbe) */
	unsigned readcd_cdda:1;	/* reading audio data using READ_CD */
	unsigned media_present:1;	/* media is present */

	/* GET_EVENT spurious event handling, blk layer guarantees exclusion */
	int tur_mismatch;		/* nr of get_event TUR mismatches */
	bool tur_changed:1;		/* changed according to TUR */
	bool get_event_changed:1;	/* changed according to GET_EVENT */
	bool ignore_get_event:1;	/* GET_EVENT is unreliable, use TUR */

	struct cdrom_device_info cdi;
	/* We hold gendisk and scsi_device references on probe and use
	 * the refs on this kref to decide when to release them */
	struct kref kref;
	struct gendisk *disk;
}TCC_CD;


int tcr_lock_door(struct cdrom_device_info *, int);
int tcr_tray_move(struct cdrom_device_info *, int);
int tcr_drive_status(struct cdrom_device_info *, int);
int tcr_disk_status(struct cdrom_device_info *);
int tcr_get_last_session(struct cdrom_device_info *, struct cdrom_multisession *);
int tcr_get_mcn(struct cdrom_device_info *, struct cdrom_mcn *);
int tcr_reset(struct cdrom_device_info *);
int tcr_select_speed(struct cdrom_device_info *cdi, int speed);
int tcr_audio_ioctl(struct cdrom_device_info *, unsigned int, void *);

int tcr_read_data(unsigned int start, unsigned int nsector, unsigned char* buff);


#define to_tcr_device(d) \
	container_of(d , struct tcr_device, ghd)
