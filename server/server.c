#include "server.h"

int endFlag = 0;
int serverNumConnections = 0;
atomic_int serverNumMessages = 0;
atomic_long serverToDoIds = 0;

pthread_mutex_t serverSocketLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t serverBufferLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t serverWriterCV = PTHREAD_COND_INITIALIZER;
pthread_cond_t serverReaderCV = PTHREAD_COND_INITIALIZER;

ToDoPakage serverBuffer;
ListElement serverListRoot;

struct WriterThreadData {
    int fd;
    char user[16];
    int endFlag;
};

void * clientSender(void * voidParams) {
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    struct WriterThreadData * threadData = (struct WriterThreadData * ) voidParams;

    while (!endFlag) {
        pthread_cond_wait( & serverWriterCV, & lock);
        if (strcmp(threadData -> user, serverBuffer.toDo.user) == 0 && !threadData -> endFlag) {
            pthread_mutex_lock( & serverSocketLock);
            write(threadData -> fd, & serverBuffer, sizeof(ToDoPakage));
            pthread_mutex_unlock( & serverSocketLock);
        }
        serverNumMessages++;
        if (serverNumMessages == serverNumConnections) {
            pthread_cond_signal( & serverReaderCV);
        }
    }
    return NULL;
}

void * serverProcessingThread(void * voidParams) {
    int sockfd = * (int * ) voidParams;
    ToDoPakage pakage;
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    //read toDo request from client
    read(sockfd, & pakage, sizeof(ToDoPakage));
    char user[16];
    strcpy(user, pakage.toDo.user);

    pthread_mutex_lock( & serverBufferLock);

    //send all user's toDos
    ListElement * currentItem = serverListRoot.next;
    while (currentItem != NULL) {
        if (strcmp(currentItem -> toDo.user, user) == 0) {
            pakage.toDo = currentItem -> toDo;
            pakage.mode = ADD_MODE;
            pthread_mutex_lock( & serverSocketLock);
            write(sockfd, & pakage, sizeof(ToDoPakage));
            pthread_mutex_unlock( & serverSocketLock);
        }
        currentItem = currentItem -> next;
    }

    serverNumConnections++;
    printf("\033[0;32m");
    printf("Server accepted the client. Username is %s. Now %i connections\n", user, serverNumConnections);
    printf("\033[0m\n");

    pthread_t clientSendr;
    struct WriterThreadData data;
    data.fd = sockfd;
    data.endFlag = 0;
    strcpy(data.user, user);
    pthread_create( & clientSendr, NULL, clientSender, & data);
    pthread_mutex_unlock( & serverBufferLock);

    while (recv(sockfd, & pakage, sizeof(ToDoPakage), 0) > 0) {
        printf("From client: %s --- processing pakage with Todo's title: %s\n", user, pakage.toDo.title);
        strcpy(pakage.toDo.user, user);
        if (pakage.mode == ADD_MODE) {
            pakage.toDo.creation_time = time(NULL);
            pakage.toDo.id = serverToDoIds++;
        }
        pthread_mutex_lock( & serverBufferLock);
        ProcessingPakage(pakage, & serverListRoot);
        serverNumMessages = 0;
        serverBuffer = pakage;
        pthread_cond_broadcast( & serverWriterCV);
        pthread_cond_wait( & serverReaderCV, & lock);
        pthread_mutex_unlock( & serverBufferLock);
    }

    data.endFlag = 1;
    pthread_mutex_lock( & serverBufferLock);
    serverNumConnections--;
    printf("\033[0;31m");
    printf("User %s disconnected. Now %i connections\n", user, serverNumConnections);
    printf("\033[0m\n");
    pthread_cancel(clientSendr);
    pthread_mutex_unlock( & serverBufferLock);
    return NULL;
}

void * mainThread(void * voidParams) {
    int sockfd;
    int connfd;
    int len;
    struct sockaddr_in servaddr;
    struct sockaddr_in cli;

    serverListRoot.next = NULL;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("\033[0;31m");
        printf("socket creation failed...\n");
        return NULL;
    } else {
        printf("\033[0;32m");
        printf("Socket successfully created..\n");
    }
    printf("\033[0m");
    bzero( & servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA * ) & servaddr, sizeof(servaddr))) != 0) {
        printf("\033[0;31m");
        printf("socket bind failed...\n");
        return NULL;
    } else {
        printf("\033[0;32m");
        printf("Socket successfully binded..\n");
    }
    printf("\033[0m");
    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        printf("\033[0;31m");
        printf("Listen failed...\n");
        return NULL;
    } else {
        printf("\033[0;32m");
        printf("Server listening..\n");
    }
    printf("\033[0m");
    len = sizeof(cli);

    // Accept the data packet from client and verification
    while (!endFlag) {
        connfd = accept(sockfd, (struct sockaddr * ) & cli, & len);
        if (connfd < 0) {
            printf("\033[0;31m");
            printf("server acccept failed...\n");
            printf("\033[0m");
            return NULL;
        } else {
            pthread_t clientThread;
            pthread_create( & clientThread, NULL, serverProcessingThread, (void * ) & connfd);
        }
    }

    // After all this sh*t close the socket
    close(sockfd);
    return NULL;
}

int startServerMode() {
    pthread_t mainListener;
    pthread_create( & mainListener, NULL, mainThread, NULL);
    while (!endFlag) {
        char c = getchar();
        if (c == 'Q' || c == 'q') {
            endFlag = 1;
        }
    }
    return 0;
}