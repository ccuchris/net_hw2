#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<signal.h>
#include<fcntl.h>
#include<errno.h>

int sock;
//int threadend=0;
//int data_flag=0;
int match_flag=0;
int nextturn=0;
char data[128];
char playername[128];
char username[128];

pthread_mutex_t data_lock=PTHREAD_MUTEX_INITIALIZER;    //創建互斥鎖
pthread_mutex_t match_mutex=PTHREAD_MUTEX_INITIALIZER;

void *sendsock(void *arg);
void *recvsock(void *arg);
void playing(int socket);

static void sig_handler(int sig)
{
	char command[128];
	memset(command, 0, sizeof(command));
	if(sig==SIGINT){
		strcpy(command, "quit");
		write(sock, command, sizeof(command));
		close(sock);
		exit(0);
	}
}

int main()
{
	int connectok;
	struct sockaddr_in serverAddress;
	const char *serverIP = "127.0.0.1";

	sock = socket(AF_INET, SOCK_STREAM, 0);		//建立一個socket,IPv4網路協定,TCP

	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(serverIP);
	serverAddress.sin_port = htons(8080);

	signal(SIGCHLD, sig_handler);	
	connectok = connect(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress));		//client透過sock向指定的server連線

	pthread_t recvsock_t, sendsock_t;
	pthread_create(&recvsock_t, NULL, recvsock, (void*)&sock);		//建立子執行緒
	pthread_create(&sendsock_t, NULL, sendsock, (void*)&sock);
	pthread_join(recvsock_t, NULL);		//等待子執行緒執行完成

	pthread_mutex_destroy(&data_lock);	//將data_lock摧毀

	close(sock);
	return 0;
}

void *recvsock(void *arg)
{
	int sock = *(int*)arg;
	char *temp;
	int n;
	while(1){
		pthread_mutex_lock(&data_lock);	//獲取互斥鎖
		n = read(sock, data, sizeof(data));
		if(strcmp(data, "quit")==0) break;
		else if(strncmp(data, "Invite", 6)==0){
			write(fileno(stdout), "\n", sizeof("\n"));
			write(fileno(stdout), data, sizeof(data));
			temp = strtok(data, " ");
			temp = strtok(NULL, " ");
			pthread_mutex_lock(&match_mutex);
			strcpy(playername, temp);
			match_flag=1;		
			pthread_mutex_unlock(&match_mutex);
			usleep(1000);
		}
		else if(strncmp(data, "Start", 5)==0){
			match_flag = 1;
			for(int i=0; i<40; i++) printf("*");
			printf("\n");
			temp = strtok(data, ";");
			printf("\t%s\n", temp);
			temp = strtok(NULL, ";");
			nextturn=0;
			if(strcmp(temp, username)==0) nextturn=1;
			temp = strtok(NULL, ";");
			printf("%s\n", temp);
		}
		else if(strncmp(data, "Reject", 6)==0){
			printf("%s\n\n", data);
		}
		else if(strcmp(data, "Win")==0){
			printf("\n\t***恭喜獲勝***\n\n");
		}
		else if(strcmp(data, "Lose")==0){
			printf("\n\t***你輸了，再接再厲***\n\n");
		}
		else if(strncmp(data, "Even", 4)==0){
			printf("\n\t***平手!!***\n\n");
		}
		else if(strncmp(data, "Leave", 5)==0){
			temp = strtok(data, ";");
			temp = strtok(NULL, ";");
			printf("%s\n", temp);
		}
		else if(strncmp(data, "username", 8)==0){
			temp = strtok(data, " ");
			temp = strtok(NULL, " ");
			memset(username, '\0', sizeof(temp));
			strcpy(username, temp);
		}
		else if(strncmp(data, "Busy", 4)==0){
			printf("%s\n", data);
		}
		else if(strcmp(data, "error")==0){
			printf("\n\t*** 沒有這位選手 !! 請再試一次 !!\n\n");
		}
		else if(strcmp(data, "Error")==0){
			printf("\n\t*** Incorrect input !! Please try again !!\n\n"); 
		}

		pthread_mutex_unlock(&data_lock);
		usleep(100000);
	}
}

