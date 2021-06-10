#pragma once

#include <time.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdatomic.h>

#define PORT 8080
#define SA struct sockaddr
#define ADD_MODE 0
#define CHANGE_MODE 1
#define DELETE_MODE 2
#define DELETE_LIST_MODE 3
#define CONNECT_MODE 5
typedef int RequestMode;

typedef struct{
    long id;
    char user[16];
    char title[32];
    char list[32];
    char description[256];
    time_t creation_time;
    int deadline_in_day;
}ToDo;

typedef struct ListElement{ 
    ToDo toDo;
    struct ListElement* next;
}ListElement;

typedef struct{ 
    RequestMode mode;
    ToDo toDo;
}ToDoPakage;

void addToDo(ToDo toDo, ListElement* f_element);
void changeToDo(ToDo toDo, ListElement* f_element);
void deleteToDo(ToDo toDo, ListElement* f_element);
void deleteToDoList(ToDo toDo, ListElement* f_element);
void ProcessingPakage(ToDoPakage toDoPackage, ListElement* f_element);
