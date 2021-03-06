#include <stdint.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
//#include "mmap_main.h"
#include "test_mmap.h"
#define RX_RING_SIZE 128        //接收环大小
#define TX_RING_SIZE 512        //发送环大小

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

static const struct rte_eth_conf port_conf_default = {
        .rxmode = { .max_rx_pkt_len = ETHER_MAX_LEN }
};

/* basicfwd.c: Basic DPDK skeleton forwarding example. */

/*
 * Initializes a given port using global settings and with the RX buffers
 * coming from the mbuf_pool passed as a parameter.
 */

/*
        指定网口的队列数，本列中指定的是但队列
        在tx、rx两个方向上，设置缓冲区
*/
static inline int
port_init(uint8_t port, struct rte_mempool *mbuf_pool)
{
        struct rte_eth_conf port_conf = port_conf_default;      //网口配置=默认的网口配置
        const uint16_t rx_rings = 1, tx_rings = 1;              //网口tx、rx队列的个数
        int retval;                     //临时变量，返回值
        uint16_t q;                     //临时变量，队列号

        if (port >= rte_eth_dev_count())//rte_eth_dev_count() 设备可用端口数量
                return -1;

        /* Configure the Ethernet device. */
        //配置以太网口设备
        //网口号、发送队列个数、接收队列个数、网口的配置
        retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);   //设置网卡设备
        if (retval != 0)
                return retval;

        /* Allocate and set up 1 RX queue per Ethernet port. */
        //RX队列初始化
        for (q = 0; q < rx_rings; q++) {        //遍历指定网口的所有rx队列

                //申请并设置一个收包队列
                //指定网口，指定队列，指定队列RING的大小，指定SOCKET_ID号，指定队列选项（默认NULL），指定内存池
                //其中rte_eth_dev_socket_id(port)不理解,通过port号来获取dev_socket_id??
                //dev_socket_id作用未知，有待研究
                retval = rte_eth_rx_queue_setup(port, q, RX_RING_SIZE,
                                rte_eth_dev_socket_id(port), NULL, mbuf_pool);
                if (retval < 0)
                        return retval;
        }

        //TX队列初始化
        /* Allocate and set up 1 TX queue per Ethernet port. */
        for (q = 0; q < tx_rings; q++) {        //遍历指定网口的所有tx队列

                //申请并设置一个发包队列
                //指定网口，指定队列，指定队列RING大小，指定SOCKET_ID号，指定选项（NULL为默认）
                //??TX为何没指定内存池，此特征有待研究
                retval = rte_eth_tx_queue_setup(port, q, TX_RING_SIZE,  //申请并设置一个发包队列
                                rte_eth_dev_socket_id(port), NULL);
                if (retval < 0)
                        return retval;
        }

        /* Start the Ethernet port. */
        retval = rte_eth_dev_start(port);       //启动网卡,这是设备启动的最后一步
        if (retval < 0)
                return retval;

        /* Display the port MAC address. */
        struct ether_addr addr;
        rte_eth_macaddr_get(port, &addr);       //获取网卡的MAC地址，并打印
        printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
                           " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
                        (unsigned)port,
                        addr.addr_bytes[0], addr.addr_bytes[1],
                        addr.addr_bytes[2], addr.addr_bytes[3],
                        addr.addr_bytes[4], addr.addr_bytes[5]);

        /* Enable RX in promiscuous mode for the Ethernet device. */
        rte_eth_promiscuous_enable(port);       //设置网卡为混杂模式

        return 0;
}

/*
 * The lcore main. This is the main thread that does the work, reading from
 * an input port and writing to an output port.
 */
