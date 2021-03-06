/*
 *
 *
 *
 * 	this file is a header file for verbs.
 * 					--------edited by hs  2019/6/22
 *
 *
 */
#ifndef __BXROCE_VERBS_H__
#define __BXROCE_VERBS_H__

#if 0  //added by hs 

struct bxroce_pbe {
	u32 pa_hi;
	u32 pa_lo;
};

struct bxroce_eth_basic {
		u8 dmac[6];
        u8 smac[6];
        __be16 eth_type;
};
struct bxroce_eth_vlan {
		u8 dmac[6];
        u8 smac[6];
        __be16 eth_type;
        __be16 vlan_tag;
        __be16 roce_eth_type;
};


struct bxroce_grh {
		__be32  tclass_flow;
        __be32  pdid_hoplimit;
        u8      sgid[16];
        u8      dgid[16];      
        u16     rsvd;
};

struct bxroce_av {
	 struct bxroce_eth_vlan eth_hdr;
	 struct bxroce_grh grh;
	 u32 valid;
};

struct bxroce_ah {
	struct ib_ah	ibah;
	struct bxroce_av *av;
	u16 sgid_index;
	u32 id;
	u8 hdr_type;
};

enum bxroce_qp_foe {
        BXROCE_Q_FULL = 0,
        BXROCE_Q_EMPTY = 1,
        BXROCE_Q_FREE = 2,
};

struct bxroce_pd {
	struct bxroce_pool_entry pelem;
	struct ib_pd ibpd;
	u32 id;
	struct bxroce_ucontext *uctx;
};
struct bxroce_cqe { // cqe need 16 byte memory.
	u32 cqe_p1;//4 byte
	u32 cqe_p2;//4 byte
	u32 cqe_p3;//4 byte
	u32 cqe_p4;//4 byte
};

struct bxroce_cq {
	struct ib_cq ibcq;
	/*three types of cq,tx,rx,xmit*/
	struct bxroce_cqe *txva;
	struct bxroce_cqe *rxva;
	struct bxroce_cqe *xmitva;
	/*wp ,rp for three types of cq*/
	struct bxroce_cqe *txwp;
	struct bxroce_cqe *txrp;
	struct bxroce_cqe *rxwp;
	struct bxroce_cqe *rxrp;
	struct bxroce_cqe *xmitwp;
	struct bxroce_cqe *xmitrp;

	dma_addr_t txpa;
	dma_addr_t rxpa;
	dma_addr_t xmitpa;
		
	u32 phase;
    u32 getp;
	u32 max_hw_cqe;

	u32 id; // allocate a unique id for cq.
    u32 cqe_cnt;//cqe count
    u32 len; // cq's len
    spinlock_t lock; //for serialize accessing to the CQ
    struct list_head sq_head, rq_head, xmit_head;
	struct bxroce_ucontext *uctx;
};


struct bxroce_mr {
	struct bxroce_pool_entry pelem;
	union{
		struct ib_mr ibmr;
		struct ib_mw ibmw;
	};
	struct bxroce_pd *pd;
	struct ib_umem	  *umem;

	u32				 lkey;
	u32				 rkey;

	enum bxroce_mem_state	state;
	enum bxroce_mem_type	type;
	u64				 va;
	u64				 iova;
	size_t			 length;
	u32				 offset;
	int				 access;

	int				 page_shift;
	int				 page_mask;
	int				 map_shift;
	int				 map_mask;

	u32				 num_buf;
	u32				 nbuf;

	u32				 max_buf;
	u32				 num_map;

	struct bxroce_map  **map;
};

struct bxroce_qp_hwq_info {
	u8 *va;
	u32 max_sges;
	u32 head,tail;
	u32 entry_size;
	u32 max_cnt;
	u32 max_wqe_idx;
	//u16 dbid;
	u32 len;
	dma_addr_t pa;
	enum bxroce_qp_foe qp_foe;
	
};

struct bxroce_sge {
	u32 addr_hi;
	u32 addr_lo;
	u32 lrkey;
	u32 len;
};

struct bxroce_rqe {
	u64  descbaseaddr;
	u32  dmalen;
	u32  opcode;
}__attribute__ ((packed));


