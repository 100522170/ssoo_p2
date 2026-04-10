#include "mycalc.h" // Includes mycalc.h
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

const int max_line = 1024;
const int max_commands = 10;
#define max_redirections 3 // stdin, stdout, stderr
#define max_args 15

/* VARS TO BE USED FOR THE STUDENTS */
char *argvv[max_args];
char *filev[max_redirections];
int background = 0;

/**
 * This function splits a char* line into different tokens based on a given
 * character
 * @return Number of tokens
 */
int tokenizar_linea(char *linea, char *delim, char *tokens[], int max_tokens) {
  int i = 0;
  char *token = strtok(linea, delim);
  while (token != NULL && i < max_tokens - 1) {
    tokens[i++] = token;
    token = strtok(NULL, delim);
  }
  tokens[i] = NULL;
  return i;
}
/**
 * This function removes surrounding double quotes from each argument token.
 * This handles the special case where echo "Hello world" should produce
 * the same output as echo Hello world, i.e. the quotes must be stripped.
 */
void eliminar_comillas(char *args[]) {
  for (int i = 0; args[i] != NULL; i++) {
    int len = strlen(args[i]);
    // If the token starts and ends with a double quote, strip them in place
    if (len >= 2 && args[i][0] == '"' && args[i][len - 1] == '"') {
      args[i][len - 1] = '\0'; // remove trailing quote
      args[i]++;               // advance pointer past leading quote
    }
  }
}
/**
 * This function processes the command line to evaluate if there are
 * redirections. If any redirection is detected, the destination file is
 * indicated in filev[i] array. filev[0] for STDIN filev[1] for STDOUT filev[2]
 * for STDERR
 */
void procesar_redirecciones(char *args[]) {
  int i = 0, first_red = -1;

  // Store the pointer to the filename if needed.
  for (i = 0; args[i] != NULL; i++) {

    if (strcmp(args[i], "<") == 0) {
      filev[0] = args[i + 1];
      if (first_red == -1)
        first_red = i;
    } else if (strcmp(args[i], ">") == 0) {
      filev[1] = args[i + 1];
      if (first_red == -1)
        first_red = i;
    } else if (strcmp(args[i], "!>") == 0) {
      filev[2] = args[i + 1];
      if (first_red == -1)
        first_red = i;
    }
  }

  // starting from the first redirectorion, all fields are set to NULL
  if (first_red != -1)
    for (i = first_red; args[i] != NULL; i++) {
      args[i] = NULL;
    }
}

/*
 * This function processes the input command line and returns in global
 * variables: argvv -- command an args as argv filev -- files for redirections.
 * NULL value means no redirection. background -- 0 means foreground; 1
 * background.
 */
