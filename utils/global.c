#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXCLIENTS 4
#define STDIN 0
#define TRUE 1
#define FALSE 0
#define INF 65535


uint16_t CONTROL_PORT;
uint16_t DATA_PORT;
uint16_t ROUTER_PORT;
int controlSock;
int fdmax;
char* myIpAddr;
int startTimer = 0;
struct timeval timeout;


struct timerPkt {
	uint32_t Ip;
	int missedCount;
	time_t startDVTime;
	time_t startDVTime_usec;
	time_t endDVTime;
	time_t endDvTime_usec;
};

struct timerPkt timerArr[5];
int isInitialized = FALSE;
int countPeriodicInt = 2;