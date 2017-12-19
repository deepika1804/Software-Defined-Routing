#include <sys/sendfile.h>
#include <sys/time.h>
#define file_data_length 1024
int transferIdArrSize = 256;
int forwardingSock ; 


struct filePkt {
	uint32_t dest_ip_addr;
	uint8_t ttl;
	uint8_t transferID;
	uint16_t seqNo;
	
};

struct dataPktHeader {
	uint32_t dest_ip_addr;
	uint8_t transferID;
	uint8_t ttl;
	
	uint16_t seqNo;
	uint16_t fin;
	uint16_t padding;
	char buff[file_data_length];
	
};
struct dataPktHeader messagesCopy[2];
struct filePkt *fileData;

struct mainTainTransfer {
	struct filePkt data;
	int sockNo;
	FILE *fd;
	FILE *fp;
	int ForwardSock;
	char pathName[20];
	char forwardPayload[1024];
};

struct mainTainTransfer tranIdPkt[256];

struct fileDataInSeq {
	uint16_t seqNo;
	char buff[file_data_length];
};
//struct fileDataInSeq fileArr[10000];



struct fileSeqTracker{
	uint8_t transferID;
	uint16_t seqNo;
	uint16_t firstSeqNo;
	uint16_t lastSeqNo;
	
	uint8_t ttl;
	
};
struct fileSeqTracker fileTracker[256];

struct statsControlPayload{
	uint8_t transferId;
};

struct statsResponsePaylaod{
	uint8_t transferId;
	uint8_t ttl;
	uint16_t padding;
	uint16_t sequenceNo;
};

void initStruct(){
	for(int i=0;i<transferIdArrSize;i++){
		memset(&fileTracker[i],'\0',sizeof(struct fileSeqTracker));
	}
	
	for(int i=0;i<2;i++){
		memset(&messagesCopy[i],'\0',sizeof(struct dataPktHeader));
	}
}
void maintainLast2Data(uint32_t dest_ip,uint8_t ttl,uint8_t transferID,uint16_t seqNo,uint16_t fin,char *data) {
	if(messagesCopy[0].seqNo != '\0'){
		struct dataPktHeader temp = messagesCopy[0];
		messagesCopy[1] = temp;
	}
		messagesCopy[0].dest_ip_addr = dest_ip;
		messagesCopy[0].ttl = ttl;
		messagesCopy[0].transferID = transferID;
		messagesCopy[0].seqNo = seqNo;
		messagesCopy[0].fin = fin;
		messagesCopy[0].padding=htons(0x00);
		memcpy(messagesCopy[0].buff,data,1024);
	
	 
}

int createSenderSocket(){
	int server_socket;
	struct sockaddr_in server_address;

	memset(&server_address, 0, sizeof(struct sockaddr_in));
	server_address.sin_family = AF_INET;
	server_address.sin_port = 0; //MYPORT
	server_address.sin_addr.s_addr = htonl(INADDR_ANY); // for getting IP address

	//open a socket on server
	server_socket = socket(AF_INET,SOCK_STREAM,0);	//socket descriptor which is open all the times.
	
	memset(&(server_address.sin_zero),'\0',8);

	return server_socket;
}


void setSocketOptionsForReuseHere(int server_socket){
	struct linger linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	int yes = 1;

	if (setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
	    perror("setsockopt");
	    
	}
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, (const char*)&yes, sizeof(yes)) < 0) {
    	perror("setsockopt(SO_REUSEPORT) failed");
	}

	if (setsockopt(server_socket, SOL_SOCKET, SO_LINGER, &linger, sizeof(struct linger)) < 0) {
    	perror("setsockopt(SO_LINGER) failed");
	}
}

