// chat_server_thread.c
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <unistd.h>
#include        <netinet/in.h>
#include        <sys/socket.h>
#include   <pthread.h>

void*       do_chat(void *);

pthread_t   thread;
pthread_mutex_t   mutex;


#define         MAX_CLIENT      10
#define         CHATDATA        1024

#define         INVALID_SOCK        -1

struct {
   int c_socket;
   char nick[20];
}list_c[MAX_CLIENT];

char       escape[] = "exit";
char       greeting[] = "Welcome to chatting room\n";
char       CODE200[] = "Sorry No More Connection\n";
char      nickname[20];
char *cut,*cut1,*cut2,*cut3;
main(int argc, char *argv[])
{
        int     c_socket, s_socket;
        struct  sockaddr_in s_addr, c_addr;
        int     len;
        int     nfds = 0;
        int     i, j, n;
        fd_set  read_fds;
   int   res;

        if (argc < 2) {
                printf("usage: %s port_number\n", argv[0]);
                exit(-1);
        }

   if (pthread_mutex_init(&mutex, NULL) != 0) {
      printf("Can not create mutex\n");
      return -1;
   }

        s_socket = socket(PF_INET, SOCK_STREAM, 0);

        memset(&s_addr, 0, sizeof(s_addr));
        s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        s_addr.sin_family = AF_INET;
        s_addr.sin_port = htons(atoi(argv[1]));

        if (bind(s_socket, (struct sockaddr *)&s_addr, sizeof(s_addr)) == -1) {
                printf("Can not Bind\n");
                return -1;
        }

        if(listen(s_socket, MAX_CLIENT) == -1) {
                printf("listen Fail\n");
                return -1;
        }

   for (i = 0; i < MAX_CLIENT; i++)
      list_c[i].c_socket = INVALID_SOCK;

   while(1) {
      len = sizeof(c_addr);
      c_socket = accept(s_socket, (struct sockaddr *)&c_addr, &len);
      if((n = read(c_socket, nickname, sizeof(nickname)))<0){
         printf("fail\n");
         return -1;
      }
      res = pushClient(c_socket,nickname);
      if (res < 0) {
         write(c_socket, CODE200, strlen(CODE200));
         close(c_socket);
      } else {
         write(c_socket, greeting, strlen(greeting));
         pthread_create(&thread, NULL, do_chat, (void *) c_socket);
      }   
   }
}

void *
do_chat(void *arg) 
{
   int c_socket = (int) arg;
        char    chatData[CHATDATA];
        char   chatData2[CHATDATA];
   int   i, n;
   
   while(1) {
      memset(chatData, 0, sizeof(chatData));
      memset(chatData2, 0, sizeof(chatData2));
             if ((n = read(c_socket, chatData, sizeof(chatData))) > 0 ) {
                cut=strtok(chatData," "); //[닉네임] 문자열
                cut1=strtok(NULL," "); // /w인지 확인 위함
                cut2=strtok(NULL," ");
                cut3=strtok(NULL,"\0");
                printf("%s\n%s\n%s\n%s",cut,cut1,cut2,cut3);
                if(!strcmp(cut1,"/w")){   
                   for(i=0;i<MAX_CLIENT;i++){
                      if(!strcmp(list_c[i].nick,cut2)){
                         sprintf(chatData2,"%s님의 귓속말-> %s",cut,cut3);
                         write(list_c[i].c_socket, chatData2, sizeof(chatData2));
                         break;
                      }
                   }
                }
                else{
                 for (i = 0; i < MAX_CLIENT; i++) {
                         if (list_c[i].c_socket != INVALID_SOCK) {
                                 write(list_c[i].c_socket, chatData, n);
               }
            }
         }

                        if(strstr(chatData, escape) != NULL) {
                            popClient(c_socket);
            break;
                        }
               }
        }
}

int
pushClient(int c_socket,char *nickname) {
   int   i;
   for (i = 0; i < MAX_CLIENT; i++) {
      pthread_mutex_lock(&mutex);
      if(list_c[i].c_socket == INVALID_SOCK) {
         list_c[i].c_socket = c_socket;
         sprintf(list_c[i].nick,nickname);
         pthread_mutex_unlock(&mutex);
         return i;
      }   
      pthread_mutex_unlock(&mutex);
   }

   if (i == MAX_CLIENT)
      return -1;
}

int
popClient(int s)
{
        int     i;

        close(s);

        for (i = 0; i < MAX_CLIENT; i++) {
      pthread_mutex_lock(&mutex);
                if ( s == list_c[i].c_socket ) {
         list_c[i].c_socket = INVALID_SOCK;
         pthread_mutex_unlock(&mutex);
         break;
      }
      pthread_mutex_unlock(&mutex);
        }

        return 0;
}
