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

enum PasswordStatus {
    TOOWEAK = 1,
    WEAK = 2,
    MODERATE = 3,
    STRONG = 4,
    TOOSTRONG = 5,
    TOOSHORT = 6
};

enum PasswordStatus passwordStatus(char *password) {
    unsigned long len = strlen(password);
    if(len < 6) return TOOSHORT;
    int hasUpper = 0, hasLower = 0, hasSymbol = 0, hasSpecial = 0, hasNum = 0, flag = 0, i = 0;
    while (password[i] != '\0') {
        char x = password[i];
        if (x >= 'a' && x <= 'z') hasLower = 1;
        else if (x >= 'A' && x <= 'X') hasUpper = 1;
        else if ((x >= 33 && x <= 47) || (x >= 58 && x <= 64) || (x >= 91 && x <= 96)) hasSymbol = 1;
        else if (x >= 123 && x <= 126) hasSpecial = 1;
        else if (x >= 48 && x <= 57) hasNum = 1;
        i++;
    }
    if (hasUpper) flag += 10;
    if (hasLower) flag += 10;
    if (hasSymbol) flag += 10;
    if (hasSpecial) flag += 10;
    if (hasNum) flag += 10;
        
    switch (flag) {
        case 50: return TOOSTRONG;
        case 40: return STRONG;
        case 30: return MODERATE;
        case 20: return WEAK;
        case 10: return TOOWEAK;
        default: return TOOWEAK;
    }
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

void registerPage_printUntilSpecifiedPassword(char *username, char *password) { 
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
    printf("\n\n");
    makeBoldRed();
    printStringCentered("Enter your password below:");
    resetFont();
    printf("\n\n");
    printStringCentered(password);
    printf("\n\n");
    enum PasswordStatus strength = passwordStatus(password);
    makeRed();
    if (strength == TOOSHORT) {
        printStringCentered("Password is too short");
    } else if (strength == TOOWEAK) {
        printStringCentered("Password strength: Too Weak");
    } else if (strength == WEAK) {
        printStringCentered("Password strength: Weak");
    } else if (strength == MODERATE) {
        printStringCentered("Password strength: Moderate");
    } else if (strength == STRONG) {
        printStringCentered("Password strength: Strong");
    } else if (strength == TOOSTRONG) {
        printStringCentered("Password strength: Too Strong");
    }
    resetFont();
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

char requestToSend[MAX];

char originalReq[MAX] = {'\0'};

void copyReqs() {
    for (int i = 0; i < MAX; i++) {
        requestToSend[i] = '\0';
    }
    for (int i = 0; i < MAX; i++) {
        if (originalReq[i] == '\0') {
            requestToSend[i] = '\n';
            requestToSend[i + 1] = '\0';
            break;
        } else {
            requestToSend[i] = originalReq[i];
        }
    }
}

void chat(int server_socket) {
    copyReqs();
    char buffer[MAX];
    int n;
    while (1) {
        bzero(buffer, sizeof(buffer));
        n = 0;
        send(server_socket, requestToSend, sizeof(requestToSend), 0);
        bzero(requestToSend, sizeof(requestToSend));
        recv(server_socket, requestToSend, sizeof(requestToSend), 0);
        return;
    }
}

void sendRequestToServer(char *request, char *result) {
    strcpy(originalReq, request);
    
    int client_socket;
    struct sockaddr_in servaddr;
    
    // Create and verify socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    // Assign IP and port
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);
    
    // Connect the client socket to server socket
    if (connect(client_socket, (SA*)&servaddr, sizeof(servaddr)) != 0) {
    }
    
    // Function for chat
    chat(client_socket);
    
    strcpy(result, requestToSend);
    
    // Close the socket
    shutdown(client_socket, SHUT_RDWR);
}

int loginServer(char username[], char password[]) { // returns -3 if wrong username, -2 if wrong password, -1 if neither
    char req[MAX] = {'\0'}, result[MAX];
    sprintf(req, "login %s, %s", username, password);
    sendRequestToServer(req, result);
    cJSON *json = cJSON_Parse(result);
    if (json == NULL) {
        cJSON_Delete(json);
        strcpy(token, "-1");
        return -1;
    }
    const cJSON *type = cJSON_GetObjectItemCaseSensitive(json, "type");
    const cJSON *_content = cJSON_GetObjectItemCaseSensitive(json, "content");
    char x[MAX] ={'\0'};
    strcpy(x, _content->valuestring);
    if (cJSON_IsString(type) && (type->valuestring != NULL)) {
        if (strcmp(type->valuestring, "Error") == 0) {
            cJSON_Delete(json);
            strcpy(token, "-1");
            if (cJSON_IsString(_content)) {
                if (strcmp(x, "Username is not valid.") == 0) {
                    return -3;
                } else if (strcmp(x, "Wrong password.") == 0) {
                    return -2;
                } else {
                    return -1;
                }
            }
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
    char req[MAX] = {'\0'}, result[MAX] = {'\0'};
    sprintf(req, "register %s, %s", username, password);
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
        } else {
            return 0;
        }
    }
    return -1;
}

int createChannelServer() { // returns -1 if error
    char req[MAX] = {'\0'}, result[MAX];
    sprintf(req, "create channel %s, %s", channel, token);
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
        } else {
            return 0;
        }
    }
    return -1;
}

