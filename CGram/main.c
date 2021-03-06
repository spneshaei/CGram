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
#define MAX 20000
#define PORT 12345
#define SA struct sockaddr

int server_socket;

//#define cursorforward(x) printf("\033[%dC", (x))
//#define cursorbackward(x) printf("\033[%dD", (x))

//#define KEY_ESCAPE  0x001b
//#define KEY_ENTER   0x000a
//#define KEY_UP      0x0105
//#define KEY_DOWN    0x0106
//#define KEY_LEFT    0x0107
//#define KEY_RIGHT   0x0108

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

// TODO: Not written in Documentation
void splitString(char str[1000000], char newString[1000][1000], int *countOfWords) {
    int j = 0, count = 0;
    for(int i = 0; i <= strlen(str); i++) {
        if (str[i] == '\0' || str[i] == ' ' || str[i] == ',' || str[i] == '\n') {
            newString[count][j] = '\0';
            count += 1;
            j = 0;
        } else {
            newString[count][j] = str[i];
            j++;
        }
    }
    *countOfWords = count;
}

void splitStringByDoubleQuotes(char str[1000000], char newString[1000][1000], int *countOfWords) {
    int j = 0, count = 0;
    for(int i = 0; i <= strlen(str); i++) {
        if (str[i] == '\0' || str[i] == '\"' || str[i] == '\n') {
            newString[count][j] = '\0';
            count += 1;
            j = 0;
        } else {
            newString[count][j] = str[i];
            j++;
        }
    }
    *countOfWords = count;
}

void findSubstring(char *destination, const char *source, int beg, int n)
{
    // extracts n characters from source string starting from beg index
    // and copy them into the destination string
    while (n > 0)
    {
        *destination = *(source + beg);
        
        destination++;
        source++;
        n--;
    }
    
    // null terminate destination string
    *destination = '\0';
}

// TODO: Find out how it works?? Does it replace words in "s" in-place??
char *replaceWord(const char *s, const char *oldW,
                  const char *newW) {
    char *result;
    int i, cnt = 0;
    int newWlen = strlen(newW);
    int oldWlen = strlen(oldW);
    
    // Counting the number of times old word
    // occur in the string
    for (i = 0; s[i] != '\0'; i++)
    {
        if (strstr(&s[i], oldW) == &s[i])
        {
            cnt++;
            
            // Jumping to index after the old word.
            i += oldWlen - 1;
        }
    }
    
    // Making new string of enough length
    result = (char *)malloc(i + cnt * (newWlen - oldWlen) + 1);
    
    i = 0;
    while (*s)
    {
        // compare the substring with the result
        if (strstr(s, oldW) == s)
        {
            strcpy(&result[i], newW);
            i += newWlen;
            s += oldWlen;
        }
        else
            result[i++] = *s++;
    }
    
    result[i] = '\0';
    return result;
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

//void exitCtrlCHandler(int s) {
//    showCursor();
//    exit(1);
//}

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

//int unicodeStrlen(char *str) {
//    int i = 0, j = 0;
//    while (str[i]) {
//        if ((str[i] & 0xC0) != 0x80)
//            j++;
//        i++;
//    }
//    return j;
//}

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

enum Colors { RED = 35, GREEN = 32, YELLOW = 33, BLUE = 34, CYAN = 36 } appColor = RED;

void makeBoldColor() {
    char x[100] = {'\0'};
    sprintf(x, "\033[1;%dm", appColor);
    printf("%s", x);
}

void makeColor() {
    char x[100] = {'\0'};
    sprintf(x, "\033[0;%dm", appColor);
    printf("%s", x);
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
    makeBoldColor();
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
    makeBoldColor();
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
    makeBoldColor();
    printStringCentered("Register in CGram");
    printf("\n\n\n");
    printStringCentered("Enter your username below, or type 'return' and then enter to return back:");
    resetFont();
    printf("\n\n");
    printStringCentered(username);
    printf("\n\n");
    makeBoldColor();
    printStringCentered("Enter your password below:");
    resetFont();
    printf("\n\n");
    printStringCentered(password);
    printf("\n\n");
    enum PasswordStatus strength = passwordStatus(password);
    makeColor();
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
    makeBoldColor();
    
    char str1[200] = {'\0'};
    sprintf(str1, "Welcome, %s!", username);
    printStringCentered(str1);
    printf("\n\n\n");
    printStringCentered("Enter the channel you want to join below:");
    printf("\n\n");
    printStringCentered("To create a new channel, type 'new' and then its name after a space.");
    printf("\n\n");
    printStringCentered("To log out from your account, type 'logout'.");
    printf("\n\n");
        printStringCentered("To select more quickly, press CTRL+R for logout.");
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
        
        char *ptr = requestToSend;
        size_t len = sizeof(requestToSend);
        int nread;
        while ((nread = recv(server_socket, ptr, len, 0)) != 0) {
            if (nread < 0) {
                printf("\nSudden Server Disconnection");
//                exit(1);
                break;
            } else {
                return;
            }
            ptr += nread;
            len -= nread;
        }
        
//        while (strcmp(requestToSend, "") == 0) {
//            recv(server_socket, requestToSend, sizeof(requestToSend), 0);
//        }
        
        
//        send(server_socket, requestToSend, sizeof(requestToSend), 0);
        // thiiiiis
//        return;
        
    }
}

