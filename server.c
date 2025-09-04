#include "server.h"

ssize_t NOT_FOUND(i32 clientFd,i8 *err_message){
     
     i8 header[HEADER_SIZE];
     u64 message_len=strlen(err_message);
     i32 header_len=snprintf(
                   header,
                   sizeof(header),

                   "HTTP/1.1 404 Not Found\r\n"
                   "Content-Length: %ld\r\n"
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


ssize_t RES_CREATED(i32 clientFd){
   i8 *res=
          "HTTP/1.1 201 CREATED\r\n"
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

   snprintf(res,header_len+1,
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



i8 *EXTRACT_USER_AGENT(i8 *buffer){
         //extract the user agent
    i8 *start_of_user_agent=strstr(buffer,"User-Agent:");
    if(start_of_user_agent){
         start_of_user_agent+=strlen("User-Agent:");

         while(*start_of_user_agent==' ') start_of_user_agent++;

         i8 *end_of_user_agent=strstr(start_of_user_agent,"\r\n");

         size_t user_agent_len= end_of_user_agent-start_of_user_agent;
         
         i8 *user_agent=malloc(user_agent_len+1);
         if(!user_agent) return NULL;
         memcpy(user_agent,start_of_user_agent,user_agent_len);
         user_agent[user_agent_len]='\0';
         return user_agent;
    }

    return NULL;
}

void LOG_REQUEST(i8 *clientIp,i8 *version,i8 *path,i8 *method,i32 status_code,size_t res_size,i8 *user_agent){
     
    time_t now=time(NULL);
    i8 t_buffer[64];

    strftime(t_buffer,sizeof(t_buffer),"%Y-%m-%d %H:%M:%S",localtime(&now));

    printf(YELLOW"[%s] %s \"%s %s %s\" %d %zu \"%s\"\n"RESET,
      t_buffer,
      clientIp,
      method,
      path,
      version,
      status_code,
      res_size,
      user_agent ? user_agent : "-");

    FILE *log_file=fopen("access.log","a");

    if(!log_file){
        error("Error while opening the log file");
    }

    fprintf(log_file,"[%s] %s \"%s %s %s\" %d %zu \"%s\"\n",
      t_buffer,
      clientIp,
      method,
      path,
      version,
      status_code,
      res_size,
      user_agent ? user_agent : "-");


      fclose(log_file);
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


    /*
       The request will have (what is received):
                  Request line
                  Headers
                  Body(can be empty)

               Request line:
                     METHOD(i.e  GET,POST etc)
                     PATH
                     HTTP version ->Http/1.1 in this case

            The longest HTTP method is 7 characters ,therefore 8(puls a null terminator) is enough for this
            The path can vary but 1024 is enough
            HTTP version is <16 characters

    */

    i8 method[8],path[1024],version[16];
    ssize_t sent_bytes=0;
    u64 total_size=0;
    i32 status_code=0;
    size_t res_size=0;
    i8 clientIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET,&ags->addr.sin_addr,clientIp,sizeof(clientIp));
    i8 *user=EXTRACT_USER_AGENT(buffer);
  

    //Handling a bad request
    if(sscanf(buffer,"%s %s %s",method,path,version)!=3){
        sent_bytes=NOT_FOUND(ags->clientfd,"Bad request");
        status_code=404;
        res_size=strlen("Bad request");
    }else{
        if(strcmp(path,"/")==0){
             sent_bytes=RES_OK(ags->clientfd);
             status_code=200;
             res_size=0;
        }else if(strncmp(path,"/files/",7)==0){
             i8 fullpath[BUFF];

             snprintf(fullpath,sizeof(fullpath),"%s%s",ags->dir,path+7);

             i8 *body=READ_FILE_CONTENTS(fullpath,ags->clientfd,&total_size);
           
             if(body){
                sent_bytes=RESPONSE_WITH_BODY(body,ags->clientfd,total_size);
                free(body);

                status_code=200;
                res_size=total_size;
             }else{
               status_code = 404;
               res_size = strlen("Requested file not found");
             }  

        }else{
            sent_bytes=NOT_FOUND(ags->clientfd,"Unkown resource");
            status_code=404;
            res_size=strlen("Uknown resource");
        }
    }

    LOG_REQUEST(clientIp,version,path,method,status_code,res_size,user);
   if(user) free(user);

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
      arg->addr=client_addr;
      pthread_t thread;
      pthread_create(&thread,NULL,handle_client,arg);
      pthread_detach(thread);
   }



   close(sockfd);
      
}