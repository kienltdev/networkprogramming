#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <sys/types.h>

#include "customSTD.h"

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048
#define NAME_LEN 32

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[NAME_LEN];
int player = 1;

pthread_t lobby_thread;
pthread_t recv_msg_thread;
pthread_t multiplayer_game;

char *ip = "172.26.16.108";
int port = 2050;

void catch_ctrl_c_and_exit()
{
    flag = 1;
}

void showPositions()
{
    printf("\nMap of Positions:");
    printf("\n 7 | 8 | 9");
    printf("\n 4 | 5 | 6");
    printf("\n 1 | 2 | 3\n");
}

void showBoard(char board[3][3], char *errorMessage)
{
    int row;

    flashScreen();

    if (*errorMessage != '\x00')
    {
        printf("Attention: %s!\n\n", errorMessage);
        *errorMessage = '\x00';
    }

    printf("#############\n");

    for (row = 0; row < 3; row++)
    {
        printf("# %c | %c | %c #", board[row][0], board[row][1], board[row][2]);

        printf("\n");
    }

    printf("#############\n");

    showPositions();
}

void createBoard(char board[3][3])
{
    int row, column;

    for (row = 0; row < 3; row++)
    {
        for (column = 0; column < 3; column++)
        {
            board[row][column] = '-';
        }
    }
}


void menu();

void *lobby(void *arg);

void recv_msg_handler();

void *multiplayerGame(void *arg)
{
    int playerTurn;
    playerTurn = player;
    char playerName1[32];
    char playerName2[32];

    strcpy(playerName1, name);

    char board[3][3];
    int iterator; // bien lap qua cac hang cot
    int playedRow, playedColumn;
    int playedPosition;
    int round = 0;
    int gameState = 1;
    int validPlay = 0;
    int played;
    int numberPlayed;
    int positions[9][2] = {{2, 0}, {2, 1}, {2, 2}, {1, 0}, {1, 1}, {1, 2}, {0, 0}, {0, 1}, {0, 2}}; // ánh xạ số nguyên 1-9 sang sang tọa độ hàng và cột bảng 3x3

    char errorMessage[255] = {'\x00'};
    char *currentPlayerName;
    char message[BUFFER_SZ] = {};

    int receive = recv(sockfd, message, BUFFER_SZ, 0);

    if (receive > 0) {
        setbuf(stdin, 0);
        trim_lf(message, strlen(message));
        sscanf(message, "%s", &playerName2[0]);

        setbuf(stdout, 0);
        setbuf(stdin, 0);

        bzero(message, BUFFER_SZ);

        createBoard(board);
        round = 0;

        while (round < 9 && gameState == 1)
        {
            if (playerTurn == 1)
            {
                currentPlayerName = (char *)&playerName1; //neu den luot cua nguoi choi 1 - currentPlayerName sẽ trỏ đến tên của người chơi 1 (player1Name).
            } 
            else
            {
                currentPlayerName = (char *)&playerName2;
            }

            showBoard(board, (char *)&errorMessage);

            printf("\nRound: %d", round);
            printf("\nPlayer: %s\n", currentPlayerName);

            while (validPlay == 0) // cho server gui turn 1 hay 2
            {
                bzero(message, BUFFER_SZ);

                int receive = recv(sockfd, message, BUFFER_SZ, 0);

                if (receive > 0) {
                    validPlay = 1;

                    setbuf(stdin, 0);
                    setbuf(stdout, 0);

                    if (strcmp(message, "turn1\n") == 0)
                    {
                        printf("Enter a position: ");
                        scanf("%d", &playedPosition);


                        playedRow = positions[playedPosition - 1][0];
                        playedColumn = positions[playedPosition - 1][1];

                        // gui nuoc di den server
                        if (validPlay == 1)
                        {
                            sprintf(message, "play %i\n", playedPosition);
                            send(sockfd, message, strlen(message), 0);
                            bzero(message, BUFFER_SZ);
                        }
                        
                    }
                    else if (strcmp(message, "turn2\n") == 0)
                    {
                        printf("The other player is playing...\n");

                        played = 0;

                        while (played == 0)
                        {
                            int receive = recv(sockfd, message, BUFFER_SZ, 0);

                            if (receive > 0) {
                                sscanf(message, "%i", &numberPlayed);

                                playedRow = positions[numberPlayed - 1][0];
                                playedColumn = positions[numberPlayed - 1][1];

                                played = 1;
                            }
                        }

                        validPlay = 1;
                    }
                }
                else
                {
                    validPlay = 0;
                }
            }

            if (playerTurn == 1)
            {
                board[playedRow][playedColumn] = 'X';
                playerTurn = 2;
            } 
            else
            {
                board[playedRow][playedColumn] = 'O';
                playerTurn = 1;
            }

            for (iterator = 0; iterator < 3; iterator++)
            {
                if (
                    (
                        (board[iterator][0] == board[iterator][1]) && (board[iterator][1] == board[iterator][2]) && board[iterator][0] != '-'
                    )
                        ||
                    (
                        (board[0][iterator] == board[1][iterator]) && (board[1][iterator] == board[2][iterator]) && board[0][iterator] != '-'
                    )
                )
                {
                    gameState = 0;
                }
            }

            if (
                (
                    (board[0][0] == board[1][1]) && (board[1][1] == board[2][2]) && board[0][0] != '-'
                )
                    ||
                (
                    (board[0][2] == board[1][1]) && (board[1][1] == board[2][0]) && board[0][2] != '-'
                )
            )
            {
                gameState = 0;
            }

            round++;
            validPlay = 0;
            bzero(message, BUFFER_SZ);
        }

        bzero(message, BUFFER_SZ);

        int receive = recv(sockfd, message, BUFFER_SZ, 0);

        if (receive > 0) {
            setbuf(stdin, 0);
            setbuf(stdout, 0);

            showBoard(board, (char *)&errorMessage);


            if (strcmp(message, "win1\n") == 0)
            {
                printf("\nPlayer '%s' won!", currentPlayerName);
            }
            else if (strcmp(message, "win2\n") == 0)
            {
                printf("\nPlayer '%s' won!", currentPlayerName);
            }

            printf("\nGame over!\n");

            sleep(6);

            if (pthread_create(&lobby_thread, NULL, &lobby, NULL) != 0) {
                printf("ERROR: pthread\n");
                return NULL;
            }

            if (pthread_create(&recv_msg_thread, NULL, (void*)recv_msg_handler, NULL) != 0) {
                printf("ERROR: pthread\n");
                return NULL;
            }

            pthread_detach(pthread_self());
            pthread_cancel(multiplayer_game);

        }
        
    }

    return NULL;
}