/*
//业务函数入口点
//__attribute__((noreturn))用法
//标明函数无返回值
//用来修饰lcore_main函数，标明lcore_main无返回值
*/
/*
//1、检测CPU与网卡是否匹配
//2、建议使用本地CPU就近网卡
//3、数据接收、发送的while(1)
*/
static __attribute__((noreturn)) void
lcore_main(void)
{
        const uint8_t nb_ports = rte_eth_dev_count();   //网口总数
        uint8_t port;                                   //临时变量，网口号

        /*
         * Check that the port is on the same NUMA node as the polling thread
         * for best performance.
         * 检测CPU和网口是否匹配
         * 
         */
        for (port = 0; port < nb_ports; port++)                 //遍历所有网口，检测port是否与cpu匹配
                if (rte_eth_dev_socket_id(port) > 0 &&          //检测port所在cpu是否与现在所在的cpu相同
                                rte_eth_dev_socket_id(port) !=
                                                (int)rte_socket_id())
                        printf("WARNING, port %u is on remote NUMA node to "
                                        "polling thread.\n\tPerformance will "
                                        "not be optimal.\n", port);

        printf("\nCore %u forwarding packets. [Ctrl+C to quit]\n",
                        rte_lcore_id());

        /* Run until the application is quit or killed. */
        /*运行 直到 应用程序 推出 或 被kill*/
        int count=0;
        for (;;) {
                /*
                 * Receive packets on a port and forward them on the paired
                 * port. The mapping is 0 -> 1, 1 -> 0, 2 -> 3, 3 -> 2, etc.
                 * 从Eth接收数据包 ，并发送到 ETH上。
                 * 发送顺序为：0 的接收 到 1的 发送，
                 *              1 的接收 到 0的 发送
                 * 每两个端口为一对
                 */
                for (port = 0; port < nb_ports; port++) {       //遍历所有网口

                        /* Get burst of RX packets, from first port of pair. */
                        struct rte_mbuf *bufs[BURST_SIZE];

                        //收包，接收到nb_tx个包
                        //端口，队列，收包队列，队列大小
                        const uint16_t nb_rx = rte_eth_rx_burst(port, 0,
                                        bufs, BURST_SIZE);
                        if(nb_rx>0) 
                        {
                                count+=nb_rx;
                              //  printf("receive:%d\n",nb_rx);
                        }
                        if (unlikely(nb_rx == 0))//收到的报文数为0
                                continue;
                        
                        for(int i=0;i<nb_rx;i++)
                        {
                                encode_pcap(bufs[i]);
                        }

                        // int fd = open("123.pcap", O_RDWR | O_CREAT, 0644);
                        // char str[N];
                        // if(fd==-1)
                        // {
                        //         printf("file error!\n");
                        // }
                        // for(int i= 0;i<nb_rx;i++)
                        // {
                        //         struct rte_mbuf *m = bufs[i];
                        //         m->buf_addr = (char*)m+sizeof(struct rte_mbuf);
                        //         char* pkt = rte_pktmbuf_mtod(bufs[i],char*);
                        //         struct timeval tv;
                        //         gettimeofday(&tv,NULL);
                        //         struct pcap_sf_pkthdr hdr;
                        //         hdr.ts.tv_sec = tv.tv_sec;
                        //         hdr.ts.tv_usec = tv.tv_usec;
                        //         hdr.caplen = m->data_len;
                        //         hdr.len = m->data_len;
                        //         memcpy(str+strlen(str),&hdr,sizeof(hdr));
                        //         //printf("%d\n",strlen(str));
                        //         memcpy(str+strlen(str),pkt,strlen(pkt));  
                        // }
                        // int length = lseek(fd, 0, SEEK_END);
                        // char* addr = (char *)mmap(NULL, length+strlen(str), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                        // //offt_t length = lseek(fd, 0, SEEK_END);
                        // if(addr)
                        // {
                        //         int f_ret = ftruncate(fd, length+strlen(str));//在此文件大小的基础上扩展至512字节大小，即文件现有大小为512字节（是个好函数，该记着。）

                        //         if (-1 == f_ret)//成功返回0

                        //         {

                        //         printf("ftruncate \n");

                        //         }
                        //         addr = (char *)mmap(NULL, length+strlen(str), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                        // }
                        // memcpy(addr,str,strlen(str));
                        // close(fd);
                        // munmap((void *)addr, length+strlen(str));
                        // printf("写入成功\n");
                        /* Send burst of TX packets, to second port of pair. */
                        //发包，发送nb_rx个包
                        //端口，队列，发送缓冲区，发包个数
                        const uint16_t nb_tx = rte_eth_tx_burst(port ^ 1, 0,
                                        bufs, nb_rx);

                        //*****注意：以上流程为：从x收到的包，发送到x^1口
                        //其中，0^1 = 1, 1^1 = 0
                        //此运算可以达到测试要求的收、发包逻辑

                        /* Free any unsent packets. */
                        //释放不发送的数据包
                        //1、收到nb_rx个包，转发了nb_tx个，剩余nb_rx-nb_tx个
                        //2、把剩余的包释放掉
                        if (unlikely(nb_tx < nb_rx)) {
                                uint16_t buf;
                                for (buf = nb_tx; buf < nb_rx; buf++)
                                        rte_pktmbuf_free(bufs[buf]);    //释放包
                        }
                }
        }
}

