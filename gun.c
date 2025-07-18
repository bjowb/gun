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


enum editorKey{
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};


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

int editorReadKey(){
    int nread;
    char c;
    while((nread = read(STDIN_FILENO,&c,1)) != 1){
        if(nread == -1 && errno != EAGAIN)
            die("read");
    }

    if(c == '\x1b'){

        char seq[3];

        if(read(STDIN_FILENO,&seq[0],1) != 1)
            return '\x1b';
        if(read(STDIN_FILENO,&seq[1],1) != 1)
            return '\x1b';
        if(seq[0] == '['){

            if(seq[1] >= '0' && seq[1] <= '9'){
                if(read(STDIN_FILENO,&seq[2],1) != 1)
                    return '\x1b';
                if(seq[2] == '~'){
                    switch (seq[1]){
                        case '1' : return HOME_KEY;
                        case '3' : return DEL_KEY;
                        case '4' : return END_KEY;
                        case '5' : return PAGE_UP;
                        case '6' : return PAGE_DOWN;
                        case '7' : return HOME_KEY;
                        case '8' : return END_KEY;
                    }
                }
            }
            else{
                switch(seq[1]){
                    case 'A': return ARROW_UP;
                    case 'B' : return ARROW_DOWN;
                    case 'C' : return ARROW_RIGHT;
                    case 'D' : return ARROW_LEFT;
                    case 'H' : return HOME_KEY;
                    case 'F' : return END_KEY;
                }
            }   
        }
        else if(seq[0] == 'O'){
            switch (seq[1]) {
                case 'H' : return HOME_KEY;
                case 'F' : return END_KEY;
            }
        }

        return '\x1b';
    }
    else{
        return c;
    }

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
void editorMoveCursor(int key){
    switch(key){
        case ARROW_LEFT:
            if (E.cx > 0) E.cx--;
            break;
        case ARROW_DOWN:
            if (E.cy < E.screenrows - 1) E.cy++;
            break;
        case ARROW_RIGHT:
            if (E.cx < E.screencols - 1) E.cx++;
            break;
        case ARROW_UP:
            if (E.cy > 0) E.cy--;
            break;
    }
}

void editorProcessKeyPress(){
    int c = editorReadKey();

    switch(c){
        case CTRL_KEY('q'):
            write(STDOUT_FILENO,"\x1b[2J",4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case HOME_KEY:
            E.cx = 0;
            break;
        
        case END_KEY:
            E.cx = E.screenrows-1;
            break;
        

        case PAGE_UP:
        case PAGE_DOWN:
            {
            int times = E.screenrows;
            while(times--){
                editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
            }
            break;

        case ARROW_UP:
        case ARROW_LEFT:
        case ARROW_DOWN:
        case ARROW_RIGHT:
            editorMoveCursor(c);
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

    char buf[32];
    snprintf(buf,sizeof(buf),"\x1b[%d;%dH",E.cy+1,E.cx+1);
    abAppend(&ab,buf,strlen(buf));

    // abAppend(&ab,"\x1b[H",3);
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