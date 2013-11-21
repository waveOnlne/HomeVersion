#ifndef THREADFUNCTION_H
#define THREADFUNCTION_H
#include "netPrograming.h"
#include <signal.h>
#include<stdio.h>
#include"picture.h"
#include<time.h>
#include<dirent.h>
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

#define PORT_SERVER 8888
#define ADDR_SERVER "192.168.1.122"
#define PORT_DEV1   "7171"
#define ADDR_DEV1   "192.168.1.230"
#define PORT_DEV2   "7070"
#define ADDR_DEV2   "192.168.1.88"

struct client_t{
        char message[4];
        unsigned char x;
        unsigned char y;
        unsigned char fps;
        unsigned char updobright;
        unsigned char updocontrast;
        unsigned char updocolors;
        unsigned char updoexposure;
        unsigned char updosize;
        unsigned char sleepon;
        } __attribute__ ((packed));

struct frame_t{
    char header[5];
    int nbframe;
    double seqtimes;
    int deltatimes;
    int w;
    int h;
    int size;
    int format;
    unsigned short bright;
    unsigned short contrast;
    unsigned short colors;
    unsigned short exposure;
    unsigned char wakeup;
    int acknowledge;
    } __attribute__ ((packed));

void thread_function(tp_work *thisThread, tp_work_desc *job);

#endif // THREADFUNCTION_H
