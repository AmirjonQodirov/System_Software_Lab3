#include <ncurses.h>
#include "client.h"

int endFlagClient = 0;
int clientSocketFD;

//GUI variables
WINDOW *windows[4];
char* titles[4] = {"Lists", "ToDos", "Details", "Edit"};
int currentWindow = 0;

//list window
int currentListNumber = 0;
int totalLists = 0;
ToDo currentList;

//todo window
int currentToDoNumber = 0;
int totalToDos = 0;
ToDo currentToDo;

//details window
int currentDetailNumber = 0;

//edit window
char editBuffer[256];


//Data
ListElement toDoRoot;
ListElement listRoot;

//Functions
void refreshEditWindow();
void refreshListWindow();
void refreshToDoWindow();
void refreshDetailsWindow();

void refreshList(){
    ListElement *currentItem = &listRoot;
    ListElement *currentToDoItem = &toDoRoot;
    while (currentItem->next != NULL){
        ListElement *item = currentItem->next;
        currentItem->next = currentItem->next->next;
        free(item);
    }
    while (currentToDoItem != NULL){
        currentItem = listRoot.next;
        int flag = 0;
        while(currentItem != NULL){
            if(strcmp(currentItem->toDo.list, currentToDoItem->toDo.list) == 0){
                flag = 1;
                break;
            }
            currentItem = currentItem->next;
        }
        if(!flag){
            addToDo(currentToDoItem->toDo, &listRoot);
        }
        currentToDoItem = currentToDoItem->next;
    }
}

void* clientReaderThread(){
    ToDoPakage toDoPackage;
    while (!endFlagClient){
        read(clientSocketFD, &toDoPackage, sizeof(ToDoPakage));
        ProcessingPakage(toDoPackage, &toDoRoot);
        if(toDoPackage.mode == DELETE_LIST_MODE || toDoPackage.mode == DELETE_MODE){
            currentToDo.id = -1;
            currentToDoNumber = 0;
            currentListNumber = 0;
        }
        refreshListList();
        refreshListWindow();
    }
    return NULL;
}

