#define payload_offset 16
#define CNTRL_OFFSET 0x0C
#define maxRouters 5
#define AUTHOR_STATEMENT "I, d25, have read and understood the course academic integrity policy."

#define ROUTNG_PACKET_SIZE 8

int noOfRouters;
int timeInterval;
int forwardingTable[maxRouters][maxRouters];
int neighboursPresent = 0;
struct routing_table {
	uint16_t routerID;
	uint16_t padding;
	uint16_t nextHopId;
	uint16_t cost;
};


struct updatePKT {
	uint16_t routerID;
	uint16_t cost;
};

struct neighbours
{
	int neighBourNo;
	uint32_t neighIp;
	uint16_t neighbourRouterId;
};

struct neighbours neighboursIdxs[maxRouters];





void author_response(int sock_index)
{
	// 
	uint16_t lengthOfData = sizeof(AUTHOR_STATEMENT) - 1;
	controller_response_pkt(sock_index,AUTHOR_STATEMENT,lengthOfData,TRUE, 0 , 0 );
	
	
}


int neighboursIdx[maxRouters];
int expectedRouterId;
struct Array{
  int *array;
  size_t size;
};

void findsockets() {
	uint16_t routerPort;
    uint16_t dataPort;

	for(int i=0 ; i < noOfRouters; i++){
		struct in_addr addr = {(routingData[i].router_Ip)};
		if(strcmp(inet_ntoa( addr  ),myIpAddr) == 0){
			routerPort = routingData[i].port1;
			dataPort = routingData[i].port2;
			create_data_router_sockets(routerPort,dataPort,routingData[i].router_Ip);
			break;
		}
	}
}

int count;
void initArray(struct neighbours arr[]) {
  for(int i=0;i<maxRouters;i++){
  	arr[i].neighBourNo = INF;
  }
}



int getMyIPIdx() {
	int myIdx;
	for(int i=0;i<noOfRouters;i++){
		struct in_addr ipaddr = {(routingData[i].router_Ip)};
		char buff[15];
		strcpy(buff,inet_ntoa(ipaddr));
		if(strcmp(buff,myIpAddr) == 0){
			myIdx = i;
			break;
		}
	}
	return myIdx;

}


void updateForwardingTable() {
	
	long minArr;
	uint16_t nextHopIdVal;
	int myIdx = getMyIPIdx();
	long value;
		for(int i=0 ;i < noOfRouters;i++){
			value = INF;
			minArr = INF;
			nextHopIdVal = INF;
			if(i != myIdx){
				for(int j=0;j<noOfRouters;j++){
					if(routingData[j].isNeighbour ==  1){
						
							value = routingData[j].cost + forwardingTable[j][i];
							if(value < minArr){
								minArr = value;
								nextHopIdVal = routingData[j].ID;

							}
						
					}

				}
				forwardingTable[myIdx][i] = minArr;
				if(minArr == INF && routingData[i].isNeighbour){
					routingData[i].nextHopId = routingData[i].ID; 
				}else{
					routingData[i].nextHopId = nextHopIdVal;
					
				}
				

			}
		}

	// 	int row, columns;
	// for (int row=0; row<noOfRouters; row++)
	// {
	//     for(int columns=0; columns<noOfRouters; columns++)
	//         {
	//          printf("%5d     ", forwardingTable[row][columns]);
	//         }
	//     printf("\n");
	//  }
}

int initForwardingTable() {
	
	
	int assignCost = 0;

	for(int i=0 ; i < noOfRouters; i++){
		struct in_addr addr = {(routingData[i].router_Ip)};
		assignCost = 0;
		
		if(strcmp(inet_ntoa( addr  ),myIpAddr) == 0){
			assignCost = 1;
		}
		
		for(int j=0 ; j < noOfRouters; j++){
			if(assignCost){
				forwardingTable[i][j] = routingData[j].cost;

				
			}else if(i == j){
				forwardingTable[i][j] = 0;
				
			}else{
				forwardingTable[i][j] = INF;
				
			}
			
		}
	}
	updateForwardingTable();
	
	return 0;
}

