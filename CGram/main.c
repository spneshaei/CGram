//
//  main.c
//  CGram
//
//  Created by Seyyed Parsa Neshaei on 12/13/19.
//  Copyright © 2019 Seyyed Parsa Neshaei. All rights reserved.
//


/*
 
 ___   ___
 / __\ / _ \_ __ __ _ _ __ ___
 / /   / /_\/ '__/ _` | '_ ` _ \
 / /___/ /_\\| | | (_| | | | | | |
 \____/\____/|_|  \__,_|_| |_| |_|
 
 */


#include <sys/ioctl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "cJSON.h"
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX 500
#define PORT 12345
#define SA struct sockaddr

int server_socket;

#define cursorforward(x) printf("\033[%dC", (x))
#define cursorbackward(x) printf("\033[%dD", (x))

#define KEY_ESCAPE  0x001b
#define KEY_ENTER   0x000a
#define KEY_UP      0x0105
#define KEY_DOWN    0x0106
#define KEY_LEFT    0x0107
#define KEY_RIGHT   0x0108

char asciiArt[5][200];

void welcomePage(void);
void loginPage(void);
void mainPage(void);
void channelPage(void);
void showCursor(void);
void resetFont(void);

char username[100];
char token[400];
char channel[100];

typedef struct {
    char name[200];
} Member;

typedef struct {
    char sender[200];
    char content[200];
} Message;

int membersCount = 0;
Member members[200];
int messagesCount = 0;
Message messages[200];

static struct termios term, oterm;

static int getch(void);
//static int kbhit(void);
//static int kbesc(void);
//static int kbget(void);

int terminalColumns = 80, terminalLines = 24;

static int getch() {
    int c = 0;
    
    tcgetattr(0, &oterm);
    memcpy(&term, &oterm, sizeof(term));
    term.c_lflag &= ~(ICANON | ECHO);
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &term);
    c = getchar();
    tcsetattr(0, TCSANOW, &oterm);
    if (c == 27) {
        showCursor();
        resetFont();
        exit(0);
    }
    return c;
}

//static int kbhit(){
//    int c = 0;
//
//    tcgetattr(0, &oterm);
//    memcpy(&term, &oterm, sizeof(term));
//    term.c_lflag &= ~(ICANON | ECHO);
//    term.c_cc[VMIN] = 0;
//    term.c_cc[VTIME] = 1;
//    tcsetattr(0, TCSANOW, &term);
//    c = getchar();
//    tcsetattr(0, TCSANOW, &oterm);
//    if (c != -1) ungetc(c, stdin);
//    return ((c != -1) ? 1 : 0);
//}

//static int kbesc(){
//    int c;
//
//    if (!kbhit()) return KEY_ESCAPE;
//    c = getch();
//    if (c == '[') {
//        switch (getch()) {
//            case 'A':
//                c = KEY_UP;
//                break;
//            case 'B':
//                c = KEY_DOWN;
//                break;
//            case 'C':
//                c = KEY_LEFT;
//                break;
//            case 'D':
//                c = KEY_RIGHT;
//                break;
//            default:
//                c = 0;
//                break;
//        }
//    } else {
//        c = 0;
//    }
//    if (c == 0) while (kbhit()) getch();
//    return c;
//}

//static int kbget(){
//    int c;
//    c = getch();
//    return (c == KEY_ESCAPE) ? kbesc() : c;
//}

int getWords(char *base, char target[10][200]) {
    int n = 0, i, j = 0;
    for (i = 0; ; i++) {
        if (base[i] != ' ') {
            target[n][j++] = base[i];
        } else {
            target[n][j++] = '\0';
            n++;
            j = 0;
        }
        if (base[i] == '\0') break;
    }
    return n;
}

int strStartsWith(const char *pre, const char *str) {
    size_t lenpre = strlen(pre),
    lenstr = strlen(str);
    return lenstr < lenpre ? 0 : memcmp(pre, str, lenpre) == 0;
}

void exitCtrlCHandler(int s) {
    exit(1);
}

void setupTerminalDimensions() {
    #ifdef TIOCGSIZE
        struct ttysize ts;
        ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
        terminalColumns = ts.ts_cols;
        terminalLines = ts.ts_lines;
    #elif defined(TIOCGWINSZ)
        struct winsize ts;
        ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
        terminalColumns = ts.ws_col;
        terminalLines = ts.ws_row;
    #endif
}