void *lobby(void *arg) // lap de doc du lieu tu ban phim 
{
    char buffer[BUFFER_SZ] = {};

    while(1)
    {
        str_overwrite_stdout();
        fgets(buffer, BUFFER_SZ, stdin);
        trim_lf(buffer, BUFFER_SZ);

        if (strcmp(buffer, "exit") == 0) {
            break;
        } else {
            send(sockfd, buffer, strlen(buffer), 0);
        }

        bzero(buffer, BUFFER_SZ);
    }

    catch_ctrl_c_and_exit(2);

    return NULL;
}

void recv_msg_handler()
{
    char message[BUFFER_SZ] = {};

    flashScreen();

    while(1)
    {
        int receive = recv(sockfd, message, BUFFER_SZ, 0);

        if (receive > 0) {
            if (strcmp(message, "ok") == 0)
            {
                printf("Command:\n");
                printf("\t -list\t\t\t  List all caro 3x3 rooms\n");
                printf("\t -create\t\t  Create one caro 3x3 room\n");
                printf("\t -join {room number}\t  Join in one caro 3x3 room\n");
                printf("\t -leave\t\t\t  Back of the one caro 3x3 room\n");
                printf("\t -start\t\t\t  Starts one caro 3x3 game\n\n");

                str_overwrite_stdout();
            }
            else if (strcmp(message, "start game\n") == 0)
            {
                pthread_cancel(lobby_thread);
                // pthread_kill(recv_msg_thread, SIGUSR1);   
                
                player = 1;
                if (pthread_create(&multiplayer_game, NULL, (void*)multiplayerGame, NULL) != 0) {
                    printf("ERROR: pthread\n");
                    exit(EXIT_FAILURE);
                }
                pthread_detach(pthread_self());
                pthread_cancel(recv_msg_thread);

                // pthread_kill(lobby_thread, SIGUSR1);   
            }
            else if (strcmp(message, "start game2\n") == 0)
            {
                pthread_cancel(lobby_thread);
                // pthread_kill(recv_msg_thread, SIGUSR1);   
                
                player = 2;
                if (pthread_create(&multiplayer_game, NULL, (void*)multiplayerGame, NULL) != 0) {
                    printf("ERROR: pthread\n");
                    exit(EXIT_FAILURE);
                }
                pthread_detach(pthread_self());
                pthread_cancel(recv_msg_thread);

                // pthread_kill(lobby_thread, SIGUSR1);   
            }
            else
            {
                printf("%s", message);
                str_overwrite_stdout();
            }
        } else if (receive == 0) {
            break;
        }

        bzero(message, BUFFER_SZ);
    }
}

