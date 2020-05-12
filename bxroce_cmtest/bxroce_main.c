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


static struct bxroce_dev *bx_add(struct bx_dev_info *dev_info)
{
	BXROCE_PR("bxroce:bx_add start\n");//added by hs for printing info
	
	struct bxroce_dev *dev;
	dev = kzalloc(sizeof(*dev),GFP_KERNEL);
	if(!dev) {
		printk("bxroce:Unable to allocate ib device\n");//to show the err information.
		return NULL;
	}

	memcpy(&dev->devinfo, dev_info, sizeof(*dev_info));

	status = bxroce_init_hw(dev);
	if(status)
		goto err_init_hw;

	


	BXROCE_PR("bxroce:bx_add succeed end\n");//added by hs for printing info

	return dev;//turn back the ib dev
err_init_hw:
	BXROCE_PR("err hw init \n");

	return NULL;

}

static void bx_remove(struct bxroce_dev *dev)
{
	BXROCE_PR("bxrove:bx_remove start\n");//added by hs for printing bx_remove info

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
