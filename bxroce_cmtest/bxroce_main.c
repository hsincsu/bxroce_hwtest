/*
 *
 * This is the main file with the main funciton.All start here!
 *
 *
 *
 * 						------edited by hs in 2019/6/18
 *
 */

#include <linux/module.h>
#include <linux/idr.h>
#include <linux/inetdevice.h>
#include <linux/if_addr.h>
#include <linux/notifier.h>


#include <rdma/rdma_netlink.h>
#include <rdma/ib_verbs.h>
#include <rdma/ib_user_verbs.h>
#include <rdma/ib_addr.h>
#include <rdma/ib_mad.h>

#include <linux/netdevice.h>
#include <net/addrconf.h>

#include "header/bxroce.h"
//#include "header/bxroce_verbs.h"
//#include "header/bxroce_ah.h"
//#include "header/bxroce_hw.h"
//#include "header/bxroce_abi.h"
//#include <rdma/ocrdma-abi.h>


MODULE_VERSION(BXROCEDRV_VER);
MODULE_AUTHOR("HS");
MODULE_LICENSE("Dual BSD/GPL");

int  len_array_0 [1024];
int  wptr_0;
int  rptr_0;
int  send_cnt_0;
int  send_len;
int  recv_cnt_0;

int  len_array_1 [1024];
int  wptr_1;
int  rptr_1;
int  send_cnt_1;
int  recv_cnt_1;


int get_len(int port_id)
{
	int len;
    if(port_id == 0)
    {
        len = len_array_0[rptr_0];
        rptr_0 ++;
        recv_cnt_0 ++;  
        if(rptr_0 == 1024)
        {
            rptr_0 = 0;
            printk("Port 0 recv %d msg\n",recv_cnt_0);
        }
    }
    else
    {
        len = len_array_1[rptr_1];
        rptr_1 ++;
        recv_cnt_1 ++;  
        if(rptr_1 == 1024)
        {
            rptr_1 = 0;
            printk("Port 1 recv %d msg\n",recv_cnt_1);
        }
    }
    return len;
}

void set_len(int port_id, int len)
{
	if(port_id == 0)
    {
        len_array_0[wptr_0] = len;
        wptr_0 ++;
        send_cnt_0 ++;
        send_len = send_len + (len >> 2) + (len%4 != 0);
        
        if(wptr_0 == 1024)
        {
            wptr_0 = 0;
            printk("Port 0 send %d msg\n",send_cnt_0);
        }
        //printf("Port 0 send_len is %d\n",send_len);   
    }
    else
    {
        len_array_1[wptr_1] = len;
        wptr_1 ++;
        send_cnt_1 ++;
        if(wptr_1 == 1024)
        {
            wptr_1 = 0;
            printk("Port 1 send %d msg\n",send_cnt_1);
        }
    }
   
}




int bxroce_cm_test_msg_send(struct bxroce_dev *dev)
{
	int addr;    	
	int rdata;
	int wdata;
	int cm_msg_4byte_len;
	int cm_msg_flit_len;
	int remain_flit;     
	int i;
	int header_flit;
	int port_id;
	unsigned long randnumber;

	void __iomem *base_addr;
	int status = 0;

	printk("------------CM MSG SEND START----------- \n");

	base_addr = dev->devinfo.base_addr;

	header_flit = 0;
	get_random_bytes(&randnumber,sizeof(unsigned long));
	cm_msg_4byte_len = randnumber % MAX_CM_MSG_4BYTE_LEN + 5;

	if(cm_msg_4byte_len % 4 == 0)
		cm_msg_flit_len = cm_msg_4byte_len/4;
	else
		cm_msg_flit_len = cm_msg_4byte_len/4 +1;

	rdata = bxroce_mpb_reg_read(base_addr,CM_CFG,CM_REG_ADDR_MSG_SEND_SRAM_STATE);

	printk("rdata is 0x%x \n",rdata);
	remain_flit = rdata & 0xffff;

	printk("cm_msg_4byte_len is %d, cm_msg_flit_len is %d, remain_flit is %d\n",cm_msg_4byte_len,cm_msg_flit_len, remain_flit);

	if (remain_flit >= cm_msg_flit_len)
	{
		rdata = bxroce_mpb_reg_read(base_addr,CM_CFG,CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_5);
		
		port_id = 0;

		rdata = rdma_set_bits(rdata,17,17,port_id);

		printk("In send, rdata is 0x%x \n",rdata);

		bxroce_mpb_reg_write(base_addr,CM_CFG,CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_5,rdata);

		set_len(port_id, cm_msg_4byte_len);

		bxroce_mpb_reg_write(base_addr,CM_CFG,CM_REG_ADDR_MSG_SEND_MSG_4BYTE_LEN, cm_msg_4byte_len);

		addr = 0;

		for (i = 0; i < 4; i++)
		{
			bxroce_mpb_reg_write(base_addr,CM_BASE,addr, 0);
			addr = addr + 1;
		}

		for (i = 0; i < cm_msg_4byte_len - 4; i++)
		{
			bxroce_mpb_reg_write(base_addr, CM_BASE, addr,((cm_msg_4byte_len & 0xffff) << 16) + (i & 0xffff));
			addr = addr + 1;
		}

		addr = CM_REG_ADDR_MSG_SRAM_OPERATE_FINISH;
		wdata = 0;

		wdata = rdma_set_bits(wdata,CM_MSG_SEND_MSG_SRAM_WR_FINISH_RANGE,1);

		bxroce_mpb_reg_write(base_addr,CM_CFG,addr,wdata);

		printk("INFO: port_%0d cm msg send:\tcm_msg_4byte_len=%08X.\n",port_id,cm_msg_4byte_len);

		

	}

	printk("------------CM MSG SEND END----------- \n");
	printk("\n");

	return 0;
}


