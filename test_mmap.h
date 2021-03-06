//  pcap.h  
//  pcaptest  
    //  
    //  Created by ly on 18-5-25.  
 
       
#ifndef pcaptest_pcap_h  
#define pcaptest_pcap_h 
 
//#include "comdef.h" 
#include <stdio.h>
//#include "pcap.h"
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include<sys/time.h>
#define MAGIC 0xa1b2c3d4
//#define MAJOR 2
//#define MINOR 4
       
typedef unsigned int  bpf_u_int32;  
typedef unsigned short  u_short;  
typedef int bpf_int32;  
       
    /* 
     Pcap文件头24B各字段说明： 
     Magic：4B：0x1A 2B 3C 4D:用来标示文件的开始 
     Major：2B，0x02 00:当前文件主要的版本号      
     Minor：2B，0x04 00当前文件次要的版本号 
     ThisZone：4B当地的标准时间；全零 
     SigFigs：4B时间戳的精度；全零 
     SnapLen：4B最大的存储长度     
     LinkType：4B链路类型 
     常用类型： 
     0            BSD loopback devices, except for later OpenBSD 
     1            Ethernet, and Linux loopback devices 
     6            802.5 Token Ring 
     7            ARCnet 
     8            SLIP 
     9            PPP 
     */  
typedef struct pcap_file_header {  
    bpf_u_int32 magic;  
    u_short major;  
    u_short minor;  
    bpf_int32 thiszone;      
    bpf_u_int32 sigfigs;     
    bpf_u_int32 snaplen;     
    bpf_u_int32 linktype;    
}pcap_file_header;  
       
    /* 
     Packet包头和Packet数据组成 
     字段说明： 
     Timestamp：时间戳高位，精确到seconds      
     Timestamp：时间戳低位，精确到microseconds 
     Caplen：当前数据区的长度，即抓取到的数据帧长度，由此可以得到下一个数据帧的位置。 
     Len：离线数据长度：网络中实际数据帧的长度，一般不大于caplen，多数情况下和Caplen数值相等。 
     Packet数据：即 Packet（通常就是链路层的数据帧）具体内容，长度就是Caplen，这个长度的后面，就是当前PCAP文件中存放的下一个Packet数据包，也就 是说：PCAP文件里面并没有规定捕获的Packet数据包之间有什么间隔字符串，下一组数据在文件中的起始位置。我们需要靠第一个Packet包确定。 
     */  
       
typedef struct  timestamp{  
    bpf_u_int32 timestamp_s;  
    bpf_u_int32 timestamp_ms;  
}timestamp;  
       
typedef struct pcap_header{  
    timestamp ts;  
    bpf_u_int32 capture_len;  
    bpf_u_int32 len;  
       
}pcap_header;  
       
       
 
//void encode_pcap(struct rte_mbuf *pktbuf);
      




 
int write_file_header(FILE *fd , pcap_file_header * pcap_file_hdr)
{
    int ret =0;
    if(fd <0 )
	return -1;
    ret = fwrite(pcap_file_hdr,sizeof(pcap_file_header),1,fd);
    if (ret != 1 )
	return -1;
    return 0;
}
 
int write_header(FILE *fd ,pcap_header * pcap_hdr)
{
    int ret =0;
    if(fd < 0 )
	return -1;
    ret = fwrite(pcap_hdr,sizeof(pcap_header),1,fd);
    if(ret != 1)
	return -1;
    return 0;
}
 
 
int write_pbuf(FILE *fd ,struct rte_mbuf *pktbuf)
{
    int ret =0;
    if (fd < 0 )
		return -1;
	char* hash_pkt = rte_pktmbuf_mtod(pktbuf,char*);
    ret = fwrite(hash_pkt,pktbuf->data_len,1,fd);
    if (ret != 1)
		return -1;
    return 0;
}


char* pos=NULL;
int f_len=0;