//we don't need the default align,we need to make sure the wqe is 48 bytes.
struct bxroce_wqe {//defaultly,we use 48 byte WQE.a queue may have 256 wqes. 48 bytes long,but length is 64 bytes.
	u32 immdt;
	u16 pkey;
	u32 rkey;
	u32 lkey;
	u32 qkey;
	u32 dmalen;
	u64 destaddr;
	u64 localaddr;
	u16 eecntx; // just the first 10 bits is for eecntx,the later 6bits is for destqp;
	u16 destqp; //just the first 4 bits is for destqp.the later 12 bits is for destsocket1.
	u32 destsocket1;
	u8 destsocket2;//just the first 4 bits is for destsocket2,the later 4 bits is for opcode.
	u8  opcode; // just the first 4 bits is for opcode .the later 4 bits is useless.
	u64 llpinfo_lo;
	u64 llpinfo_hi; // 
}__attribute__ ((packed));


enum bxroce_qp_state {
	BXROCE_QPS_RST				=0,
	BXROCE_QPS_INIT			=1,
	BXROCE_QPS_RTR				=2,
	BXROCE_QPS_RTS				=3,
	BXROCE_QPS_SQE				=4,
	BXROCE_QPS_SQ_DRAINING			=5,
	BXROCE_QPS_ERR				=6,
	BXROCE_QPS_SQD				=7,

};

struct qp_change_info
{
	u32 qkey;
	bool signaled;
	u32 destqp;
	u32 pkey_index;
	int sgid_idx;
	u8 mac_addr[6];
};

struct bxroce_qp {
	struct ib_qp ibqp;
	
	u32 id; // qp unique id.
	u32 len; // qp len.send queue is same to recv queue.
	u32 max_inline_data;
	struct {
		uint64_t wrid;
		uint16_t dpp_wqe_idx;
		uint16_t dpp_wqe;
		uint8_t  signaled;
		uint8_t  rsvd[3];
	} *wqe_wr_id_tbl;
	u64 *rqe_wr_id_tbl;

	struct bxroce_cq *sq_cq;
	struct bxroce_cq *rq_cq;
	struct bxroce_pd *pd;

	spinlock_t q_lock ____cacheline_aligned;
	struct bxroce_qp_hwq_info rq;
	struct bxroce_qp_hwq_info sq;
	struct list_head rq_entry;
	struct list_head sq_entry;

	enum ib_qp_type qp_type;
	enum bxroce_qp_state qp_state;
	
	u32 qkey;
	bool signaled;
	u32 destqp;
	u32 pkey_index;
	int sgid_idx;
	u8 mac_addr[6]; // dest mac addr
	struct qp_change_info *qp_change_info;

	struct bxroce_ucontext *uctx;

};


struct bxroce_pbl {
	void *va;
	dma_addr_t pa;
	u32 size;
};
#endif


struct bxroce_dev{
	struct ib_device ibdev;
	struct ib_device_attr attr;
	struct bx_dev_info devinfo;
//	unsigned long *pd_id; // for allocate an unique id to each pd.
//	struct mutex pd_mutex;
	struct mutex dev_lock; 
	//not finished ,added later.

	//struct bxroce_pool mr_pool;
	//struct bxroce_pool pd_pool;
	//u8 *mem_resources; // for bitmap memory
	//unsigned long *allocated_cqs; // allocate id for cqs
	//unsigned long *allocated_qps;//allocated id for qps
	//struct bxroce_qp **qp_table;
	//struct bxroce_cq **cq_table;

	//struct {
	//	struct bxroce_av *va;
	//	dma_addr_t pa;
	//	u32 size;
	//	u32 num_ah;

	//	spinlock_t lock;/*for synchronization*/
	//	u32 ahid;
	//	struct bxroce_pbl pbl;
//	} av_tbl;

#if 0 //added by hs
	u32 next_cq;
	u32 next_qp;
	u32 used_cqs;
	u32 used_qps;
	u64 ioaddr; // to keep io addr of pcie.

	spinlock_t resource_lock; //for cq,qp resource access
	spinlock_t qptable_lock;

	/*GSI need these*/
	int Is_qp1_allocated;
	struct bxroce_cq *gsi_sqcq;
	struct bxroce_cq *gsi_rqcq;
#endif
};

#if 0 //added by hs
struct bxroce_ucontext {
	struct ib_ucontext ibucontext;
	struct list_head mm_head; 
	struct mutex mm_list_lock; //protect list entries of mm type

	struct bxroce_pd *ctx_pd;
	int pd_in_use;

	struct {
		u32 *va;
		dma_addr_t pa;
		u32 len;
	}ah_tbl;

};

struct bxroce_mm {
	struct {
		u64 phy_addr;
		unsigned long len;
	} key;
	struct list_head entry;
};
#endif
/*some function definition*/
//static int bxroce_alloc_lkey(struct bxroce_dev *dev,struct bxroce_mr *mr,u32 pdid, int acc);//alter later --2019/10/28 hs
#endif