void init_create_data(char *payload) {
	struct initPayload *data = (struct initPayload *) payload;
	noOfRouters = ntohs(data->noOfRouters);
	timeInterval = ntohs(data->timeInterval);
	routingData = ( struct initPayload * ) malloc (noOfRouters * sizeof (struct initPayload));
	//initialize the neighbours vector

	initArray(neighboursIdxs);

	for(int i=0;i<noOfRouters;i++){
		
	    struct initPayload *newdata = (struct initPayload *) payload;
	    routingData[i].noOfRouters = noOfRouters;
	    routingData[i].timeInterval = timeInterval;
	    routingData[i].ID = ntohs(newdata->ID);
	    routingData[i].port1 = ntohs(newdata->port1);
	    routingData[i].port2 = ntohs(newdata->port2);
	    routingData[i].cost = ntohs(newdata->cost);
	    struct in_addr addr = {(newdata->router_Ip)};
	    routingData[i].router_Ip = newdata->router_Ip;

	    if(routingData[i].cost != INF && routingData[i].cost != 0){
	    	for(int j=0;j<noOfRouters;j++){
	    		if(neighboursIdxs[j].neighBourNo == INF){
	    			neighboursIdxs[j].neighBourNo = i;
	    			
	    			neighboursIdxs[j].neighIp = routingData[i].router_Ip;
	    			neighboursIdxs[j].neighbourRouterId = routingData[i].ID;
	    			
	    			break;
	    		}
	    	}
	    	
	    }
	    if(routingData[i].cost >= INF){
	    	routingData[i].nextHopId = INF;
	    	routingData[i].isNeighbour = 0;
	    }else if(routingData[i].cost > 0 && routingData[i].cost < INF) {
	    	routingData[i].nextHopId = routingData[i].ID;
	    	routingData[i].isNeighbour = 1;
	    	neighboursPresent = 1;
	    }else if(routingData[i].cost == 0 ){
	    	routingData[i].nextHopId = routingData[i].ID;
	    	routingData[i].isNeighbour = 0;
	    }
	    
	    payload = payload + CNTRL_OFFSET;
	}
	initForwardingTable();

	
	
} 





time_t getCurrentTime(){
	time_t currTime;
	struct timeval timerVal;
	gettimeofday(&timerVal,NULL);
	currTime = timerVal.tv_sec;
	
	return currTime;
}



