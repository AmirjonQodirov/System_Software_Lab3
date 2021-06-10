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

void * serverWriterThread(void * voidParams) {
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

void * serverReaderThread(void * voidParams) {
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
  printf("Server accepted the client. Username is %s. Now %i connections\n", user, serverNumConnections);
  
  pthread_t writer;
  struct WriterThreadData data;
  data.fd = sockfd;
  data.endFlag = 0;
  strcpy(data.user, user);
  pthread_create( & writer, NULL, serverWriterThread, & data);
  pthread_mutex_unlock( & serverBufferLock);

  while (recv(sockfd, & pakage, sizeof(ToDoPakage), 0) > 0) {
    printf("From client:\n\t Title : %s", pakage.toDo.title);
    printf("\t Description: %s", pakage.toDo.description);
    struct tm tm = * localtime( & pakage.toDo.creation_time);
    printf("\t Created: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
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
  printf("User %s disconnected. Now %i connections\n", user, serverNumConnections);
  pthread_cancel(writer);
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
    printf("socket creation failed...\n");
    return NULL;
  } else
    printf("Socket successfully created..\n");
  bzero( &servaddr, sizeof(servaddr));

  // assign IP, PORT
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(PORT);

  // Binding newly created socket to given IP and verification
  if ((bind(sockfd, (SA* ) & servaddr, sizeof(servaddr))) != 0) {
    printf("socket bind failed...\n");
    return NULL;
  } else
    printf("Socket successfully binded..\n");

  // Now server is ready to listen and verification
  if ((listen(sockfd, 5)) != 0) {
    printf("Listen failed...\n");
    return NULL;
  } else
    printf("Server listening..\n");
  len = sizeof(cli);

  // Accept the data packet from client and verification
  while (!endFlag) {
    connfd = accept(sockfd, (struct sockaddr * ) & cli, & len);
    if (connfd < 0) {
      printf("server acccept failed...\n");
      return NULL;
    } else {
      pthread_t clientReader;
      pthread_create( & clientReader, NULL, serverReaderThread, (void * ) & connfd);
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