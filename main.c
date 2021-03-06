#include <stdint.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
//#include "mmap_main.h"
#include "test_mmap.h"
#define RX_RING_SIZE 128        //���ջ���С
#define TX_RING_SIZE 512        //���ͻ���С

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
        ָ�����ڵĶ�������������ָ�����ǵ�����
        ��tx��rx���������ϣ����û�����
*/
static inline int
port_init(uint8_t port, struct rte_mempool *mbuf_pool)
{
        struct rte_eth_conf port_conf = port_conf_default;      //��������=Ĭ�ϵ���������
        const uint16_t rx_rings = 1, tx_rings = 1;              //����tx��rx���еĸ���
        int retval;                     //��ʱ����������ֵ
        uint16_t q;                     //��ʱ���������к�

        if (port >= rte_eth_dev_count())//rte_eth_dev_count() �豸���ö˿�����
                return -1;

        /* Configure the Ethernet device. */
        //������̫�����豸
        //���ںš����Ͷ��и��������ն��и��������ڵ�����
        retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);   //���������豸
        if (retval != 0)
                return retval;

        /* Allocate and set up 1 RX queue per Ethernet port. */
        //RX���г�ʼ��
        for (q = 0; q < rx_rings; q++) {        //����ָ�����ڵ�����rx����

                //���벢����һ���հ�����
                //ָ�����ڣ�ָ�����У�ָ������RING�Ĵ�С��ָ��SOCKET_ID�ţ�ָ������ѡ�Ĭ��NULL����ָ���ڴ��
                //����rte_eth_dev_socket_id(port)�����,ͨ��port������ȡdev_socket_id??
                //dev_socket_id����δ֪���д��о�
                retval = rte_eth_rx_queue_setup(port, q, RX_RING_SIZE,
                                rte_eth_dev_socket_id(port), NULL, mbuf_pool);
                if (retval < 0)
                        return retval;
        }

        //TX���г�ʼ��
        /* Allocate and set up 1 TX queue per Ethernet port. */
        for (q = 0; q < tx_rings; q++) {        //����ָ�����ڵ�����tx����

                //���벢����һ����������
                //ָ�����ڣ�ָ�����У�ָ������RING��С��ָ��SOCKET_ID�ţ�ָ��ѡ�NULLΪĬ�ϣ�
                //??TXΪ��ûָ���ڴ�أ��������д��о�
                retval = rte_eth_tx_queue_setup(port, q, TX_RING_SIZE,  //���벢����һ����������
                                rte_eth_dev_socket_id(port), NULL);
                if (retval < 0)
                        return retval;
        }

        /* Start the Ethernet port. */
        retval = rte_eth_dev_start(port);       //��������,�����豸���������һ��
        if (retval < 0)
                return retval;

        /* Display the port MAC address. */
        struct ether_addr addr;
        rte_eth_macaddr_get(port, &addr);       //��ȡ������MAC��ַ������ӡ
        printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
                           " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
                        (unsigned)port,
                        addr.addr_bytes[0], addr.addr_bytes[1],
                        addr.addr_bytes[2], addr.addr_bytes[3],
                        addr.addr_bytes[4], addr.addr_bytes[5]);

        /* Enable RX in promiscuous mode for the Ethernet device. */
        rte_eth_promiscuous_enable(port);       //��������Ϊ����ģʽ

        return 0;
}

/*
 * The lcore main. This is the main thread that does the work, reading from
 * an input port and writing to an output port.
 */
