#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

//allocated space for the command line arguments
#define BUFFER_SIZE 64
//number of builtin commands, useful for referencing builtins array
#define NUM_BUILTINS 7

//globals
char *prompt;
char *line;
char **args;
int arg_count;
char *builtins[] = { "exit", "pid", "ppid", "cd", "pwd", "set", "get" };

int main(int argc, char **argv) {
	//checks for -p command line argument and sets the prompt appropiately
	if(argc > 1) {
		if(strcmp(argv[1], "-p") == 0 && argv[2] != NULL) {
			prompt = argv[2];
		} else {
			printf("Usage: ./shell -p <prompt>\n");
			return 0;
		}
	} else {
		prompt = "308sh";
	}
	//starts the continuous shell loop
	init_shell();
	return 0;
}

void init_shell() {
	//status variable for exit
	int status;
	do {
		//shell loop
		printf("%s> ", prompt);
		//calls background processes
		waitpid(-1, &status, WNOHANG);
		read_line();
		parse_line();
		status = launch();
		free(line);
		free(args);
	} while(status);
}

//reads user input
void read_line() {
	ssize_t buffer = 0;
	getline(&line, &buffer, stdin);
}

//seperates the user input into tokens and stores them in args
void parse_line() {
	char *pos;
	char *token = strtok(line, " ");
	//allocates space for the arguments
	args = malloc(BUFFER_SIZE * sizeof(char*));
	arg_count = 0;
	while(token != NULL) {
		//elimates new line characters from the tokens
		if((pos = strchr(token, '\n')) != NULL) {
			*pos = '\0';
		}
		args[arg_count] = token;
		arg_count++;
		token = strtok(NULL, " ");
	}
	args[arg_count] = NULL;
	arg_count--;
}

//determines if the command is builtin or not
int launch() {
	int i;
	//empty
	if(args[0] == NULL || strcmp(args[0], "\0") == 0) {
		printf("Command is empty.\n");
		return 1;
	}
	//builtin
	for(i = 0; i < NUM_BUILTINS; i++) {
		if(strcmp(args[0], builtins[i]) == 0) {
			return execute_builtins(i);
		}
	}
	//executable
	return execute();
}

//executes non-builtin functions
int execute() {
	int background = 0;
	int status;
	int exit_status;
	pid_t pid = fork();
	//checks for background process
	if(strcmp(args[arg_count], "&") == 0) {
		//removes & argument and sets background flag
		args[arg_count] = NULL;
		background = 1;
	}
	//parent process
	if(pid > 0) {
		//send to child to background
		if(background) {
			return 1;
		}	
		//waits for child process to finish	
		waitpid(pid, &status, WUNTRACED);
	//child process
	} else if(pid == 0) {
		printf("[%d]\n", getpid());
		if(execvp(args[0], args) == -1) {
			perror("Failed to execute child process: ");
		}
		_exit(EXIT_FAILURE);
	//error forking
	} else {
		perror("Failed to fork: ");
	}
	//return pid and exit status
	if(WIFEXITED(status)) {
		exit_status = WEXITSTATUS(status);
		printf("[%d] EXIT %d\n", pid, exit_status);
	}
	return 1;
}

//switch statement determines which builtin function to execute
int execute_builtins(int id) {
	int status;
	switch(id) {
		case 0 :
			status = builtin_exit();
			break;
		case 1 :
			status = builtin_pid();
			break;
		case 2 :
			status = builtin_ppid();
			break;
		case 3 :
			status = builtin_cd();
			break;
		case 4 :
			status = builtin_pwd();
			break;
		case 5 :
			status = builtin_set();
			break;
		case 6 :
			status = builtin_get();
			break;
		default :
			printf("Builtin command does not exsist.\n");
	}
	return status;	
}

//sets status to zero in the shell loop to exit the program
int builtin_exit() {
	return 0;
}

//returns the pid
int builtin_pid() {
	printf("[%d]\n", getpid());
	return 1;
}

//returns the ppid
int builtin_ppid() {
	printf("[%d]\n", getppid());
	return 1;
}

//changes directory
int builtin_cd() {
	if(args[1] != NULL) {
		if(chdir(args[1]) == -1) {
			perror("Failed to change the working directory: ");
		}
	} else {
		chdir(getenv("HOME"));
	}
	return 1;
}

//returns current working directory
int builtin_pwd() {
	char cwd[1024];
   	if(getcwd(cwd, sizeof(cwd)) == NULL) {
		perror("Failed to return current working directory: ");
	}
    	printf("%s\n", cwd);
	return 1;
}

//sets environment variable
int builtin_set() {
	if(args[2] != NULL) {
		if(setenv(args[1], args[2], 1) == -1) {
			perror("Failed to set environment variable: ");
		}
	} else {
		if(unsetenv(args[1]) == -1) {
			perror("Failed to delete environment variable: ");
		}
	}
	return 1;
}

//returns variable of environment variable
int builtin_get() {
	if(getenv(args[1]) == NULL) {
		perror("Failed to retrieve the value of environment variable: ");
	} else {
		printf("%c\n", *getenv(args[1]));
	}
	return 1;
}