int unicodeStrlen(char *str) {
    int i = 0, j = 0;
    while (str[i]) {
        if ((str[i] & 0xC0) != 0x80)
            j++;
        i++;
    }
    return j;
}

void printStringCentered(char *str) {
    int long len = strlen(str);//unicodeStrlen(str);
    int width = (int)(len + (terminalColumns - len) / 2);
    printf("%*s", width, str);
}

void hideCursor() {
    printf("\e[?25l");
}

void showCursor() {
    printf("\e[?25h");
}

void makeBoldRed() {
    printf("\033[1;35m");
}

void makeRed() {
    printf("\033[0;35m");
}

void resetFont() {
    printf("\x1b[0m");
}

void loginPage_printUntilSpecifiedUsername(char *username) {
    system("clear");
    hideCursor();
    printf("\n\n\n\n\n");
    makeBoldRed();
    printStringCentered("Login to CGram");
    printf("\n\n\n");
    printStringCentered("Enter your username below, or type 'return' and then enter to return back:");
    resetFont();
    printf("\n\n");
    printStringCentered(username);
}

void registerPage_printUntilSpecifiedUsername(char *username) {
    system("clear");
    hideCursor();
    printf("\n\n\n\n\n");
    makeBoldRed();
    printStringCentered("Register in CGram");
    printf("\n\n\n");
    printStringCentered("Enter your username below, or type 'return' and then enter to return back:");
    resetFont();
    printf("\n\n");
    printStringCentered(username);
}

void mainPage_printUntilSpecifiedChannel(char *channel) {
    system("clear");
    hideCursor();
    printf("\n\n\n\n\n");
    makeBoldRed();
    
    char str1[200] = {'\0'};
    sprintf(str1, "Welcome, %s!", username);
    printStringCentered(str1);
    printf("\n\n\n");
    printStringCentered("Enter the channel you want to join below:");
    printf("\n\n");
    printStringCentered("To create a new channel, type 'new' and then its name after a space.");
    printf("\n\n");
    printStringCentered("To log out from your account, type 'logout'.");
    printf("\n\n\n");
    resetFont();
    printStringCentered(channel);
}

int sendRequestToServer(char *request, char *result) {  // returns -1 if no result from the server
    unsigned long requestLen = strlen(request);
    char r[requestLen];
    for (int i = 0; i <= requestLen; i++) {
        if (request[i] == '\0') {
            break;
        }
        r[i] = request[i];
    }
    
    int clientSocket;
    struct sockaddr_in servaddr;
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);
    
    connect(clientSocket, (SA*)&servaddr, sizeof(servaddr));
    send(server_socket, r, sizeof(r), 0);
    recv(clientSocket, result, sizeof(result), 0);
    shutdown(clientSocket, SHUT_RDWR);
    
    if (strcmp(result, "") == 0) {
        return -1;
    }
    return 0;
}

int loginServer(char username[], char password[]) { // returns -1 if error
    char req[300] = {'\0'}, result[500];
    sprintf(req, "login %s, %s", username, password);
    sendRequestToServer(req, result);
    cJSON *json = cJSON_Parse(result);
    if (json == NULL) {
        cJSON_Delete(json);
        strcpy(token, "-1");
        return -1;
    }
    const cJSON *type = cJSON_GetObjectItemCaseSensitive(json, "type");
    if (cJSON_IsString(type) && (type->valuestring != NULL)) {
        if (strcmp(type->valuestring, "Error") == 0) {
            cJSON_Delete(json);
            strcpy(token, "-1");
            return -1;
        } else if (strcmp(type->valuestring, "AuthToken") == 0) {
            const cJSON *_token = cJSON_GetObjectItemCaseSensitive(json, "content");
            if (cJSON_IsString(_token) && (_token->valuestring != NULL)) {
                strcpy(token, _token->valuestring);
                cJSON_Delete(json);
                return 0;
            } else {
                cJSON_Delete(json);
                strcpy(token, "-1");
                return -1;
            }
        } else {
            cJSON_Delete(json);
            strcpy(token, "-1");
            return -1;
        }
    }
    cJSON_Delete(json);
    strcpy(token, "-1");
    return -1;
}

