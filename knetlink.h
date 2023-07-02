#include <linux/skbuff.h>

#define MAX_PAYLOAD	512

extern void nl_recv_msg(struct sk_buff *skb);
extern int nl_read_toc_buffer(unsigned long *buffer, unsigned int size);
extern int nl_send_msf(unsigned long msf);
extern int nl_read_msf_response(char *res);
extern int nl_init(void);
extern void  nl_exit(void);

extern void set_State_eject(int status);	//defined in cdrom.c


