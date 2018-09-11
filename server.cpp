#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ThreadPool.h"
#include <fstream>

#include <string.h>
#include <string>
#include <iostream>

#define PORT 5000

void respond (int sock);
bool complete_sender(int _socket, const char* text, int text_size);
std::string copy_from_file(int& body_len, std::string add_path);
std::string get_content_type(std::string address_link);
std::string split_buffer(char* proto_buffer, int buffer_size);

int line_cnt;
char *ROOT;
//bool is404;

int main( int argc, char *argv[] ) {
  line_cnt = 1;
  int sockfd, newsockfd, portno = PORT;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  clilen = sizeof(cli_addr);
  ROOT = getenv("PWD");

  /* First call to socket() function */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) {
    perror("ERROR opening socket");
    exit(1);
  }

  // port reusable
  int tr = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }

  /* Initialize socket structure */
  bzero((char *) &serv_addr, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  /* TODO : Now bind the host address using bind() call.*/
  if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
    perror("ERROR on binding");
    exit(1);
  }

  /* TODO : listen on socket you created */
  listen(sockfd, 5);


  printf("Server is running on port %d\n", portno);

  ThreadPool pool(5);

  while (1) {
    /* TODO : accept connection */
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

    // TODO : implement processing. there are three ways - single threaded, multi threaded, thread pooled
    // respond(newsockfd);
    pool.enqueue([newsockfd]{
      respond(newsockfd);
    });
  }

  return 0;
}

void respond(int sock) {
  //is404 = 0;
  printf("started respond()\n");
  int n;
  char buffer[9999];
  char abs_path[256];

  bzero(buffer,9999);
  n = recv(sock,buffer,9999, 0);
  if (n < 0) {
    printf("recv() error\n");
    return;
  } else if (n == 0) {
    printf("Client disconnected unexpectedly\n");
    return;
  } else {
    //std::string method = strtok(buffer, " \t");
    std::string method = split_buffer(buffer, 9999);	
	std::cout << method << std::endl;
    if(method != "GET"){
      printf("%d this is not GET request\n", line_cnt++);
      return;
    }
    printf("%d Get request\n",line_cnt++);
    int method_len = method.size();
    char* temp_buf = buffer + method_len;
//    method = strtok(NULL, " \t");
    method = split_buffer(temp_buf, 9999);  
    std::string add_path = method;

    if(add_path == "/"){
      add_path = "/index.html";
      printf("%d request to index.html\n", line_cnt++);
    }
    add_path = ROOT + add_path;
    int body_len = 0;
    std::cout << line_cnt++ <<  " request come with url " << add_path << std::endl;
    std::string body = copy_from_file(body_len, add_path);
    std::cout << line_cnt++ << " copied from file or failed " << std::endl;
    std::string status_line = "HTTP/1.1 200 OK\n";
/*    
if(body == "no content"){
      status_line = "HTTP/1.1 204 OK\n";
    }*/
    std::string content_length = "Content-Length: " + std::to_string(body_len) + "\n";
    std::string connection = "Connection: close\n";
    std::string _type = get_content_type(add_path);
    std::string content_type = "Content-Type: " + _type + "\n\n";
    
    if(_type == "GG" || body == "no content"){ 
      std::cout << line_cnt++ << " started 404 request" << std::endl;
      //add_path = "/404.html";
      //add_path = ROOT + add_path;
      body = 
	"<html> <title> 404 error: Bad request </title> <head><h1> 404 error: Bad request </h1></title> </html>";
      body_len = body.size();
      std::cout << line_cnt++ <<  " request come with url " << "404" << std::endl;
      status_line = "HTTP/1.1 404 Not Found\n";    
      content_length = "Content-Length: " + std::to_string(body_len) + "\n";
      content_type = "Content-Type: text/html \n\n";
    }
    
    std::cout << line_cnt++ << " obrained content type" << std::endl;
    std::string total_text = status_line + content_length + connection + content_type + body;
    int len = total_text.size();
    const char* c_total_text = total_text.c_str();
    complete_sender(sock, c_total_text, len);
         std::cout << line_cnt++ << " finished all details" << std::endl;
    // TODO : parse received message from client
    // make proper response and send it to client

  }

  shutdown(sock, SHUT_RDWR);
  close(sock);

}

bool complete_sender(int _socket, const char* text, int text_size){
  for(int cnt = text_size, sent_bytes; cnt > 0; cnt = cnt - sent_bytes, text = text + sent_bytes){
    if(cnt <= 0)
      return true;
    sent_bytes = send(_socket, text, text_size, 0);
    if(!(sent_bytes > 0))
      return false;
  }
  return true;
}

std::string copy_from_file(int& body_len, std::string add_path){
  std::string _body;
  std::ifstream _file(add_path);
  if(_file.is_open())
    std::cout << line_cnt++ << " file: " << add_path << " is opened\n";
  else{
    std::cout << line_cnt++ << " file: " << add_path << " is not opened\n";
    return "no content";
  }
  body_len = 0;
  while(!_file.eof()){
    body_len++;
    char current_character;
    current_character = _file.get();
    _body.push_back(current_character);
  }
  _file.close();
  std::cout << line_cnt++ << " file closed" << std::endl;
  return _body;
}

std::string get_content_type(std::string address_link){
  bool status = 0;
  int address_link_size = address_link.size();
  std::string probably_answer, answer;
  for(int i = 0; i < address_link_size; i++){
    if(address_link[i] == '.'){
      status = 1;
      continue;
    }
    if(status){
      probably_answer.push_back(address_link[i]);
    }
  }
  if(status){
    std::cout << line_cnt++ << " " << probably_answer << std::endl;
    if(probably_answer == "html")
      answer = "text/html";
    else if(probably_answer == "js")
      answer = "text/javascript";
    else if(probably_answer == "css")
      answer = "text/css";
    else if(probably_answer == "jpg")
      answer = "image/jpeg";
    else return "GG";
    //else{ std::cout << line_cnt++ << " " << probably_answer << " is gonna be bad request" << std::endl; is404 = true; }
    return answer;
  }
  else{/* std::cout << line_cnt++ << " is gonna be bad request 1" << std::endl; is404 = true;*/ return "GG"; }
}

std::string split_buffer(char* proto_buffer, int buffer_size){
  std::string first_word;
  bool stat = 0;
  for(int i = 0; i < buffer_size; i++){
    if(stat == 0 && (proto_buffer[i] == ' ' || proto_buffer[i] == '\t'))
	continue;
    else if(proto_buffer[i] == ' ' || proto_buffer[i] == '\t')
	break;
    else{
        stat = 1;
	first_word.push_back(proto_buffer[i]);
    }
  }
  return first_word;
}
