#include <stdio.h>
#include "server.h"
#include "client.h"
#include "string.h"

int main(int argc, char * argv[]) {
    if (strcmp(argv[1],"server") == 0){
        server(argc-1, argv+1);
    } else {
        client(argc-1, argv+1);
    }
    return 0;
}