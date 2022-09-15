/****************
LE2: Introduction to Unnamed Pipes
****************/
#include <unistd.h> // pipe, fork, dup2, execvp, close
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <unistd.h>
#include <sys/wait.h>

#include <cstring>
#include <string>

#define WRITE 1
#define READ 0

#include <vector>
using namespace std;

int main () {
    // lists all the files in the root directory in the long format
    char* cmd1[] = {(char*) "ls", (char*) "-al", (char*) "/", nullptr};
    // translates all input from lowercase to uppercase
    char* cmd2[] = {(char*) "tr", (char*) "a-z", (char*) "A-Z", nullptr};



    int fd[2];
    // Create pipe
    pipe(fd);

    pid_t pid1 = fork();

    if (pid1 == 0){ //child

    // STDIN_FILENO == READ == 0
    // STDOUT_FILENO  == WRITE == 1

        dup2(fd[STDOUT_FILENO], STDOUT_FILENO);

        close(fd[STDIN_FILENO]);
        close(fd[STDOUT_FILENO]);

        if(execvp(cmd1[0], cmd1) < 0){ // nothing below executes
			perror("execvp");
			exit(EXIT_FAILURE);
		}

        
    } else { //parent
        
        pid1 = fork();

        if (pid1 == 0){

            dup2(fd[STDIN_FILENO], STDIN_FILENO);

            close(fd[STDOUT_FILENO]);
            close(fd[STDIN_FILENO]);

            if(execvp(cmd2[0], cmd2) < 0){ // nothing below executes
                perror("execvp");
                exit(EXIT_FAILURE);
		    }


        } else {
            int status;

            close(fd[STDIN_FILENO]);
            close(fd[STDOUT_FILENO]);

            waitpid(pid1, &status, 0);

        }
        
    }

}
