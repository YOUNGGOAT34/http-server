#include "server.h"

ssize_t NOT_FOUND(i32 clientFd){
     i8 *res="HTTP/1.1 404 Not Found\r\n";
     
     return send(clientFd,res,strlen(res),0);
}


ssize_t RES_OK(i32 clientFd){
   i8 *res="HTTP/1.1 200 OK\r\n";
   return send(clientFd,res,strlen(res),0);
}

i8 *READ_FILE_CONTENTS(i8 *path,i32 clientFd,u64 *total_len){
   FILE *file=fopen(path,"rb");
   
   if(!file){
      ssize_t sent_bytes=NOT_FOUND(clientFd);
      if(sent_bytes<0){
         error("Error sending response to client\n");
     }

     return NULL;
   }
       
      fseek(file,0,SEEK_END);
      u64 filesize=ftell(file);
      rewind(file);
      
     i8 *res=malloc(filesize+HEADER_SIZE);
     i8 *file_content=malloc(filesize);
  
     if(fread(file_content,1,filesize,file)!=filesize){
         error("Error reading file into buffer\n");
     }
  
  
  
      i32 header_len=snprintf(res,filesize+HEADER_SIZE,
         "HTTP/1.1 200 OK\r\n"
         "Content-Type: application/octet-stream\r\n"
         "Content-Length: %ld\r\n"
         "\r\n",
         filesize
      );
   
     memcpy(res+header_len,file_content,filesize);


   *total_len=(u64)header_len+filesize;
    fclose(file);
    free(file_content);
    return res;
}

ssize_t RESPONSE_WITH_BODY(i8 *buffer,i32 clientFD,u64 total_sz){
      return send(clientFD,buffer,total_sz,0);
}

void error(i8* msg){
     fprintf(stderr,RED"%s :%s\n"RESET,msg,strerror(errno));
     exit(EXIT_FAILURE);
}


void *handle_client(void *args){
   client_arg *ags=(client_arg *)args;
  
    i8 buffer[BUFF];
   
    ssize_t received_bytes=recv(ags->clientfd,buffer,BUFF,0);
      
    if(received_bytes<0){
       error("Error receivin client request");
    }
   
    buffer[received_bytes]='\0';
 
    char *line=strtok(buffer,"/");
    
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
    ssize_t sent_bytes=0;
   
    u64 total_size=0;
    i8 *body=READ_FILE_CONTENTS(path,ags->clientfd,&total_size);
    if(body){
       sent_bytes=RESPONSE_WITH_BODY(body,ags->clientfd,total_size);
       free(body);
    }
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