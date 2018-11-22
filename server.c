/*
Based on the requirements of this OS class project:
http://pages.cs.wisc.edu/~dusseau/Classes/CS537-F07/Projects/P2/p2.html

TODO: Clean up debug output.
*/
#include "threadpool.h"
#include "utils.h"
#include "string_util.h"

#include <assert.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

const char* GET = "GET";
const char* slash = "/";

char *root;
const int MAX_CWD = 100;

void writeln_to_socket(int sockfd, const char *message) {
  write(sockfd, message, strlen(message));
  write(sockfd, "\r\n", 2);
}

void print_type(int sockfd, int mode){
  switch(mode){
    case 1:
      writeln_to_socket(sockfd, "Content-Type: text/html");
      break;
    case 2:
      writeln_to_socket(sockfd, "Content-Type: text/css");
      break;
    case 3:
      writeln_to_socket(sockfd, "Content-Type: text/x-h");
      break;
    case 4:
      writeln_to_socket(sockfd, "Content-Type: text/x-c");
      break;
    case 5:
      writeln_to_socket(sockfd, "Content-Type: application/json");
      break;
    case 6:
      writeln_to_socket(sockfd, "Content-Type: directory");
      break;
    default:
      writeln_to_socket(sockfd, "Content-Type:");
      break;
  }  
}

void http_404_reply(int sockfd) {
  writeln_to_socket(sockfd, "HTTP/1.x 404 Not_FOUND");
  writeln_to_socket(sockfd, "Content-Type:");
  writeln_to_socket(sockfd, "Server: httpserver/1.x");
}

void http_400_reply(int sockfd){
  writeln_to_socket(sockfd, "HTTP/1.x 400 BAD_REQUEST");
  writeln_to_socket(sockfd, "Content-Type:");
  writeln_to_socket(sockfd, "Server: httpserver/1.x");
}

void http_405_reply(int sockfd){
  writeln_to_socket(sockfd, "HTTP/1.x 405 METHOD_NOT_ALLOWED");
  writeln_to_socket(sockfd, "Content-Type:");
  writeln_to_socket(sockfd, "Server: httpserver/1.x");
}

void http_415_reply(int sockfd){
  writeln_to_socket(sockfd, "HTTP/1.x 415 UNSUPPORT_MEDIA_TYPE");
  writeln_to_socket(sockfd, "Content-Type:");
  writeln_to_socket(sockfd, "Server: httpserver/1.x");
}

void http_get_reply(int sockfd, const char *content, int mode) {
  
  writeln_to_socket(sockfd, "HTTP/1.x 200 OK");
  // char length_str[100];
  // sprintf(length_str, "%d", (int)strlen(content));

  // char *content_length_str = concat("Content-Length: ", length_str);
  print_type(sockfd, mode);
  writeln_to_socket(sockfd, "Server: httpserver/1.x");
  // writeln_to{_socket(sockfd, content_length_str);

  writeln_to_socket(sockfd, "");
  writeln_to_socket(sockfd, content);

  // free(content_length_str);
}

int is_get(char *text) {
  return starts_with(text, GET);
}
int is_slash(char *text) {
  return starts_with(text, slash);
}

char *get_path(char *text) {
  int beg_pos = strlen(GET) + 1;
  char *end_of_path = strchr(text + beg_pos, ' ');
  int end_pos = end_of_path - text;

  int pathlen = end_pos - beg_pos;
  char *path = malloc(pathlen + 1);
  substr(text, beg_pos, pathlen, path);
  path[pathlen] = '\0';

  return path;
}

char *read_file(FILE *fpipe) {
  int capacity = 10;
  char *buf = malloc(capacity);
  int index = 0;

  int c;
  while ((c = fgetc(fpipe)) != EOF) {
    assert(index < capacity);
    buf[index++] = c;

    if (index == capacity) {
      char *newbuf = malloc(capacity * 2);
      memcpy(newbuf, buf, capacity);
      free(buf);
      buf = newbuf;
      capacity *= 2;
    }
  }
  // TODO(petko): Test with feof, ferror?

  buf[index] = '\0';
  return buf;
}

