/***            includes            ***/

#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>



/***            defines            ***/

#define CTRL_KEY(k) ((k)& 0x1f)



/***            data            ***/

struct editorConfig{
    struct termios orig;
};

struct editorConfig E;


/***            terminal            ***/

void die(const char *s){
    perror(s);
    exit(1);
}

void disableRawMode(){
    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&E.orig) == -1)
        die("tcsetattr");
}

void enableRawMode(){
    
    if(tcgetattr(STDIN_FILENO,&E.orig) == -1)
        die("tcgetattr");
    atexit(disableRawMode);


    struct termios raw = E.orig;
    
    raw.c_iflag &= ~(IXON | ICRNL| INPCK | BRKINT | ISTRIP);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_oflag &= ~(OPOST);

    raw.c_cflag |= (CS8);

    raw.c_cc[VTIME] = 1;
    raw.c_cc[VMIN] = 0;

    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw) == -1)
        die("tcsetattr");
}

char editorReadKey(){
    int nread;
    char c;
    while((nread = read(STDIN_FILENO,&c,1)) != 1){
        if(nread == -1 && errno != EAGAIN)
            die("read");
    }
    return c;
}


/***            input              ***/

void editorProcessKeyPress(){
    char c = editorReadKey();

    switch(c){
        case CTRL_KEY('q'):
            write(STDOUT_FILENO,"\x1b[2J",4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
    }
}


/***            output              ***/

void editorDrawRows(){
    int y;
    for(y = 0;y < 24;y++){
        write(STDOUT_FILENO,"~\r\n",3);
    }
}


void editorRefreshScreen(){
    write(STDOUT_FILENO,"\x1b[2J",4);
    write(STDOUT_FILENO, "\x1b[H", 3); 
    
    editorDrawRows();

    write(STDOUT_FILENO,"\x1b[H",3);
}



/***            init            ***/

int main(){

    enableRawMode();

    
    while(1){
        editorRefreshScreen();
        editorProcessKeyPress();
    }
    return 0;
}