int connectToNextHop(int sock_index,uint32_t destIP) {
	struct sockaddr_in dest_server_address;
	memset(&(dest_server_address.sin_zero),'\0',8);
	dest_server_address.sin_family = AF_INET;
	int nextHopIndex;
	for(int i=0;i<noOfRouters;i++){
		if(routingData[i].router_Ip == destIP ){
			nextHopIndex = routingData[i].nextHopId;
			break;
		}
	}
	int myIdx = getMyIPIdx();
	int getNewSockFd = INF;
	if(nextHopIndex != INF){
		for(int i=0;i<noOfRouters;i++){
			if(routingData[i].ID == nextHopIndex) {
				struct in_addr addr = {(routingData[i].router_Ip)};
				// printf("sending to destination with next hop %s and port  %" PRIu16 "\n",inet_ntoa(addr),(routingData[i].ID.port2) );
				char buff[15];
				strcpy(buff,inet_ntoa(addr));
				printf("creating connection to  %s and next hop %ld\n", inet_ntoa(addr),routingData[i].ID);
				inet_pton(AF_INET, buff, &(dest_server_address.sin_addr));
				dest_server_address.sin_port = htons(routingData[i].port2); 
				break;
			}
		}

		getNewSockFd = createSenderSocket();

		int connection_status = connect(getNewSockFd, (struct sockaddr*) &dest_server_address, sizeof(dest_server_address));
		if(connection_status == -1) {
			perror("connection error abort\n");
			exit(0);
		}
	}
	return getNewSockFd;

}

void setStats(uint8_t transferID , uint16_t sequenceNo,uint8_t ttlVal) {
	int addnew = 0;
    for(int i=0;i<transferIdArrSize;i++){
    	if(fileTracker[i].transferID == transferID){
    		fileTracker[i].ttl = ttlVal;
    		fileTracker[i].lastSeqNo = sequenceNo;
    		addnew = 0;
    		break;
    	}else{
    		addnew = 1;
    	}
    }
    if(addnew){
    	for(int i=0;i<transferIdArrSize;i++){
    		if(fileTracker[i].transferID == '\0'){
	    		addnew = 1;
	    		fileTracker[i].transferID = transferID;
	    		fileTracker[i].seqNo = sequenceNo;
	    		fileTracker[i].firstSeqNo = sequenceNo;
	    		fileTracker[i].lastSeqNo = sequenceNo;
	    		fileTracker[i].ttl = ttlVal;
	    		break;
	    	}
    	}
    }

    
}

char* create_data_header(char *payload,uint16_t lastBit,int increment,int transferId)
{
    char *buffer;
    
    struct dataPktHeader *data_resp_header;
    
    struct filePkt *tranIdData;
    struct sockaddr_in addr;
    socklen_t addr_size;

    memcpy(&tranIdData,&tranIdPkt[transferId].data, sizeof (struct filePkt));
    
    buffer = (char *) malloc(sizeof(char)* 1036);
    
    data_resp_header = (struct dataPktHeader *) buffer;
    
    data_resp_header->dest_ip_addr = (tranIdData->dest_ip_addr);
    data_resp_header->transferID = (tranIdData->transferID);
    data_resp_header->ttl = (tranIdData->ttl);
    data_resp_header->seqNo = htons(ntohs(tranIdData->seqNo) + increment);
    if(lastBit == 0){
    	data_resp_header->fin = (0);
    }else{
    	data_resp_header->fin = (0x8000);
    }
    data_resp_header->padding = htons(0x00);
    memset(data_resp_header->buff,0,sizeof(data_resp_header->buff));
    memcpy(data_resp_header->buff,payload,1024);
    uint16_t sequenceNo = ntohs(tranIdData->seqNo) + increment;
    setStats(tranIdData->transferID,sequenceNo,tranIdData->ttl);
    maintainLast2Data(tranIdData->dest_ip_addr,tranIdData->ttl,tranIdData->transferID,sequenceNo,data_resp_header->fin,payload);

    return buffer;
}

void data_pkt(int sock_index,char *payload,int buffLen,int isLastBit,int increment,int transferID) {
	printf("sending daat %d\n",transferID);
	uint16_t payload_len, response_len;
    char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;
    
    payload_len = buffLen;
    cntrl_response = create_data_header(payload,isLastBit,increment,transferID);

    struct dataPktHeader *checkHeader;
    checkHeader = (struct dataPktHeader *) cntrl_response_header;
 
    response_len = 1036;
    
 	sendALL(sock_index, cntrl_response, 1036);
    
}




