#ifndef SERVER_H
#define SERVER_H

#include<stdio.h>
#include<stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

//output colors

#define RED   "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW "\033[1;33m"
#define RESET "\033[0m"

#define BUFF 1024
#define HEADER_SIZE 512

#define SA struct sockaddr_in

//signed data types
typedef char i8;
typedef signed short int i16;
typedef signed int i32;
typedef signed long int i64;
//unsigned data types
typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned long int u64;

typedef struct {
   i32 clientfd;
   i8 *dir;
   SA addr;
}client_arg;


//functions
void server(i8 *);
void error(i8* );
void *handle_client(void *);
void init_add(SA *,i32);
ssize_t NOT_FOUND(i32,i8 *);
i8 *READ_FILE_CONTENTS(i8 *,i32 ,u64 *);
ssize_t GET_REQUEST(i8 *,i32 ,u64 );
ssize_t RES_OK(i32 );
ssize_t RES_CREATED(i32 clientFd);
void LOG_REQUEST(i8 *,i8 *,i8 *,i8 *,i32,size_t,i8 *);

#endif