int registerServer(char username[], char password[]) { // returns -1 if error
    char req[300] = {'\0'}, result[100];
    sprintf(req, "register %s, %s", username, password);
    sendRequestToServer(req, result);
    // TODO: Check result and return good value
    return -1;
}

int createChannelServer() { // returns -1 if error
    // TODO: IMPORTANT: It seems it doesn't do normal things as findChannel...
    char req[300] = {'\0'}, result[100];
    sprintf(req, "create channel %s, %s", channel, token);
    sendRequestToServer(req, result);
    // TODO: Check result and return good value
    return -1;
}

int findChannelServer() { // returns -1 if error
    char req[300] = {'\0'}, result[500];
    sprintf(req, "join channel %s, %s", channel, token);
    sendRequestToServer(req, result);
    // TODO: Parse result, set global arrays and return good value
    return -1;
}

int logoutServer() { // returns -1 if error
    // TODO: IMPORTANT: It seems it doesn't do normal things as findChannel...
    char req[300] = {'\0'}, result[100];
    sprintf(req, "logout %s", token);
    sendRequestToServer(req, result);
    // TODO: Check result and return good value
    return -1;
}

int reloadMembersServer() { // returns -1 if error
    char req[300] = {'\0'}, result[500];
    sprintf(req, "channel members %s", token);
    sendRequestToServer(req, result);
    // TODO: Parse result, set global arrays and return good value
    return -1;
}

int reloadMessagesServer() { // returns -1 if error
    for (int i = 0; i < messagesCount; i++) {
        strcpy(messages[i].content, "");
        strcpy(messages[i].sender, "");
    }
    messagesCount = 0;
    char req[300] = {'\0'}, result[1000];
    sprintf(req, "refresh %s", token);
    sendRequestToServer(req, result);
    cJSON *json = cJSON_Parse(result);
    if (json == NULL) {
        cJSON_Delete(json);
        return -1;
    }
    const cJSON *type = cJSON_GetObjectItemCaseSensitive(json, "type");
     if (cJSON_IsString(type) && (type->valuestring != NULL)) {
         if (strcmp(type->valuestring, "Error") == 0) {
             cJSON_Delete(json);
             return -1;
         } else if (strcmp(type->valuestring, "List") == 0) {
             const cJSON *content = cJSON_GetObjectItemCaseSensitive(json, "content");
             const cJSON *eachMessage = NULL;
             cJSON_ArrayForEach(eachMessage, content) {
                 const cJSON *eachMessageSender = cJSON_GetObjectItemCaseSensitive(eachMessage, "sender");
                 const cJSON *eachMessageContent = cJSON_GetObjectItemCaseSensitive(eachMessage, "content");
                 if (cJSON_IsString(eachMessageSender) && (eachMessageSender->valuestring != NULL)
                     &&
                     cJSON_IsString(eachMessageContent) && (eachMessageContent->valuestring != NULL)) {
                     Message m;
                     strcpy(m.sender, eachMessageSender->valuestring);
                     strcpy(m.content, eachMessageContent->valuestring);
                     messages[messagesCount] = m;
                     messagesCount++;
                 }
             }
         } else {
             cJSON_Delete(json);
             return -1;
         }
     }
    return -1;
}

int sendMessageServer(char message[]) { // returns -1 if error
    char req[300] = {'\0'}, result[100];
    sprintf(req, "send %s, %s", message, token);
    sendRequestToServer(req, result);
    // TODO: Check result and return good value
    return -1;
}

int leaveChannelServer() { // returns -1 if error
    char req[300] = {'\0'}, result[100];
    sprintf(req, "leave %s", token);
    sendRequestToServer(req, result);
    // TODO: Check result and return good value
    return -1;
}

