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




ssize_t GET_REQUEST(i8 *buffer,i32 clientFD,u64 total_sz){
      return send(clientFD,buffer,total_sz,0);
}

ssize_t POST_REQUEST(client_arg *ags,i8 *buffer,i8 *path){
   ssize_t sent_bytes;
   i8 fullpath[BUFF];
   snprintf(fullpath,sizeof(fullpath),"%s%s",ags->dir,path+7);

   i8 *content_length=strstr(buffer,"Content-Length:");
   if(!content_length){
       sent_bytes=NOT_FOUND(ags->clientfd,"Missing content length");
   }else{
       content_length+=strlen("Content-Length:");
       while(*content_length==' ') content_length++;

       u32 cont_length=(int)strtol(content_length,NULL,0);
       
       i8 *body=strstr(buffer,"\r\n\r\n");

       if(body){
            body+=strlen("\r\n\r\n");

            FILE *file=fopen(fullpath,"wb");

            if(!file){
                sent_bytes=NOT_FOUND(ags->clientfd,"Failed to create file");
            }

            fwrite(body,1,cont_length,file);
            fclose(file);

            sent_bytes=RES_CREATED(ags->clientfd);
       }else{
         sent_bytes=NOT_FOUND(ags->clientfd,"Empty body");
       }
   }

   return sent_bytes;

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



i8 *FILE_ENCODING(i8 *encoding_format,i32 *res_size,i8 *str){
   encoding_format+=strlen("Accept-Encoding:");
   while(*encoding_format==' ') encoding_format++;
   i8 *encodings=strdup(encoding_format);
   i8 *token=strtok(encodings,",");
   i8 *encoding_choice=NULL;
   bool suports_gzip=false;
   
   while(token){
       while(*token==' ') token++;
       if(strncmp(token,"gzip",4)==0){
            suports_gzip=true;

            encoding_choice=strdup(token);
            break;
       }else{
          printf("False\n");
       }

       token=strtok(NULL,",");
   }

  free(encodings);

   if(encoding_choice){

      char *contains_crlf=strstr(encoding_choice,"\r\n");
      char *contains_new_line=strstr(encoding_choice,"\n");
   
      if(contains_new_line || contains_crlf){
         char *newline = contains_crlf ? contains_crlf : contains_new_line;
         *newline = '\0'; 
      }
   }

   i8 *res;
    if(suports_gzip){
       uLong str_len=strlen(str);
       uLong compressed_len=compressBound(str_len)+32;
       Bytef *compressed_data=malloc(compressed_len);

       if(!compressed_data){
           error("Failed to allocate memory for the compressed data");
       }

       uLong output_len=compressed_len;
       
       i32 ret=gzip_compression((Bytef *)str,str_len,compressed_data,&output_len);

      if(ret!=0){
         
          free(compressed_data);
          error("Failed to compress");
      }

       i32 header_len=snprintf(
               NULL,0,
               "HTTP/1.1 200 OK\r\n"
               "Content-Encoding: %s\r\n"
               "Content-Length: %lu\r\n"
               "Content-Type: text/plain\r\n"
               "Connection: close\r\n"
               "\r\n",
               encoding_choice,
               output_len
       );

       res=malloc(header_len+output_len+1);
       memset(res,0,header_len+output_len+1);
       snprintf(
          res,header_len+1,
          "HTTP/1.1 200 OK\r\n"
          "Content-Encoding: %s\r\n"
          "Content-Length: %lu\r\n"
          "Content-Type: text/plain\r\n"
          "Connection: close\r\n"
          "\r\n",
          encoding_choice,
          output_len
      );

      memcpy(res+header_len,compressed_data,output_len);
      i32 total_size=header_len+output_len;
      *res_size=total_size;
      free(compressed_data);
      free(encoding_choice);

    }else{
        i32 header_len=snprintf(
          NULL,0,
          "HTTP/1.1 200 OK\r\n"
          "Content-Type:text/plain\r\n"
          "\r\n"
       );

      
     res=malloc(header_len+1);
     snprintf(
             res,header_len+1,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type:text/plain\r\n"
              "\r\n"
              
         );

         *res_size=header_len;
        
    }
 
    return res;
}


int gzip_compression(Bytef *source,uLong src_len,Bytef *dest,uLong *dst_len){
       z_stream stream;

       stream.zalloc=Z_NULL;
       stream.zfree=Z_NULL;
       stream.opaque=Z_NULL;

       if(deflateInit2(&stream,Z_BEST_COMPRESSION,Z_DEFLATED,15+16,8,Z_DEFAULT_STRATEGY)!=Z_OK){
             return -1;
       }

       stream.avail_in=src_len;
       stream.next_in=source;
       stream.avail_out=*dst_len;
       stream.next_out=dest;

       i32 ret;
       
       do{

          ret=deflate(&stream,Z_FINISH);
       }while(ret==Z_OK);
     
       deflateEnd(&stream);

       if(ret!=Z_STREAM_END){
         printf("%s\n",strerror(errno));
         return -2;
       } 

      *dst_len=stream.total_out;
      return 0;
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
        }else if(strcmp(method,"GET")==0 && strncmp(path,"/files/",7)==0){
             i8 fullpath[BUFF];

             snprintf(fullpath,sizeof(fullpath),"%s%s",ags->dir,path+7);

             i8 *body=READ_FILE_CONTENTS(fullpath,ags->clientfd,&total_size);
           
             if(body){
                sent_bytes=GET_REQUEST(body,ags->clientfd,total_size);
                free(body);

                status_code=200;
                res_size=total_size;
             }else{
               status_code = 404;
               res_size = strlen("Requested file not found");
             }  

        }else if(strcmp(method,"POST")==0 && strncmp(path,"/files/",7)==0){
                  sent_bytes=POST_REQUEST(ags,buffer,path);

        }else if(strcmp(method,"GET")==0 && strncmp(path,"/echo/",6)==0){
                    i8 *str=path+strlen("/echo/");
                    i8 *encoding_format=strstr(buffer,"Accept-Encoding: ");
                    if(!encoding_format){
                        sent_bytes=NOT_FOUND(ags->clientfd,"Bad request");
                    }else{
                        i32 total_len;
                        i8 *res=FILE_ENCODING(encoding_format,&total_len,str);
                        sent_bytes=send(ags->clientfd,res,total_len,0);
                        free(res);
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