int bxroce_cm_test_msg_recv(struct bxroce_dev *dev)
{
	char *tst;
 	int rdata;
	int wdata;
	int golden_cm_msg_4byte_len;
	int i;
	
	int addr;
	int port_id;
	int header_flit;

	void __iomem *base_addr;
	int status = 0;
	printk("------------CM MSG RECV START----------- \n");

	base_addr = dev->devinfo.base_addr;
	header_flit = 0;

	rdata = bxroce_mpb_reg_read(base_addr,CM_CFG,CMERRINTSTA);

	printk("cm_test_msg_recv: 0x%x \n",rdata);
	if (rdma_get_bits(rdata, 0, 0) == 1)
	{
		BXROCE_PR("have intr\n");
		while (1)
		{
			rdata = bxroce_mpb_reg_read(base_addr,CM_CFG,CMMSGRECEIVESRAMSTATE);
			if (rdma_get_bits(rdata, 0, 0) == 1)
			{
				bxroce_mpb_reg_write(base_addr,CM_CFG,CM_REG_ADDR_ERR_INT_STA_CLR,0x3); // clear intr
				break;
			}
			else
			{
				rdata = bxroce_mpb_reg_read(base_addr,CM_CFG,CM_REG_ADDR_MSG_RECEIVE_MSG_LLP_INFO_5);
				port_id = rdma_get_bits(rdata,17,17);
				printk("cm_random_test_msg_recv get_len start \n");
				 golden_cm_msg_4byte_len = get_len(port_id);

				 rdata = bxroce_mpb_reg_read(base_addr,CM_CFG,CM_REG_ADDR_MSG_RECEIVE_MSG_4BYTE_LEN);
				 if (rdata != golden_cm_msg_4byte_len)
				 {
					 printk("SIMERR: port_%d receive_msg_4byte_len (%08X) is not equal with golden_msg_len(%08X).\n",port_id,rdata,golden_cm_msg_4byte_len);
					 status = -1;
					 break;
				 }

				 addr = 0;
				 for (i = 0; i < 4; i++)
				 {
					 rdata = bxroce_mpb_reg_read(base_addr,CM_BASE,addr);
#ifndef NO_CHECK_CM_MSG
						 if (rdata != header_flit)
						 {
								printk("SIMERR: port_%d rdata (%08X) is not equal with header_flit(%08X).\n",port_id,rdata,header_flit);
								status = -1;
								break;
						 }
					 addr = addr + 1;
#endif
				 }

				  for(i = 0; i < golden_cm_msg_4byte_len - 4; i++) 
                    {
                        rdata = bxroce_mpb_reg_read(base_addr,CM_BASE,addr);
                    #ifndef NO_CHECK_CM_MSG
                        if(rdata != (golden_cm_msg_4byte_len << 16) + (i & 0xffff))
                        {
                            printk("SIMERR: port_%d rdata (%08X) is not equal with golden_cm_rdata(%08X).\n",port_id,rdata,(golden_cm_msg_4byte_len << 16) + (i & 0xffff)); 
                            status = -1;
							break;
                        }    
                    #endif
                        addr = addr + 1;
                    }



				 wdata = 0;
				 wdata = rdma_set_bits(wdata,CM_MSG_RECEIVE_MSG_SRAM_RD_FINISH_RANGE,1);
				 bxroce_mpb_reg_write(base_addr,CM_CFG,CM_REG_ADDR_MSG_SRAM_OPERATE_FINISH,wdata);
				 
				 printk("INFO: port_%0d cm msg recv:\tcm_msg_4byte_len=%08X.\n",port_id,golden_cm_msg_4byte_len);

			}

		}
	}
	else
		printk("cm_random_test_msg_recv with get_bit(rdata,0)=%d \n",rdma_get_bits(rdata,0,0));

	printk("------------CM MSG RECV END----------- \n");
	printk("\n");
	return status;
}



