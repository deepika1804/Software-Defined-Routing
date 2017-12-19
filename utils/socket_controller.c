/**
 * @d25_assignment3
 * @author  Deepika Chaudhary <d25@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
#include <sys/time.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <unistd.h> // close function

#include "../../include/control_header.h"

#include "./send-Recv-handler.c"
#include "./response-header-handler.c"
#include "./control-code-handler.c"
#include "./routing-table-handler.c"
#include "./sendFile-handler.c"
#include "./conn-handler.c"



fd_set master;    // master file descriptor list
fd_set read_fds;  // temp file descriptor list for select()
int fdmax;
int controlSock;
int appDataSock;
int appRouterSock;
uint32_t myIpVal;


void create_data_router_sockets(uint16_t routerPort,uint16_t dataPort,uint32_t ipVal){
	
	DATA_PORT = dataPort;
	ROUTER_PORT = routerPort;
	appDataSock = createServerSocket(DATA_PORT,FALSE);
	FD_SET(appDataSock,&master);
	printf("appData sock is  %d at post %ld\n",appDataSock,dataPort );
	if(fdmax < appDataSock){
		fdmax = appDataSock;
	}
	//------------------------//
	
	appRouterSock = createRouterSocket();
	FD_SET(appRouterSock,&master);
	if(fdmax < appRouterSock){
		fdmax = appRouterSock;
	}
	myIpVal = ipVal;
	
	setSelectInTimerMode(appRouterSock,myIpVal);
	
}

void setSocket(int sock_idx){
	// printf("setting the socket\n");
	FD_SET(sock_idx,&master);
	FD_SET(sock_idx,&read_fds);
	if(fdmax < sock_idx){
		fdmax = sock_idx;
	}
}

void closeSocket(int sock_idx) {
	// printf("closing the socket\n");
	FD_CLR(sock_idx,&master);
	FD_CLR(sock_idx,&read_fds);
	close(sock_idx);
}


void connectionHandler(){
	while(1){
		read_fds = master;
		int selectReturnVal; 
		if(startTimer){
			selectReturnVal = select(fdmax+1, &read_fds, NULL, NULL, &timeout);
			if (selectReturnVal < 0) {
				perror("error");
				
		    }
		}else{
			selectReturnVal = select(fdmax+1, &read_fds, NULL, NULL, NULL);
			if (selectReturnVal < 0) {
				perror("error");
				
		    }
		}
		
	    
	    int maxSockets = fdmax + 1; 
	    if(selectReturnVal > 0){
	    	for(int i=0;i < maxSockets ;i++){
		    	if(FD_ISSET(i,&read_fds)){
		    		if(i == controlSock){
		    			printf("accept control messgae\n");
		    			int connection_sock = new_control_connection(controlSock);
			    		FD_SET(connection_sock,&master);
			    		FD_SET(connection_sock,&read_fds);
			    		if(fdmax < connection_sock){
		    				fdmax = connection_sock;
		    			}
		    		}else if(i == appDataSock) {
		    			
		    			int data_sock = new_data_connection(appDataSock);
		    			printf("\naccept data messgae on %d\n",data_sock);
		    			FD_SET(data_sock,&master);
			    		FD_SET(data_sock,&read_fds);
			    		if(fdmax < data_sock){
		    				fdmax = data_sock;
		    			}
		    		}else if(i == appRouterSock) {
		    			int sock = appRouterSock;
		    			int response = rcvDistanceVector(appRouterSock);
		    			if(!response){
		    				FD_CLR(i,&master);
							FD_CLR(i,&read_fds);
		    			}
		    		}else if(i != controlSock && i != appDataSock && i != appRouterSock ){
		    			
		    			if(isControl(i)){
		    				printf("connection on control socket %d\n",i );
		    				int result = create_control_msg_header(i);
			    			if(!result){
			    				printf("here\n");
			    				FD_CLR(i,&master);
								FD_CLR(i,&read_fds);
			    			}
			    		}else if(isData(i)){
			    			//printf("connection on data socket %d\n",i );
			    			int result = handle_data_input(i);
			    			if(!result){
			    				FD_CLR(i,&master);
								FD_CLR(i,&read_fds);
								
			    			}
			    			
			    		}else{
			    			printf("closing the connection\n %d",i);
			    			close(i);
			    			FD_CLR(i,&master);
							FD_CLR(i,&read_fds);
			    			
			    		}
		    		}else{
		    			printf("closing the connection here %d\n",i);
		    			close(i);
			    			FD_CLR(i,&master);
							FD_CLR(i,&read_fds);
		    			
		    		}
		    		
		    	}
		    }
		    if(startTimer && timeout.tv_sec == 0 && timeout.tv_usec == 0){
		    	if(timerArr[0].Ip!=0 && timerArr[0].Ip == myIpVal){
		    		// printf("sending update------\n" );
		    		int sock = appRouterSock;
		    		updateSendingTimeOfMyRouter(0,sock,myIpVal);
		    		
		    	}
		    }
	    }else{
	    	//select timedout
	    	//select a function to call according to timeouts
	    	if(timerArr[0].Ip == myIpVal){
	    		
	    		updateSendingTimeOfMyRouter(0,appRouterSock,myIpVal);
	    		
	    	}else{
	    		int socketAlive = findExpectedNeighDv();
	    		if(!socketAlive){
	    			close(appRouterSock);
	    			FD_CLR(appRouterSock,&master);
	    			FD_CLR(appRouterSock,&read_fds);
	    			//create new
	    			appRouterSock = createRouterSocket();
	    			setSocket(appRouterSock);
	
	    			
	    		}
	    	}
	    	

	    	
	    }
	    
	}
	

}



void init(){
	controlSock = createServerSocket(CONTROL_PORT,TRUE);
	FD_ZERO(&master);    // clear the master and temp sets
	FD_ZERO(&read_fds);
	FD_SET(controlSock,&master);
	FD_SET(STDIN,&master);
	fdmax = controlSock;
	connectionHandler();
	
}

