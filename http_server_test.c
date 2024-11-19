#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static volatile int end_program = false;

void intHandler(int sig) {
    if(sig == SIGINT){
        end_program = true;
    }
}

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN);

    //struct sigaction act;
    //act.sa_handler = intHandler;
    //sigaction(SIGINT, &act, NULL);

    socklen_t peer_address_size;
    struct sockaddr_in my_addr, peer_addr;

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock < 0){
        fprintf(stderr, "Could not create a socket: %s\n", strerror(errno));
        exit(1);
    }

    int option = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(8080);
    my_addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    if(bind(sock, (struct sockaddr *) &my_addr, sizeof(my_addr)) == -1) {
        fprintf(stderr, "Could not bind: %s\n", strerror(errno));
        exit(1);
    }

    if(listen(sock, 128) == -1){
        fprintf(stderr, "Could not listen: %s\n", strerror(errno));
        exit(1);
    }

    peer_address_size = sizeof(peer_addr);
    char buf[512] = {0};

    for(;;){
        int peer_sock = accept(sock, (struct sockaddr *) &peer_addr, &peer_address_size);

        if(peer_sock == -1){
            fprintf(stderr, "Could not accept: %s\n", strerror(errno));
            exit(1);
        }

        printf("Somebody is calling: %s\n", inet_ntoa(peer_addr.sin_addr));
        memset(buf, 0, sizeof(buf));
        read(peer_sock, buf, sizeof(buf));
        printf("Request: %s\n", buf);
        /*
        char response[] = "\
HTTP/1.0 200\r\n\
Content-type: text/plain;charset=utf8\r\n\
Content-length: 19\r\n\
Connection: close\r\n\r\n\
Thanks for calling!";*/
        char content[] = "\
<!DOCTYPE html>\
<html>\
<h1>Hello from C</h1>\
</html>";
        char response[] = "\
HTTP/1.0 200\r\n\
Content-type: text/html;charset=utf8\r\n\
Content-length: %d\r\n\
Connection: close\r\n\r\n";

        size_t written = dprintf(peer_sock, response, strlen(content));
        written = write(peer_sock, content, strlen(content));
        if(written < 0) {
            fprintf(stderr, "Could not write: %s\n", strerror(errno));
        }
        close(peer_sock);

        if(end_program){
            printf("Will close socket...\n");
            if(close(sock) == -1){
                fprintf(stderr, "Could not close: %s\n", strerror(errno));
                exit(1);
            }
            printf("Goodbye!\n");
            exit(0);
        }
    }
}
