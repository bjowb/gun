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

struct termios orig;




/***            terminal            ***/

void die(const char *s){
    perror(s);
    exit(1);
}

void disableRawMode(){
    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&orig) == -1)
        die("tcsetattr");
}

void enableRawMode(){
    
    if(tcgetattr(STDIN_FILENO,&orig) == -1)
        die("tcgetattr");
    atexit(disableRawMode);


    struct termios raw = orig;
    
    raw.c_iflag &= ~(IXON | ICRNL| INPCK | BRKINT | ISTRIP);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_oflag &= ~(OPOST);

    raw.c_cflag |= (CS8);

    raw.c_cc[VTIME] = 1;
    raw.c_cc[VMIN] = 0;

    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw) == -1)
        die("tcsetattr");
}




/***            init            ***/

int main(){

    enableRawMode();

    
    while(1){
        char c = '\0';
        if(read(STDIN_FILENO,&c,1) == -1 && errno != EAGAIN){
            die("read");
        }
        if(iscntrl(c)){
            printf("%d\r\n",c);
        }
        else{
            printf("%d ('%c')\r\n",c,c);
        }

        if(c == CTRL_KEY('q'))
            break;
    }
    return 0;
}