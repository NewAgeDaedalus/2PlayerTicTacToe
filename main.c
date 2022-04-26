#include <stdio.h>
#include <sys/socket.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#define PRAZNO 0
#define KRIZIC 1
#define KRUZIC 2 

int grid[3][3];
int whosplayerTurn= 1;

int checkWon(int znak){
        //check verticals and horizontals
        for (int i = 0; i < 2; i++){
                if( grid[i][0] == grid[i][1] && grid[i][1] ==grid[i][2]  && grid[i][2] == znak)
                        return 1;
                if (grid[0][i] == grid[1][i] && grid[1][i] == grid[2][i] && grid[2][i] == znak)
                        return 1;
        } 
        if (grid[0][0] == grid[1][1] && grid[1][1] == grid[2][2] && grid[2][2] ==  znak)
                return 1;
        if (grid[0][2] == grid[1][1] && grid[1][1] == grid[2][0] && grid[2][0] ==  znak)
                return 1;
        return 0;
}

ssize_t readn(int sock, void *ptr, size_t n){
        size_t nleft = n;
        ssize_t nread; 
        char *msg = (char*) ptr;
        while ( nleft > 0){
                if((nread = read(sock, msg, nleft)) < 0){
                        if (errno == EINTR)
                                nread = 0;
                        else
                                return -1;
                }else if(nread == 0)
                        break;
                nleft -=nread;
                msg += nread;
        }
        return n -nleft;
}

ssize_t writen(int sock, void *ptr, size_t n){
        size_t nleft = n;
        ssize_t nwritten = 0;
        char *msg = (char*)ptr;
        while (nleft > 0){
                if ((nwritten = write(sock, msg, nleft)) <= 0){
                        if(errno == EINTR && nwritten < 0)
                                nwritten = 0;
                        else{
                                err(1,"write error");
                                return -1;
                        }
                }
                nleft -=nwritten;
                ptr += nwritten;
        }
        return n;
}

void printGrid(){
        for (int i = 0; i < 3; i++){
                for (int j = 0; j < 3; j++){
                        switch (grid[i][j]){
                                case PRAZNO:
                                        printf("     ");
                                        break;
                                case KRIZIC:
                                        printf("  X  ");
                                        break;
                                case KRUZIC:
                                        printf("  O  ");
                                        break;
                        } 
                        if (j != 2)
                                printf("|");
                }
                printf("\n");
                if(i != 2)
                        printf("----------------\n");
        }
}

void playGame(int peerSock, int znak, int playerNumber){
        int gameOn = 1;
        char played[2];
        int protZnak;
        if (znak == KRIZIC)
                protZnak = KRUZIC;
        else
                protZnak = KRIZIC;
        while (gameOn){
                printGrid();
                if (playerNumber == whosplayerTurn){
                        printf("Your move:");
                        int x,y; 
                        scanf("%d%d",&x, &y);
                        while (grid[x][y] != PRAZNO){
                               printf("Taken");
                               scanf("%d%d",&x, &y);
                        }
                        grid[x][y] = znak;
                        played[0] =(char) (x+'0') ;
                        played[1] = (char) (y+'0');
                        writen(peerSock,played, 2);
                } else{
                        int x, y;
                        readn(peerSock, played,2);
                        x = (int) (played[0] -'0'); 
                        y = (int) (played[1] - '0');
                        grid[x][y] = protZnak;
                }
                whosplayerTurn = whosplayerTurn%2 +1;
                if (checkWon(znak)){
                       printf("Pobijedio si");
                       break;
                }
                if(checkWon(protZnak)){
                        printf("Izgubio si");
                        break;
                }
        }
}

int main(int argc , char *argv[]){
        int c, host, con;
        char address[100]; 
        strcpy(address,"127.0.0.1");
        int port;
        while ((c=getopt(argc,argv, "h:c:")) != -1){
                switch(c){
                        case 'h':
                                port = atoi(optarg);
                                host = 1;
                                con = 0;
                                break;
                        case 'c':
                                con = 1;
                                host = 0;
                                break;
                }
        }
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        int peerSock; 
        if (sock == -1)
                err(1,"Setup host sock error\n");
        struct sockaddr_in hostAddress;
        memset(&hostAddress,0, sizeof(hostAddress));
        if (con){
                struct sockaddr_in peerAddr;
                hostAddress.sin_port = 0;
                hostAddress.sin_family = AF_INET;
                hostAddress.sin_addr.s_addr = INADDR_ANY;
                memset(&peerAddr, 0, sizeof(peerAddr));
                strcpy(address, argv[2]);
                port = atoi(argv[3]);
                peerAddr.sin_family = AF_INET;
                peerAddr.sin_addr.s_addr = inet_addr(address);
                peerAddr.sin_port = port;
                socklen_t sin_len;
                if (bind(sock,(struct sockaddr *) &hostAddress,sizeof(hostAddress))==-1)
                        err(1, "Bind error");
                sin_len = sizeof(struct sockaddr);
                if (connect(sock,(struct sockaddr*)&peerAddr, sin_len) == -1)
                        err(1,"Failed to connect");
                printf("Connected\n");
                playGame(sock, KRUZIC, 2);
        }else{
                struct sockaddr peerAddr;
                memset(&peerAddr, 0, sizeof(peerAddr));
                hostAddress.sin_family = AF_INET;
                hostAddress.sin_port = port;
                socklen_t sin_len;
                if (bind(sock,(struct sockaddr *) &hostAddress,sizeof(hostAddress))==-1)
                        err(1, "Bind error");
                if (listen(sock,1) == -1)
                        err(1, "Listen error");
                sin_len = sizeof(peerAddr);
                peerSock = accept(sock,&peerAddr, &sin_len); 
                printf("%d\n", peerSock);
                printf("Connected\n");
                //printf("%d",(*(struct sockaddr_in *)&peerAddr).sin_port);
                //Start playing the game
                playGame(peerSock, KRIZIC, 1);
        }
        close(sock);
        return 0;
}