int is_regular_file(char *path){

  struct stat buf;
  int result;
  result = stat(path, &buf);
  if(result!=0)
    printf("fail");
  else{
    return S_ISDIR(buf.st_mode);
  }
}
void output_static_file(int sockfd,char *fullpath,int mode) {

  printf("Opening static file: [%s]\n\n", fullpath);

  FILE *f = fopen(fullpath, "r");

  if (!f) {
    perror("Problem with fopen");
    http_404_reply(sockfd);
  } else {
    char *result = read_file(f);
    http_get_reply(sockfd, result,mode);
    free(result);
  }

}
void output_folder(int sockfd,char *fullpath,int mode){

  printf("Opening static folder: [%s]\n\n", fullpath);
  DIR *dir;
  struct dirent *ent;
  char *result="";

  // open directory
  dir = opendir(fullpath);
  if(dir == NULL){
    printf("canot open");
    http_404_reply(sockfd);
  }else{
    // list all the dir/file in directory
    while((ent = readdir(dir))!=NULL){
      if(strcmp(ent->d_name,".")!=0 &strcmp(ent->d_name,"..")!=0){
        result = concat(result,ent->d_name);
        result = concat(result," ");
      }
    }
    result = concat(result,"\0");
    closedir(dir);
    http_get_reply(sockfd, result,mode);
  }
}
void *handle_socket_thread(void* sockfd_arg) {
  int sockfd = *((int *)sockfd_arg);

  printf("Handling socket: %d\n\n", sockfd);
  
  char *text = read_text_from_socket(sockfd);
  printf("From socket: \n%s\n", text);
  char *path = get_path(text);
  char* fullpath = malloc(strlen(root) + strlen(path) + 1);

  strcpy(fullpath, root);
  strcat(fullpath, path);
  printf("%s\n",fullpath);
  int flag = is_regular_file(fullpath);
  int mode = 0;

  mode = ends_with(path,".htm")  == 1 ? 1:mode;
  mode = ends_with(path,".html") == 1 ? 1:mode;
  mode = ends_with(path,".css")  == 1 ? 2:mode;
  mode = ends_with(path,".h")    == 1 ? 3:mode;
  mode = ends_with(path,".hh")   == 1 ? 3:mode;
  mode = ends_with(path,".c")    == 1 ? 4:mode;
  mode = ends_with(path,".cc")   == 1 ? 4:mode;
  mode = ends_with(path,".json") == 1 ? 5:mode;

  printf("flag %d   mode %d\n",flag,mode);

  if(flag==1 || flag ==4){
    if (is_slash(path)) {
      if(is_get(text)){
        printf("enter\n");
        mode = 6;
        output_folder(sockfd,fullpath,mode);
        printf("back\n");
      }else {
        http_405_reply(sockfd);
      }
    }else{
      http_400_reply(sockfd);
    }
  }else{
    printf("file\n");
    if (is_slash(path)) {
      // char curdir[MAX_CWD];

      // if (!getcwd(curdir, MAX_CWD)) {
      //   error("Couldn't read curdir");
      // }
      if(is_get(text)){
        if(mode){
          printf("cwd[%s]\n", root);
          printf("path[%s]\n", path);
          output_static_file(sockfd, fullpath,mode);
          free(path);
        }else{
          http_415_reply(sockfd);
        }
      }else {
        http_405_reply(sockfd);
      }

    } else {
      // The server only supports GET.
      http_400_reply(sockfd);
    }
  }
  free(fullpath);
  free(text);
  close(sockfd);
  free(sockfd_arg);

  return NULL;
}

int main(int argc, char *argv[]) {

  // read argv
  root = argv[2];
  char *port = argv[4];
  char *thread = argv[6];
  int portnum, threadnum;

  portnum = atoi(port);
  threadnum = atoi(thread);
  
  // create socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    error("ERROR opening socket");
  }

  // setting socket info
  struct sockaddr_in serv_addr;
  bzero(&serv_addr, sizeof(serv_addr));
  
  serv_addr.sin_family = PF_INET;
  serv_addr.sin_port = htons(portnum);
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  // bind socket and address
  bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
 
  // setting listening list and capacity  SOMAXCONN
  if (listen(sockfd, 5) < 0) error("Couldn't listen");

  printf("Running on port: %d\n", portnum);
//--------complete 

  // making a threadpool
  struct thread_pool* pool = pool_init(threadnum);
  printf("Testing threadpool of %d threads.\n", pool_get_max_threads(pool));

  // accept the client one by one
  struct sockaddr_in client_addr;
  int cli_len = sizeof(client_addr);

  while (1) {
    int newsockfd = accept(sockfd, (struct sockaddr *) &client_addr, (socklen_t *) &cli_len);
    if (newsockfd < 0) error("Error on accept");
    printf("New socket: %d\n", newsockfd);

    // view newsockfd as argument for task's workrountine
    int *arg = malloc(sizeof(int));
    *arg = newsockfd;
    pool_add_task(pool, handle_socket_thread, arg);
  }

  // close socket
  close(sockfd);

  return 0;
}