void encode_pcap(struct rte_mbuf *pktbuf)
{
    
  //  printf("into endecode \n");
    u_short major =2;
    u_short minor =4;
    //char *pbuf = (char*)pktbuf;
    char* pbuf = rte_pktmbuf_mtod(pktbuf,char*);
    uint16_t len =pktbuf->data_len;
    pcap_file_header pcap_file_hdr;
    timestamp pcap_timemp;
    pcap_header pcap_hdr;
 
    // printf("[pcap_file_hdr]=%d\n",sizeof(pcap_file_hdr));
    // printf("[pcap_hdr] = %d \n",sizeof(pcap_hdr));
    // printf("[pcap_file_header]  = %d  \n",sizeof(pcap_file_header));
    // printf("[pcap_header]  =  %d  \n",sizeof(pcap_header));
 
 
//初始化pcap头
   // printf("----struct--\n");
    pcap_file_hdr.magic = 0xa1b2c3d4;
    //printf("magic\n");
    pcap_file_hdr.major = major;
    pcap_file_hdr.minor = minor;
    pcap_file_hdr.thiszone = 0;
    pcap_file_hdr.sigfigs  = 0;
    //printf("snaplen\n");
    pcap_file_hdr.snaplen  = 65535;
    //printf("pcap_file_hdr\n");
    pcap_file_hdr.linktype =1;
    
    struct timeval tv;
    gettimeofday(&tv,NULL);
    
    pcap_timemp.timestamp_s =0;
    pcap_timemp.timestamp_ms= 0;
 
    pcap_hdr.capture_len = (uint32_t)pktbuf->pkt_len;
    pcap_hdr.len = (uint32_t)pktbuf->pkt_len;
    //pcap_hdr.ts = pcap_timemp;
    int ret =0;
 
    // ret = access("encode1.pcap",0);//只写入一个文件，首先判断文件是否存在
    // if(ret == -1)
    // {//不存在创建，名且写入pcap文件头，和pcap头，还有报文（tcp\ip 头和数据）
    //     FILE *fd;
    //     int ret =0;
	// fd = fopen("encode1.pcap","wb+");
	// if (fd == NULL )
	// {
	//     printf("w+:can't open the file\n");
	//     return;
	// }

    // int f_ret = ftruncate((int)fd, sizeof(pcap_file_hdr)+sizeof(pcap_hdr)+pktbuf->pkt_len);
    // char* addr = (char *)mmap(NULL, sizeof(pcap_file_hdr)+sizeof(pcap_hdr)+pktbuf->pkt_len, PROT_READ | PROT_WRITE, MAP_SHARED, (int)fd, 0);

    // memcpy(addr,&pcap_file_hdr,sizeof(pcap_file_hdr));
    // memcpy(addr+sizeof(pcap_file_hdr),&pcap_hdr,sizeof(pcap_hdr));

    // memcpy(addr+sizeof(pcap_file_hdr)+sizeof(pcap_hdr),pbuf,pktbuf->pkt_len);
    // fclose(fd);
    // munmap((void *)addr, 0);
    // //     ret = write_file_header(fd,&pcap_file_hdr);
	// // if(ret == -1 )
	// // {
	// //     printf("write file header error!\n");
	// //     return;
	// // }
	// // fseek(fd,0,SEEK_END);
	// // ret = write_header(fd,&pcap_hdr);
	// // if(ret == -1 )
	// // {
	// //     printf("write header error!\n");
	// //     return;
	// // }
	// // fseek(fd,0,SEEK_END);
	// // ret = write_pbuf(fd,pktbuf);
	// // if (ret == -1)
	// // {
	// //     printf("write pbuf error!\n");
	// //     return;
	// // }
    // //     fclose(fd);
    // //     return;
    // }
    int fd_pcap;//如果文件已存在，直接向文件末尾写入，pcap头和数据
    int ret_a =0;
    fd_pcap = open("encode1.pcap",O_RDWR | O_CREAT, 0644);
    if (fd_pcap == -1)
    {
        printf("a+:can't no open file !\n");
        return;
    }
    struct stat st;
    ret_a = fstat(fd_pcap,&st);
    if(ret_a==-1)
    {
        printf("获取文件大小失败:%m\n");
        close(fd_pcap);
        return; 
    }
    int length = st.st_size;//lseek(fd_pcap,0,SEEK_END);
    pos+=f_len;
    if(length==0)
    {
        int f_ret = ftruncate(fd_pcap, sizeof(pcap_file_hdr)+sizeof(pcap_hdr)+pktbuf->pkt_len);
        printf("first\n");
        if(f_ret)
        {
            printf("ftruncate\n");
        }

        char* addr = (char *)mmap(pos, sizeof(pcap_file_hdr)+sizeof(pcap_hdr)+pktbuf->pkt_len, PROT_WRITE, MAP_SHARED, fd_pcap, 0);

        printf("first222\n");
        memcpy(addr,&pcap_file_hdr,sizeof(pcap_file_hdr));
        printf("-----%x\n",*(uint32_t*)addr);
        memcpy(addr+sizeof(pcap_file_hdr),&pcap_hdr,sizeof(pcap_hdr));
        printf("%u\n",sizeof(pcap_file_hdr));
        printf("%x\n",*(uint32_t*)(addr+sizeof(pcap_file_hdr)));
        printf("%u\n",sizeof(pcap_file_hdr)+sizeof(pcap_hdr));
        memcpy(addr+sizeof(pcap_file_hdr)+sizeof(pcap_hdr),pbuf,pktbuf->pkt_len);
        //printf("%s",addr+sizeof(pcap_file_hdr)+sizeof(pcap_hdr));
        f_len=sizeof(pcap_file_hdr)+sizeof(pcap_hdr)+pktbuf->pkt_len;
        printf("%u---%u----%u\n",pktbuf->pkt_len,pktbuf->data_len,sizeof(pcap_file_hdr)+sizeof(pcap_hdr)+pktbuf->pkt_len);
        printf("first333\n");
        
        munmap((void *)addr, sizeof(pcap_file_hdr)+sizeof(pcap_hdr)+pktbuf->pkt_len);
        close(fd_pcap);
    }else
    {
        char buf[128];
        //fgets(buf,128,fd_pcap);
        //buf = fd_pcap.read();
        int ret_a = read(fd_pcap,buf,128);
        //if(ret_a)
        // if(length<1024)
        // {
            printf("-----%x\n",*(uint32_t*)buf);
        
        
        
        int f_ret = ftruncate(fd_pcap, length+sizeof(pcap_hdr)+pktbuf->pkt_len);
        if(f_ret)
        {
            printf("ftruncate\n");
        }
        char* addr = (char *)mmap(NULL, length+sizeof(pcap_hdr)+pktbuf->pkt_len, PROT_WRITE, MAP_SHARED, fd_pcap, 0);

        // //memcpy(addr+length,&pcap_file_hdr,sizeof(pcap_file_hdr));
        memcpy(addr+length,&pcap_hdr,sizeof(pcap_hdr));
        memcpy(addr+length+sizeof(pcap_hdr),pbuf,pktbuf->data_len);
        //pos-=f_len;
        f_len =sizeof(pcap_hdr)+pktbuf->pkt_len;
        //close(fd_pcap);
        munmap((void *)addr, length+sizeof(pcap_hdr)+pktbuf->pkt_len);
       // }
        close(fd_pcap);
    }
    


    // ret_a = write_header(fd_pcap,&pcap_hdr);
    // if(ret_a == -1)
    // {
	// printf("write header error! \n");
	// return;
    // }
    // fseek(fd_pcap,0,SEEK_END);
    // ret_a =write_pbuf(fd_pcap,pktbuf);
    // if (ret_a == -1)
    // {
	// printf("write pbuf error! \n");
	// return;
    // }
    // fclose(fd_pcap);
   // printf("111end\n");
    return;
      
}
    #endif  