
#include <string.h>


#define ROUTER_PAYLOAD_SIZE 12

char* create_response_header(int sock_index, uint8_t control_code, uint8_t response_code, uint16_t payload_len)
{
    char *buffer;
    
    struct CONTROL_RESPONSE_HEADER *cntrl_resp_header;
    
    
    struct sockaddr_in addr;
    socklen_t addr_size;

    buffer = (char *) malloc(sizeof(char)*CNTRL_RESP_HEADER_SIZE);
    
    cntrl_resp_header = (struct CONTROL_RESPONSE_HEADER *) buffer;
    
    

    addr_size = sizeof(struct sockaddr_in);
    getpeername(sock_index, (struct sockaddr *)&addr, &addr_size);

    //#ifdef PACKET_USING_STRUCT
        /* Controller IP Address */
        memcpy(&(cntrl_resp_header->controller_ip_addr), &(addr.sin_addr), sizeof(struct in_addr));
        /* Control Code */
        cntrl_resp_header->control_code = control_code;
        /* Response Code */
        cntrl_resp_header->response_code = response_code;
        /* Payload Length */
        cntrl_resp_header->payload_len = htons(payload_len);
    //#endif

    

    return buffer;
}

void controller_response_pkt(int sock_index,char *payload,uint16_t lengthOfData,int sendPayload,uint8_t controlCode,uint8_t responseCode) {
    uint16_t payload_len, response_len;
    char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;
    if(sendPayload){
        payload_len = lengthOfData;
        cntrl_response_payload = (char *) malloc(payload_len);
        memcpy(cntrl_response_payload, payload, payload_len);
    }else{
        payload_len = 0;
    }

    cntrl_response_header = create_response_header(sock_index, controlCode, responseCode, payload_len);
    // printf("size of payload is is %ld\n",payload_len);
    response_len = CNTRL_RESP_HEADER_SIZE + payload_len;
    cntrl_response = (char *) malloc(response_len);
    /* Copy Header */
    memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
    /* Copy Payload */
    if(sendPayload){
        memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
       
    }
    
    
    sendALL(sock_index, cntrl_response, response_len);
   
}