void chatPage() {
    // TODO: h
    system("clear");
    system("clear");
    system("clear");
    hideCursor();
    printf("\n\n");
    makeBoldRed();
    printStringCentered(channel);
    resetFont();
    printf("\n\n");
    makeRed();
    printStringCentered("Messages sent in this channel are shown here.");
    printf("\n");
    printStringCentered("Type a new message and press enter to send it.");
    printf("\n");
    printStringCentered("To receive the latest messages, type 'reload'.");
    printf("\n");
    printStringCentered("To see the name of other members of this channel, type 'members'.");
    printf("\n");
    printStringCentered("To leave this channel, type 'leave'.");
    resetFont();
    printf("\n\n");
    for (int i = 0; i < messagesCount; i++) {
        Message m = messages[i];
        makeRed();
        printf("%s\t\t", m.sender);
        resetFont();
        printf("%s\n", m.content);
    }
    showCursor();
    makeRed();
    printf("\n\nType your message ==> ");
    resetFont();
    char s[200];
    scanf("%s", s);
    printf("\n\n");
    hideCursor();
    if (strcmp(s, "reload") == 0) {
        makeBoldRed();
        printStringCentered("Reloading the messages...");
        resetFont();
        printf("\n\n");
        int result = reloadMessagesServer();
        if (result == -1) {
            makeBoldRed();
            printStringCentered("Reloading failed. Press 'r' to return.");
            resetFont();
            while (1) {
                char c = getch();
                if (c == 'r') {
                    chatPage();
                    return;
                }
            }
        } else {
            makeBoldRed();
            printStringCentered("Reloading successful. Press 's' to show new messages.");
            resetFont();
            while (1) {
                char c = getch();
                if (c == 's') {
                    chatPage();
                    return;
                }
            }
        }
    } else if (strcmp(s, "members") == 0) {
        makeBoldRed();
        printStringCentered("Finding all the members...");
        resetFont();
        printf("\n\n");
        int result = reloadMembersServer();
        if (result == -1) {
            makeBoldRed();
            printStringCentered("Finding failed. Press 'r' to return.");
            resetFont();
            while (1) {
                char c = getch();
                if (c == 'r') {
                    chatPage();
                    return;
                }
            }
        } else {
            makeBoldRed();
            printStringCentered("All Channel Members:");
            resetFont();
            printf("\n\n");
            for (int i = 0; i < membersCount; i++) {
                printStringCentered(members[i].name);
                printf("\n");
//                if (i % 2 == 0) {
//                    printf("%s\t", members[i].name);
//                } else {
//                    printf("%s\n", members[i].name);
//                }
            }
            printf("\n\n");
            makeBoldRed();
            printStringCentered("Press 'r' to return.");
            resetFont();
            while (1) {
                char c = getch();
                if (c == 'r') {
                    chatPage();
                    return;
                }
            }
        }
    } else if (strcmp(s, "leave") == 0) {
        makeBoldRed();
        printStringCentered("Leaving the channel...");
        resetFont();
        printf("\n\n");
        int result = leaveChannelServer();
        if (result == -1) {
            makeBoldRed();
            printStringCentered("Leaving failed. Press 'r' to return.");
            resetFont();
            while (1) {
                char c = getch();
                if (c == 'r') {
                    chatPage();
                    return;
                }
            }
        } else {
            mainPage();
            return;
        }
    }else {
        makeBoldRed();
        printStringCentered("Sending the message...");
        resetFont();
        printf("\n\n");
        int result1 = sendMessageServer(s);
        int result2 = reloadMessagesServer();
        if (result1 == -1 || result2 == -1) {
            makeBoldRed();
            printStringCentered("Sending failed. Press 'r' to return.");
            resetFont();
            while (1) {
                char c = getch();
                if (c == 'r') {
                    chatPage();
                    return;
                }
            }
        } else {
            makeBoldRed();
            printStringCentered("Message sent. Press 'r' to return.");
            resetFont();
            while (1) {
                char c = getch();
                if (c == 'r') {
                    chatPage();
                    return;
                }
            }
        }
    }
}

