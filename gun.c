#include <stdlib.h>
#include <unistd.h>
#include <termios.h>




struct termios orig;

void disableRawMode(){
    tcsetattr(STDIN_FILENO,TCSAFLUSH,&orig);
}

void enableRawMode(){
    
    tcgetattr(STDIN_FILENO,&orig);
    atexit(disableRawMode);


    struct termios raw = orig;
    
    raw.c_cflag &= ~(ECHO);

    tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw);
}


int main(){

    enableRawMode();

    char c;
    while(read(STDIN_FILENO,&c,1) == 1 && c != 'q');
    return 0;
}