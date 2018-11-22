#include "utils.h"
#include "string_util.h"

#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <unistd.h>

void readsocket(int sockfd){
  char *result = read_text_from_socket(sockfd);
  char *temp = malloc(strlen(result));
  strcpy(temp,result);
  char *test[100]={};
  int i = 1;
  char *requestlist = strtok(result,"\n");

  // get line
  *test = requestlist;
  while(requestlist!=NULL){
    *(test+i) = strtok(NULL,"\n");
    requestlist = *(test+i);
    i++;
  }
  i = 0;
    // printf("q%s\n",result);
    // printf("%s\n%d\n",*(test+1),strlen(test));
  // dir
  if(contains(*(test+1),"directory")){
    // get each content in the dir
    char *con[20],query[50];
    con[i] = strtok(*(test+4)," ");
    while(con[i]!=NULL){
      i++;
      con[i] = strtok(NULL," ");
    }
    printf("%s", temp);
  }else{ // file
    printf("%s\n\n", temp);
  }
  free(result);
}
int main(int argc, char *argv[]) {

  char request[200];
  char *query = argv[2];
  char *ip = argv[4];
  char *port = argv[6];
  int portnum;

  portnum = atoi(port);


  // scanf("%s",ip);
  // printf("query: %s",query);
  // printf("ip: %s",ip);
  // printf("port: %d",portnum);

  if (argc != 7) {
    printf("Syntax: client <url>\n");
    exit(1);
  }


  // char *url = argv[1];
// connecting 
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  // Check create socket or not
  if (sockfd == -1) {
    // error("socket");
    printf("\n Error : Could not create socket\n");
    return 1;
  }

  // uint16_t port = portnum;

  struct sockaddr_in serv_addr;

  bzero(&serv_addr, sizeof(serv_addr));
  // setting sockaddr_in
  serv_addr.sin_family = PF_INET;
  serv_addr.sin_port = htons(portnum);
  serv_addr.sin_addr.s_addr = inet_addr(ip);
  // serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  // inet_pton convert IPv4 and IPv6 addresses from text to binary form
  // if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<=0) {     
  //     printf("\n inet_pton error occured\n");
  //     return 1;
  // }

  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in)) == -1) {
    /* Try running the client without the server started and you'll get this error */
    // error("connect");
    printf("\n Error : Connect Failed \n");
    return 1;
  }
// complete connecting

  char *get_str = malloc(128 + strlen(query));
  // char *t1 = malloc(128 + strlen(query));
  // char *t2 = malloc(128 + strlen(query));

  // sprintf(get_str, "GET %s HTTP/1.x\n", url);
  sprintf(get_str,"GET %s HTTP/1.x\r\nHOST: %s:%s\r\n\r\n",query,ip,port);  
  printf("%s", get_str);
  write_to_socket(sockfd, get_str);
  // write_to_socket(sockfd, "GET /static/example_org.html HTTP/1.1"); // get_str);


  // sprintf(t1,"GET /secfolder/status.c HTTP/1.x\r\nHOST: %s:%s\r\n\r\n",ip,port);  
  // sprintf(t2,"GET /secfolder/db.json HTTP/1.x\r\nHOST: %s:%s\r\n\r\n",ip,port);  
  
  readsocket(sockfd);
  
  free(get_str);
  close(sockfd);

  return 0;
}