void mainPage() {
    char x[100] = {'\0'};
    strcpy(channel, x);
    // TODO: h
    system("clear");
    hideCursor();
    printf("\n\n\n\n\n");
    makeBoldRed();
    
    char str1[200] = {'\0'};
    sprintf(str1, "Welcome, %s!", username);
    printStringCentered(str1);
    printf("\n\n\n");
    printStringCentered("Enter the channel you want to join below:");
    printf("\n\n");
    printStringCentered("To create a new channel, type 'new' and then its name after a space.");
    printf("\n\n");
    printStringCentered("To log out from your account, type 'logout'.");
    printf("\n\n\n");
    resetFont();
    int shouldContinue = 1;
    while (shouldContinue) {
        char c = getch();
        if (c == '\n') {
            if (strcmp(channel, "logout") == 0) {
                printf("\n\n");
                makeBoldRed();
                printStringCentered("Logging out...");
                resetFont();
                printf("\n\n");
                int res = logoutServer();
                if (res == -1) {
                    makeBoldRed();
                    printStringCentered("Logout failed. Press 't' to retry.");
                    resetFont();
                    while (1) {
                        char c = getch();
                        if (c == 't') {
                            mainPage();
                            return;
                        }
                    }
                } else {
                    shouldContinue = 0;
                    username[0] = '\0';
                    loginPage();
                    return;
                }
            } else if (strStartsWith("new ", channel)) {
                printf("\n\n");
                char target[10][200];
                getWords(channel, target);
                strcpy(channel, target[1]);
                if (strcmp(channel, "") == 0) {
                    makeBoldRed();
                    printStringCentered("Invalid channel name. Press 't' to retry.");
                    resetFont();
                    while (1) {
                        char c = getch();
                        if (c == 't') {
                            mainPage();
                            return;
                        }
                    }
                }
                makeBoldRed();
                printStringCentered("Creating the channel...");
                resetFont();
                printf("\n\n\n");
                
                int result1 = createChannelServer();
                // TODO: should be any result for joinChannel also here?
                int result2 = reloadMessagesServer();
                
                if (result1 == -1 || result2 == -1) {
                    makeBoldRed();
                    printStringCentered("Creating the channel failed. Press 't' to retry.");
                    resetFont();
                    while (1) {
                        char c = getch();
                        if (c == 't') {
                            mainPage();
                            return;
                        }
                    }
                } else {
                    chatPage();
                    return;
                }
            }
            break;
        } else {
            if (c == '\x7f' || c == 8) {
                channel[strlen(channel) - 1] = '\0';
            } else {
                char oneCharArr[2];
                oneCharArr[0] = c;
                oneCharArr[1] = '\0';
                strcat(channel, oneCharArr);
            }
            mainPage_printUntilSpecifiedChannel(channel);
        }
    }
    
    printf("\n\n");
    
    if (strcmp(channel, "") == 0) {
        makeBoldRed();
        printStringCentered("Invalid channel name. Press 't' to retry.");
        resetFont();
        while (1) {
            char c = getch();
            if (c == 't') {
                mainPage();
                return;
            }
        }
    }
    
    makeBoldRed();
    printStringCentered("Looking for the channel...");
    resetFont();
    printf("\n\n\n");
    
    int result1 = findChannelServer();
    int result2 = reloadMessagesServer();
    
    if (result1 == -1 || result2 == -1) {
        makeBoldRed();
        printStringCentered("Looking for the channel failed. Press 't' to retry.");
        resetFont();
        while (1) {
            char c = getch();
            if (c == 't') {
                mainPage();
                return;
            }
        }
    } else {
        chatPage();
        return;
    }
    
}