time_t getCurrentT(){
	time_t currTime;
	struct timeval timerVal;
	gettimeofday(&timerVal,NULL);
	currTime = timerVal.tv_sec;
	
	return currTime;
}




void sendFile(int sock_index,char *payload,int lenOfFile) {
	
	struct filePkt *fileData;
	fileData = ( struct filePkt * ) malloc (sizeof (struct filePkt));

	char buf[file_data_length];
	int ret;
	
	struct filePkt *readData = (struct filePkt *) payload;
	size_t len;
	int transferId = readData -> transferID;

	

	fileData->dest_ip_addr = (readData->dest_ip_addr);
	fileData->ttl = (readData->ttl);
	fileData->transferID = (readData->transferID);
	fileData->seqNo = (readData->seqNo);

	

	struct in_addr addr = {(readData->dest_ip_addr)};
	char pathName[lenOfFile];
	snprintf(pathName,lenOfFile,"%s",payload+0x08);

	if(fileData->ttl == 0) return;

	tranIdPkt[transferId].fd = fopen(pathName, "rb");
	printf("transfer is is%d\n",transferId);
	memcpy(&tranIdPkt[transferId].data, &fileData, sizeof (struct filePkt));
	// tranIdPkt[transferId]->data = ( struct filePkt * ) malloc (sizeof (struct filePkt));
	//tranIdPkt[transferId].data = fileData;
	memcpy(tranIdPkt[transferId].pathName,pathName,lenOfFile);


	
	if (tranIdPkt[transferId].fd  == NULL){
            printf("File error\n");

            exit(EXIT_FAILURE);
    }else{
    	

    	tranIdPkt[transferId].sockNo = connectToNextHop(sock_index,fileData->dest_ip_addr);
    	 printf("connection successful on port %d\n", tranIdPkt[transferId].sockNo);
    	if(tranIdPkt[transferId].sockNo == INF) return;
    	int prev=ftell(tranIdPkt[transferId].fd);
	    fseek(tranIdPkt[transferId].fd, 0L, SEEK_END);
	    int sz=ftell(tranIdPkt[transferId].fd);
	    fseek(tranIdPkt[transferId].fd,0,SEEK_SET); //go back to where we were
	    int maxItr = (sz / 1024);


	    int i=0;
    	printf("start time BROADCASTING %d\n",getCurrentT() );
    	while(i < maxItr) {
			len = fread (buf, 1, 1024, tranIdPkt[transferId].fd);
	        if (i == maxItr-1) {
	            data_pkt(tranIdPkt[transferId].sockNo,buf,len,1,i,transferId);
	            controller_response_pkt(sock_index,'\0',0,FALSE,5,0);
	            break;
	        }
	        data_pkt(tranIdPkt[transferId].sockNo,buf,len,0,i,transferId);
	        i = i+1;
	    }
	   
	    close(tranIdPkt[transferId].sockNo);
	    fclose(tranIdPkt[transferId].fd);
	    printf("closing sender socket\n ");
	    

    }
}





void forwardDatPkt(int sock_index,uint32_t destinationIp,uint8_t ttlVal,uint8_t transferID,uint16_t sequenceNo,uint16_t finishBit,char *payload) {
	uint16_t response_len;
    char *cntrl_response_header;
	struct dataPktHeader *data_resp_header;
    
    response_len = 1036;
// printf("here seg fault?? %d\n",transferID );
    cntrl_response_header = (char *) malloc(sizeof(char)* response_len);
    
    data_resp_header = (struct dataPktHeader *) cntrl_response_header;
    
    
    /* forward data without ntohs */
    
    /* Control Code */
    data_resp_header->dest_ip_addr = (destinationIp);
    /* Response Code */
    data_resp_header->transferID = transferID;
    /* Payload Length */
    data_resp_header->ttl = ttlVal;
    data_resp_header->seqNo = htons(sequenceNo);
    data_resp_header->fin = (uint16_t)(finishBit);
    data_resp_header->padding = htons(0x00);
    memcpy(data_resp_header->buff,payload,1024);
    setStats(data_resp_header->transferID,sequenceNo,data_resp_header->ttl);
    if(data_resp_header->ttl > 0){
    	sendALL(sock_index, cntrl_response_header, response_len);
    	
    }

    
    // free(cntrl_response_header);
    cntrl_response_header = NULL;
}


