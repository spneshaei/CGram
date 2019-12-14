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

#define cursorforward(x) printf("\033[%dC", (x))
#define cursorbackward(x) printf("\033[%dD", (x))

#define KEY_ESCAPE  0x001b
#define KEY_ENTER   0x000a
#define KEY_UP      0x0105
#define KEY_DOWN    0x0106
#define KEY_LEFT    0x0107
#define KEY_RIGHT   0x0108

void welcomePage(void);
void loginPage(void);
void mainPage(void);
void channelPage(void);

char username[100];
char token[200];
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
static int kbhit(void);
static int kbesc(void);
static int kbget(void);

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
    return c;
}

static int kbhit(){
    int c = 0;
    
    tcgetattr(0, &oterm);
    memcpy(&term, &oterm, sizeof(term));
    term.c_lflag &= ~(ICANON | ECHO);
    term.c_cc[VMIN] = 0;
    term.c_cc[VTIME] = 1;
    tcsetattr(0, TCSANOW, &term);
    c = getchar();
    tcsetattr(0, TCSANOW, &oterm);
    if (c != -1) ungetc(c, stdin);
    return ((c != -1) ? 1 : 0);
}

static int kbesc(){
    int c;
    
    if (!kbhit()) return KEY_ESCAPE;
    c = getch();
    if (c == '[') {
        switch (getch()) {
            case 'A':
                c = KEY_UP;
                break;
            case 'B':
                c = KEY_DOWN;
                break;
            case 'C':
                c = KEY_LEFT;
                break;
            case 'D':
                c = KEY_RIGHT;
                break;
            default:
                c = 0;
                break;
        }
    } else {
        c = 0;
    }
    if (c == 0) while (kbhit()) getch();
    return c;
}

