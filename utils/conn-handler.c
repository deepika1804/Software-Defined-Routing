#include <stdbool.h>

#include <sys/socket.h>
#define data_header_size 12


typedef struct node {

	int sockfd;
	struct node * next;
	
}connection;

connection *controlhead = NULL;
connection *datahead = NULL;

void setSocketOptionsForReuse(int server_socket){
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

char * getExternalIpAddress() {
	struct ifaddrs *ifap, *ifa;
	struct sockaddr_in *sa;
	char *addr;

    getifaddrs (&ifap);

    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
    	if (ifa->ifa_addr->sa_family==AF_INET) {
        	sa = (struct sockaddr_in *)ifa->ifa_addr;
        	if(strcmp(ifa->ifa_name,"eth0") == 0){
				addr = inet_ntoa(sa->sin_addr);
            }
        }
    }
    freeifaddrs(ifap);
    
    return addr;
}

void insert_node(int sock){



	connection *current;

	current = controlhead;

	if(current == NULL){
		controlhead = malloc(sizeof(connection));
		controlhead->sockfd = sock;
		controlhead->next = NULL;
		current = controlhead;
	}else{
		while(current->next != NULL){
			current = current->next;
		}
		current = malloc(sizeof(connection));
		current->sockfd = sock;
		current->next = NULL;
	}
	
}


void insert_data_node(int sock,connection **head){
	
	connection *current = *head;
	connection *new_node = malloc(sizeof(connection));
	
	new_node->sockfd = sock;
	new_node->next = NULL;

	if(*head == NULL){
		*head = new_node;
		return;
	}
	
	while(current != NULL){
		
		current = current->next;
	}
	
	current->next = new_node;
	connection *check = NULL; 
	while(check != NULL){
		printf("data node is %d,",check->sockfd);
		check = check -> next;
	}
	return;
	
	
}

int createServerSocket(uint16_t PORT,int isNewIP) {
	int server_socket;
	struct sockaddr_in server_address;

	memset(&server_address, 0, sizeof(struct sockaddr_in));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT); //MYPORT
	server_address.sin_addr.s_addr = htonl(INADDR_ANY); // for getting IP address

	//open a socket on server
	server_socket = socket(AF_INET,SOCK_STREAM,0);	//socket descriptor which is open all the times.
	//sets some options for reusing port and Ip addresses
	setSocketOptionsForReuse(server_socket);

	memset(&(server_address.sin_zero),'\0',8);

	
	if(isNewIP){
		char *externalIpAddr = getExternalIpAddress();
		myIpAddr = (char *) malloc(sizeof(externalIpAddr));
		strcpy(myIpAddr, externalIpAddr);
		printf("\nExternal Ip address is %s\n",myIpAddr);
	}
	
	
	if(bind(server_socket,(struct sockaddr*) &server_address,sizeof(server_address)) < 0){
		perror("Error in Binding: ");
		exit(0);
	}

	if(listen(server_socket, MAXCLIENTS) < 0){
		perror("Error in listening");
		exit(0);
	}

	
	
	return server_socket;
}


int createRouterSocket() {
	int sock_socket;
	struct sockaddr_in sock_address;
	memset(&sock_address, 0, sizeof(struct sockaddr_in));
	sock_address.sin_family = AF_INET;
	sock_address.sin_port = htons(ROUTER_PORT); //MYPORT
	sock_address.sin_addr.s_addr = htonl(INADDR_ANY); // for getting IP address

	//open a socket on server
	sock_socket = socket(AF_INET,SOCK_DGRAM,0);	//socket descriptor which is open all the times.
	//sets some options for reusing port and Ip addresses
	setSocketOptionsForReuse(sock_socket);

	memset(&(sock_address.sin_zero),'\0',8);

	
	if(bind(sock_socket,(struct sockaddr*) &sock_address,sizeof(sock_address)) < 0){
		perror("Error in Binding: ");
		exit(0);
	}


	return sock_socket;

}



int isControl(int sock_idx){
	connection *current;
	
	current = controlhead;
	
	if(current == NULL) return FALSE;
	while(current != NULL){
		
		if(current->sockfd == sock_idx){
			return TRUE;
		}else{
			current = current->next;
		}
	}
	return FALSE;
}


