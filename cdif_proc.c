

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>

//#include <mach/irqs.h>
//#include <linux/irqreturn.h>
#include <mach/tca_i2s.h>
#include "cdif_drv.h"


#define MODULE_NAME "cdif"
#define PROC_REG_FILENAME "cdif_register"
#define PROC_DATA_FILENAME "cdif_data"



static int cdif_register_proc_read(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	int len = 0;
	volatile unsigned int *pCICR;
	volatile PADMA pADMA;

	pCICR = (volatile unsigned int *)tcc_p2v(HwCICR);
	/** 
	 * @author sjpark@cleinsoft
	 * @date 2014/03/06
	 * CDIF Register Address Set - CDIF Audio Channel changed from Audio0 to Audio1 on daudio 3rd board.
	 **/
	pADMA = (volatile PADMA)tcc_p2v(CDIF_BASE_ADDR_ADMA);

	printk ("KERNEL : CICR       Register VALUE = [0x%08X]\n", *pCICR);
	printk ("KERNEL : TransCtrl Register VALUE = [0x%08X]\n", pADMA->TransCtrl);
	printk ("KERNEL : ChCtrl     Register VALUE = [0x%08X]\n", pADMA->ChCtrl);
	
	return len;
}

static int cdif_data_proc_read(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	int len = 0;
	volatile unsigned int *cddi0, *cddi1, *cddi2, *cddi3;

	cddi0 = (volatile unsigned int *)tcc_p2v(HwCDDI_0);
	cddi1 = (volatile unsigned int *)tcc_p2v(HwCDDI_1);
	cddi2 = (volatile unsigned int *)tcc_p2v(HwCDDI_2);
	cddi3 = (volatile unsigned int *)tcc_p2v(HwCDDI_3);

	//---------------Display CDIF data -------------------
	printk ("KERNEL : CD Input0 Register = [0x%08X]\n", *cddi0);
	printk ("KERNEL : CD Input1 Register = [0x%08X]\n", *cddi1);
	printk ("KERNEL : CD Input2 Register = [0x%08X]\n", *cddi2);
	printk ("KERNEL : CD Input3 Register = [0x%08X]\n", *cddi3);

	return len;
}


struct proc_dir_entry *cdif_dir, *cdif_preg, *cdif_pdata;

int cdif_proc_start(void)
{
	int ret = 0;

	printk("cdif_proc, /proc filesystem! \n");

	cdif_dir = proc_mkdir(MODULE_NAME, NULL);

	if (cdif_dir == NULL)
	{
		ret = -ENOMEM;
	}else{
		
		if ((cdif_preg = create_proc_entry(PROC_REG_FILENAME, S_IRUGO, cdif_dir)) == NULL)
		{
			ret = -EACCES;
		}else{
			cdif_preg->read_proc = cdif_register_proc_read;
			cdif_preg->write_proc = NULL;
			cdif_preg->data = NULL;
		}

		if ((cdif_pdata = create_proc_entry(PROC_DATA_FILENAME, S_IRUGO, cdif_dir)) == NULL)
		{
			ret = -EACCES;
		}else{
			cdif_pdata->read_proc = cdif_data_proc_read;
			cdif_pdata->write_proc = NULL;
			cdif_pdata->data = NULL;
		}
	}

	return ret;
}

//EXPORT_SYMBOL(cdif_proc_start);

void cdif_proc_stop(void)
{
	printk("cdif_proc, Good bye! \n");
	remove_proc_entry(PROC_REG_FILENAME, cdif_dir);
	remove_proc_entry(PROC_DATA_FILENAME, cdif_dir);
	remove_proc_entry(MODULE_NAME, NULL);
}

//EXPORT_SYMBOL(cdif_proc_stop);



