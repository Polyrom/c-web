#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 8003
#define REQ_QUEUE_SIZE 10
#define BUFFER_SIZE 1024

#define LOG_INFO "INFO:"
#define LOG_ERROR "ERROR:"

struct req_props {
  char *method;
  char *resource;
  char *protocol;
  bool error;
};

struct req_props parse_request(char request[]) {
  struct req_props props;
  props.error = true;
  // char *token;
  const char *whitespace = " ";
  const char *line_terminator = "\r";
  props.method = strsep(&request, whitespace);
  if (props.method == NULL) {
    fprintf(stderr, "%s Parsing method failed: %s\n", LOG_ERROR,
            strerror(errno));
  }
  props.resource = strsep(&request, whitespace);
  if (props.resource == NULL) {
    fprintf(stderr, "%s Parsing resource failed: %s\n", LOG_ERROR,
            strerror(errno));
  }
  props.protocol = strsep(&request, line_terminator);
  if (props.protocol == NULL) {
    fprintf(stderr, "%s Parsing protocol failed: %s\n", LOG_ERROR,
            strerror(errno));
  }
  props.error = false;
  return props;
}

int handle_request(int new_socket) {
  // write request into a buffer
  char buffer[BUFFER_SIZE];
  int bytes_read = read(new_socket, buffer, BUFFER_SIZE - 1);
  if (bytes_read < 0) {
    fprintf(stderr, "%s Reading request failed: %s\n", LOG_ERROR,
            strerror(errno));
    return -1;
  }

  buffer[bytes_read] = '\0';

  // parse the request
  struct req_props req_props = parse_request(buffer);
  if (req_props.error == true) {
    fprintf(stderr, "%s Error parsing request: %s\n", LOG_ERROR,
            strerror(errno));
    return -1;
  }
  printf("%s Request %s %s %s\n", LOG_INFO, req_props.method,
         req_props.resource, req_props.protocol);
  if (strcmp(req_props.resource, "/") == 0) {
    const char *response = "HTTP/1.1 200 OK\r\nContent-Type: "
                           "text/html\r\n\r\n<h1>Welcome to the page!</h1>\n";
    write(new_socket, response, strlen(response));
  } else {
    const char *response = "HTTP/1.1 404 Not Found\r\nContent-Type: "
                           "text/html\r\n\r\n<h1>404 Not Found</h1>\n";
    write(new_socket, response, strlen(response));
  }
  return 0;
}

int set_up_listener(struct sockaddr_in address) {
  // create socket
  int server_fd;
  printf("%s Creating socket\n", LOG_INFO);
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    fprintf(stderr, "%s Socket creation failed: %s\n", LOG_ERROR,
            strerror(errno));
    exit(EXIT_FAILURE);
  }

  // ensure socket reuse
  int opt = 1;
  printf("%s Setting reuse options for socket\n", LOG_INFO);
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    fprintf(stderr, "%s Setting reuse options failed: %s\n", LOG_ERROR,
            strerror(errno));
    exit(EXIT_FAILURE);
  }

  // bind socket to address and port
  int addrlen = sizeof(address);
  printf("%s Binding socket to port %d\n", LOG_INFO, SERVER_PORT);
  if ((bind(server_fd, (struct sockaddr *)&address, addrlen)) < 0) {
    fprintf(stderr, "%s Binding socket failed: %s\n", LOG_ERROR,
            strerror(errno));
    exit(EXIT_FAILURE);
  }

  // listen to address on socket
  printf("%s Initializing listening on port %d\n", LOG_INFO, SERVER_PORT);
  if ((listen(server_fd, REQ_QUEUE_SIZE)) < 0) {
    fprintf(stderr, "%s Listening to socket failed: %s\n", LOG_ERROR,
            strerror(errno));
    exit(EXIT_FAILURE);
  }

  printf("%s Server listening on port %d\n", LOG_INFO, SERVER_PORT);
  return server_fd;
}
int listen_and_serve(struct sockaddr_in address) {
  int server_fd, cli_addrlen, new_socket;
  struct sockaddr cli_address;
  cli_addrlen = sizeof(cli_address);
  server_fd = set_up_listener(address);

  for (;;) {
    if ((new_socket = accept(server_fd, (struct sockaddr *)&cli_address,
                             (socklen_t *)&cli_addrlen)) < 0) {
      fprintf(stderr, "%s Failed to accept a request: %s\n", LOG_ERROR,
              strerror(errno));
      return -1;
    }
    if (handle_request(new_socket) < 0) {
      fprintf(stderr, "%s Handle request: %s\n", LOG_ERROR, strerror(errno));
    }
    close(new_socket);
  }
  return 0;
}

int main(void) {
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(SERVER_PORT);
  if (listen_and_serve(address) < 0) {
    fprintf(stderr, "%s Server error: %s\n", LOG_ERROR, strerror(errno));
    exit(EXIT_FAILURE);
  }
  return 0;
}