void *sendsock(void *arg)
{
	int sock = *(int*)arg;
	char command[128];
	char response[128];
	fd_set readset;
	int flags;
	int n;
	int maxfdp;
	FILE *fp = stdin;
	pthread_detach(pthread_self());

	printf("\n----------------Welcome to the Tic-Tac-Toe Game-----------------\n");
	printf("menu : 顯示能使用的功能\n");
	printf("list : 顯示在線玩家\n");
	printf("match : 開始遊戲\n");
	printf("quit : 離開遊戲\n");
	printf("------------------------------------------------------------------\n");

	while(1){
		printf("## Command : ");
		scanf("%s", command);

		if(strcmp(command, "quit")==0){
			write(sock, command, sizeof(command));
			printf("\n\n\t離開遊戲\n");
			break;
		}
		else if(strcmp(command, "list")==0){
			write(sock, command, sizeof(command));
			pthread_mutex_lock(&data_lock);
			data[strlen(data)-1]='\0';
			printf("\n-------player(s) online--------\n");	
			printf("%s\n\n\n", data);
			pthread_mutex_unlock(&data_lock);
			usleep(1000);
		}
		else if(strcmp(command, "menu")==0){
			printf("\n\t***Here is the Game Menu***\n");
			printf("(menu) Display the command you can use\n");
			printf("(list) Display the players who are online\n");
			printf("(match) Create the new room\n");
			printf("(quit) Exit the game\n\n");
		}
		else if(strcmp(command, "match")==0){
			printf("Choose the one player you want to invite : ");
			scanf("%s", playername);
			strcat(command, " ");
			strcat(command, playername);
			write(sock, command, sizeof(command));
			printf("\t\n Wait !!\n");
			pthread_mutex_lock(&data_lock);
			pthread_mutex_unlock(&data_lock);
			usleep(1000);
			if(match_flag) playing(sock);
			match_flag = 0;
		}
		else if(strcmp(command, "Y")==0){
			int enterflag = 0;
			pthread_mutex_lock(&match_mutex);
			if(match_flag){
				memset(response, 0, sizeof(response));
				strcpy(response, "Accept ");
				strcat(response, playername);
				write(sock, response, sizeof(response));
				enterflag = 1;
			}
			else printf("\n\t*** Error: Cannot find the command, please try again! ***\n");
			pthread_mutex_unlock(&match_mutex);
			usleep(1000);
			
			if(enterflag) playing(sock);
			match_flag = 0;
		}
		else if(strcmp(command, "N")==0){
			pthread_mutex_lock(&match_mutex);
			if(match_flag){
				memset(response, 0, sizeof(response));	
				strcpy(response, "Reject ");
				strcat(response, playername);
				write(sock, response, sizeof(response));
			}
			else printf("\n\t*** Error: Cannot find the command, please try again! ***\n");
			match_flag = 0;
			pthread_mutex_unlock(&match_mutex);
			usleep(1000);
		}
		else{
			printf("\n\t*** Error: Cannot find the command, please try again! ***\n");
		}
	}
}

void playing(int socket)
{
	int next;
	char temp[2];
	char response[128];

	while(1){
		pthread_mutex_lock(&data_lock);
		if(strcmp(data, "quit")==0 || strcmp(data, "Win")==0 || strcmp(data, "Lose")==0 || strncmp(data, "Even", 4)==0 || strncmp(data, "Leave", 5)==0){
			pthread_mutex_unlock(&data_lock);
			break;
		}
		else if(nextturn){
			while(1){
				printf("You turn (-1 is quit) : ");
				scanf("%d", &next);
				if(next>=1 && next<=9 || next==-1) break;
				else printf("*** Incorrect input !! Please enter the correct number(from  1 to 9) !!\n\n");
			}
			if(next ==-1 || strcmp(data, "quit")==0){
				printf("\n\t*** You leave the game !! ***\n");
				memset(response, 0, sizeof(response));
				strcpy(response, "Leave");
				write(socket, response, sizeof(response));
				pthread_mutex_unlock(&data_lock);
				break;
			}
			memset(response, 0, sizeof(response));
			temp[1]='\0';
			temp[0]=next+48;
			strcpy(response, "Next;");
			strcat(response, temp);
			write(socket, response, sizeof(response));
		}
		pthread_mutex_unlock(&data_lock);
		usleep(1000);
	}
}