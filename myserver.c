#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>

#define MAX_CLIENT 10
#define CHATDATA 1024
#define INVALID_SOCK -1
#define PORT 9000

struct user_controll
{
	int user_code;
	char user_name[CHATDATA];
};

struct user_controll uc[MAX_CLIENT];


void* do_chat(void *);

pthread_t thread;
pthread_mutex_t mutex;

int list_c[MAX_CLIENT];

char escape[] = "exit";
char greeting[] = "Welcome to chatting room\n";
char CODE200[] = "Sorry No More Connection\n";
char username[CHATDATA];


main(int argc, char *argv[])
{
	int c_socket, s_socket;
	struct sockaddr_in s_addr, c_addr;
	int len;
	int nfds = 0;
	int i, j, n;
	fd_set read_fds;
	int res;

	int count;
	for(count=0; count<MAX_CLIENT; count++)
	{
		uc[count].user_code = INVALID_SOCK;
	}


	if(pthread_mutex_init(&mutex, NULL) !=0)
	{
		printf("Can not create mutex\n");
		return -1;
	}

	s_socket = socket(PF_INET, SOCK_STREAM, 0);
	memset(&s_addr, 0, sizeof(s_addr));
	s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(PORT);


	if( bind(s_socket, (struct sockaddr *)&s_addr, sizeof(s_addr)) == -1)
	{
		printf("Can not Bind\n");
		return -1;
	}


	if(listen(s_socket, MAX_CLIENT) == -1)	
	{
		printf("listen Fail\n");
		return -1;
	}

	for(i=0; i<MAX_CLIENT; i++)
	{
		list_c[i] = INVALID_SOCK;
	}

	while(1)
	{
		len = sizeof(c_addr);
		c_socket = accept(s_socket, (struct sockaddr *)&c_addr, &len);
		memset(username,0,sizeof(username));
		read(c_socket,username,sizeof(username));
		printf("%s님이 입장하셨습니다.\n",username);
		res = pushClient(c_socket, username);
		
		


        if(res < 0) { //MAX_CLIENT만큼 이미 클라이언트가 접속해 있다면,
            write(c_socket, CODE200, strlen(CODE200));
            close(c_socket);
        } else {
            write(c_socket, greeting, strlen(greeting)); //c_socket에 환영인사를 보낸다.
           		 //채팅관련 쓰레드 생성 - 서버
			pthread_create(&thread,NULL,do_chat,(void *)&c_socket);
        }
			

		
	}
}




void *do_chat(void *arg)
{
	int c_socket = *((int *) arg);
	char chatData[CHATDATA];
	int i, n;
	int u_count;

	char *ptr = NULL;
	char *ear = NULL;
	char *cutname = NULL;
	char *talk = NULL;
	char copychat[CHATDATA];
	char copyname[CHATDATA];
	char earchatgo[CHATDATA];

	
	while(1)
	{
		memset(chatData, 0, sizeof(chatData));
		if( (n=read(c_socket, chatData, sizeof(chatData)))>0 )
		{
			strcpy(copychat, chatData);
			ptr = strtok(copychat," "); //my name
			ear = strtok(NULL, " "); // "/w"

			if(strncasecmp(ear, "/w", 2)==0)
			{
				cutname = strtok(NULL, " ");
				talk = strtok(NULL, "\0");
				printf("귓속말 상대 %s, 할 말 : %s", cutname, talk);
				strcpy(copyname, cutname);
				for(u_count=0; u_count<MAX_CLIENT; u_count++)
				{
					if(strncasecmp(uc[u_count].user_name, copyname, strlen(copyname))==0)
					{
						printf("귓속말 전송 완료");
						sprintf(earchatgo, "[귓속말(%s -> %s)] : %s \n", ptr,cutname, talk);
						write(uc[u_count].user_code, earchatgo, strlen(earchatgo));
					}
				}
			}

			else
			{
				for(i=0; i<MAX_CLIENT; i++)
				{
					if(uc[i].user_code != INVALID_SOCK)
					{
						write(uc[i].user_code, chatData, n);
					}
				}
			}






			if( strstr(chatData, escape) != NULL)	
			{
				popClient(c_socket);
				break;
			}

		}
	}
}






int pushClient(int c_socket, char *name)
{
	int i;
	
	for(i=0; i<MAX_CLIENT; i++)
	{
		
		if(uc[i].user_code == INVALID_SOCK)
		{
			uc[i].user_code = c_socket;
			strcpy(uc[i].user_name, name);
			return i;
		}

	}


	if(i==MAX_CLIENT)
		return -1;
}




int popClient(int s)
{
	int i;
	close(s);

	for(i=0; i<MAX_CLIENT; i++)
	{
		pthread_mutex_lock(&mutex);
		if(s==list_c[i])
		{
			list_c[i] = INVALID_SOCK;
			pthread_mutex_unlock(&mutex);
			break;
		}
		pthread_mutex_unlock(&mutex);
	}

	return 0;
}