void sortTimerPktAccToStartTime(){
	for(int i=0;i<noOfRouters-1;i++){
		if(timerArr[i].Ip != '\0' && timerArr[i+1].Ip != '\0' ){
			if((timerArr[i].startDVTime > timerArr[i+1].startDVTime) || (timerArr[i].startDVTime == timerArr[i+1].startDVTime && timerArr[i].startDVTime_usec > timerArr[i+1].startDVTime_usec)){
				struct timerPkt temp = timerArr[i];
				timerArr[i] = timerArr[i+1];
				timerArr[i+1] = temp;
			}
			struct in_addr addr = {(timerArr[i].Ip )};
			
		}
		
	}
}
void createTimerPkt(int pos,uint32_t ipVal,int Val){
	time_t max_usec = 1000000;
	
	time_t extraTime;
	time_t currTime;
	time_t currTime_usec;
	extraTime = 0;
	if(timerArr[0].Ip == '\0'){
		currTime = 0;
		currTime_usec = 0;
	}else if(timerArr[0].Ip != '\0' && timeout.tv_sec >= 0 && timeout.tv_usec  > 0){


		struct in_addr addr = {(timerArr[0].Ip)};
		
		currTime = timerArr[0].endDVTime - timeout.tv_sec;

		if(timerArr[0].endDvTime_usec < timeout.tv_usec && currTime > 0){
			currTime_usec = max_usec - (timeout.tv_usec - timerArr[0].endDvTime_usec);
			currTime = currTime - 1;
		}else{
			currTime_usec = timerArr[0].endDvTime_usec - timeout.tv_usec;
		}
		
	}else{
		//timer expired to end time of expected packet
		currTime = timerArr[0].endDVTime;
		currTime_usec = timerArr[0].endDvTime_usec;
	}
	int isPresent = 0;
	int index;
	for(int i=0;i<noOfRouters;i++){
		if(timerArr[i].Ip != '\0'){
			struct in_addr checkaddr = {(timerArr[i].Ip)};
			struct in_addr givenaddr = {(ipVal)};
			char addrbuff1[15];
			char addrbuff2[15];
			strcpy(addrbuff1,inet_ntoa(checkaddr));
			strcpy(addrbuff2,inet_ntoa(givenaddr));

			if(strcmp(addrbuff1,addrbuff2) == 0){
				isPresent = 1;
				index = i;
			}
		}
	}

	if(isPresent && (timeout.tv_sec != 0 || timeout.tv_usec != 0)){
		if(currTime == timerArr[index].endDVTime && (currTime_usec - timerArr[index].endDvTime_usec>0 && currTime_usec - timerArr[index].endDvTime_usec < 1000)){
			return;
		}
	}
	struct in_addr yaddr = {(timerArr[0].Ip)};
	struct in_addr xaddr = {(ipVal)};
	char buff[15];
	char buff2[15];
	strcpy(buff,inet_ntoa(yaddr));
	strcpy(buff2,inet_ntoa(xaddr));

	if(isPresent && strcmp(buff,buff2) == 0 && (timeout.tv_sec != 0 || timeout.tv_usec != 0)){
    	timerArr[pos].Ip = ipVal;
		timerArr[pos].missedCount = Val;
		timerArr[pos].startDVTime = timerArr[pos].endDVTime;
		timerArr[pos].startDVTime_usec = timerArr[pos].endDvTime_usec;
		timerArr[pos].endDVTime = timerArr[pos].startDVTime + timeInterval;
		timerArr[pos].endDvTime_usec = timerArr[pos].startDVTime_usec + 0;


    	sortTimerPktAccToStartTime();
    	time_t lowerIdx = timerArr[0].endDVTime - currTime;
		time_t upperIdx;



		if(timerArr[0].endDvTime_usec < currTime_usec && lowerIdx > 0) {
			upperIdx = max_usec - (currTime_usec - timerArr[0].endDvTime_usec);
			lowerIdx = lowerIdx - 1;
			
		}else{
			upperIdx = (timerArr[0].endDvTime_usec - currTime_usec);
		}
		timeout.tv_sec = lowerIdx;
		
		timeout.tv_usec = upperIdx;
		return;
    }

	timerArr[pos].Ip = ipVal;
	timerArr[pos].missedCount = Val;
	timerArr[pos].startDVTime = currTime;
	timerArr[pos].startDVTime_usec = currTime_usec;
	timerArr[pos].endDVTime = timerArr[pos].startDVTime + timeInterval;
	timerArr[pos].endDvTime_usec = timerArr[pos].startDVTime_usec + 0;
	
    sortTimerPktAccToStartTime();
   
	
	
	if(timeout.tv_sec == 0 && timeout.tv_usec == 0) {

		//set timer to next expected packet
		time_t lowerIdx = timerArr[0].endDVTime - currTime;
		time_t upperIdx;
		if(timerArr[0].endDvTime_usec < currTime_usec && lowerIdx > 0) {
			upperIdx = max_usec - (currTime_usec - timerArr[0].endDvTime_usec);
			lowerIdx = lowerIdx - 1;
			
		}else{
			upperIdx = (timerArr[0].endDvTime_usec - currTime_usec);
		}

		

		timeout.tv_sec = lowerIdx;
		timeout.tv_usec = upperIdx;
	}

	struct in_addr addr = {(timerArr[0].Ip)};
	
	
	
}

void checkifNExistElseAdd(int neighBourNo,uint32_t neighbourIp,int Id){
	
	//inserted the router neighbour updates
	int isPresent=FALSE;
	for(int i=0;i<noOfRouters;i++){

		if(timerArr[i].Ip == neighbourIp){
			createTimerPkt(i,neighbourIp,3);
			isPresent = TRUE;
			
			break;
			
		}
	}
	if(!isPresent){
		for(int i=0 ; i< noOfRouters;i++){
			if(timerArr[i].Ip == '\0'){
				createTimerPkt(i,neighbourIp,3);
				break;
			}
		}
	}
	
}