static int kbget(){
    int c;
    c = getch();
    return (c == KEY_ESCAPE) ? kbesc() : c;
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

void loginPage_printUntilSpecifiedUsername(char *username) {
    system("clear");
    hideCursor();
    printf("\n\n\n\n\n");
    makeBoldRed();
    printStringCentered("Login to CGram");
    resetFont();
    printf("\n\n\n");
    printStringCentered("Enter your username below, or type 'return' and then enter to return back:");
    printf("\n\n");
    printStringCentered(username);
}

void registerPage_printUntilSpecifiedUsername(char *username) {
    system("clear");
    hideCursor();
    printf("\n\n\n\n\n");
    makeBoldRed();
    printStringCentered("Register in CGram");
    resetFont();
    printf("\n\n\n");
    printStringCentered("Enter your username below, or type 'return' and then enter to return back:");
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
    resetFont();
    printf("\n\n\n");
    printStringCentered("Enter the channel you want to join below:");
    printf("\n\n");
    printStringCentered("To create a new channel, type 'new' and then its name after a space.");
    printf("\n\n");
    printStringCentered("To log out from your account, type 'logout'.");
    printf("\n\n\n");
    printStringCentered(channel);
}

void loginServer(char username[], char password[], char *token) { // returns token and -1 if error
    // TODO: Server login
    strcpy(token, "0");
}

int registerServer(char username[], char password[]) { // returns -1 if error
    // TODO: Server register
    return -1;
}

int createChannelServer() { // returns -1 if error
    // TODO: Channel creation
    return -1;
}

int findChannelServer() { // returns -1 if error
    // TODO: Channel finding
    return -1;
}

int logoutServer() { // returns -1 if error
    // TODO: Logging out
    return -1;
}

int reloadMembersServer() { // returns -1 if error
    // TODO: members
    return -1;
}

int reloadMessagesServer() { // returns -1 if error
    // TODO: refreshing
    return -1;
}

int sendMessageServer(char message[]) { // returns -1 if error
    // TODO: sending
    return -1;
}

int leaveChannelServer() { // returns -1 if error
    // TODO: leaving
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
        printStringCentered("Reloading the messages...");
        printf("\n\n");
        int result = reloadMessagesServer();
        if (result == -1) {
            printStringCentered("Reloading failed. Press 'r' to return.");
            char c = getch();
            if (c == 'r') {
                chatPage();
                return;
            }
        } else {
            printStringCentered("Reloading successful. Press 's' to show new messages.");
            char c = getch();
            if (c == 's') {
                chatPage();
                return;
            }
        }
    } else if (strcmp(s, "members") == 0) {
        printStringCentered("Finding all the members...");
        printf("\n\n");
        int result = reloadMembersServer();
        if (result == -1) {
            printStringCentered("Finding failed. Press 'r' to return.");
            char c = getch();
            if (c == 'r') {
                chatPage();
                return;
            }
        } else {
            printStringCentered("All Channel Members:");
            printf("\n\n");
            for (int i = 0; i < membersCount; i++) {
                if (i % 2 == 0) {
                    printf("%s\t", members[i].name);
                } else {
                    printf("%s\n", members[i].name);
                }
            }
            printf("\n\n");
            printStringCentered("Press 'r' to return.");
            char c = getch();
            if (c == 'r') {
                chatPage();
                return;
            }
        }
    } else if (strcmp(s, "leave") == 0) {
        printStringCentered("Leaving the channel...");
        printf("\n\n");
        int result = leaveChannelServer();
        if (result == -1) {
            printStringCentered("Leaving failed. Press 'r' to return.");
            char c = getch();
            if (c == 'r') {
                chatPage();
                return;
            }
        } else {
            mainPage();
            return;
        }
    }else {
        printStringCentered("Sending the message...");
        printf("\n\n");
        int result1 = sendMessageServer(s);
        int result2 = reloadMessagesServer();
        if (result1 == -1 || result2 == -1) {
            printStringCentered("Sending failed. Press 'r' to return.");
            char c = getch();
            if (c == 'r') {
                chatPage();
                return;
            }
        } else {
            printStringCentered("Message sent. Press 'r' to return.");
            char c = getch();
            if (c == 'r') {
                chatPage();
                return;
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
    resetFont();
    printf("\n\n\n");
    printStringCentered("Enter the channel you want to join below:");
    printf("\n\n");
    printStringCentered("To create a new channel, type 'new' and then its name after a space.");
    printf("\n\n");
    printStringCentered("To log out from your account, type 'logout'.");
    printf("\n\n\n");
    int shouldContinue = 1;
    while (shouldContinue) {
        char c = getch();
        if (c == '\n') {
            if (strcmp(channel, "logout") == 0) {
                printStringCentered("Logging out...");
                printf("\n\n");
                int res = logoutServer();
                if (res == -1) {
                    printStringCentered("Logout failed. Press 't' to retry.");
                    char c = getch();
                    if (c == 't') {
                        mainPage();
                        return;
                    }
                } else {
                    shouldContinue = 0;
                    username[0] = '\0';
                    loginPage();
                    return;
                }
            } else if (strStartsWith("new ", channel)) {
                char target[10][200];
                getWords(channel, target);
                strcpy(channel, target[1]);
                if (strcmp(channel, "") == 0) {
                    printStringCentered("Invalid channel name. Press 't' to retry.");
                    char c = getch();
                    if (c == 't') {
                        mainPage();
                        return;
                    }
                }
                printStringCentered("Creating the channel...");
                printf("\n\n\n");
                
                int result1 = createChannelServer();
                // TODO: should be any result for joinChannel also here?
                int result2 = reloadMessagesServer();
                
                if (result1 == -1 || result2 == -1) {
                    printStringCentered("Creating the channel failed. Press 't' to retry.");
                    char c = getch();
                    if (c == 't') {
                        mainPage();
                        return;
                    }
                } else {
                    chatPage();
                    return;
                }
            }
            break;
        } else {
            char oneCharArr[2];
            oneCharArr[0] = c;
            oneCharArr[1] = '\0';
            strcat(channel, oneCharArr);
            mainPage_printUntilSpecifiedChannel(channel);
        }
    }
    
    printf("\n\n");
    
    if (strcmp(channel, "") == 0) {
        printStringCentered("Invalid channel name. Press 't' to retry.");
        char c = getch();
        if (c == 't') {
            mainPage();
            return;
        }
    }
    
    
    printStringCentered("Looking for the channel...");
    printf("\n\n\n");
    
    int result1 = findChannelServer();
    int result2 = reloadMessagesServer();
    
    if (result1 == -1 || result2 == -1) {
        printStringCentered("Looking for the channel failed. Press 't' to retry.");
        char c = getch();
        if (c == 't') {
            mainPage();
            return;
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
    resetFont();
    printf("\n\n\n");
    printStringCentered("Enter your username below, or type 'return' and then enter to return back:");
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
            char oneCharArr[2];
            oneCharArr[0] = c;
            oneCharArr[1] = '\0';
            strcat(username, oneCharArr);
            registerPage_printUntilSpecifiedUsername(username);
        }
    }
    
    printf("\n\n");
    
    if (strcmp(username, "") == 0) {
        printStringCentered("Invalid username. Press 't' to retry or 'r' to return.");
        char c = getch();
        if (c == 't') {
            registerPage();
            return;
        } else if (c == 'r') {
            welcomePage();
            return;
        }
    }
    
    printStringCentered("Enter your password below (it will be hidden for safety reasons):");
    
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
        printStringCentered("Invalid password. Press 't' to retry or 'r' to return.");
        char c = getch();
        if (c == 't') {
            registerPage();
            return;
        } else if (c == 'r') {
            welcomePage();
            return;
        }
    }
    
    printStringCentered("Registring...");
    printf("\n\n\n");
    
    int result = registerServer(username, password);
    
    if (result == -1) {
        printStringCentered("Registration failed. Press 't' to retry or 'r' to return.");
        char c = getch();
        if (c == 't') {
            registerPage();
            return;
        } else if (c == 'r') {
            welcomePage();
            return;
        }
    } else {
        printStringCentered("Registration succesful! Press any key to return.");
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
    resetFont();
    printf("\n\n\n");
    printStringCentered("Enter your username below, or type 'return' and then enter to return back:");
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
            char oneCharArr[2];
            oneCharArr[0] = c;
            oneCharArr[1] = '\0';
            strcat(username, oneCharArr);
            loginPage_printUntilSpecifiedUsername(username);
        }
    }
    
    printf("\n\n");
    
    if (strcmp(username, "") == 0) {
        printStringCentered("Invalid username. Press 't' to retry or 'r' to return.");
        char c = getch();
        if (c == 't') {
            loginPage();
            return;
        } else if (c == 'r') {
            welcomePage();
            return;
        }
    }
    
    printStringCentered("Enter your password below (it will be hidden for safety reasons):");
    
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
        printStringCentered("Invalid password. Press 't' to retry or 'r' to return.");
        char c = getch();
        if (c == 't') {
            loginPage();
            return;
        } else if (c == 'r') {
            welcomePage();
            return;
        }
    }
    
    printStringCentered("Logging in...");
    printf("\n\n\n");
    
    char token[100];
    loginServer(username, password, token);
    
    if (strcmp(token, "-1") == 0) {
        printStringCentered("Login failed. Press 't' to retry or 'r' to return.");
        char c = getch();
        if (c == 't') {
            loginPage();
            return;
        } else if (c == 'r') {
            welcomePage();
            return;
        }
    } else {
        mainPage();
    }
    
}

void welcomePage() {
    // TODO: h
    system("clear");
    hideCursor();
    printf("\n\n\n\n\n");
    makeBoldRed();
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

int main (int argc, const char * argv[]) {
    // IDEA: command line arguments to facilitate using app
    // IDEA: Password strength
    // IDEA: Activity Indicator
    // BUG: ESC when working with app works badly...
    // BUG: press r to return or t to retry... if pressed any other key, then we could do nothing to get rid of the situation!!
    // BUG: pressing backspace while typing out username and stuff on login and register and... is sooo bad!
    // UPTO: New channel interface should be made. and, asciiart!
    
    
    
//    signal(SIGINT, exitCtrlCHandler);
    
    // TODO: Here
    //system("clear");//system("@cls||clear");
    setupTerminalDimensions();
    atexit(showCursor);
    
    welcomePage();
    
    return 0;
}