void send_msg_handler()
{
    char buffer[BUFFER_SZ] = {};

    while(1)
    {
        str_overwrite_stdout();
        fgets(buffer, BUFFER_SZ, stdin);
        trim_lf(buffer, BUFFER_SZ);

        if (strcmp(buffer, "exit") == 0) {
            break;
        } else {
            send(sockfd, buffer, strlen(buffer), 0);
        }

        bzero(buffer, BUFFER_SZ);
    }

    catch_ctrl_c_and_exit(2);
}

int conectGame()
{
    setbuf(stdin, 0);

    printf("Enter your name: ");
    fgets(name, BUFFER_SZ, stdin);
    trim_lf(name, BUFFER_SZ);

    // strcpy(name, "murilo");

    if (strlen(name) > NAME_LEN - 1 || strlen(name) < 2) {
        printf("Enter name corretly\n");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    
    //socket settings
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    // connect to the server
    int err = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (err == -1) {
        printf("ERROR: connect\n");
        return EXIT_FAILURE;
    }

    // send the name
    send(sockfd, name, NAME_LEN, 0);

    if (pthread_create(&lobby_thread, NULL, &lobby, NULL) != 0) {
        printf("ERROR: pthread\n");
        return EXIT_FAILURE;
    }

    if (pthread_create(&recv_msg_thread, NULL, (void*)recv_msg_handler, NULL) != 0) {
        printf("ERROR: pthread\n");
        return EXIT_FAILURE;
    }

    while(1)
    {
        if (flag) {
            printf("\nBye\n");
            break;
        }
    }

    close(sockfd);

    return 0;
}

void game()
{
    char board[3][3];
    int iterator;
    int playedRow, playedColumn;
    int playedPosition;
    int playerTurn = 1;
    int round = 0;
    int gameState = 1;
    int restartOption;
    int gaming = 1;

    char errorMessage[255] = {'\x00'};

    char playerName1[32];
    char playerName2[32];
    char *currentPlayerName;

    setbuf(stdin, 0);

    printf("Name of the 1st player: ");
    fgets(playerName1, 32, stdin);

    playerName1[strlen(playerName1) - 1] = '\x00';

    printf("Name of the 2nd player: ");
    fgets(playerName2, 32, stdin);

    playerName2[strlen(playerName2) - 1] = '\x00';

    while (gaming == 1)
    {
        createBoard(board);
        round = 0;
        playerTurn = 1;

        while (round < 9 && gameState == 1)
        {
            showBoard(board, (char *)&errorMessage);

            if (playerTurn == 1)
            {
                currentPlayerName = (char *)&playerName1;
            } 
            else
            {
                currentPlayerName = (char *)&playerName2;
            }

            int posicoes[9][2] = {{2, 0}, {2, 1}, {2, 2}, {1, 0}, {1, 1}, {1, 2}, {0, 0}, {0, 1}, {0, 2}};

            printf("\nRound: %d", round);
            printf("\nPlayer: %s\n", currentPlayerName);
            printf("Enter a position: ");
            scanf("%d", &playedPosition);

            if (playedPosition < 1 || playedPosition > 9) {
                errorMessage[0] = '\x70';
                errorMessage[1] = '\x6F';
                errorMessage[2] = '\x73';
                errorMessage[3] = '\x69';
                errorMessage[4] = '\x63';
                errorMessage[5] = '\x61';
                errorMessage[6] = '\x6F';
                continue;
            }

            playedRow = posicoes[playedPosition - 1][0];
            playedColumn = posicoes[playedPosition - 1][1];

            if (board[playedRow][playedColumn] != '-')
            {
                errorMessage[0] = '\x70';
                errorMessage[1] = '\x6F';
                errorMessage[2] = '\x73';
                errorMessage[3] = '\x69';
                errorMessage[4] = '\x63';
                errorMessage[5] = '\x61';
                errorMessage[6] = '\x6F';
                errorMessage[7] = '\x20';
                errorMessage[8] = '\x6A';
                errorMessage[9] = '\x61';
                errorMessage[10] = '\x20';
                errorMessage[11] = '\x65';
                errorMessage[12] = '\x6D';
                errorMessage[13] = '\x20';
                errorMessage[14] = '\x75';
                errorMessage[15] = '\x73';
                errorMessage[16] = '\x6F';
                continue;
            }

            if (playerTurn == 1)
            {
                board[playedRow][playedColumn] = 'X';
                playerTurn = 2;
            } 
            else
            {
                board[playedRow][playedColumn] = 'O';
                playerTurn = 1;
            }

            for (iterator = 0; iterator < 3; iterator++)
            {
                if (
                    (
                        (board[iterator][0] == board[iterator][1]) && (board[iterator][1] == board[iterator][2]) && board[iterator][0] != '-'
                    )
                        ||
                    (
                        (board[0][iterator] == board[1][iterator]) && (board[1][iterator] == board[2][iterator]) && board[0][iterator] != '-'
                    )
                )
                {
                    gameState = 0;
                }
            }

            if (
                (
                    (board[0][0] == board[1][1]) && (board[1][1] == board[2][2]) && board[0][0] != '-'
                )
                    ||
                (
                    (board[0][2] == board[1][1]) && (board[1][1] == board[2][0]) && board[0][2] != '-'
                )
            )
            {
                gameState = 0;
            }

            round++;
        }

        showBoard(board, (char *)&errorMessage);

        printf("\nPlayer '%s' won!", currentPlayerName);

        printf("\nGame over!\n");
        printf("\nDo you want to restart the game?");
        printf("\n1 - Yes");
        printf("\n2 - No");
        printf("\nChoose an option and press ENTER: ");

        scanf("%d", &restartOption);


        switch (restartOption)
        {
            case 1:
                gameState = 1;
                break;
            case 2:
                menu();
                break;
        }
    }
}

void menu()
{
    int option = 0;

    flashScreen();

    signal(SIGINT, catch_ctrl_c_and_exit);

    while (option < 1 || option > 3)
    {
        printf("Welcome to the Game");
        printf("\n1 - Play Locally");
        printf("\n2 - Play Online");
        printf("\n3 - About");
        printf("\n4 - Quit");
        printf("\nChoose an option and press ENTER: ");

        scanf("%d", &option);

        switch (option)
        {
            case 1:
                flashScreen();
                option = 0;
                game();
                break;
            case 2:
                flashScreen();
                option = 0;
                exit(conectGame());
                break;
            case 3:
                flashScreen();
                option = 0;
                printf("About the game!\n");
                break;
            case 4:
                flashScreen();
                option = 0;
                printf("You exited!\n");
                exit(0);
                break;
            default:
                flashScreen();
                option = 0;
                printf("Invalid option!\n");
                continue;
                break;
        }
    }
}

int main(int argc, char **argv)
{
    menu();

    return 0;
}