/*
//ҵ������ڵ�
//__attribute__((noreturn))�÷�
//���������޷���ֵ
//��������lcore_main����������lcore_main�޷���ֵ
*/
/*
//1�����CPU�������Ƿ�ƥ��
//2������ʹ�ñ���CPU�ͽ�����
//3�����ݽ��ա����͵�while(1)
*/
static __attribute__((noreturn)) void
lcore_main(void)
{
        const uint8_t nb_ports = rte_eth_dev_count();   //��������
        uint8_t port;                                   //��ʱ���������ں�

        /*
         * Check that the port is on the same NUMA node as the polling thread
         * for best performance.
         * ���CPU�������Ƿ�ƥ��
         * 
         */
        for (port = 0; port < nb_ports; port++)                 //�����������ڣ����port�Ƿ���cpuƥ��
                if (rte_eth_dev_socket_id(port) > 0 &&          //���port����cpu�Ƿ����������ڵ�cpu��ͬ
                                rte_eth_dev_socket_id(port) !=
                                                (int)rte_socket_id())
                        printf("WARNING, port %u is on remote NUMA node to "
                                        "polling thread.\n\tPerformance will "
                                        "not be optimal.\n", port);

        printf("\nCore %u forwarding packets. [Ctrl+C to quit]\n",
                        rte_lcore_id());

        /* Run until the application is quit or killed. */
        /*���� ֱ�� Ӧ�ó��� �Ƴ� �� ��kill*/
        int count=0;
        for (;;) {
                /*
                 * Receive packets on a port and forward them on the paired
                 * port. The mapping is 0 -> 1, 1 -> 0, 2 -> 3, 3 -> 2, etc.
                 * ��Eth�������ݰ� �������͵� ETH�ϡ�
                 * ����˳��Ϊ��0 �Ľ��� �� 1�� ���ͣ�
                 *              1 �Ľ��� �� 0�� ����
                 * ÿ�����˿�Ϊһ��
                 */
                for (port = 0; port < nb_ports; port++) {       //������������

                        /* Get burst of RX packets, from first port of pair. */
                        struct rte_mbuf *bufs[BURST_SIZE];

                        //�հ������յ�nb_tx����
                        //�˿ڣ����У��հ����У����д�С
                        const uint16_t nb_rx = rte_eth_rx_burst(port, 0,
                                        bufs, BURST_SIZE);
                        if(nb_rx>0) 
                        {
                                count+=nb_rx;
                              //  printf("receive:%d\n",nb_rx);
                        }
                        if (unlikely(nb_rx == 0))//�յ��ı�����Ϊ0
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
                        //         int f_ret = ftruncate(fd, length+strlen(str));//�ڴ��ļ���С�Ļ�������չ��512�ֽڴ�С�����ļ����д�СΪ512�ֽڣ��Ǹ��ú������ü��š���

                        //         if (-1 == f_ret)//�ɹ�����0

                        //         {

                        //         printf("ftruncate \n");

                        //         }
                        //         addr = (char *)mmap(NULL, length+strlen(str), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                        // }
                        // memcpy(addr,str,strlen(str));
                        // close(fd);
                        // munmap((void *)addr, length+strlen(str));
                        // printf("д��ɹ�\n");
                        /* Send burst of TX packets, to second port of pair. */
                        //����������nb_rx����
                        //�˿ڣ����У����ͻ���������������
                        const uint16_t nb_tx = rte_eth_tx_burst(port ^ 1, 0,
                                        bufs, nb_rx);

                        //*****ע�⣺��������Ϊ����x�յ��İ������͵�x^1��
                        //���У�0^1 = 1, 1^1 = 0
                        //��������Դﵽ����Ҫ����ա������߼�

                        /* Free any unsent packets. */
                        //�ͷŲ����͵����ݰ�
                        //1���յ�nb_rx������ת����nb_tx����ʣ��nb_rx-nb_tx��
                        //2����ʣ��İ��ͷŵ�
                        if (unlikely(nb_tx < nb_rx)) {
                                uint16_t buf;
                                for (buf = nb_tx; buf < nb_rx; buf++)
                                        rte_pktmbuf_free(bufs[buf]);    //�ͷŰ�
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
        struct rte_mempool *mbuf_pool;  //ָ���ڴ�ؽṹ��ָ�����
        unsigned nb_ports;              //���ڸ���
        uint8_t portid;                 //���ںţ���ʱ�ı�Ǳ���

        /* Initialize the Environment Abstraction Layer (EAL). */
        int ret = rte_eal_init(argc, argv);     //��ʼ��
        if (ret < 0)
                rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

        argc -= ret;                    //��ʾ��ȥ��ʼ���ɹ���ʣ��Ĳ�������
        argv += ret;                    //ʣ��Ĳ�������λ��

        /* Check that there is an even number of ports to send/receive on. */
        nb_ports = rte_eth_dev_count_avail(); //��ȡ��ǰ��Ч���ڵĸ���
        int total_ports = rte_eth_dev_count_total();
        if((nb_ports&1)==1) nb_ports-=1;
        printf("port total:%d\nport num:%d\n",total_ports,nb_ports);
        if (nb_ports < 2 || (nb_ports & 1))     //�����Ч������С��2����Ч������Ϊ����0�������
                rte_exit(EXIT_FAILURE, "Error: number of ports must be even\n");

        /* Creates a new mempool in memory to hold the mbufs. */
        /*����һ���µ��ڴ��*/
        //"MBUF_POOL"�ڴ����, NUM_MBUFS * nb_ports���������ڴ�ص��ж���ĸ���,
        //      MBUF_CACHE_SIZE Ӧ�ò�Ϊcpu׼���Ļ���, 0, RTE_MBUF_DEFAULT_BUF_SIZE Ĭ�ϵ�һ�����ݱ��ĵĳ���, rte_socket_id()����cpu
        //�˺���Ϊrte_mempoll_create()�ķ�װ
        mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports,
                MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
        //���û�����Ϊ�����ڴ������ռ��ǣ����ȴ�ÿ��cpu��������Ƿ���ڿ��еĶ���Ԫ�أ������ring������ȡ��
		//ά������Ӧ�ò�Ϊcpu׼���Ļ���,�Ӷ����ٶ��cpuͬʱ�����ڴ���ϵ�Ԫ�أ����پ���
        if (mbuf_pool == NULL)
                rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

        //��ʼ�����е�����
        /* Initialize all ports. */
        for (portid = 0; portid < nb_ports; portid++)   //������������
                if (port_init(portid, mbuf_pool) != 0)  //��ʼ��ָ�����ڣ���Ҫ���ںź��ڴ�أ����ڴ�ط����ÿ��port���˺���Ϊ�Զ��庯������ǰ�涨��
                        rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu8 "\n",
                                        portid);

        //����߼���������>1 ,��ӡ������Ϣ���˳����ò��϶���߼�����
        //�߼����Ŀ���ͨ�����ݲ��� -c �߼�������������
        if (rte_lcore_count() > 1)
                printf("\nWARNING: Too many lcores enabled. Only 1 used.\n");

        /* Call lcore_main on the master core only. */
        //ִ��������
        lcore_main();

        return 0;
}