void registerPage() {
    // TODO: h
    system("clear");
    hideCursor();
    printf("\n\n\n\n\n");
    makeBoldRed();
    printStringCentered("Register in CGram");
    printf("\n\n\n");
    printStringCentered("Enter your username below, or type 'return' and then enter to return back:");
    resetFont();
    printf("\n\n");
    char username[100] = {'\0'}, password[100] = {'\0'};
    int shouldContinue = 1;
    while (shouldContinue) {
        char c = getch();
        if (c == '\n') {
            if (strcmp(username, "return") == 0) {
                shouldContinue = 0;
                welcomePage();
                return;
            }
            break;
        } else {
            if (c == '\x7f' || c == 8) {
                username[strlen(username) - 1] = '\0';
            } else {
                char oneCharArr[2];
                oneCharArr[0] = c;
                oneCharArr[1] = '\0';
                strcat(username, oneCharArr);
            }
            registerPage_printUntilSpecifiedUsername(username);
        }
    }
    
    printf("\n\n");
    
    if (strcmp(username, "") == 0) {
        makeBoldRed();
        printStringCentered("Invalid username. Press 't' to retry or 'r' to return.");
        resetFont();
        while (1) {
            char c = getch();
            if (c == 't') {
                registerPage();
                return;
            } else if (c == 'r') {
                welcomePage();
                return;
            }
        }
    }
    
    makeBoldRed();
    printStringCentered("Enter your password below (it will be hidden for safety reasons):");
    resetFont();
    
    shouldContinue = 1;
    while (shouldContinue) {
        char c = getch();
        if (c == '\n') {
            break;
        } else {
            char oneCharArr[2];
            oneCharArr[0] = c;
            oneCharArr[1] = '\0';
            strcat(password, oneCharArr);
        }
    }
    
    printf("\n\n\n\n");
    
    if (strcmp(password, "") == 0) {
        makeBoldRed();
        printStringCentered("Invalid password. Press 't' to retry or 'r' to return.");
        resetFont();
        while (1) {
            char c = getch();
            if (c == 't') {
                registerPage();
                return;
            } else if (c == 'r') {
                welcomePage();
                return;
            }
        }
    }
    
    makeBoldRed();
    printStringCentered("Registring...");
    resetFont();
    printf("\n\n\n");
    
    int result = registerServer(username, password);
    
    if (result == -1) {
        makeBoldRed();
        printStringCentered("Registration failed. Press 't' to retry or 'r' to return.");
        resetFont();
        while (1) {
            char c = getch();
            if (c == 't') {
                registerPage();
                return;
            } else if (c == 'r') {
                welcomePage();
                return;
            }
        }
    } else {
        makeBoldRed();
        printStringCentered("Registration succesful! Press any key to return.");
        resetFont();
        getch();
        welcomePage();
        return;
    }
    
}

void loginPage() {
    char x[100] = {'\0'};
    strcpy(username, x);
    // TODO: h
    system("clear");
    hideCursor();
    printf("\n\n\n\n\n");
    makeBoldRed();
    printStringCentered("Login to CGram");
    printf("\n\n\n");
    printStringCentered("Enter your username below, or type 'return' and then enter to return back:");
    resetFont();
    printf("\n\n");
    char password[100] = {'\0'};
    int shouldContinue = 1;
    while (shouldContinue) {
        char c = getch();
        if (c == '\n') {
            if (strcmp(username, "return") == 0) {
                shouldContinue = 0;
                welcomePage();
                return;
            }
            break;
        } else {
            if (c == '\x7f' || c == 8) {
                username[strlen(username) - 1] = '\0';
            } else {
                char oneCharArr[2];
                oneCharArr[0] = c;
                oneCharArr[1] = '\0';
                strcat(username, oneCharArr);
            }
            loginPage_printUntilSpecifiedUsername(username);
        }
    }
    
    printf("\n\n");
    
    if (strcmp(username, "") == 0) {
        makeBoldRed();
        printStringCentered("Invalid username. Press 't' to retry or 'r' to return.");
        resetFont();
        while (1) {
            char c = getch();
            if (c == 't') {
                loginPage();
                return;
            } else if (c == 'r') {
                welcomePage();
                return;
            }
        }
    }
    
    makeBoldRed();
    printStringCentered("Enter your password below (it will be hidden for safety reasons):");
    resetFont();
    
    shouldContinue = 1;
    while (shouldContinue) {
        char c = getch();
        if (c == '\n') {
            break;
        } else {
            char oneCharArr[2];
            oneCharArr[0] = c;
            oneCharArr[1] = '\0';
            strcat(password, oneCharArr);
        }
    }
    
    printf("\n\n\n\n");
    
    if (strcmp(password, "") == 0) {
        makeBoldRed();
        printStringCentered("Invalid password. Press 't' to retry or 'r' to return.");
        resetFont();
        while (1) {
            char c = getch();
            if (c == 't') {
                loginPage();
                return;
            } else if (c == 'r') {
                welcomePage();
                return;
            }
        }
    }
    
    makeBoldRed();
    printStringCentered("Logging in...");
    resetFont();
    printf("\n\n\n");
    
    int res = loginServer(username, password);
    
    if (strcmp(token, "-1") == 0 || res == -1) {
        makeBoldRed();
        printStringCentered("Login failed. Press 't' to retry or 'r' to return.");
        resetFont();
        while (1) {
            char c = getch();
            if (c == 't') {
                loginPage();
                return;
            } else if (c == 'r') {
                welcomePage();
                return;
            }
        }
    } else {
        mainPage();
    }
    
}