FILE *fp;
int get_Data_Response(int sock_index) {
	int lengthExpected = 1036;
    char cntrl_payload_len[lengthExpected];
    char cntrl_payload[1024];
    memset(cntrl_payload,'\0',sizeof(cntrl_payload));
    bzero(cntrl_payload_len, lengthExpected);

    //printf("receiving data \n");
    if(recvALL(sock_index, cntrl_payload_len, lengthExpected) < 0){
    	printf("closing connection \n");
        close(sock_index);
        return FALSE;
    }


    struct dataPktHeader *header = (struct dataPktHeader *) cntrl_payload_len;

    uint32_t destinationIp = (header->dest_ip_addr);
    uint8_t ttlVal = (header->ttl);
    
    uint8_t transferID = (header->transferID);
    uint16_t sequenceNo = ntohs(header->seqNo);
    uint16_t finishBit = (header->fin);
    memcpy(cntrl_payload,header->buff,1024);
    
    //decrease ttl
    ttlVal = ttlVal - 1;

    
    if(ttlVal != 0){
    	struct in_addr addr = {(destinationIp)};
		if(strcmp(inet_ntoa(addr),myIpAddr) == 0){

			int createFile = 1;
			for(int i=0;i<transferIdArrSize;i++){
				if(fileTracker[i].transferID == transferID){
					createFile = 0;
					break;
				}
			}
			if(createFile){
				printf("seg fault>>\n");
				char fileName[16];
				snprintf(fileName, sizeof(char) * 16, "file-%i", transferID);
				tranIdPkt[transferID].fp = fopen(fileName, "wb");//if file does not exist, create it
				printf("writing on file\n");
				if(tranIdPkt[transferID].fp == NULL) 
			    {
			        printf("error in creating\n");
			        exit(0);
			    }
			}

		    //collect and write to file
		    
		    char writeBuff[1024];
			 int lenWrite = fwrite(cntrl_payload , 1 , 1024 , tranIdPkt[transferID].fp );
		    setStats(transferID,sequenceNo,ttlVal);
			maintainLast2Data(destinationIp,ttlVal,transferID,sequenceNo,header->fin,cntrl_payload);

		    if(finishBit == 32768){
		    	fclose(tranIdPkt[transferID].fp);
		    	printf("closing the receiving/writing connection\n");
	    		return FALSE;
	    	
		    }
		}else{
			//decrease ttl and forward to destination
			
			int createConnection = 1;
			for(int i=0;i<transferIdArrSize;i++){
    			if(fileTracker[i].transferID == transferID){
    				createConnection = 0;
    				break;
    			}
    		}
    		if(createConnection){

    			tranIdPkt[transferID].ForwardSock = connectToNextHop(sock_index,destinationIp);
    			printf("transfer id is %d in forward\n", transferID);
    			printf("Forwarding the data on socket %d\n",tranIdPkt[transferID].ForwardSock);
    			if(tranIdPkt[transferID].ForwardSock == INF) return FALSE;
    			
    		}
			
	    	memcpy(tranIdPkt[transferID].forwardPayload,header->buff,1024);
	    	maintainLast2Data(destinationIp,ttlVal,transferID,sequenceNo,header->fin,tranIdPkt[transferID].forwardPayload);
	    	printf("seg fault forward>> %d\n",transferID);
	    	forwardDatPkt(tranIdPkt[transferID].ForwardSock,destinationIp,ttlVal,transferID,sequenceNo,(header->fin),tranIdPkt[transferID].forwardPayload);
	    	if(finishBit == 32768){
	    		printf("closing  the forwarding on socket %d\n",tranIdPkt[transferID].ForwardSock);
	    		close(tranIdPkt[transferID].ForwardSock);
	    		return FALSE;
	    	}
	    	
		}
    }else{
    	//printf("ttl is %d so packet is dropped and not processed\n",ttlVal );
    }
    
    return TRUE;
}




