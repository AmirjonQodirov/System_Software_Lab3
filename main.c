#include "client/client.h"
#include "server/server.h"

int main(int argc, char *argv[]) {
    if(argc < 2){
        printf("\nWELCOME!\n");
        printf("\033[0;33m");
        printf("'--server' start server mode\n");
        printf("'--client username server_address' start client mode");
        printf("\033[0m\n");
        return 0;
    }
    if(strcmp(argv[1], "--server") == 0){
        return startServerMode();
    }
    if(strcmp(argv[1], "--client") == 0){
        if(argc < 3){
            printf("\033[0;31m");
            printf("Username is missing");
            printf("\033[0m");
            return 0;
        }
        if(argc < 4){
            printf("\033[0;31m");
            printf("Server address is missing");
            printf("\033[0m");
            return 0;
        }
        return startClientMode(argv[2], argv[3]);
    }
}


