/***************************************************************************#
# The heartbeat message
#                                                                           #
#***************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/time.h>
#include<syslog.h>
#include<sys/wait.h>

#define MAXFILE 65535
#define DEV_DOOR            '1'
#define DEV_LIGHT           '2'

#define GET_LOG             1
#define GET_SYS             2
#define GET_PICTURE         3
#define SET_PICTURE         4
#define GET_VEDIO           5
#define OPEN_DOOR           6
#define TURN_ON_LIGHT       7
#define TURN_OFF_LIGHT      8
#define CMP_PASSWD          9
#define HEART_JMP           10
#define TURN_ON_CAMERA      11
#define CLIENT_LOGIN        12
#define CHANGE_CLIENT_PASSWD 13
#define CHANGE_PASSWD       14
#define VIDEO_PALETTE_JPEG  21
#define MAXSIZE 100

#define ADDR_LOCAL  "127.0.0.1"
#define PORT_SERVER 8888
#define ADDR_SERVER "192.168.1.122"
#define PORT_DEV1   7171
#define ADDR_DEV1   "192.168.1.230"
#define PORT_DEV2   7070
#define ADDR_DEV2   "192.168.1.88"

void heart_jmp();
void usr_func();

int main(int argc, char **argv)
{
    int i;
    pid_t pid,sid;

    if((pid = fork()) < 0){
        perror("fock");
        exit(EXIT_FAILURE);
    }else if(pid > 0){
        exit(EXIT_SUCCESS);
    }else{
        signal(SIGPIPE,SIG_IGN);
        openlog("demo_update",LOG_PID, LOG_DAEMON);
        if((sid=setsid())<0){
          syslog(LOG_ERR, "%s\n", "setsid");
          exit(EXIT_FAILURE);
        }
        if((sid=chdir("/"))<0){
          syslog(LOG_ERR, "%s\n", "chdir");
          exit(EXIT_FAILURE);
        }
        umask(0);
        for(i=0;i<MAXFILE;i++)
          close(i);
        if((pid = fork()) < 0){
            exit(EXIT_FAILURE);
        }else{
            while(1){
                if(pid > 0){
                    if((pid = fork()) < 0)
                        exit(EXIT_FAILURE);
                    else if(pid > 0){
                        waitpid(pid,NULL,0);
                        sleep(5);
                    }else if(pid == 0){
                        heart_jmp();
                        return;
                    }
                    continue;
                }else if(pid == 0){
                    if((pid = fork()) < 0)
                        exit(EXIT_FAILURE);
                    else if(pid > 0){
                        waitpid(pid,NULL,0);
                        sleep(5);
                    }else if(pid == 0){
                        usr_func();
                        return;
                    }

                }
            }
        }
    }
}
/**
  * heartbeat message function

  * para:
  * 	null

  * return:
  * 	null
  */
void heart_jmp()
{
    //////////dev types/////////////
    char msg = '2';
    ////////////////////////////////
    int sockfd,tmp;
    struct sockaddr_in serveraddr;
    memset(&serveraddr,0,sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(PORT_SERVER);
    inet_pton(AF_INET,ADDR_SERVER, &serveraddr.sin_addr);
    sockfd =socket(AF_INET,SOCK_STREAM,0);
    tmp = HEART_JMP;
    struct timeval tv_out;
    tv_out.tv_sec = 10;
    tv_out.tv_usec = 0;
    setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO,&tv_out,sizeof(tv_out));
    while(connect(sockfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr)) < 0);
    if(send(sockfd,&tmp,sizeof(int),0) < 0){
        syslog(LOG_ERR, "%s\n", "send1");
        close(sockfd);
    }
    while(1){
        sleep(5);
        if(send(sockfd,&msg,sizeof(msg),0) < 0)
        {
            syslog(LOG_ERR, "%s\n", "send2");
            close(sockfd);
            break;
        }
    }
    return;
}
/**
  * solve the request from server

  * para:
  * 	null

  * return:
  * 	null
  */
void usr_func()
{
    int sockfd;
    int msg = 0;
    int ret;
    struct sockaddr_in clientaddr;
    struct sockaddr_in serveraddr;
    memset(&clientaddr,0,sizeof(clientaddr));
    memset(&serveraddr,0,sizeof(serveraddr));
    clientaddr.sin_family = AF_INET;
    clientaddr.sin_port = htons(PORT_DEV1);
    inet_pton(AF_INET,ADDR_LOCAL, &clientaddr.sin_addr);
    sockfd =socket(AF_INET,SOCK_STREAM,0);
    socklen_t size = sizeof(serveraddr);
    if(bind(sockfd,(const struct sockaddr*)&clientaddr,sizeof(clientaddr)) < 0){
        syslog(LOG_ERR, "%s\n", "bind");
        close(sockfd);
    }
    listen(sockfd,10);
    while(1){
        if((ret = accept(sockfd,(struct sockaddr*)&serveraddr,&size)) < 0){
            syslog(LOG_ERR, "%s\n", "accept");
            close(sockfd);
            break;
        }
        if(recv(ret,(void*)&msg,sizeof(int),0) < 0){
            syslog(LOG_ERR, "%s\n", "recv");
            close(ret);
            break;
        }
        /////////////////////////////////
        //////////to do something///////////
        //////////////////////////////////
        msg = 1;
        if(send(ret,(const void*)&msg,sizeof(int),0) < 0){
            syslog(LOG_ERR, "%s\n", "send");
            close(ret);
            break;
        }
        else
            close(ret);
    }
    return;
}