void sendFileStats(int sock_index,char* payload){
	char *responsePayload;
	struct statsControlPayload *input = (struct statsControlPayload *) payload;
	uint8_t transferIdAsked = (input->transferId);
	struct statsResponsePaylaod *stats_update_packet;

	int j=0;
	int transferIdPos = INF;
	for(int i=0;i<transferIdArrSize;i++){
	    	if(fileTracker[i].transferID == transferIdAsked){
	    		transferIdPos = i;
	    		
	    		break;
	    	}
	 }
	 if(transferIdPos != INF){
	 	uint16_t start = fileTracker[transferIdPos].firstSeqNo;
	    uint16_t end = fileTracker[transferIdPos].lastSeqNo;
	    int dat = (end - start + 1) * 2;
		responsePayload = (char *) malloc(sizeof(char)*(4 + ( dat)));

	    stats_update_packet = (struct statsResponsePaylaod *) responsePayload;
	    stats_update_packet->transferId = (fileTracker[transferIdPos].transferID);
	    stats_update_packet->ttl = (fileTracker[transferIdPos].ttl);
	    stats_update_packet->padding = htons(0x00);
	    int offset = 0;
	    
	    for(int k=start;k<=end;k++){
	    	struct statsResponsePaylaod *router_packet = (struct statsResponsePaylaod *) (responsePayload + offset);
	        router_packet->sequenceNo = (uint16_t) htons(k);
	        offset = offset + 2;
	    }
	    struct statsResponsePaylaod *checkHeader;
	    checkHeader = (struct statsResponsePaylaod *) responsePayload;
	    offset = 0;
		for(int k=start;k<=end;k++){
	    	struct statsResponsePaylaod *router_packet_check = (struct statsResponsePaylaod *) (responsePayload + offset);
	        offset = offset + 2;
	    }
	    int lengthOfData = dat + 4;
		controller_response_pkt(sock_index,responsePayload,lengthOfData,TRUE,6,0);
	 }else{
	 	controller_response_pkt(sock_index,'\0',0,FALSE,6,0);
	 }
	
}

char* createDataPktBuff(struct dataPktHeader message){
	char *buffer;
	struct dataPktHeader *data_packet;
	
	int payload_len = 1024;
	buffer = (char *) malloc(sizeof(char)* (12+payload_len));

    data_packet = (struct dataPktHeader *) buffer;
    /* Control Code */
    data_packet->dest_ip_addr = (message.dest_ip_addr);
    /* Response Code */
    data_packet->transferID = (message.transferID);
    /* Payload Length */
    data_packet->ttl = (message.ttl);
    data_packet->seqNo = (uint16_t) htons(message.seqNo);
    
    	data_packet->fin = htons(message.fin);
    
    data_packet->padding = htons(0x00);
    memcpy(data_packet->buff,(message.buff),1024);

    return buffer;
}
void sendLastData(int sock_index){
	char *responsePayload;
	uint16_t lengthOfData;
	if(messagesCopy[0].seqNo != '\0'){
		responsePayload = createDataPktBuff(messagesCopy[0]);
	    lengthOfData = 1036;
		controller_response_pkt(sock_index,responsePayload,lengthOfData,TRUE,7,0);
	}else{
		lengthOfData = 0;
		controller_response_pkt(sock_index,responsePayload,lengthOfData,FALSE,7,0);
	}
	
	
}

void sendPenultimateData(int sock_index){
	char *responsePayload;
	uint16_t lengthOfData;
	if(messagesCopy[1].seqNo != '\0'){
		responsePayload = createDataPktBuff(messagesCopy[1]);
	    lengthOfData = 1036;
		controller_response_pkt(sock_index,responsePayload,lengthOfData,TRUE,8,0);
	}else{
		lengthOfData = 0;
		controller_response_pkt(sock_index,responsePayload,lengthOfData,FALSE,8,0);
	}
}