void sortNeighbours(){
	for(int i=0;i<noOfRouters-1;i++){
		if(neighboursIdxs[i].neighBourNo > neighboursIdxs[i+1].neighBourNo){
			struct neighbours temp = neighboursIdxs[i];
			neighboursIdxs[i] = neighboursIdxs[i+1];
			neighboursIdxs[i+1] = temp;
		}
	}
}





void readDataAndUpdateDV(char *router_header) {
	struct ROUTER_UPDATE_FIELDS *data = (struct ROUTER_UPDATE_FIELDS *) router_header;
    char src_ip_addr[15];
    uint16_t noOfUpdateFields;
    uint16_t selectedRouterId;

    noOfUpdateFields = ntohs(data->noOfUpdateFields);
    struct in_addr addr = {(data->src_ip_addr)};
    

    strcpy(src_ip_addr,inet_ntoa(addr));

    int idx,myIdx;
    for(int i=0; i < noOfRouters ;i++) {
    	struct in_addr ipaddr = {(routingData[i].router_Ip)};
    	char buff[15];
    	strcpy(buff,inet_ntoa(ipaddr));
    	if(strcmp(buff,src_ip_addr) == 0){
			selectedRouterId = routingData[i].ID;
			idx = i;
			//add neighbour if not present in list
			checkifNExistElseAdd(i,routingData[i].router_Ip,routingData[i].ID);

	    	break;
		}
    }
    myIdx = getMyIPIdx();
    int offset = 0;
    
    for(int i=0;i<noOfUpdateFields;i++){
		
	    struct ROUTER_UPDATE_HEADER *newdata = (struct ROUTER_UPDATE_HEADER *) (router_header + 8 + offset);
	    struct in_addr addr = {(newdata->router_ip_addr)};
	    for(int j=0;j<noOfUpdateFields;j++){
			if(routingData[j].ID == ntohs(newdata->routerID)){
				forwardingTable[idx][j] = ntohs(newdata->routerCost);
				break;
			}
		}
		
		offset = offset + ROUTER_PAYLOAD_SIZE;
		
	}
///updating forwading table with DV
	updateForwardingTable();

	// int row, columns;
	// for (int row=0; row<noOfRouters; row++)
	// {
	//     for(int columns=0; columns<noOfRouters; columns++)
	//         {
	//          printf("%5d     ", forwardingTable[row][columns]);
	//         }
	//     printf("\n");
	//  }
	 
}

int rcvDistanceVector(int routerSock){
	
	
	int sizeExpected = (ROUTER_UPDATE_HEADER_SIZE + ((noOfRouters) * 12)) ;
	char router_header[sizeExpected];
	if(recvAllUdpPkt(routerSock, router_header, sizeExpected) < 0){
		return FALSE;
    }
    readDataAndUpdateDV(router_header);
    return TRUE;
}



