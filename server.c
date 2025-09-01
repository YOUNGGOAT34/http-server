#include "server.h"

void error(i8* msg){
     fprintf(stderr,RED"%s :%s\n"RESET,msg,strerror(errno));
     exit(EXIT_FAILURE);
}

void init_add(SA *addr,i32 port){
   memset(addr,0,sizeof(*addr));
   addr->sin_family=AF_INET;
   addr->sin_port=htons(port);
   addr->sin_addr.s_addr=INADDR_ANY;
 
}
void server(void){
    i32 sockfd=socket(AF_INET,SOCK_STREAM,0);

    if(sockfd<0){
        error("socket creation error");
    }

    u32 opt=1;

   if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))<0){
      close(sockfd);
       error("Setting reusable address");
   }

    SA server_address;
    u16 PORT=4221;
    init_add(&server_address,PORT);
  
   if(bind(sockfd,(const struct sockaddr *)&server_address,sizeof(server_address))<0){
       close(sockfd);
       error("Binding failed");
   }

   i32 backlog=5;
   if(listen(sockfd,backlog)<0){
       close(sockfd);
      error("Failed to listen on incomming connections\n");
   }
   
   printf(GREEN"Server Listening on PORT:%hd\n"RESET,PORT);
   
   SA client_addr;
   socklen_t cadd_len=sizeof(client_addr);
   i32 client_fd=accept(sockfd,(struct sockaddr *)&client_addr,&cadd_len);
   if(client_fd<0){
      close(sockfd);
      error("Failed to accept incoming connections");
   }

   printf(GREEN"Accepted connection from %s:%hd\n"RESET,
     inet_ntoa(client_addr.sin_addr),
     ntohs(client_addr.sin_port)
   );

   i8 buffer[BUFF];

   ssize_t received_bytes=recv(client_fd,buffer,BUFF,0);

   if(received_bytes<0){
      error("Error receivin client request");
   }

   char *method=strtok(buffer," ");
   char *path=strtok(NULL," ");

   i8 *ok_message="HTTP/1.1 200 OK\r\n\r\n";
   i8 *error_message="HTTP/1.1 404 Not Found\r\n\r\n";
   
   ssize_t sent_bytes=0;
   if(strcmp("/",path)!=0){
      sent_bytes=send(client_fd,error_message,strlen(error_message),0);
   }else{
      sent_bytes=send(client_fd,ok_message,strlen(ok_message),0);
   }
   
   if(sent_bytes<0){
       error("Error sending response to client\n");
   }



   close(sockfd);
   close(client_fd);
      
}