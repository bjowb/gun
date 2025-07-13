/***            includes            ***/

#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>




/***            defines            ***/

#define GUN_VERSION "GLOCK-43"
 
#define CTRL_KEY(k) ((k)& 0x1f)
#define ABUF_INIT {NULL,0}



/***            data            ***/

struct editorConfig{
    int cx,cy;
    int screenrows;
    int screencols;
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

int getWindowSize(int *rows,int *cols){
    struct winsize ws;

    if(ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws) == -1 || ws.ws_col == 0){
        return -1;
    }
    else{
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/***            append buffer           ***/

struct abuf{
    char *b;
    int len;
};

void abAppend(struct abuf *ab,const char *s,int len){
    char *new = realloc(ab->b, ab->len+len);

    if(new == NULL){
        return;
    }
    memcpy(&new[ab->len],s,len);
    ab->b = new;
    ab->len += len;
 }

 void abFree(struct abuf *ab){
    free(ab->b);
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

void editorDrawRows(struct abuf *ab){
    int y;
    for(y = 0;y < E.screenrows;y++){

        if(y == 0){
            //made string buffer
            char welcome[80];

            //snprintf used to interpolate our define version to string.
            int welcomelen = snprintf(welcome,sizeof(welcome),"GUN EDITOR --version %s",GUN_VERSION);

            //truncating if necessary
            if(welcomelen > E.screencols)
                welcomelen = E.screencols;
            

            //insert padding
            int padding = (E.screencols - welcomelen)/2;
            if(padding > 0){
                abAppend(ab,"~",1);
                padding--;
            }
            while(padding--)
                abAppend(ab," ",1);

            //write string 
            abAppend(ab,welcome,welcomelen);
        }
        else{
            abAppend(ab,"~",1);
        }
        abAppend(ab, "\x1b[K", 3);
        if(y < E.screenrows-1)
            abAppend(ab,"\r\n",2);
    }
    
}


void editorRefreshScreen(){

    struct abuf ab = ABUF_INIT;

      abAppend(&ab, "\x1b[?25l", 6);


    //clear entire screen
    // abAppend(&ab,"\x1b[2J",4);


    abAppend(&ab,"\x1b[H", 3);
    // write(STDOUT_FILENO,);
    // write(STDOUT_FILENO, ); 
    
    editorDrawRows(&ab);

    abAppend(&ab,"\x1b[H",3);
    // write(STDOUT_FILENO,"\x1b[H",3);

    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO,ab.b,ab.len);
    abFree(&ab);
}



/***            init            ***/

void initEditor(){
    //initializing cursor position
    E.cx = 0;
    E.cy = 0;

    if(getWindowSize(&E.screenrows,&E.screencols) == -1)
        die("getWindowSize");
}

int main(){

    enableRawMode();
    initEditor();

    
    while(1){
        editorRefreshScreen();
        editorProcessKeyPress();
    }
    return 0;
}