/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
int
main(int argc, char *argv[])
{
        struct rte_mempool *mbuf_pool;  //指向内存池结构的指针变量
        unsigned nb_ports;              //网口个数
        uint8_t portid;                 //网口号，临时的标记变量

        /* Initialize the Environment Abstraction Layer (EAL). */
        int ret = rte_eal_init(argc, argv);     //初始化
        if (ret < 0)
                rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

        argc -= ret;                    //表示减去初始化成功后剩余的参数个数
        argv += ret;                    //剩余的参数数组位置

        /* Check that there is an even number of ports to send/receive on. */
        nb_ports = rte_eth_dev_count_avail(); //获取当前有效网口的个数
        int total_ports = rte_eth_dev_count_total();
        if((nb_ports&1)==1) nb_ports-=1;
        printf("port total:%d\nport num:%d\n",total_ports,nb_ports);
        if (nb_ports < 2 || (nb_ports & 1))     //如果有效网口数小于2或有效网口数为奇数0，则出错
                rte_exit(EXIT_FAILURE, "Error: number of ports must be even\n");

        /* Creates a new mempool in memory to hold the mbufs. */
        /*创建一个新的内存池*/
        //"MBUF_POOL"内存池名, NUM_MBUFS * nb_ports网口数，内存池的中对象的个数,
        //      MBUF_CACHE_SIZE 应用层为cpu准备的缓存, 0, RTE_MBUF_DEFAULT_BUF_SIZE 默认的一个数据报文的长度, rte_socket_id()所在cpu
        //此函数为rte_mempoll_create()的封装
        mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports,
                MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
        //设置缓存是为了向内存池申请空间是，首先从每个cpu缓存查找是否存在空闲的对象元素，无则从ring队列中取，
		//维护的是应用层为cpu准备的缓存,从而减少多个cpu同时访问内存池上的元素，减少竞争
        if (mbuf_pool == NULL)
                rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

        //初始化所有的网口
        /* Initialize all ports. */
        for (portid = 0; portid < nb_ports; portid++)   //遍历所有网口
                if (port_init(portid, mbuf_pool) != 0)  //初始化指定网口，需要网口号和内存池，将内存池分配给每个port，此函数为自定义函数，看前面定义
                        rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu8 "\n",
                                        portid);

        //如果逻辑核心总数>1 ,打印警告信息，此程序用不上多个逻辑核心
        //逻辑核心可以通过传递参数 -c 逻辑核掩码来设置
        if (rte_lcore_count() > 1)
                printf("\nWARNING: Too many lcores enabled. Only 1 used.\n");

        /* Call lcore_main on the master core only. */
        //执行主函数
        lcore_main();

        return 0;
}