int connected = 0; // TODO: FOR SECOND PHASE
int client_socket;

void sendRequestToServer(char *request, char *result) {
    strcpy(originalReq, request);
//    if (!connected) {
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
            connected = 1;
        }
//    }
    
    
    // Function for chat
    chat(client_socket);
    
    strcpy(result, requestToSend);
    
    // TODO: For second phase
    // Close the socket
    shutdown(client_socket, SHUT_RDWR);
}

//void saveTokenAndUsername() {
//    strcpy(token, "");
//    strcpy(username, "");
//    FILE *fptr;
//    char name[] = "token.txt";
//    char secondName[] = "username.txt";
//    fptr = fopen(name, "w");
//    if (fptr != NULL) {
//        fprintf(fptr, "%s\n", token);
//    }
//    fclose(fptr);
//    fptr = fopen(secondName, "w");
//    if (fptr != NULL) {
//        fprintf(fptr, "%s\n", username);
//    }
//    fclose(fptr);
//}
//
//void loadTokenAndUsername() {
//    FILE *fptr;
//    char str[MAX], uStr[MAX];
//    char filename[] = "token.txt";
//    fptr = fopen(filename, "r");
//    if (fptr == NULL) {
//        return;
//    }
//    while (fgets(str, MAX, fptr) != NULL);
//    strcpy(token, str);
//    fclose(fptr);
//    char secondFilename[] = "username.txt";
//    fptr = fopen(secondFilename, "r");
//    if (fptr == NULL) {
//        return;
//    }
//    while (fgets(uStr, MAX, fptr) != NULL);
//    strcpy(username, uStr);
//    fclose(fptr);
//}

int loginServer(const char username[], const char password[]) { // returns -3 if wrong username, -2 if wrong password, -1 if neither
    char req[MAX] = {'\0'}, result[MAX];
    sprintf(req, "login %s, %s", username, password);
    sendRequestToServer(req, result);
    replaceWord(result, "\n", "");
    replaceWord(result, "\t", "");
    if (strcmp(result, "{\"type\":\"Error\",\"content\":\"Username is not valid.\"}") == 0) {
        strcpy(token, "-1");
        return -3;
    } else if (strcmp(result, "{\"type\":\"Error\",\"content\":\"Wrong password.\"}") == 0) {
        strcpy(token, "-1");
        return -2;
    } else if (strstr(result, "AuthToken")) {
        findSubstring(token, result, 31, (int)(strlen(result) - 33));
        return 0;
    } else {
        strcpy(token, "-1");
        return -1;
    }
}

int loginServer_cJSON(const char username[], const char password[]) { // returns -3 if wrong username, -2 if wrong password, -1 if neither
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
    if (strcmp(result, "{\"type\":\"Error\",\"content\":\"this username is not available.\"}") == 0) {
        return -1;
    } else if (strcmp(result, "{\"type\":\"Successful\",\"content\":\"\"}") == 0) {
        return 0;
    } else {
        return -1;
    }
}

int registerServer_cJSON(char username[], char password[]) { // returns -1 if error
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
    char req[MAX] = {'\0'}, result[MAX] = {'\0'};
    sprintf(req, "create channel %s, %s", channel, token);
    sendRequestToServer(req, result);
    if (strcmp(result, "{\"type\":\"Error\",\"content\":\"Channel name is not available.\"}") == 0) {
        return -1;
    } else if (strcmp(result, "{\"type\":\"Error\",\"content\":\"You are in another channel.\"}") == 0) {
        return -1;
    } else if (strcmp(result, "{\"type\":\"Successful\",\"content\":\"\"}") == 0) {
        return 0;
    } else {
        return -1;
    }
}

int createChannelServer_cJSON() { // returns -1 if error
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
    if (strcmp(result, "{\"type\":\"Error\",\"content\":\"You are in another channel.\"}") == 0) {
        return -1;
    } else if (strcmp(result, "{\"type\":\"Error\",\"content\":\"Channel not found.\"}") == 0) {
        return -1;
    } else if (strcmp(result, "{\"type\":\"Successful\",\"content\":\"\"}") == 0) {
        return 0;
    } else {
        return -1;
    }
}

int findChannelServer_cJSON() { // returns -1 if error
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
    if (strcmp(result, "{\"type\":\"Error\",\"content\":\"Authentication failed!\"}") == 0) {
        return -1;
    } else if (strcmp(result, "{\"type\":\"Successful\",\"content\":\"\"}") == 0) {
        return 0;
    } else {
        return -1;
    }
}