void welcomePage() {
    // TODO: h
    system("clear");
    hideCursor();
    makeBoldRed();
    printf("\n\n");
    for (int i = 0; i < 5; i++) {
        printf("%s", asciiArt[i]);
    }
    printf("\n\n\n");
    printStringCentered("Welcome to CGram");
//    makeRed();
    printf("\n\n");
    printStringCentered("The most advanced messaging app designed ever in pure C");
    printf("\n\n\n\n\n");
    makeRed();
    printStringCentered("Designed and developed by Seyed Parsa Neshaei");
    printf("\n\n");
    printStringCentered("Don't forget to check out http://spneshaei.com for more great projects!");
    resetFont();
    printf("\n\n\n\n\n\n");
    printStringCentered("Press 'l' on your keyboard to log in quickly, or 'q' to exit CGram.");
    printf("\n\n\n");
    printStringCentered("If you don't have an account, just press 'r' to register in a snap!");
    printf("\n\n");
    int shouldContinue = 1;
    while (shouldContinue) {
        char c = getch();
        switch (c) {
            case 'q':
            case 'Q':
                printf("\n\n");
                printStringCentered("Are you sure you want to quit? Press 'y' for YES and 'n' for NO.");
                while (1) {
                    char r = getch();
                    if (r == 'y') {
                        // TODO: Here
                        system("clear");//system("@cls||clear");
                        printf("\n");
                        makeBoldRed();
                        printStringCentered("Thank you for using CGram!");
                        resetFont();
                        printf("\n\n");
                        exit(0);
                    } else if (r == 'n') {
                        shouldContinue = 0;
                        welcomePage();
                        break;
                    }
                }
                
                break;
            case '\n':
                break;
            case 'l':
            case 'L':
                shouldContinue = 0;
                loginPage();
                break;
            case 'r':
            case 'R':
                shouldContinue = 0;
                registerPage();
                break;
            default:
//                printf("\n");
//                char str1[100] = {'\0'};
//                sprintf(str1, "Sorry, I don't know the meaning of '%c'!", c);
//                printStringCentered(str1);
                break;
        }
    }
    
}

void initializeAsciiArt() {
    strcpy(asciiArt[0], " ______     ______     ______     ______     __    __    \n");
    strcpy(asciiArt[1], "/\\  ___\\   /\\  ___\\   /\\  == \\   /\\  __ \\   /\\ \"-./  \\   \n");
    strcpy(asciiArt[2], "\\ \\ \\____  \\ \\ \\__ \\  \\ \\  __<   \\ \\  __ \\  \\ \\ \\-./\\ \\  \n");
    strcpy(asciiArt[3], " \\ \\_____\\  \\ \\_____\\  \\ \\_\\ \\_\\  \\ \\_\\ \\_\\  \\ \\_\\ \\ \\_\\ \n");
    strcpy(asciiArt[4], "  \\/_____/   \\/_____/   \\/_/ /_/   \\/_/\\/_/   \\/_/  \\/_/ \n");
}

void setupSocket() {
    int clientSocket;
    struct sockaddr_in servaddr;
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);
    connect(clientSocket, (SA*)&servaddr, sizeof(servaddr));
    server_socket = clientSocket;
}

void endSocket() {
    shutdown(server_socket, SHUT_RDWR);
}

int main (int argc, const char * argv[]) {
    // IDEA: command line arguments to facilitate using app
    // IDEA: Password strength
    // IDEA: Activity Indicator
    // FEATURE: esc exits app
    
    
    
//    signal(SIGINT, exitCtrlCHandler);
    
    // TODO: Here
    //system("clear");//system("@cls||clear");
    
//    setupSocket();
    
    // --------- HERE IS THE TESTING NETWORK CODE -----------
    
    char result[500];
    sendRequestToServer("register Alireza 1234", result);

    return 0;
    
    setupTerminalDimensions();
    atexit(showCursor);
    initializeAsciiArt();
    
    welcomePage();
    
    return 0;
}