static int bxroce_cm_test(struct bxroce_dev *dev)
{
	struct bx_dev_info *devinfo = &dev->devinfo;

	void __iomem *base_addr;
	unsigned long randnumber;
	unsigned long testnumber; 
	int status = 0;
	u32 regval = 0;

	rptr_0      = 0;
    rptr_1      = 0;
    wptr_0      = 0;
    wptr_1      = 0;

    send_cnt_0  = 0;
    recv_cnt_0  = 0;
    send_cnt_1  = 0;
    recv_cnt_1  = 0;
    send_len    = 0;

	base_addr = dev->devinfo.base_addr;

	printk("cm init config\n");
	regval = bxroce_mpb_reg_read(base_addr,CM_CFG,0x0);
	printk("cmcfg: offset 0x0: 0x%x \n",regval);//
	regval = bxroce_mpb_reg_read(base_addr,CM_CFG,0x1);
	printk("cmcfg: offset 0x1: 0x%x \n",regval);//
	regval = bxroce_mpb_reg_read(base_addr,CM_CFG,0x2);
	printk("cmcfg: offset 0x2: 0x%x \n",regval);//


	testnumber = 1000;
	printk("------------------CM_RANDOME_TEST START--------------- \n");
	while (testnumber-- )
	{
		get_random_bytes(&randnumber,sizeof(unsigned long));
		printk("randnumber is %x \n",randnumber);
		switch (randnumber % 3) {
		case 0 : status = bxroce_cm_test_msg_recv(dev);
				 if(status == -1)
					 return status;

		default: status = bxroce_cm_test_msg_send(dev);
			if(status == -1)
					 return status;
		}

	}
	printk("------------------CM_RANDOME_TEST END--------------- \n");


	//clear the msg sram and clear the flit

	printk("--------------------DMA_CH_CA   printing info start --------------------------\n");
		regval = bxroce_mpb_reg_read(base_addr,CM_CFG,CM_REG_ADDR_MSG_SEND_SRAM_STATE);
		BXROCE_PR("CM_REG_ADDR_MSG_SEND_SRAM_STATE: 0x%x \n",regval);

		regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_CH_CA_TDLR));
		BXROCE_PR("DMA_CH_CA_TDLR: 0x%x \n",regval);

		regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_CH_CA_TDHR));
		BXROCE_PR("DMA_CH_CA_TDHR: 0x%x \n",regval);
		
		regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_CH_CA_RDLR));
		BXROCE_PR("DMA_CH_CA_RDLR: 0x%x \n",regval);

		regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_CH_CA_RDHR));
		BXROCE_PR("DMA_CH_CA_RDHR: 0x%x \n",regval);

		regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_CH_CA_TBLR));
		BXROCE_PR("DMA_CH_CA_TBLR: 0x%x \n",regval);
	
		regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_CH_CA_TBHR));
		BXROCE_PR("DMA_CH_CA_TBHR: 0x%x \n",regval);

		regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_CH_CA_RBLR));
		BXROCE_PR("DMA_CH_CA_RBLR: 0x%x \n",regval);
	
		regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_CH_CA_RBHR));
		BXROCE_PR("DMA_CH_CA_RBHR: 0x%x \n",regval);
	
		printk("--------------------DMA_CH_CA  printing info end --------------------------\n");


	return status;
}


static struct bxroce_dev *bx_add(struct bx_dev_info *dev_info)
{
	BXROCE_PR("bxroce:bx_add start\n");//added by hs for printing info
	
	struct bxroce_dev *dev;
	int status = 0;

	dev = kzalloc(sizeof(*dev),GFP_KERNEL);
	if(!dev) {
		printk("bxroce:Unable to allocate ib device\n");//to show the err information.
		return NULL;
	}

	memcpy(&dev->devinfo, dev_info, sizeof(*dev_info));

	status = bxroce_init_hw(dev);
	if(status)
		goto err_init_hw;

	status = bxroce_get_hwinfo(dev);
	if(status)
		goto err_init_hw;

	status = bxroce_cm_test(dev);
	if(status)
		goto err_cm_test;


	BXROCE_PR("bxroce:bx_add succeed end\n");//added by hs for printing info

	return dev;//turn back the ib dev
err_init_hw:
	BXROCE_PR("err hw init \n");
err_cm_test:
	BXROCE_PR("err cm test \n");
	return NULL;

}

static void bx_remove(struct bxroce_dev *dev)
{
	BXROCE_PR("bxrove:bx_remove start\n");//added by hs for printing bx_remove info
	if(dev)
	kfree(dev);
	BXROCE_PR("bxroce:bx_remove succeed end \n");//added by hs for printing bx_remove info
}


struct bxroce_driver bx_drv = {
	.name 		="bxroce_hwtest",
	.add  		=bx_add,
	.remove 	=bx_remove,
	//not finished , added later
};

static int __init bx_init_module(void)
{
	int status;
	BXROCE_PR("bxroce:init module start!\n");//added by hs for printing init info

	status = bx_roce_register_driver(&bx_drv);	
	if(status)
		goto err_reg;
	BXROCE_PR("bxroce:init module exit succeed!\n");//added by hs for printing init info
	return 0;
	
err_reg:
	return status;
}

static void __exit bx_exit_module(void)
{
	BXROCE_PR("bxroce:exit module start\n");//added by hs for printing info
	bx_roce_unregister_driver(&bx_drv);
	BXROCE_PR("bxroce:exit module succeed!\n");//added by hs for print exit info
}

module_init(bx_init_module);
module_exit(bx_exit_module);