int logoutServer_cJSON() { // returns -1 if error
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

// TODO: No error handling here
int reloadMembersServer() { // returns -1 if error
    for (int i = 0; i < membersCount; i++) {
        strcpy(members[i].name, "");
    }
    membersCount = 0;
    char req[MAX] = {'\0'}, result[MAX];
    sprintf(req, "channel members %s", token);
    sendRequestToServer(req, result);
    char line[MAX] = {}, *subString;
    strcpy(line, result);
    subString = strtok(line, "\"");
    subString = strtok(NULL, "\"");
    subString = strtok(NULL, "\"");
    subString = strtok(NULL, "\"");
    subString = strtok(NULL, "\"");
    subString = strtok(NULL, "\"");
    subString = strtok(NULL, "\"");
    subString = strtok(NULL, "\"");
    if (subString) {
        strcpy(members[membersCount].name, subString);
        membersCount++;
    } else {
        return -1;
    }
    for (int i = 1; i < MAX; i++) {
        subString = strtok(NULL, "\"");
        subString = strtok(NULL, "\"");
        if (subString) {
            strcpy(members[membersCount].name, subString);
            membersCount++;
        } else {
            break;
        }
    }
    return 1;
}

int reloadMembersServer_cJSON() { // returns -1 if error
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

int searchMessageServer(char *word) { // returns -1 if error
//    char req[MAX] = {'\0'}, result[MAX];
//    sprintf(req, "searchBetweenMessages %s, %s", word, token);
//    sendRequestToServer(req, result);
//    if (strcmp(result, "{\"type\":\"List\",\"content\":[]}") == 0) {
//        return -1;
//    }
//    messagesCount = 0;
//    char line[MAX] = {}, *subString;
//    strcpy(line, result);
//    subString = strtok(line, "\"");
//    subString = strtok(NULL, "\"");
//    subString = strtok(NULL, "\"");
//    subString = strtok(NULL, "\"");
//    subString = strtok(NULL, "\"");
//    subString = strtok(NULL, "\"");
//    // type, List, content == up to here
//    subString = strtok(NULL, "\"");
//    subString = strtok(NULL, "\"");
//    subString = strtok(NULL, "\"");
//    subString = strtok(NULL, "\""); // sender got
//    if (subString) {
//        strcpy(messages[messagesCount].sender, subString);
//    } else {
//        return 0;
//    }
//    subString = strtok(NULL, "\"");
//    subString = strtok(NULL, "\"");
//    subString = strtok(NULL, "\"");
//    subString = strtok(NULL, "\""); // content got
//    if (subString) {
//        strcpy(messages[messagesCount].content, subString);
//        messagesCount++;
//    } else {
//        return 0;
//    }
//    for (int i = 1; i < MAX; i++) {
//        subString = strtok(NULL, "\"");
//        subString = strtok(NULL, "\"");
//        subString = strtok(NULL, "\"");
//        subString = strtok(NULL, "\""); // sender got
//        if (subString) {
//            strcpy(messages[messagesCount].sender, subString);
//        } else {
//            break;
//        }
//        subString = strtok(NULL, "\"");
//        subString = strtok(NULL, "\"");
//        subString = strtok(NULL, "\"");
//        subString = strtok(NULL, "\""); // content got
//        if (subString) {
//            strcpy(messages[messagesCount].content, subString);
//            messagesCount++;
//        } else {
//            break;
//        }
//    }
//    return 0;
    
    Message copyMessages[200];
    for (int i = 0; i < 200; i++) {
        strcpy(copyMessages[i].sender, messages[i].sender);
        strcpy(copyMessages[i].content, messages[i].content);
    }
    messagesCount = 0;
    for (int i = 0; i < 200; i++) {
        char newString[1000][1000];
        int c = -1;
        splitString(copyMessages[i].content, newString, &c);
        if (c < 1) {
            continue;
        }
        for (int j = 0; j < c; j++) {
            if (strcmp(newString[j], word) == 0) {
                strcpy(messages[messagesCount].sender, copyMessages[i].sender);
                strcpy(messages[messagesCount].content, copyMessages[i].content);
                messagesCount += 1;
            }
        }
    }
    return 0;
}

// TODO: No error handling here at all!
int reloadMessagesServer() { // returns -1 if error
    char req[MAX] = {'\0'}, result[MAX];
    sprintf(req, "refresh %s", token);
    sendRequestToServer(req, result);
    messagesCount = 0;
    char line[MAX] = {}, *subString;
    strcpy(line, result);
    subString = strtok(line, "\"");
    subString = strtok(NULL, "\"");
    subString = strtok(NULL, "\"");
    subString = strtok(NULL, "\"");
    subString = strtok(NULL, "\"");
    subString = strtok(NULL, "\"");
    // type, List, content == up to here
    subString = strtok(NULL, "\"");
    subString = strtok(NULL, "\"");
    subString = strtok(NULL, "\"");
    subString = strtok(NULL, "\""); // sender got
    if (subString) {
        strcpy(messages[messagesCount].sender, subString);
    } else {
        return 0;
    }
    subString = strtok(NULL, "\"");
    subString = strtok(NULL, "\"");
    subString = strtok(NULL, "\"");
    subString = strtok(NULL, "\""); // content got
    if (subString) {
        strcpy(messages[messagesCount].content, subString);
        messagesCount++;
    } else {
        return 0;
    }
    for (int i = 1; i < MAX; i++) {
        subString = strtok(NULL, "\"");
        subString = strtok(NULL, "\"");
        subString = strtok(NULL, "\"");
        subString = strtok(NULL, "\""); // sender got
        if (subString) {
            strcpy(messages[messagesCount].sender, subString);
        } else {
            break;
        }
        subString = strtok(NULL, "\"");
        subString = strtok(NULL, "\"");
        subString = strtok(NULL, "\"");
        subString = strtok(NULL, "\""); // content got
        if (subString) {
            strcpy(messages[messagesCount].content, subString);
            messagesCount++;
        } else {
            break;
        }
    }
    return 0;
}

int reloadMessagesServer_cJSON() { // returns -1 if error
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
             int i = 0;
             messagesCount = 0;
             cJSON_ArrayForEach(eachMessage, content) {
                 const cJSON *eachMessageSender = cJSON_GetObjectItemCaseSensitive(eachMessage, "sender");
                 const cJSON *eachMessageContent = cJSON_GetObjectItemCaseSensitive(eachMessage, "content");
                 if (cJSON_IsString(eachMessageSender) && (eachMessageSender->valuestring != NULL)
                     &&
                     cJSON_IsString(eachMessageContent) && (eachMessageContent->valuestring != NULL)) {
                     Message m;
                     strcpy(m.sender, eachMessageSender->valuestring);
                     strcpy(m.content, eachMessageContent->valuestring);
                     messages[i] = m;
                     i++;
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
    if (strcmp(result, "{\"type\":\"Successful\",\"content\":\"\"}") == 0) {
        return 0;
    } else {
        return -1;
    }
}

int sendMessageServer_cJSON(char message[]) { // returns -1 if error
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

int searchMemberServer(char *memberName) { // returns -1 if error
    char req[MAX] = {'\0'}, result[MAX];
    sprintf(req, "searchBetweenMembers %s, %s", memberName, token);
    sendRequestToServer(req, result);
    if (strcmp(result, "{\"type\":\"Error\",\"content\":\"Error\"}") == 0) {
        return -1;
    } else if (strcmp(result, "{\"type\":\"Successful\",\"content\":\"\"}") == 0) {
        return 0;
    } else {
        return -1;
    }
}

int leaveChannelServer() { // returns -1 if error
    char req[MAX] = {'\0'}, result[MAX];
    sprintf(req, "leave %s", token);
    sendRequestToServer(req, result);
    if (strcmp(result, "{\"type\":\"Error\",\"content\":\"You aren\'t in any channel\"}") == 0) {
        return -1;
    } else if (strcmp(result, "{\"type\":\"Successful\",\"content\":\"\"}") == 0) {
        return 0;
    } else {
        return -1;
    }
}

int leaveChannelServer_cJSON() { // returns -1 if error
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
    makeBoldColor();
    printStringCentered(channel);
    resetFont();
    printf("\n\n");
    makeColor();
    printStringCentered("Messages sent in this channel are shown here.");
    printf("\n");
    printStringCentered("Type a new message and press enter to send it.");
    printf("\n");
    printStringCentered("To see messages including a certain word, type 'find'.");
    printf("\n");
    printStringCentered("To see all messages again, simply type 'reload'.");
    printf("\n");
    printStringCentered("To see the name of other members of this channel, type 'members'.");
    printf("\n");
    printStringCentered("To search the channel for a member, type 'search_member'.");
    printf("\n");
    printStringCentered("To leave this channel, type 'leave'");
    resetFont();
    printf("\n\n");
    for (int i = 0; i < messagesCount; i++) {
        Message m = messages[i];
        makeColor();
        printf("%s\t\t", m.sender);
        resetFont();
        printf("%s\n", m.content);
    }
    showCursor();
    makeColor();
    printf("\n\nType your message ==> ");
    resetFont();
    char s[200] = {'\0'};
    fgets(s, 200, stdin);
    printf("\n\n");
    hideCursor();
    if (strcmp(s, "") == 0 || strcmp(s, "\n") == 0) {
        chatPage();
        return;
    } else if (strcmp(s, "find\n") == 0) {
        printf("\nEnter the word to search for ==> ");
        char word[200] = {};
        scanf("%s", word);
        printf("\n\n");
        int result = searchMessageServer(word);
        if (result == -1) {
            makeBoldColor();
            printStringCentered("No message found. Press 'r' or CTRL+R to return.");
            resetFont();
            while (1) {
                char c = getch();
                if (c == 18) {
//                    getch();
//                    switch (getch()) {
//                        case 'A':
                            chatPage();
                            return;
//                            break;
//                        default:
//                            break;
//                    }
                } else if (c == 'r' || c == 'R') {
                    chatPage();
                    return;
                }
            }
        } else {
            chatPage();
            return;
        }
    } else if (strcmp(s, "search_member\n") == 0) {
        printf("\nEnter the name of the member to search for ==> ");
        char memberName[200] = {};
        scanf("%s", memberName);
        printf("\n\n");
        int result = searchMemberServer(memberName);
        if (result == -1) {
            makeBoldColor();
            printStringCentered("No such user found. Press 'r' or CTRL+R to return.");
            resetFont();
            while (1) {
                char c = getch();
                if (c == 18) {
//                    getch();
//                    switch (getch()) {
//                        case 'A':
                            chatPage();
                            return;
//                            break;
//                        default:
//                            break;
//                    }
                } else if (c == 'r' || c == 'R') {
                    chatPage();
                    return;
                }
            }
        } else {
            makeBoldColor();
            printStringCentered("User is a member of this channel. Press 'r' or CTRL+R to return.");
            resetFont();
            while (1) {
                char c = getch();
                if (c == 18) {
//                    getch();
//                    switch (getch()) {
//                        case 'A':
                            chatPage();
                            return;
//                            break;
//                        default:
//                            break;
//                    }
                } else if (c == 'r' || c == 'R') {
                    chatPage();
                    return;
                }
            }
        }
    } else if (strcmp(s, "reload\n") == 0) {
        makeBoldColor();
        printStringCentered("Reloading the messages...");
        resetFont();
        printf("\n\n");
        int result = reloadMessagesServer();
        if (result == -1) {
            makeBoldColor();
            printStringCentered("Reloading failed. Press 'r' or CTRL+R to return.");
            resetFont();
            while (1) {
                char c = getch();
                if (c == 18) {
//                    getch();
//                    switch (getch()) {
//                        case 'A':
                            chatPage();
                            return;
//                            break;
//                        default:
//                            break;
//                    }
                } else if (c == 'r' || c == 'R') {
                    chatPage();
                    return;
                }
            }
        } else {
            chatPage();
            return;
        }
    } else if (strcmp(s, "members\n") == 0) {
        makeBoldColor();
        printStringCentered("Finding all the members...");
        resetFont();
        printf("\n\n");
        int result = reloadMembersServer();
        if (result == -1) {
            makeBoldColor();
            printStringCentered("Finding failed. Press 'r' or CTRL+R to return.");
            resetFont();
            while (1) {
                char c = getch();
                if (c == 18) {
//                    getch();
//                    switch (getch()) {
//                        case 'A':
                            chatPage();
                            return;
//                            break;
//                        default:
//                            break;
//                    }
                } else if (c == 'r' || c == 'R') {
                    chatPage();
                    return;
                }
            }
        } else {
            makeBoldColor();
            printStringCentered("All Channel Members:");
            resetFont();
            printf("\n\n");
            for (int i = 0; i < membersCount; i++) {
                printStringCentered(members[i].name);
                printf("\n");
            }
            printf("\n\n");
            makeBoldColor();
            printStringCentered("Press 'r' or CTRL+R to return.");
            resetFont();
            while (1) {
                char c = getch();
                if (c == 18) {
//                    getch();
//                    switch (getch()) {
//                        case 'A':
                            chatPage();
                            return;
//                            break;
//                        default:
//                            break;
//                    }
                } else if (c == 'r' || c == 'R') {
                    chatPage();
                    return;
                }
            }
        }
    } else if (strcmp(s, "leave\n") == 0) {
        makeBoldColor();
        printStringCentered("Leaving the channel...");
        resetFont();
        printf("\n\n");
        int result = leaveChannelServer();
        if (result == -1) {
            makeBoldColor();
            printStringCentered("Leaving failed. Press 'r' or CTRL+R to return.");
            resetFont();
            while (1) {
                char c = getch();
                if (c == 18) {
//                    getch();
//                    switch (getch()) {
//                        case 'A':
                            chatPage();
                            return;
//                            break;
//                        default:
//                            break;
//                    }
                } else if (c == 'r' || c == 'R') {
                    chatPage();
                    return;
                }
            }
        } else {
            mainPage();
            return;
        }
    } else {
        makeBoldColor();
        printStringCentered("Sending the message...");
        resetFont();
        printf("\n\n");
        int result1 = sendMessageServer(s);
        int result2 = reloadMessagesServer();
        if (result1 == -1 || result2 == -1) {
            makeBoldColor();
            printStringCentered("Sending failed. Press 'r' or CTRL+R to return.");
            resetFont();
            while (1) {
                char c = getch();
                if (c == 18) {
//                    getch();
//                    switch (getch()) {
//                        case 'A':
                            chatPage();
                            return;
//                            break;
//                        default:
//                            break;
//                    }
                } else if (c == 'r' || c == 'R') {
                    chatPage();
                    return;
                }
            }
        } else {
            chatPage();
            return;
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
    makeBoldColor();
    
    char str1[200] = {'\0'};
    sprintf(str1, "Welcome, %s!", username);
    printStringCentered(str1);
    printf("\n\n\n");
    printStringCentered("Enter the channel you want to join below:");
    printf("\n\n");
    printStringCentered("To create a new channel, type 'new' and then its name after a space.");
    printf("\n\n");
    printStringCentered("To log out from your account, type 'logout'.");
    printf("\n\n");
    printStringCentered("To select more quickly, press CTRL+R for logout.");
    printf("\n\n\n");
    resetFont();
    int shouldContinue = 1;
    while (shouldContinue) {
        char c = getch();
        if (c == 18) {
                        printf("\n\n");
                        makeBoldColor();
                        printStringCentered("Logging out...");
                        resetFont();
                        printf("\n\n");
                        int res = logoutServer();
                        if (res == -1) {
                            makeBoldColor();
                            printStringCentered("Logout failed. Press 't' or CTRL+R to retry.");
                            resetFont();
                            while (1) {
                                char c = getch();
                                if (c == 18) {
//                                    getch();
//                                    switch (getch()) {
//                                        case 'A':
                                            chatPage();
                                            return;
//                                            break;
//                                        default:
//                                            break;
//                                    }
                                } else if (c == 't' || c == 'T') {
                                    mainPage();
                                    return;
                                }
                            }
                        } else {
                            shouldContinue = 0;
                            username[0] = '\0';
                            token[0] = '\0';
                            loginPage();
                            return;
                        }
        } else if (c == '\n') {
            if (strcmp(channel, "logout") == 0) {
                printf("\n\n");
                makeBoldColor();
                printStringCentered("Logging out...");
                resetFont();
                printf("\n\n");
                int res = logoutServer();
                if (res == -1) {
                    makeBoldColor();
                    printStringCentered("Logout failed. Press 't' or CTRL+R to retry.");
                    resetFont();
                    while (1) {
                        char c = getch();
                        if (c == 18) {
//                            getch();
//                            switch (getch()) {
//                                case 'A':
                                    chatPage();
                                    return;
//                                    break;
//                                default:
//                                    break;
//                            }
                        } else if (c == 't' || c == 'T') {
                            mainPage();
                            return;
                        }
                    }
                } else {
                    shouldContinue = 0;
                    username[0] = '\0';
                    token[0] = '\0';
                    loginPage();
                    return;
                }
                // TODO: Possible bug here due to \xff-ing...
            } else if (strStartsWith("new ", channel) || strStartsWith("\xffnew ", channel) || strStartsWith("\xff\xffnew ", channel) || strStartsWith("\xff\xff\xffnew ", channel) || strStartsWith("\xff\xff\xff\xffnew ", channel) || strStartsWith("\xff\xff\xff\xff\xffnew ", channel) || strStartsWith("\xff\xff\xff\xff\xff\xffnew ", channel) || strStartsWith("\xff\xff\xff\xff\xff\xff\xffnew ", channel)) {
                printf("\n\n");
                char target[10][200];
                getWords(channel, target);
                strcpy(channel, target[1]);
                if (strcmp(channel, "") == 0) {
                    mainPage();
                    return;
                }
                makeBoldColor();
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
                    makeBoldColor();
                    printStringCentered("Channel name exists. Press 't' or CTRL+R to retry.");
                    resetFont();
                    while (1) {
                        char c = getch();
                        if (c == 18) {
//                            getch();
//                            switch (getch()) {
//                                case 'A':
                                    chatPage();
//                                    return;
//                                    break;
//                                default:
//                                    break;
//                            }
                        } else if (c == 't' || c == 'T') {
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
    
    makeBoldColor();
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
        makeBoldColor();
        printStringCentered("The channel doesn't exist. Press 't' or CTRL+R to retry.");
        resetFont();
        while (1) {
            char c = getch();
            if (c == 18) {
//                getch();
//                switch (getch()) {
//                    case 'A':
                        chatPage();
                        return;
//                        break;
//                    default:
//                        break;
//                }
            } else if (c == 't' || c == 'T') {
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
    makeBoldColor();
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
        makeBoldColor();
        printStringCentered("Invalid username. Press 't' to retry or 'r' to return.");
        resetFont();
        while (1) {
            char c = getch();
            if (c == 't' || c == 'T') {
                registerPage();
                return;
            } else if (c == 'r' || c == 'R') {
                welcomePage();
                return;
            }
        }
    }
    
    makeBoldColor();
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
        makeBoldColor();
        printStringCentered("Invalid password. Press 't' to retry or 'r' to return.");
        resetFont();
        while (1) {
            char c = getch();
            if (c == 't' || c == 'T') {
                registerPage();
                return;
            } else if (c == 'r' || c == 'R') {
                welcomePage();
                return;
            }
        }
    }
    
    makeBoldColor();
    // TODO: Check spelling of Registering!!
    printStringCentered("Registering...");
    resetFont();
    printf("\n\n\n");
    
    int result = registerServer(username, password);
    
    if (result == -1) {
        makeBoldColor();
        printStringCentered("Username is not available. Press 't' to retry or 'r' to return.");
        resetFont();
        while (1) {
            char c = getch();
            if (c == 't' || c == 'T') {
                registerPage();
                return;
            } else if (c == 'r' || c == 'R') {
                welcomePage();
                return;
            }
        }
    } else {
//        strcpy(token, "");
//        strcpy(username, "");
//        saveTokenAndUsername();
        makeBoldColor();
        printStringCentered("Registration successful! Press any key to return.");
        resetFont();
        getch();
        welcomePage();
        return;
    }
    
}

void loginPage() {
    if (strcmp(token, "") != 0 && strcmp(token, "-1") != 0) {
        mainPage();
        return;
    }
    char x[100] = {'\0'};
    strcpy(username, x);
    // TODO: h
    system("clear");
    hideCursor();
    printf("\n\n\n\n\n");
    makeBoldColor();
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
        makeBoldColor();
        printStringCentered("Invalid username. Press 't' to retry or 'r' to return.");
        resetFont();
        while (1) {
            char c = getch();
            if (c == 't' || c == 'T') {
                loginPage();
                return;
            } else if (c == 'r' || c == 'R') {
                welcomePage();
                return;
            }
        }
    }
    
    makeBoldColor();
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
        makeBoldColor();
        printStringCentered("Invalid password. Press 't' to retry or 'r' to return.");
        resetFont();
        while (1) {
            char c = getch();
            if (c == 't' || c == 'T') {
                loginPage();
                return;
            } else if (c == 'r' || c == 'R') {
                welcomePage();
                return;
            }
        }
    }
    
    makeBoldColor();
    printStringCentered("Logging in...");
    resetFont();
    printf("\n\n\n");
    
    int res = loginServer(username, password);
    
    if (res == -2) {
        makeBoldColor();
        printStringCentered("Wrong password. Press 't' to retry or 'r' to return.");
        resetFont();
        while (1) {
            char c = getch();
            if (c == 't' || c == 'T') {
                loginPage();
                return;
            } else if (c == 'r' || c == 'R') {
                welcomePage();
                return;
            }
        }
    } else if (res == -3) {
        makeBoldColor();
        printStringCentered("Username does not exist. Press 't' to retry or 'r' to return.");
        resetFont();
        while (1) {
            char c = getch();
            if (c == 't' || c == 'T') {
                loginPage();
                return;
            } else if (c == 'r' || c == 'R') {
                welcomePage();
                return;
            }
        }
    }
    
    if (strcmp(token, "-1") == 0 || res == -1) {
        makeBoldColor();
        printStringCentered("Login failed. Press 't' to retry or 'r' to return.");
        resetFont();
        while (1) {
            char c = getch();
            if (c == 't' || c == 'T') {
                loginPage();
                return;
            } else if (c == 'r' || c == 'R') {
                welcomePage();
                return;
            }
        }
    } else {
//        saveTokenAndUsername();
        mainPage();
    }
    
}

void colorChangePage() {
    // TODO: h
    system("clear");
    hideCursor();
    makeBoldColor();
    printf("\n\n");
    printStringCentered("Press the first letter of the given colors to change CGram's color scheme.");
    resetFont();
    printf("\n\n\n");
    printf("");
    appColor = RED;
    makeBoldColor();
    printStringCentered("Red - Press 'r'");
    resetFont();
    printf("\n\n");
    appColor = GREEN;
    makeBoldColor();
    printStringCentered("Green - Press 'g'");
    resetFont();
    printf("\n\n");
    appColor = YELLOW;
    makeBoldColor();
    printStringCentered("Yellow - Press 'y'");
    resetFont();
    printf("\n\n");
    appColor = BLUE;
    makeBoldColor();
    printStringCentered("Blue - Press 'b'");
    resetFont();
    printf("\n\n");
    appColor = CYAN;
    makeBoldColor();
    printStringCentered("Cyan - Press 'c'");
    resetFont();
    printf("\n\n");
    FILE *fptr;
    char name[] = "appcolor.txt";
    fptr = fopen(name, "w");
    int shouldContinue = 1;
    while (shouldContinue) {
        char c = getch();
        if (c == 'r' || c == 'R') {
            appColor = RED;
            if (fptr != NULL) {
                fprintf(fptr, "r");
            }
            fclose(fptr);
            shouldContinue = 0;
            welcomePage();
            return;
        } else if (c == 'g' || c == 'G') {
            appColor = GREEN;
            if (fptr != NULL) {
                fprintf(fptr, "g");
            }
            fclose(fptr);
            shouldContinue = 0;
            welcomePage();
            return;
        } else if (c == 'y' || c == 'Y') {
            appColor = YELLOW;
            if (fptr != NULL) {
                fprintf(fptr, "y");
            }
            fclose(fptr);
            shouldContinue = 0;
            welcomePage();
            return;
        } else if (c == 'b' || c == 'B') {
            appColor = BLUE;
            if (fptr != NULL) {
                fprintf(fptr, "b");
            }
            fclose(fptr);
            shouldContinue = 0;
            welcomePage();
            return;
        } else if (c == 'c' || c == 'C') {
            appColor = CYAN;
            if (fptr != NULL) {
                fprintf(fptr, "c");
            }
            fclose(fptr);
            shouldContinue = 0;
            welcomePage();
            return;
        }
    }
}

void welcomePage() {
    // TODO: h
    system("clear");
    hideCursor();
    makeBoldColor();
    printf("\n\n");
    for (int i = 0; i < 5; i++) {
        printf("%s", asciiArt[i]);
    }
    printf("\n\n\n");
    printStringCentered("Welcome to CGram");
    printf("\n\n\n");
    printStringCentered("The most advanced messaging app designed ever in pure C");
    printf("\n\n\n");
    makeColor();
    printStringCentered("Designed and developed by Seyed Parsa Neshaei");
    resetFont();
    printf("\n\n\n\n");
    printStringCentered("Press 'l' on your keyboard to log in quickly, or 'q' to exit CGram.");
    printf("\n\n");
    printStringCentered("If you don't have an account, just press 'r' to register in a snap!");
    printf("\n\n");
    printStringCentered("To adjust CGram's color scheme, press 'c'.");
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
                    if (r == 'y' || r == 'Y') {
                        // TODO: Here
                        system("clear");//system("@cls||clear");
                        printf("\n");
                        makeBoldColor();
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
            case 'c':
            case 'C':
                shouldContinue = 0;
                colorChangePage();
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

void initializeAppColor() {
    FILE *fptr;
    char filename[] = "appcolor.txt";
    fptr = fopen(filename, "r");
    if (fptr == NULL) {
        appColor = RED;
        return;
    }
    char ch = fgetc(fptr);
    if (ch == EOF) {
        appColor = RED;
        return;
    }
    switch (ch) {
        case 'r':
            appColor = RED;
            break;
        case 'g':
            appColor = GREEN;
            break;
        case 'y':
            appColor = YELLOW;
            break;
        case 'b':
            appColor = BLUE;
            break;
        case 'c':
            appColor = CYAN;
            break;
        default:
            appColor = RED;
            break;
    }
    fclose(fptr);
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

void commandLineLogin(const char *_username, const char *password) {
    // TODO: h
    system("clear");
    hideCursor();
    makeBoldColor();
    printf("\n\n");
    printStringCentered("Welcome to CGram");
    printf("\n\n\n");
    printStringCentered("Logging in...");
    resetFont();
    printf("\n\n\n");
    
    strcpy(username, _username);
    int res = loginServer(username, password);
    
    if (res == -2) {
        makeBoldColor();
        printStringCentered("Wrong password. Press 'm' to go to the main page or 'q' to quit.");
        resetFont();
        while (1) {
            char c = getch();
            if (c == 'm' || c == 'M') {
                welcomePage();
                return;
            } else if (c == 'q') {
                // TODO: Here
                system("clear");//system("@cls||clear");
                printf("\n");
                makeBoldColor();
                printStringCentered("Thank you for using CGram!");
                resetFont();
                printf("\n\n");
                exit(0);
            }
        }
    } else if (res == -3) {
        makeBoldColor();
        printStringCentered("Username does not exist. Press 'm' to go to the main page or 'q' to quit.");
        resetFont();
        while (1) {
            char c = getch();
            if (c == 'm' || c == 'M') {
                welcomePage();
                return;
            } else if (c == 'q') {
                // TODO: Here
                system("clear");//system("@cls||clear");
                printf("\n");
                makeBoldColor();
                printStringCentered("Thank you for using CGram!");
                resetFont();
                printf("\n\n");
                exit(0);
            }
        }
    }
    
    if (strcmp(token, "-1") == 0 || res == -1) {
        makeBoldColor();
        printStringCentered("Login failed. Press 'm' to go to the main page or 'q' to quit.");
        resetFont();
        while (1) {
            char c = getch();
            if (c == 'm' || c == 'M') {
                welcomePage();
                return;
            } else if (c == 'q') {
                // TODO: Here
                system("clear");//system("@cls||clear");
                printf("\n");
                makeBoldColor();
                printStringCentered("Thank you for using CGram!");
                resetFont();
                printf("\n\n");
                exit(0);
            }
        }
    } else {
        mainPage();
    }
}

void commandLineInvalid() {
    printf("\n\n");
    makeBoldColor();
    printStringCentered("Invalid arguments passed to CGram. Use --help to get help.");
    resetFont();
    printf("\n\n");
}

void commandLineHelp() {
    printf("\n\n");
    makeBoldColor();
    // TODO: Check if 1) should the beginning of Help be capitialized and 2) what CLI stands for and...!
    printStringCentered("CGram CLI Help");
    resetFont();
    printf("\n\n\n");
    makeColor();
    printStringCentered("Open CGram without arguments to go to the welcome page.");
    printf("\n\n");
    printStringCentered("Pass username and password to login quickly.");
    printf("\n\n");
    printStringCentered("Pass '--register' to quickly go to the register form.");
    printf("\n\n");
    printStringCentered("Pass '--color' to quickly change CGram's color scheme.");
    printf("\n\n");
    printStringCentered("Pass --help to see this guide.");
    printf("\n\n\n");
    resetFont();
}

int main (int argc, const char * argv[]) {
    // Done Feature: command line arguments to facilitate using app
    // Done Feature: Password strength
    
    // IDEA: Activity Indicator
    // IDEA: App sketches
    // IDEA: Auto replace emojis
    // Done Feature: Change app color
    // Done Feature: Save app color to file
    
    // FEATURE: esc exits app
    
    // BUG: Uncentered in color chooses screen
    // BUG: Not very good password strength finder!
    
    setupTerminalDimensions();
    atexit(showCursor);
    initializeAsciiArt();
    initializeAppColor();
//    loadTokenAndUsername();
    
    switch (argc) {
        case 0:
        case 1:
            welcomePage();
            break;
        case 2:
            if (strcmp(argv[1], "--help") == 0) {
                commandLineHelp();
            } else if (strcmp(argv[1], "--register") == 0) {
                registerPage();
            } else if (strcmp(argv[1], "--color") == 0) {
                colorChangePage();
            } else {
                commandLineInvalid();
            }
            break;
        case 3:
            commandLineLogin(argv[1], argv[2]);
            break;
     
        default:
            break;
    }
    
    return 0;
}
