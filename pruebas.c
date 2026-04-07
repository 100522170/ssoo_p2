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
          proccess_line(buf);
        }

        pos = 0; // reset buffer for the next line

      } else {
        if (pos < max_line - 1) {
          buf[pos++] = c; // summing the char in buffer
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
          proccess_line(buf);
        }
      }

      // close file
      close(fd);

      return 0;
    }
  }
}