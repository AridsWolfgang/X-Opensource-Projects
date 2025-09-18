#include <stdio.h>      // For printf, geline, stdin 
#include <stdlib.h>     // For exit, EXIT_SUCCESS, EXIT_FAILURE, free 
#include <string.h>     // For strcmp, strtok 
#include <unistd.h>     // For fork, execve, getpid, chdir 
#include <sys/wait.h>   // For waitpid,WIFEXITED, WEXISTSTATUS 

extern char **environ;

#define MAX_LINE 1024   // Maximum lenght of a command line 
#define MAX_ARGS 64     // Maximum number of arguments 


// Function to print the shell prompt
void printPrompt(){
  printf("arids~shell -> ");
  fflush(stdout);       // Ensure the prompt is displayed immediately
}

// Function to parse the input line into arguments 
char **parseLine(char *line){
  int buffsize = MAX_ARGS;
  char **tokens = malloc(buffsize * sizeof(char *));
  char *token;
  int position = 0;

  if(!tokens){
    fprintf(stderr, "arids~shell: allocation error\n");
    exit(EXIT_FAILURE);
  }

// strtok modifies the original string, so we work on 'line' directly
token = strtok(line, "\t\r\n\a");   // Delimiters: space, tab, carriage return, newline, and alert 

while(token != NULL){
    tokens[position] = token;
    position++;

    // If we exceed the buffer, reallocate 
    if (position >= buffsize){
      buffsize += MAX_ARGS;
      tokens = realloc(tokens, buffsize * sizeof(char *));
      if(!tokens){
        fprintf(stderr, "arids~shell: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
    token = strtok(NULL, "\t\r\n\a");     // Continue from where strtok left off
  }
  tokens[position] = NULL;            // Null-terminate the array of arguments
  return tokens;
}

// Function to execute an external command 
int executeExternalCommand(char **args){
  pid_t pid;
  int status;

  pid = fork();     // Create a child process 
  
  if(pid == -1){
    // Fork failed 
    perror("arids~shell");
    return 1;     // Return 1 to indicate an error 
  } else if (pid == 0){
    // This is the child process
    // execve expects the full path to the executable.
    // For simplicity, we'll try common paths like /bin/ and /usr/bin/. 
    // A robus shell would search the PATH environment variable.
    

    // Construct potential paths for the command 
    char fullPath[MAX_LINE];

    // Try /bin/ 
    snprintf(fullPath, sizeof(fullPath), "/bin/%s", args[0]);
    execve(fullPath, args, environ);      // environ is a global variable holding environment variables 
    
    // Try /usr/bin/ 
    snprintf(fullPath, sizeof(fullPath), "/usr/bin/%s", args[0]);
    execve(fullPath, args, environ);

    // If execve returns, it means an error occured 
    perror("arids~shell");    // Print error message if command not found or cannot be executed 
    _exit(1);   // Child process exits immediately to avoid running parent's code 
  } else{
    // This is the parent process
    // Wait for the child process to complete
    do{
      waitpid(pid, &status, WUNTRACED);   // WUNTRACED returns if child is stopped
    } while(!WIFEXITED(status) && !WIFSIGNALED(status));   // Loop while child is not exited or killed 
  }
  return 0;   // Return 0 to indicate success
}

// Function to handle built-in command
int handle_builtin(char **args){
  if(strcmp(args[0], "exit") == 0){
    // Free allocated memory before exiting 
    // (In this simple version, 'parseLine' allcoates 'tokens'
    // which needs to be freed. more complex shels would need to free the 'line' buffer as well.)
    if(args){
      free(args);   // Free the array of pointers
    }
    exit(EXIT_SUCCESS);
  }
  // add more built-in commands here later (like 'cd')
  return 0;       // 0 indicates it was built in comand 
}

int main(){
  char *line = NULL;        // Pointer to the input line 
  size_t len = 0;           // Size of the buffer for getline 
  ssize_t read;             // Number of characters read by getline 
  char **args;              // array of arguments 
  

  extern char **environ;    // Declare environ for execve 
  
  while (1){
    printPrompt();         // Display the shell prompt 

    // Read a line from stdin 
    read = getline(&line, &len, stdin);

    if(read == -1){
      // Error or EOF (ctrl + D)
      if(feof(stdin)){
        printf("\nExiting arids~shell........\n");
        break;        // Exit loop on EOF 
      } else {
        perror("arids~shell: getline");
        exit(EXIT_FAILURE);
      }
    }

    // remove trailing newline character from the inut 
    if(line[read - 1] == '\n'){
      line[read - 1] == '\0';
    }
    // Handle empty input 
    if (strlen(line) == 0){
      continue;
    }
    
    args = parseLine(line);       // Parse the input line 

    if(args[0] == NULL){
      // Handle empty command after parsing 
      free (args);
      continue;
    }

    // Check if it's a built-in command 
    if (handle_builtin(args) == 0) {
      // It was a built-in command, or built-in didnt match (for now only 'exit' is a built in)
      // If handle_builtin handled the command (e.g. 'exit'), it would have exited already.
      // If it returns 0 and it's not 'exit', we try to execute it as an external command
      if(strcmp(args[0], "exit") != 0){
        // Avoid re-executing if it was "exit"
        executeExternalCommand(args);
      }
    }

    // Free the allocated arguments array 
    free(args);
  }

  // Free the line buffer allocated by getline 
  if(line){
    free(line);
  }

  return EXIT_SUCCESS;
}