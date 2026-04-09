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


/**
 * This function processes the input command line and returns in global
 * variables: argvv -- command an args as argv filev -- files for redirections.
 * NULL value means no redirection. background -- 0 means foreground; 1
 * background.
 */
/**
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
 
  // Variables para controlar los pipes entre iteraciones
  int prev_fd = -1; // Guardará el extremo de lectura del pipe anterior
  int fd[2];        // Descriptores para el pipe actual
  pid_t last_pid = -1;   // Inicializamos para que el compilador no se queje
  // Finish processing
  for (int i = 0; i < num_comandos; i++) {


    tokenizar_linea(comandos[i], " \t\n", argvv, max_args);

    // Llamamos a la función para que rellene el array filev
    procesar_redirecciones(argvv);
    // If no command then we go
    if (argvv[0] == NULL)
      continue;


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


    // 1. Crear el pipe si NO es el último comando
    if (i < num_comandos - 1) {
        if (pipe(fd) == -1) {
            perror("pipe");
            exit(-1); // Salimos si falla la llamada al sistema [cite: 65]
        }
    }


    // fork
    pid_t pid = fork();


    if (pid == -1) {
      // error in fork, exit.
      perror("fork");
      exit(-1); // Salimos si falla la llamada al sistema [cite: 65]
    }


    if (pid == 0) {


      // --- CÓDIGO DEL HIJO ---


      // Si NO es el primer comando, recibe entrada del pipe anterior
      if (i > 0) {
          dup2(prev_fd, STDIN_FILENO);
          close(prev_fd); // Ya está duplicado, cerramos el original
      }


      // Si NO es el último comando, envía su salida al pipe actual
      if (i < num_comandos - 1) {
          close(fd[0]); // El hijo no va a leer de su propio pipe
          dup2(fd[1], STDOUT_FILENO);
          close(fd[1]); // Ya está duplicado, cerramos el original
      }

      // --- REDIRECCIONES DE ARCHIVOS ---

      // Entrada estándar (<) SOLO en el primer comando
      if (i == 0 && filev[0] != NULL) {
          int fd_in = open(filev[0], O_RDONLY);
          if (fd_in < 0) {
              perror(filev[0]); // Si el archivo no existe, da error [cite: 108]
              exit(-1);
          }
          dup2(fd_in, STDIN_FILENO);
          close(fd_in);
      }

      // Salida estándar (>) SOLO en el último comando
      if (i == num_comandos - 1 && filev[1] != NULL) {
          // O_CREAT crea si no existe, O_TRUNC lo sobrescribe si ya existe [cite: 113, 114]
          int fd_out = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
          if (fd_out < 0) {
              perror(filev[1]);
              exit(-1);
          }
          dup2(fd_out, STDOUT_FILENO);
          close(fd_out);
      }

      // Salida de error (!>) SOLO en el último comando
      if (i == num_comandos - 1 && filev[2] != NULL) {
          // O_CREAT crea si no existe, O_TRUNC lo sobrescribe [cite: 121, 122]
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
      exit(-1); // Un error de comando falla, pero el shell sigue (este es el exit del hijo) [cite: 537]


    } else {
      // --- CÓDIGO DEL PADRE ---


      last_pid = pid; // Guardamos el PID por si es background


      // Cerramos el pipe anterior, el padre ya no lo necesita
      if (i > 0) {
          close(prev_fd);
      }


      // Preparamos prev_fd para la siguiente iteración
      if (i < num_comandos - 1) {
          close(fd[1]); // El padre no va a escribir en este pipe
          prev_fd = fd[0]; // Guardamos el extremo de lectura para el próximo hijo
      }
    }
  }


  // 3. Gestión de esperas (Wait y Background)
  if (background == 0) {
      // Modo Foreground: Esperar a TODOS los hijos
      for (int i = 0; i < num_comandos; i++) {
          wait(NULL);
      }
      // Reap zombies
      while (waitpid(-1, NULL, WNOHANG) > 0);


  } else {
      // Modo Background: Imprimir el PID sin \n (estrictamente como pide el FAQ) [cite: 564, 565]
      printf("%d", last_pid);
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