int findExpectedNeighDv() {
	struct in_addr ad = {(timerArr[0].Ip)};
		
	int myIdx;
	struct timerPkt currentPkt = timerArr[0];
	if(currentPkt.missedCount == 0){
		//remove this from neighbours and from timerArr
		//set cost to infinity
		myIdx = getMyIPIdx();
		int index;
		for(int j=0;j<noOfRouters;j++){
			struct in_addr addr = {(routingData[j].router_Ip)};
			struct in_addr ipaddr = {(currentPkt.Ip)};
			//set value to inf in forwarding table
			char buff[15];
			char buff2[15];
			strcpy(buff,inet_ntoa(addr));
			strcpy(buff2,inet_ntoa(ipaddr));
			if(strcmp(buff,buff2) == 0){
				routingData[j].cost = INF;
				routingData[j].isNeighbour = 0;
				
				index = j;
				for(int k=0;k<noOfRouters;k++){
					if(neighboursIdxs[k].neighBourNo != INF  && neighboursIdxs[k].neighIp == routingData[j].router_Ip ){
						neighboursIdxs[k].neighBourNo = INF;
						neighboursIdxs[k].neighIp = INF;
						neighboursIdxs[k].neighbourRouterId = INF;
						sortNeighbours();
						break;
					}
				}
			}
			
			
		}
		//exit(0);
		updateForwardingTable();
		time_t currTime = currentPkt.endDVTime;
		time_t currTime_usec = currentPkt.endDvTime_usec;
		//remove from timer pkt
		time_t max_usec = 1000000;

		for(int i=0;i<noOfRouters-1;i++){
			if(timerArr[i].Ip != '\0'){
				timerArr[i] = timerArr[i+1];
			}
			
		}


		time_t lowerIdx = timerArr[0].endDVTime - currTime;
		time_t upperIdx;
		if(timerArr[0].endDvTime_usec < currTime_usec) {
			upperIdx = max_usec - (currTime_usec - timerArr[0].endDvTime_usec);
			lowerIdx = lowerIdx - 1;
		}else{
			upperIdx = (timerArr[0].endDvTime_usec - currTime_usec);
		}
		timeout.tv_sec = lowerIdx;
		timeout.tv_usec = upperIdx;
		struct in_addr ad = {(timerArr[0].Ip)};
		return FALSE;
		//
	}else{
		timerArr[0].missedCount = timerArr[0].missedCount - 1;
		createTimerPkt(0,timerArr[0].Ip,timerArr[0].missedCount);
		return TRUE;
	}
}


void create_routing_header(char *outStr,int noOfRouters ,struct initPayload routeTable[] , uint16_t srcRouterPort, uint32_t srcIp ,int selectedRouter) {
  
    struct ROUTER_UPDATE_HEADER *router_update_packet;
    struct ROUTER_UPDATE_FIELDS *router_initial_fields;
    struct sockaddr_in addr;
    socklen_t addr_size;
    int n = 8 + ((noOfRouters) * 12);
     
   char control_response[n];
    char buffer[n];
    router_initial_fields = (struct ROUTER_UPDATE_FIELDS *) control_response;
    router_initial_fields->noOfUpdateFields = htons(noOfRouters);
    router_initial_fields->srcPort = htons(srcRouterPort);
    router_initial_fields->src_ip_addr = (srcIp);
    
    int offset = 0;
    
    for(int i=0; i<noOfRouters;i++){
        struct ROUTER_UPDATE_HEADER *router_packet = (struct ROUTER_UPDATE_HEADER *) (control_response + 8 + offset);
        
        struct in_addr addr = {(routeTable[i].router_Ip)};
        router_packet->router_ip_addr = (routeTable[i].router_Ip);
        router_packet->routerPort = htons(routeTable[i].port1);
        router_packet->padding = htons(0x00);
        router_packet->routerID = htons(routeTable[i].ID);
        router_packet->routerCost = htons(forwardingTable[selectedRouter][i]);
        offset = offset + ROUTER_PAYLOAD_SIZE;

    }
    
    for(int i=0; i < n; ++i){
        outStr[i] = control_response[i];
      }
    // return control_response[80];

}




//broadcast this every 3 secs
void sendDataToNeighbouringRouters(int routerSock,int isInit,uint32_t ipVal) {
	if(!neighboursPresent) return;
	
	int n = 8 + (noOfRouters * 12);
   char sendingBuffer[n];

  
	//find neighbours
   int newSock = createRouterSocket();
   setSocket(newSock);
   
   int myIdx = getMyIPIdx();
   // for (int row=0; row<noOfRouters; row++)
   //  {
   //      for(int columns=0; columns<noOfRouters; columns++)
   //          {
   //           printf("%5d     ", forwardingTable[row][columns]);
   //          }
   //      printf("\n");
   //   }
   
   int pktLen = (ROUTER_UPDATE_HEADER_SIZE + ((noOfRouters ) * 12));
	create_routing_header(sendingBuffer,noOfRouters , routingData , routingData[myIdx].port1, routingData[myIdx].router_Ip, myIdx );
	
	for(int i=0;i<noOfRouters;i++){
		for(int j=0;j<noOfRouters;j++){
			
			if(neighboursIdxs[i].neighBourNo != INF && neighboursIdxs[i].neighbourRouterId == routingData[j].ID){
				//send to udp command
					struct in_addr addr = {(routingData[j].router_Ip)};
					sendAllUdpPkt(newSock,sendingBuffer,pktLen,inet_ntoa( addr ),routingData[j].port1);
			}
		}
	}
	
	closeSocket(newSock);
	//free(cntrl_response);
	
}


