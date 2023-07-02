
/* #####################################################################
 * SD / MMC / EMMC Hidden Partioin Layout
 * #####################################################################
 */


#define MMC_HIDDEN_MAX_COUNT		10
//#define MMC_BOOT_PAGESIZE		(2*1024*1024/512)
//#define MMC_BOOT_PAGESIZE		0
#define MMC_BOOT_PAGESIZE		1


/* MMC Hidden partition configuration */

#define MMC_HIDDEN0_PAGESIZE		(8*1024*1024/512) /* Hidden 0 / KEY STORE */
#define MMC_HIDDEN1_PAGESIZE		(800*1024*1024/512)
#define MMC_HIDDEN2_PAGESIZE		0
#define MMC_HIDDEN3_PAGESIZE		0
#define MMC_HIDDEN4_PAGESIZE		0
#define MMC_HIDDEN5_PAGESIZE		0
#define MMC_HIDDEN6_PAGESIZE		0
#define MMC_HIDDEN7_PAGESIZE		0
#define MMC_HIDDEN8_PAGESIZE		0
#define MMC_HIDDEN9_PAGESIZE		0		 /* Hidden 9 */


#define MMC_PATH	"/dev/block/mmcblk0"
#define SIZE_PATH	"/sys/class/block/mmcblk0/size"



/* Hidden Access Base Structure */

typedef struct MMC_Hidden_Size{
	loff_t SectorSize[MMC_HIDDEN_MAX_COUNT];
	loff_t HiddenStart[MMC_HIDDEN_MAX_COUNT];
}sHidden_Size_T, *pHidden_Size_T;

/* Hidden Area Access Function */
int MMC_ReadHidden(char* buf, size_t len, size_t offset, int hidden_idx);
int MMC_WriteHidden(char* buf, size_t len, size_t offset, int hidden_idx);