void setUpSocket(char* username, char* host){
    struct sockaddr_in servaddr;

    clientSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocketFD == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(host);
    servaddr.sin_port = htons(PORT);

    // connect the client socket to server socket
    if (connect(clientSocketFD, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }

    pthread_t receiver;
    pthread_create(&receiver, NULL, clientReaderThread, NULL);
    
    ToDoPakage toDoPackage;

    //handshake
    strcpy(toDoPackage.toDo.user, username);
    toDoPackage.mode = CONNECT_MODE;
    write(clientSocketFD, &toDoPackage, sizeof(ToDoPakage));
}

void sendRequest(int mode, ToDo toDo){
    ToDoPakage toDoPackage;
    toDoPackage.mode = mode;
    toDoPackage.toDo = toDo;
    write(clientSocketFD, &toDoPackage, sizeof(ToDoPakage));
}

void refreshListWindow(){
    wclear(windows[0]);
    currentWindow == 0 ? box(windows[0], '|', '-') : box(windows[0], 0, 0);
    wprintw(windows[0], titles[0]);
    int counter = 0;
    ListElement* item = listRoot.next;
    while (item != NULL){
        if(counter == currentListNumber){
            mvwprintw(windows[0], counter+2, 1, ">");
            currentList = item->toDo;
        }
        mvwprintw(windows[0], counter+2, 2, item->toDo.list);
        item = item->next;
        counter++;
    }
    totalLists = counter - 1;
    wrefresh(windows[0]);
    refreshToDoWindow();
}

void refreshToDoWindow(){
    wclear(windows[1]);
    currentWindow == 1 ? box(windows[1], '|', '-') : box(windows[1], 0, 0);
    wprintw(windows[1], titles[1]);
    int counter = 0;
    currentToDo.id = -1;
    ListElement* item = toDoRoot.next;
    while (item != NULL){
        if(strcmp(item->toDo.list, currentList.list) == 0){
            if(counter == currentToDoNumber){
                mvwprintw(windows[1], counter+2, 1, ">");
                currentToDo = item->toDo;
            }
            mvwprintw(windows[1], counter+2, 2, item->toDo.title);
            counter++;
        }
        item = item->next;
    }
    totalToDos = counter;
    wrefresh(windows[1]);
    refreshDetailsWindow();
}

void refreshDetailsWindow(){
    wclear(windows[2]);
    currentWindow == 2 ? box(windows[2], '|', '-') : box(windows[2], 0, 0);
    wprintw(windows[2], titles[2]);
    struct tm tm;
    char buff[32];
    if(currentToDo.id != -1){
        mvwprintw(windows[2], 2, 2, "Title:");
        mvwprintw(windows[2], 2, 15, currentToDo.title);

        mvwprintw(windows[2], 3, 2, "ToDo list:");
        mvwprintw(windows[2], 3, 15, currentToDo.list);

        mvwprintw(windows[2], 4, 2, "Description:");
        mvwprintw(windows[2], 4, 15, currentToDo.description);

        tm = *localtime(&currentToDo.creation_time);
        sprintf(buff, "Created:     %d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        mvwprintw(windows[2], 5, 2, buff);

        if(currentToDo.deadline_in_day != 0){
            sprintf(buff, "Deadline:    %d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday+currentToDo.deadline_in_day, tm.tm_hour, tm.tm_min, tm.tm_sec);
            mvwprintw(windows[2], 6, 2, buff);
        } else{
            mvwprintw(windows[2], 6, 2, "Deadline:    -");
        }

        if(currentDetailNumber==3){
            mvwprintw(windows[2], currentDetailNumber + 3, 1, ">");
        }else{
            mvwprintw(windows[2], currentDetailNumber + 2, 1, ">");
        }

        if(currentDetailNumber == 0){
            strcpy(editBuffer, currentToDo.title);
        }
        if(currentDetailNumber == 1){
            strcpy(editBuffer, currentToDo.list);
        }
        if(currentDetailNumber == 2){
            strcpy(editBuffer, currentToDo.description);
        }
        if(currentDetailNumber == 3){
            sprintf(editBuffer, "%d", currentToDo.deadline_in_day);
        }
    }
    wrefresh(windows[2]);
    refreshEditWindow();
}

void refreshEditWindow(){
    wclear(windows[3]);
    currentWindow == 3 ? box(windows[3], '*', '*') : box(windows[3], 0, 0);
    wprintw(windows[3], titles[3]);
    mvwprintw(windows[3], 2, 2, editBuffer);
    wrefresh(windows[3]);
}


int startClientMode(char* username, char* host){

    toDoRoot.next = NULL;
    listRoot.next = NULL;

    setUpSocket(username, host);

    const int width = 21;
    const int height = 7;

    initscr();
    noecho();
    raw();
    curs_set(0);
    refresh();

    windows[0] = newwin(LINES - height, width, 0, 0);
    windows[1] = newwin(LINES - height, width, 0, width);
    windows[2] = newwin(LINES - height, COLS - width * 2, 0, width * 2);
    windows[3] = newwin(height, COLS, LINES - height, 0);
    refreshListWindow();

    while (!endFlagClient){
        char c = getch();
        if(c == 27){
            endFlagClient = 1;
        }
        else if(c == 127 && currentWindow == 3){
            editBuffer[strlen(editBuffer)-1] = '\0';
            refreshEditWindow();
        }
        else if(c == 10){
            if (currentWindow == 2){
                if(currentToDo.id >= 0){
                    sendRequest(CHANGE_MODE, currentToDo);
                } else{
                    sendRequest(ADD_MODE, currentToDo);
                }
            }
            if (currentWindow == 3){
                if(currentDetailNumber == 0){
                    strcpy(currentToDo.title, editBuffer);
                }
                if(currentDetailNumber == 1){
                    strcpy(currentToDo.list, editBuffer);
                }
                if(currentDetailNumber == 2){
                    strcpy(currentToDo.description, editBuffer);
                }
                if(currentDetailNumber == 3){
                    currentToDo.deadline_in_day = atoi(editBuffer);
                }
                currentWindow = 2;
                refreshDetailsWindow();
            }
        }
        else if(/*tab*/c == 9){
            currentWindow = (currentWindow + 1) % (currentToDo.id == -1 ? 2 : 4);
            if(currentWindow == 3){
                refreshDetailsWindow();
            } else{
                refreshListWindow();
            }
        }
        else if (currentWindow == 3){
            strncat(editBuffer, &c, 1);
            refreshEditWindow();
        }
        else if(c == 'w' || c == 'W'){
            if(currentWindow == 0){
                currentListNumber = currentListNumber == 0 ? totalLists - 1: currentListNumber - 1;
                refreshListWindow();
            }
            if(currentWindow == 1){
                currentToDoNumber = currentToDoNumber == 0 ? totalToDos - 1: currentToDoNumber - 1;
                refreshToDoWindow();
            }
            if(currentWindow == 2){
                currentDetailNumber = currentDetailNumber == 0 ? 3 : currentDetailNumber - 1;
                refreshDetailsWindow();
            }
        }
        else if(c == 's' || c == 'S'){
            if(currentWindow == 0){
                currentListNumber = (currentListNumber + 1) % totalLists;
                refreshListWindow();
            }
            if(currentWindow == 1){
                currentToDoNumber = (currentToDoNumber + 1) % totalToDos;
                refreshToDoWindow();
            }
            if(currentWindow == 2){
                currentDetailNumber = (currentDetailNumber + 1) % 4;
                refreshDetailsWindow();
            }
        }
        else if(c == 'd' || c == 'D'){
            if(currentWindow == 0){
                sendRequest(DELETE_LIST_MODE, currentList);
            }
            if(currentWindow == 1){
                sendRequest(DELETE_MODE, currentToDo);
            }
        }
        else if((c == 'n' || c == 'N') && currentWindow == 1){
            currentToDo.id = -2;
            strcpy(currentToDo.title, "");
            strcpy(currentToDo.description, "");
            strcpy(currentToDo.list, "");
            currentToDo.creation_time = time(NULL);
            currentToDo.deadline_in_day = 0;
            currentWindow = 2;
            refreshDetailsWindow();
        }else{

        }
    }
    endwin();
    return 0;
}
