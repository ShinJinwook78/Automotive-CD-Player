#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/mutex.h>
#include "knetlink.h"
#include "cdif_drv.h"

#define NETLINK_USER 31

struct sock *nl_sk = NULL;
struct sk_buff *skb_out;

//unsigned char toc_buffer[MAX_PAYLOAD];
unsigned long toc_buffer[4];
char seek_result[2];

int recv_pid;
struct nlmsghdr *recv_nlh;
int msg_size;


static DEFINE_MUTEX(nl_lock);

int nl_send_msf(unsigned long msf)
{
	int res=0;

	msg_size = sizeof(unsigned long);
	skb_out = nlmsg_new(msg_size+2,GFP_KERNEL);
	recv_nlh = nlmsg_put(skb_out, 0, 0, 0, msg_size, 0);
	NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */

	memcpy(NLMSG_DATA(recv_nlh), &msf, sizeof(msf));

	//printk("msf : %ld      ", msf );
	//printk("send nlmsg_data : %ld\n", *(unsigned long *)nlmsg_data(recv_nlh));

	if(recv_pid > 0)
		res = nlmsg_unicast(nl_sk,skb_out,recv_pid);

	if(res<0)
		printk(KERN_INFO "Netlink Error while sending bak to user\n");

	return 0;
}

void nl_recv_msg(struct sk_buff *skb)
{
	struct nlmsghdr *nlh = NULL; 

	if(skb == NULL) {
		printk("skb is NULL \n");
		return ;
	}
	nlh = (struct nlmsghdr *)skb->data;
	recv_pid = nlh->nlmsg_pid;

	memcpy(seek_result, nlmsg_data(nlh), sizeof(seek_result));
	if(seek_result[0] == 'U')
	{
		set_State_eject(seek_result[1]);
	}
	else if(seek_result[0] == 'S')
	{
		CD_SeekResultMonitor(seek_result);
	}
	else		// toc recive
	{
		memcpy(toc_buffer, nlmsg_data(nlh), nlh->nlmsg_len - 16);	

//		printk("toc recv data \n");
//		for(i=0; i<3; i++)
//		printk("toc_buffer[%d] = %ld \n", i, toc_buffer[i]);
	}

	memset(seek_result, 0, sizeof(seek_result));
}

int nl_read_toc_buffer(unsigned long *buffer, unsigned int size)
{
	printk("toc read data \n");
	
	memcpy(buffer, toc_buffer, sizeof(unsigned long) * size);
	return 0;
}

int nl_read_msf_response(char *res)
{
	if(seek_result[0] == 'S')
	{
		strncpy(res, seek_result, sizeof(seek_result));
		memset(seek_result, 0, sizeof(seek_result));
		return 1;
	}
	else
		return -1;
}

int nl_init(void) 
{

//	printk("Netlink Entering: %s\n",__FUNCTION__);
	nl_sk=netlink_kernel_create(&init_net, NETLINK_USER, 0, nl_recv_msg,
								NULL, THIS_MODULE);
	if(!nl_sk)
	{
		printk(KERN_ALERT "Netlink Error creating socket.\n");
		return -10;
	}
	return 0;
}

void  nl_exit(void) 
{
//	printk(KERN_INFO "Netlink exiting hello module\n");
	kfree_skb(skb_out);
	netlink_kernel_release(nl_sk);
}