int procesar_linea(char *linea) {

  char *comandos[max_commands];
  int num_comandos = tokenizar_linea(linea, "|", comandos, max_commands);
  background = 0;

  // Check if background is indicated
  if (strchr(comandos[num_comandos - 1], '&')) {
    background = 1;
    char *pos = strchr(comandos[num_comandos - 1], '&');
    // removes character &
    *pos = '\0';
  }

  filev[0] = NULL;
  filev[1] = NULL;
  filev[2] = NULL;

  // Variables to manage the pipes between iterations
  int prev_fd = -1;    // Saves the read en of last pipe
  int fd[2];           // Actual pipe descriptors
  pid_t last_pid = -1; // We initialice so the compiler does not complain
  // Finish processing
  for (int i = 0; i < num_comandos; i++) {

    tokenizar_linea(comandos[i], " \t\n", argvv, max_args);
    // Remove surrounding double quotes from arguments (e.g. echo "Hello world")
    eliminar_comillas(argvv);

    // Call the function so it fills the array filev
    procesar_redirecciones(argvv);
    // If no command then we go
    if (argvv[0] == NULL)
      continue;

    // exit internal command
    if (strcmp(argvv[0], "exit") == 0) {
      // 1. Check whether there is an argument
      if (argvv[1] == NULL) {
        fprintf(stderr, "[ERROR] Missing exit code\n");
        continue; // Pass to the next line of the script
      }

      // 2. Check whether the number is valid
      char *endptr;
      long val = strtol(argvv[1], &endptr, 10);

      // if endptr point to something diferent to '\0', this means there are no
      // numerical characters

      if (*endptr != '\0') {
        fprintf(stderr, "[ERROR] The exit code must be an integer\n");
        continue; // Pass to the next line of the script
      }

      // 3. If it is valid, wait for all the childs (background or zombies)
      while (wait(NULL) > 0)
        ;

      // 4. print the goodbye message and end the shell
      printf("Goodbye %ld\n", val);
      exit((int)val);
    }

    // lines for Mylcalc
    if (strcmp(argvv[0], "mycalc") == 0) {
      // getting argc
      int argc_calc = 0;
      while (argvv[argc_calc] != NULL) {
        argc_calc++;
      }
      // we call mycalc
      mycalc(argc_calc, argvv);
      continue;
    }

    // 1. Create the pipe if it is not last command
    if (i < num_comandos - 1) {
      if (pipe(fd) == -1) {
        perror("pipe");
        exit(-1); // We exit if the system call fails
      }
    }

    // fork
    pid_t pid = fork();

    if (pid == -1) {
      // error in fork, exit.
      perror("fork");
      exit(-1); // we exit if the system call fails
    }

    if (pid == 0) {

      // --- Child code ---

      // If it is NOT the first command, recieves entry of the last pipe
      if (i > 0) {
        dup2(prev_fd, STDIN_FILENO);
        close(prev_fd); // Its already duplicated, we close the original
      }

      // If it is NOT the first command, sends its exit to the actual pipe
      if (i < num_comandos - 1) {
        close(fd[0]); // son is not going to read from its own pipe
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]); // Its already duplicated, we close the original
      }

      // --- Files redirections ---

      // Standart input (<) ONLY on the first command
      if (i == 0 && filev[0] != NULL) {
        int fd_in = open(filev[0], O_RDONLY);
        if (fd_in < 0) {
          perror(filev[0]); // If the files does not exist, raises error
          exit(-1);
        }
        dup2(fd_in, STDIN_FILENO);
        close(fd_in);
      }

      // Standard output (>) ONLY on the last command
      if (i == num_comandos - 1 && filev[1] != NULL) {
        // O_CREAT creates if it does not exist, O_TRUNC overwrites
        int fd_out = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd_out < 0) {
          perror(filev[1]);
          exit(-1);
        }
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
      }

      // Standard error (!>) ONLY on the last command
      if (i == num_comandos - 1 && filev[2] != NULL) {
        // O_CREAT creates if it does not exist, O_TRUNC overwrites
        int fd_err = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd_err < 0) {
          perror(filev[2]);
          exit(-1);
        }
        dup2(fd_err, STDERR_FILENO);
        close(fd_err);
      }
      execvp(argvv[0], argvv);

      perror(argvv[0]);
      exit(-1); // A command error fails, but the shell keeps going
      //  on (this is the son exit)

    } else {
      // --- Parent code ---

      last_pid = pid; // We save the PID in case it is background

      // We close the last pipe
      if (i > 0) {
        close(prev_fd);
      }

      // Prepare prev_fd for the next iteration
      if (i < num_comandos - 1) {
        close(fd[1]);    // Parent wont overwrite on this pipe
        prev_fd = fd[0]; // Save the read end for the next son
      }
    }
  }

  // 3. Wait management (Wait and Background)
  if (background == 0) {
    // Foreground Mode: Wait for all the childs
    for (int i = 0; i < num_comandos; i++) {
      wait(NULL);
    }
    // Reap zombies
    while (waitpid(-1, NULL, WNOHANG) > 0)
      ;

  } else {
    // Background Mode: Print the PID without \n

    printf("%d", last_pid);
    fflush(stdout);
  }

  return num_comandos;
}

int main(int argc, char *argv[]) {

  // checking if 1 arg
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
    exit(-1);
  }

  // opening the file
  int fd = open(argv[1], O_RDONLY);
  if (fd == -1) {
    perror(argv[1]);
    exit(-1);
  }

  // verifying that first line is ## Uc3mshell P2"
  char buf[max_line];
  int pos = 0;
  char c;
  ssize_t bytes_read;

  // reading char by char until  \n or EOF
  while ((bytes_read = read(fd, &c, 1)) > 0) {
    if (c == '\n') {
      buf[pos] = '\0'; // end
      break;
    }
    if (pos < max_line - 1) {
      buf[pos++] = c;
    }
  }

  // error if read failed
  if (bytes_read == -1) {
    perror("read");
    close(fd);
    exit(-1);
  }

  // verifying that first line is ## Uc3mshell P2"
  if (strcmp(buf, "## Uc3mshell P2") != 0) {
    perror("Invalid script header");
    close(fd);
    exit(-1);
  }

  // reading all the file
  pos = 0;
  while ((bytes_read = read(fd, &c, 1)) > 0) {

    if (c == '\n') {
      buf[pos] = '\0'; // end

      // ignoring empty lines and comments
      if (buf[0] != '\0' && buf[0] != '#') {
        procesar_linea(buf);
      }

      pos = 0; // reset buffer for the next line

    } else {
      if (pos < max_line - 1) {
        buf[pos++] = c; // summing the char in buffer
      }
    }
  }

  // in case read failed:
  if (bytes_read == -1) {
    perror("read");
    close(fd);
    exit(-1);
  }

  // processing last line in case the file doesnt end in '\n'
  if (pos > 0) {
    buf[pos] = '\0';
    if (buf[0] != '\0' && buf[0] != '#') {
      procesar_linea(buf);
    }
  }

  // close file
  close(fd);

  return 0;
}
