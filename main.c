#include "server.h"

int main(int argc,char *argv[]){
   if(argc<2){
      //  printf(RED"Usage: ./main --directory <path>\n"RESET);
      //  exit(EXIT_SUCCESS);

      server(NULL);
   }else{
      i8 *dir=NULL;
   
     for(int i=0;i<argc-1;i++){
         if(strcmp(argv[i],"--directory")==0){
             dir=argv[i+1];
             break;
         }
     }
   
      server(dir);

   }
   

  
   return 0;
}