void setSelectInTimerMode(int sock_idx,uint32_t myIpVal){

	startTimer = 1;//set intial timer
	
	createTimerPkt(0,myIpVal,INF);	
	//set intial timer
	timeout.tv_sec = timeInterval;
	// printf("setting timer Val to%ld\n",timeout.tv_sec );
	timeout.tv_usec=0;
		
}

void updateSendingTimeOfMyRouter(int pos,int routerSock,uint32_t myIpVal){
	// printf("updating time of my router --------------------_______________\n");
	createTimerPkt(pos,myIpVal,INF);
	sendDataToNeighbouringRouters(routerSock,FALSE,myIpVal);
}



void init_response(int sock_index,char *payload)
{
	uint16_t lengthOfData = sizeof(payload) - 1;
	controller_response_pkt(sock_index,payload,lengthOfData,FALSE, 1 , 0 );
	if(!isInitialized){
		isInitialized = TRUE;
		init_create_data(payload);
		findsockets();
		
	}
	
	
}




void update_router_costs(int sock_index,char *payload) {
	
	if(isInitialized){
		int myIdx = getMyIPIdx();
		struct updatePKT *newdata = (struct updatePKT *) payload;
		
		

		//printf("update the value of %d to %d\n", ntohs(newdata->routerID),ntohs(newdata->cost));
		uint16_t idVal = ntohs(newdata->routerID);
		uint16_t updatedCost = ntohs(newdata->cost);
		for(int i=0;i<noOfRouters;i++){
			if(routingData[i].ID == idVal){
					forwardingTable[myIdx][i] = updatedCost;
					routingData[i].cost = updatedCost;
				
			}
		}
		
		
		updateForwardingTable();
		
	}
	controller_response_pkt(sock_index,'\0',0,FALSE, 3 , 0 );
	// int row, columns;
	// for (int row=0; row<noOfRouters; row++)
	// {
	//     for(int columns=0; columns<noOfRouters; columns++)
	//         {
	//          printf("%5d     ", forwardingTable[row][columns]);
	//         }
	//     printf("\n");
	//  }
	


}

void crash(int sock_index) {
	//close the router

	uint16_t lengthOfData = 0;
	controller_response_pkt(sock_index,'\0',lengthOfData,FALSE, 4 , 0 );

	exit(0);


}




// int noOfRouters = 5;



void createRoutingResponse(char *str){
	int n = noOfRouters * 8;
    char buffer[n];
    char sBuff[n];
    memset(buffer,'\0',sizeof(buffer));
    struct routing_table *routing_packet;
    
    struct sockaddr_in addr;
    socklen_t addr_size;
	routing_packet = (struct routing_table *) buffer;

    int myIdx = getMyIPIdx();
    uint16_t offset = 0;

    for(int i=0; i<noOfRouters;i++){
        struct routing_table *new_router_packet = (struct routing_table *) (buffer + offset);
        
        new_router_packet->routerID = htons(routingData[i].ID);
        new_router_packet->padding = htons(0x00);
        new_router_packet->nextHopId = htons(routingData[i].nextHopId);
        new_router_packet->cost = htons(forwardingTable[myIdx][i]);
        
        
        offset = offset + ROUTNG_PACKET_SIZE;
    }
    

    for(int i=0; i < n; ++i){
        str[i] = buffer[i];
      }
}



void router_table_response(int sock_index)
{
    int n = noOfRouters * 8;
	char payload[n];
    memset(payload,'\0',sizeof(payload));
	uint16_t payload_len, response_len;
	
    if(ROUTER_PORT){
        createRoutingResponse(payload);
        payload_len = n;
        controller_response_pkt(sock_index,payload,payload_len,TRUE, 2 , 0 );
    }else{
        controller_response_pkt(sock_index,'\0',0,FALSE, 2 , 0 );
    }
   
	
	
}