int isData(int sock_idx){
	connection *current;
	current= datahead;
	printf("broke here\n");
	if(current != NULL){
		// printf("current head is %d\n", datahead->sockfd);
	}
	while(current != NULL){
		if(current->sockfd == 2048) exit(0);
		if(current->sockfd == sock_idx){
			return TRUE;
		}else{
			current = current->next;
		}
	}
	return FALSE;
}

void remove_conn(int sock_index){
	connection *current,*prev;
	current = controlhead;
	prev = current;
	while(current != NULL){
		if(current->sockfd == sock_index){
			if(current->next == NULL){
				current = NULL;
				prev = NULL;
				controlhead = NULL;
				
			}else{
				prev->next = current->next;
			}
		}else{
			prev = current;
			current=current->next;
		}
	}
	close(sock_index);
	
}

void remove_data_conn(int sock_index){
	connection *current,*prev;
	current = datahead;
	prev = current;
	while(current != NULL){
		if(current->sockfd == sock_index){
			if(current->next == NULL){
				current = NULL;
				prev = NULL;
				datahead = NULL;
				
			}else{
				prev->next = current->next;
			}
		}else{
			prev = current;
			current=current->next;
		}
	}
	close(sock_index);
	connection *checkDel = datahead;
	while(checkDel != NULL){
		
		checkDel = checkDel->next;
	}
}

int create_control_msg_header(int sock_idx){
	char *cntrl_header, *cntrl_payload;
	uint8_t control_code;
    uint16_t payload_len;
   
	cntrl_header = (char *) malloc(CNTRL_HEADER_SIZE * sizeof(char));
	bzero(cntrl_header, CNTRL_HEADER_SIZE);
	

	if(recvALL(sock_idx, cntrl_header, CNTRL_HEADER_SIZE) < 0){
		 remove_conn(sock_idx);
        close(sock_idx); 
        return FALSE;
    }
    struct CONTROL_HEADER *header = (struct CONTROL_HEADER *) cntrl_header;
    
    control_code = header->control_code;
    payload_len = ntohs(header->payload_len);

    if(payload_len != 0){
    	cntrl_payload = (char *) malloc(sizeof(char)*payload_len);
        bzero(cntrl_payload, payload_len);

        if(recvALL(sock_idx, cntrl_payload, payload_len) < 0){
            remove_conn(sock_idx);
            close(sock_idx);
            return FALSE;
        }
    }
    
    int lenthOfData=payload_len - 8 + 1;
    switch(control_code){
        case 0: author_response(sock_idx);
                break;
        case 1: init_response(sock_idx, cntrl_payload);
                break;
        case 2: router_table_response(sock_idx);
        		break;
        case 3: update_router_costs(sock_idx,cntrl_payload);
        		break;
        case 4: crash(sock_idx);
        		break;
        case 5: sendFile(sock_idx,cntrl_payload,lenthOfData);
        		break;
        case 6: sendFileStats(sock_idx,cntrl_payload);
        		break;
        case 7: sendLastData(sock_idx);
        		break;
        case 8: sendPenultimateData(sock_idx);
        		break;

    }
    if(payload_len != 0) free(cntrl_payload);
    
    return TRUE;
}

int handle_data_input(int sock_index){
	
    int getResponse = get_Data_Response(sock_index);
    if(getResponse == FALSE){
    	remove_data_conn(sock_index);

    }
    return getResponse;
}

int new_control_connection(int sock_index) {
	
	struct sockaddr_in remote_controller_addr;
	int remote_controller_addr_len;
	remote_controller_addr_len = sizeof(remote_controller_addr);
	int new_sock = accept(sock_index, (struct sockaddr *)&remote_controller_addr, &remote_controller_addr_len); //new sock descriptor for accepted connection to send and receive data.
    if(new_sock == -1){
    	perror("error in accept : ");
    }

    insert_data_node(new_sock,&controlhead);
    return new_sock;
}

int new_data_connection(int socketFD) {
	struct sockaddr_in remote_controller_addr;
	int remote_controller_addr_len;
	remote_controller_addr_len = sizeof(remote_controller_addr);
	int new_sock = accept(socketFD, (struct sockaddr *)&remote_controller_addr, &remote_controller_addr_len); //new sock descriptor for accepted connection to send and receive data.
    if(new_sock == -1){
    	perror("error in accept : ");
    }
    setSocketOptionsForReuse(new_sock);
   
    insert_data_node(new_sock,&datahead);
    connection *check = datahead;
	while(check != NULL){
		check = check->next;
	}
	
    return new_sock;
}

