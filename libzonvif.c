/****************************************
	Author:9crk	admin@9crk.com
	www.9crk.com
****************************************/
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<netinet/in.h>
char ODM_FIND[] ="<?xml version=\"1.0\" encoding=\"utf-8\"?><SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:wsd=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" 	xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\">\r\n<SOAP-ENV:Header>\r\n<wsa:MessageID>urn:uuid:6a9fd739-c1fe-4db1-bb55-ed835ec98fb0</wsa:MessageID>\r\n<wsa:To>urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsa:To>\r\n<wsa:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</wsa:Action>\r\n</SOAP-ENV:Header>\r\n<SOAP-ENV:Body>\r\n<wsd:Probe>\r\n<wsd:Types>dn:NetworkVideoTransmitter</wsd:Types>\r\n<wsd:Scopes />\r\n</wsd:Probe>\r\n</SOAP-ENV:Body>\r\n</SOAP-ENV:Envelope>\r\n\r\n";

int gMcast_fd;
static int getServiceUrl(char*inBuf,char*outBuf);
static int server_thread(void*arg)
{
	struct sockaddr_in local_addr;
	int err;	 
	int addr_len;
	struct timeval tv_out;
 
	int port = 3702;
	char ipStr[20] = "239.255.255.250";
	int cnt;
	fd_set set;
	struct timeval tv; 
	
    tv_out.tv_sec = 10;//等待1秒
    tv_out.tv_usec = 0;
	gMcast_fd = socket(AF_INET,SOCK_DGRAM,0);
	if(gMcast_fd < 0){printf("socket err\n");return -1;}
	memset(&local_addr,0,sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(port);
	addr_len = sizeof(local_addr);
	err = bind(gMcast_fd,(struct sockaddr *)&local_addr,sizeof(local_addr));
	if(err < 0){
		perror("bind");
		exit(1);
	} else {
		//printf("bind sucessful\n");
	}
	int loop = 1;
	err = setsockopt(gMcast_fd,IPPROTO_IP,IP_MULTICAST_LOOP,&loop,sizeof(loop));
	if(err < 0){
		perror("setsocket():IP MULTICAST_LOOP");
		return -1;
	} else {
		//printf("IP_MULTICAST_LOOP SUCESSFUL\n");
	}
	int ttl=1;
	if(setsockopt(gMcast_fd,IPPROTO_IP,IP_MULTICAST_TTL,&ttl,sizeof(ttl))<0){
    	perror("IP_MULTICAST_TTL");
   		return -1;
  	}
	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr(ipStr);
	mreq.imr_interface.s_addr = inet_addr("192.168.1.194");
	
	err = setsockopt(gMcast_fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq));
	if(err < 0){
		printf("setsockopt():IP ADD MEMBURSHIP\n");
		return -1;
	} else {
		//printf("setsockopt() IP ADD MEMBURSHIP sucessful\n");
	}
	if(setsockopt(gMcast_fd,SOL_SOCKET,SO_RCVTIMEO,(char*)&tv_out,sizeof(tv_out))<0){
		printf("setsocket NONE block ERR\n");
		return -1;
	}
	printf("--------init recv=%d\n",gMcast_fd);
 	char buff[15000];
	char srvUrl[512];
	while(1){		
		FD_ZERO(&set);
		FD_SET(gMcast_fd,&set);
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		err = select(gMcast_fd+1,&set,NULL,NULL,&tv);
		if(err > 0){
			memset(buff,0,15000);
			cnt = recvfrom(gMcast_fd,buff,15000,0,(struct sockaddr*)&local_addr,&addr_len);
			if(cnt > 0){
				memset(srvUrl,0,512);
				err = getServiceUrl(buff,srvUrl);
				if(err==0)printf("==========================%s\n",srvUrl);
			}
		}
	}
}
int start_listen()
{
	pthread_t pid;
	pthread_create(&pid,NULL,server_thread,NULL);
}
static int getServiceUrl(char*inBuf,char*outBuf)
{
	int i;
	char*p1 = strstr(inBuf,"<d:XAddrs>");
	char*p2 = strstr(inBuf,"</d:XAddrs>");
	int cpyLen = p2-p1-10;
	if(p1 == NULL || p2 == NULL)return -1;
	strncpy(outBuf,p1+10,cpyLen);
	for(i=0;i<cpyLen;i++){
		if(outBuf[i] == ' ')outBuf[i] = '\0'; 
	}
	return 0;
	//<d:XAddrs>http://192.168.1.31/onvif/device_service</d:XAddrs>
}
static int getServiceMedia()
{
	
} 
int mcast_send(char*str,int len)
{
	struct sockaddr_in mcast_addr;
 
	int icnt,cnt;
	int port = 3702;
	char ipStr[20] = "239.255.255.250";
	cnt = 0;
#if 0
	gMcast_fd = socket(AF_INET,SOCK_DGRAM,0);
	if(gMcast_fd < 0){printf("socket err\n");return -1;}
#endif	
	memset(&mcast_addr,0,sizeof(mcast_addr));
	mcast_addr.sin_family = AF_INET;
	mcast_addr.sin_addr.s_addr = inet_addr(ipStr);
	mcast_addr.sin_port = htons(port);
	while(len > 1400){
		icnt = sendto(gMcast_fd,&str[cnt],1400,0,(struct sockaddr *)&mcast_addr,sizeof(mcast_addr));
		cnt +=icnt;
		len -= icnt;
		//usleep(100);
	}
	sendto(gMcast_fd,&str[cnt],len,0,(struct sockaddr *)&mcast_addr,sizeof(mcast_addr));
}
int tcp_send()
{
	//socket实现一个http get post
}
int main()
{
	
	start_listen();
	sleep(1);
	
	mcast_send(ODM_FIND,strlen(ODM_FIND));
	//mcast_send(strFoundCMD_B,strlen(strFoundCMD_B));
	 
	sleep(1000000);
}