int findChannelServer() { // returns -1 if error
    char req[MAX] = {'\0'}, result[MAX];
    sprintf(req, "join channel %s, %s", channel, token);
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
        } else {
            return 0;
        }
    }
    return -1;
}

int logoutServer() { // returns -1 if error
    char req[MAX] = {'\0'}, result[MAX];
    sprintf(req, "logout %s", token);
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
        } else {
            return 0;
        }
    }
    return -1;
}

int reloadMembersServer() { // returns -1 if error
    for (int i = 0; i < membersCount; i++) {
        strcpy(members[i].name, "");
    }
    membersCount = 0;
    char req[MAX] = {'\0'}, result[MAX];
    sprintf(req, "channel members %s", token);
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
            const cJSON *eachMember = NULL;
            cJSON_ArrayForEach(eachMember, content) {
                if (cJSON_IsString(eachMember) && (eachMember->valuestring != NULL)) {
                    Member m;
                    strcpy(m.name, eachMember->valuestring);
                    members[membersCount] = m;
                    membersCount++;
                }
            }
            return 1;
        } else {
            cJSON_Delete(json);
            return -1;
        }
    }
    return -1;
}

int reloadMessagesServer() { // returns -1 if error
    char req[MAX] = {'\0'}, result[MAX];
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
             return 0;
         } else {
             cJSON_Delete(json);
             return -1;
         }
     }
    return -1;
}

int sendMessageServer(char message[]) { // returns -1 if error
    char req[MAX] = {'\0'}, result[MAX];
    unsigned long len = strlen(message);
    if (message[len - 1] == '\n') {
        message[len - 1] = '\0';
    }
    sprintf(req, "send %s, %s", message, token);
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
        } else {
            return 0;
        }
    }
    return -1;
}

int leaveChannelServer() { // returns -1 if error
    char req[MAX] = {'\0'}, result[MAX];
    sprintf(req, "leave %s", token);
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
        } else {
            return 0;
        }
    }
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
    char s[200] = {'\0'};
    fgets(s, 200, stdin);
    printf("\n\n");
    hideCursor();
    if (strcmp(s, "reload\n") == 0) {
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
    } else if (strcmp(s, "members\n") == 0) {
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
    } else if (strcmp(s, "leave\n") == 0) {
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
                // TODO: Possible bug here due to \xff-ing...
            } else if (strStartsWith("new ", channel) || strStartsWith("\xffnew ", channel)) {
                printf("\n\n");
                char target[10][200];
                getWords(channel, target);
                strcpy(channel, target[1]);
                if (strcmp(channel, "") == 0) {
                    mainPage();
                    return;
                }
                makeBoldRed();
                printStringCentered("Creating the channel...");
                resetFont();
                printf("\n\n\n");
                
                int result1 = createChannelServer();
                for (int i = 0; i < messagesCount; i++) {
                    strcpy(messages[i].content, "");
                    strcpy(messages[i].sender, "");
                }
                messagesCount = 0;
                int result3 = reloadMessagesServer();
                
                if (result1 == -1 || result3 == -1) {
                    makeBoldRed();
                    printStringCentered("Channel name exists. Press 't' to retry.");
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
        mainPage();
        return;
    }
    
    makeBoldRed();
    printStringCentered("Looking for the channel...");
    resetFont();
    printf("\n\n\n");
    
    int result1 = findChannelServer();
    for (int i = 0; i < messagesCount; i++) {
        strcpy(messages[i].content, "");
        strcpy(messages[i].sender, "");
    }
    messagesCount = 0;
    int result2 = reloadMessagesServer();
    
    if (result1 == -1 || result2 == -1) {
        makeBoldRed();
        printStringCentered("The channel doesn't exist. Press 't' to retry.");
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
    printStringCentered("Enter your password below:");
    resetFont();
    
    shouldContinue = 1;
    while (shouldContinue) {
        char c = getch();
        if (c != '\n') {
            if (c == '\x7f' || c == 8) {
                password[strlen(password) - 1] = '\0';
            } else {
                char oneCharArr[2];
                oneCharArr[0] = c;
                oneCharArr[1] = '\0';
                strcat(password, oneCharArr);
            }
            registerPage_printUntilSpecifiedPassword(username, password);
        } else {
            shouldContinue = 0;
            break;
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
    // TODO: Check spelling of Registering!!
    printStringCentered("Registering...");
    resetFont();
    printf("\n\n\n");
    
    int result = registerServer(username, password);
    
    if (result == -1) {
        makeBoldRed();
        printStringCentered("Username is not available. Press 't' to retry or 'r' to return.");
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
        printStringCentered("Registration successful! Press any key to return.");
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
    
    if (res == -2) {
        makeBoldRed();
        printStringCentered("Wrong password. Press 't' to retry or 'r' to return.");
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
    } else if (res == -3) {
        makeBoldRed();
        printStringCentered("Username does not exist. Press 't' to retry or 'r' to return.");
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
    printf("\n\n");
    printStringCentered("The most advanced messaging app designed ever in pure C");
    printf("\n\n\n\n\n");
    makeRed();
    printStringCentered("Designed and developed by Seyed Parsa Neshaei");
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
    // BUG: Wrong login crash...! Or not??
    // BUG: Not very good password strength finder!
    
    setupTerminalDimensions();
    atexit(showCursor);
    initializeAsciiArt();
    
    welcomePage();
    
    return 0;
}
