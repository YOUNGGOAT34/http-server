#include "server.h"

ssize_t NOT_FOUND(i32 clientFd,i8 *err_message){
     
     i8 header[HEADER_SIZE];
     u64 message_len=strlen(err_message);
     i32 header_len=snprintf(
                   header,
                   sizeof(header),

                   "HTTP/1.1 404 Not Found\r\n"
                   "Content-Length:%ld\r\n"
                   "Content-Type: text/plain\r\n"
                   "\r\n",
                   message_len
     );

     if(header_len<0 || (u64)header_len>sizeof(header)){
         error("Header formatting error");
     }

     i8 *res=malloc(header_len+message_len+1);
     if(!res){
        error("Failed to allocate memory");
     }

     memcpy(res,header,header_len);
     memcpy(res+header_len,err_message,message_len);

     ssize_t sent_bytes= send(clientFd,res,strlen(res),0);
     free(res);
     return sent_bytes;
}


ssize_t RES_OK(i32 clientFd){
   i8 *res=
          "HTTP/1.1 200 OK\r\n"
           "Content-Length: 0\r\n"
           "Content-Type:text/plain\r\n"
           "\r\n"
           ;
   return send(clientFd,res,strlen(res),0);
}

i8 *READ_FILE_CONTENTS(i8 *path,i32 clientFd,u64 *total_len){
   FILE *file=fopen(path,"rb");
   
   if(!file){
      ssize_t sent_bytes=NOT_FOUND(clientFd,"Requested file not found");
      if(sent_bytes<0){
         error("Error sending response to client\n");
     }

     return NULL;
   }
       
      fseek(file,0,SEEK_END);
      u64 filesize=ftell(file);
      rewind(file);
      
      

     i32 header_len=snprintf(NULL,0,
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: application/octet-stream\r\n"
      "Content-Length: %ld\r\n"
      "\r\n",
      filesize
   );

   i8 *res=malloc(filesize+header_len+1);//one is for a null terminator added implicitly by snprintf

   if(!res){
      error("Memory allocation failed");
   }

   snprintf(res,header_len,
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: application/octet-stream\r\n"
      "Content-Length: %ld\r\n"
      "\r\n",
      filesize
   );
   

   if(fread(res+header_len,1,filesize,file)!=filesize){
       
       error("Error reading file into buffer\n");
   }

   *total_len=(u64)header_len+filesize;
    fclose(file);
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
       free(args);
       error("Error receivin client request");
    }
   
    buffer[received_bytes]='\0';
 
    char *line=strtok(buffer,"/");
    ssize_t sent_bytes=0;
    u64 total_size=0;
    //if we have a directory from the commandline then it means the client requested a file
    if(ags->dir){
       
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
       i8 *body=READ_FILE_CONTENTS(path,ags->clientfd,&total_size);
       if(body){
          sent_bytes=RESPONSE_WITH_BODY(body,ags->clientfd,total_size);
          free(body);
       }
    }else{
          sent_bytes=RES_OK(ags->clientfd);
    }


    if(sent_bytes<0){
        free(args);
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
      if(!dir){
        
         arg->dir=NULL;
      }else{

         arg->dir=dir;
      }
     
      arg->clientfd=client_fd;
      
      pthread_t thread;
      pthread_create(&thread,NULL,handle_client,arg);
      pthread_detach(thread);
   }



   close(sockfd);
      
}