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
char SSDP_FIND[]="M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: \"ssdp:discover\"\r\nMX: 10\r\nST: ssdp:all\r\n";
char  GET_SERVICE[] = "POST /onvif/device_service HTTP/1.1\r\nhost: 192.168.1.33\r\nContent-Type: application/soap+xml; charset=utf-8\r\nContent-Length: 346\r\n\r\n<?xml version=\"1.0\" encoding=\"utf-8\"?><s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"><GetServices xmlns=\"http://www.onvif.org/ver10/device/wsdl\"><IncludeCapability>true</IncludeCapability></GetServices></s:Body></s:Envelope>";
char GET_MEDIA[]="POST /onvif/device_service HTTP/1.1\r\nHost: 192.168.1.33\r\nContent-Type: application/soap+xml; charset=utf-8\r\nContent-Length: 347\r\n\r\n<?xml version=\"1.0\" encoding=\"utf-8\"?><s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"><GetServices xmlns=\"http://www.onvif.org/ver10/device/wsdl\"><IncludeCapability>false</IncludeCapability></GetServices></s:Body></s:Envelope>";
char GET_PROFILE[]="POST /onvif/Media HTTP/1.1\r\nHost: 192.168.1.33\r\nContent-Type: application/soap+xml; charset=utf-8\r\nContent-Length: 289\r\n\r\n<?xml version=\"1.0\" encoding=\"utf-8\"?><s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"><GetProfiles xmlns=\"http://www.onvif.org/ver10/media/wsdl\"/></s:Body></s:Envelope>";
char AUTH_STR[]="POST /onvif/Media HTTP/1.1\r\nHost: 192.168.1.33\r\nContent-Type: application/soap+xml; charset=utf-8\r\nAuthorization: Digest username=\"admin\", realm=\"DS-2CD5026EFWD\", qop=\"auth\", algorithm=\"MD5\", uri=\"/onvif/Media\", nonce=\"4d6b5a474f546777516b55364e6d566d5a4749334d54673d\", nc=00000001, cnonce=\"712AB89D57F59525E8713FFE3C16197C\", response=\"64aa077b472188f4e862e85752b94719\"\r\nContent-Length: 289\r\n\r\n<?xml version=\"1.0\" encoding=\"utf-8\"?><s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"><GetProfiles xmlns=\"http://www.onvif.org/ver10/media/wsdl\"/></s:Body></s:Envelope>";
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
	
	tv_out.tv_sec = 10;//µÈ´ý1Ãë
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
				printf("000000000000000000000%s\n",buff);
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
static int getServiceMedia(char* inBuf,char*outBuf)
{
	int i;
	char*p1 = strstr(inBuf,"<tds:XAddr>");
	char*p2 = strstr(inBuf,"</tds:XAddr>");
	int cpyLen = p2-p1-11;
	if(p1 == NULL || p2 == NULL)return -1;
	strncpy(outBuf,p1+11,cpyLen);
	return 0;
	//<tds:XAddr>http://192.168.1.33/onvif/Media</tds:XAddr>
} 
int mcast_send(char*str,int len,char*ipStr,int port)
{
	struct sockaddr_in mcast_addr;
 
	int icnt,cnt;
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
static int http_cmd(char*url,char*sndBuf,int sndLen,char*recvBuf,int*recvLen)
{
    int sock;  
    struct sockaddr_in remote;
    if((sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))<0){  
        perror("Can't create TCP socket!\n");  
        exit(1);  
    }
    bzero(&remote,sizeof(remote));
    remote.sin_family = AF_INET;
    if(inet_aton(url,&remote.sin_addr) == 0) //服务器的IP地址来自程序的参数
    {
        printf("Server IP Address Error!\n");
        exit(1);
    }
    remote.sin_port = htons(80);
     int addr_len = sizeof(remote);
    //向服务器发起连接,连接成功后client_socket代表了客户机和服务器的一个socket连接
    if(connect(sock,(struct sockaddr*)&remote, addr_len) < 0)
    {
        printf("Can Not Connect To %s!\n",url);
        exit(1);
    }  
    send(sock,sndBuf,sndLen,0);
    *recvLen = recv(sock,recvBuf,100000,0);
    return *recvLen;
}
extern int zMd5(char*inBuf,int inLen,char*outBuf,int*outLen); 
extern int zSHA1(char*inBuf,int inLen,char*outBuf,int*outLen);
int main()
{
	int i;
	char buff[10000];
	char buff2[100];
	
	char  auth_qop[100];
	char  auth_realm[100];
	char auth_nonce[100];
	int len;
	start_listen();
	sleep(1);
	memset(buff,0,10000);
	memset(buff2,0,100);
	//http_cmd("192.168.1.33",GET_SERVICE,strlen(GET_SERVICE),buff,&len);
	//getServiceMedia(buff,buff2);
	http_cmd("192.168.1.33",GET_MEDIA,strlen(GET_MEDIA),buff,&len);
	printf("%s\n",buff);
	printf("_______________________________________________________________\n");	
	memset(buff,0,10000);
	http_cmd("192.168.1.33",GET_PROFILE,strlen(GET_PROFILE),buff,&len);
	printf("%s\n",buff);
	//strstr(buff,)
	printf("_______________________________________________________________\n");
	memset(buff,0,10000);
	http_cmd("192.168.1.33",AUTH_STR,strlen(AUTH_STR),buff,&len);
	printf("%s\n",buff);
	
	zMd5("abcd",4,buff,&len);
	for(i=0;i<len;i++)printf("%02x",(unsigned char)buff[i]);printf("\n");
	zSHA1("abcd",4,buff,&len);
	for(i=0;i<len;i++)printf("%02x",(unsigned char)buff[i]);printf("\n");
	
	//mcast_send(ODM_FIND,strlen(ODM_FIND));
	//mcast_send(ODM_FIND,strlen(ODM_FIND),"239.255.255.250",3702);
	//mcast_send(strFoundCMD_B,strlen(strFoundCMD_B));
	 
	sleep(1000000);
}
