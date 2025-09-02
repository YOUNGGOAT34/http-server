#include "server.h"

void error(i8* msg){
     fprintf(stderr,RED"%s :%s\n"RESET,msg,strerror(errno));
     exit(EXIT_FAILURE);
}


void *handle_client(void *args){
   client_arg *ags=(client_arg *)args;
  
    
   // printf(GREEN"Accepted connection from %s:%hd\n"RESET,
   //    inet_ntoa(client_addr.sin_addr),
   //    ntohs(client_addr.sin_port)
   //  );
 
    i8 buffer[BUFF];
    printf("%d\n",ags->clientfd);
    ssize_t received_bytes=recv(ags->clientfd,buffer,BUFF,0);
 
    if(received_bytes<0){
       error("Error receivin client request");
    }
   
    buffer[received_bytes]='\0';
 
    char *line=strtok(buffer,"/");
   
 
   //  write(1,buffer,received_bytes);
 
    i8 path[BUFF];

    i8 *filename=NULL;
    while(line!=NULL){
 
       if(strncmp(line,"files",5)==0){

             filename=strtok(NULL," ");
             break;
       }

       line=strtok(NULL,"/");
    }

  

    snprintf(path,sizeof(path),"%s%s",ags->dir,filename);
   
    FILE *file=fopen(path,"rb");
    i8 res[BUFF];
    ssize_t sent_bytes=0;

    if(!file){
      snprintf(res,sizeof(res),
      "HTTP/1.1 200 OK\r\n"
     //  "Content-Type: text/plain\r\n"
     //  "Content-Length: %ld\r\n"
     //  "\r\n"
     //  "%s",
     //  strlen(agent),
     //  agent

   );


  
   
   
     
    }else{

       fseek(file,0,SEEK_END);
       u64 filesize=ftell(file);
       rewind(file);
   
      i8 file_content[filesize];
   
      if(fread(file_content,1,filesize,file)!=filesize){
          error("Error reading file into buffer\n");
      }
   
   
   
       snprintf(res,sizeof(res),
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: application/octet-stream\r\n"
          "Content-Length: %ld\r\n"
          "\r\n"
          "%s",
          filesize,
          file_content
    
       );
    }

    
    
   
 
    sent_bytes=send(ags->clientfd,res,strlen(res),0);
    if(sent_bytes<0){
        error("Error sending response to client\n");
    }
 
 
    close(ags->clientfd);
    free(args);

   return NULL;
}

void init_add(SA *addr,i32 port){
   memset(addr,0,sizeof(*addr));
   addr->sin_family=AF_INET;
   addr->sin_port=htons(port);
   addr->sin_addr.s_addr=INADDR_ANY;
 
}
void server(i8 *dir){
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

   while(1){

      // i32 *client_fd=malloc(sizeof(i32));
      i32 client_fd=accept(sockfd,(struct sockaddr *)&client_addr,&cadd_len);
      if(client_fd<0){
         close(sockfd);
         error("Failed to accept incoming connections");
      }

      client_arg *arg=malloc(sizeof(client_arg));
      arg->dir=dir;
     
      arg->clientfd=client_fd;
      
      pthread_t thread;
      pthread_create(&thread,NULL,handle_client,arg);
      pthread_detach(thread);
   }



   close(sockfd);
      
}