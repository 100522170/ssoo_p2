#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {

  // Check that exactly 2 arguments are provided (source and destination)
  if (argc != 3) {
    printf("Usage: %s <source_file> <destination_file>\n", argv[0]);
    return -1;
  }

  // Open source file in read-only mode
  int fd_src = open(argv[1], O_RDONLY);
  if (fd_src == -1) {
    perror(argv[1]);
    return -1;
  }

  // Open/create destination file — overwrite if it already exists
  int fd_dst = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd_dst == -1) {
    perror(argv[2]);
    close(fd_src);
    return -1;
  }

  // Copy loop: read from source, write to destination
  char buf[4096];
  ssize_t bytes_read;

  while ((bytes_read = read(fd_src, buf, sizeof(buf))) > 0) {
    ssize_t bytes_written = write(fd_dst, buf, bytes_read);
    if (bytes_written == -1) {
      perror("write");
      close(fd_src);
      close(fd_dst);
      return -1;
    }
  }

  // Check if read failed
  if (bytes_read == -1) {
    perror("read");
    close(fd_src);
    close(fd_dst);
    return -1;
  }

  // Close both file descriptors
  close(fd_src);
  close(fd_dst);

  return 0;
}