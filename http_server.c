#include "http_server.h"
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define REQ_QUEUE_SIZE 10
#define BUFFER_SIZE 1024

#define LOG_INFO "INFO:"
#define LOG_ERROR "ERROR:"

router_t *new_router() {
  router_t *router = (router_t *)malloc(sizeof(router_t));
  router->routes = NULL;
  return router;
}

void free_router(router_t *router) {
  route_t *current = router->routes;
  while (current != NULL) {
    route_t *temp = current;
    current = current->next;
    free(temp);
  }
  free(router);
}

void register_route(router_t *router, char *path, void (*handler)(int sock)) {
  route_t *new_route = (route_t *)malloc(sizeof(route_t));
  new_route->path = path;
  new_route->handler = handler;
  new_route->next = router->routes;
  router->routes = new_route;
}

int parse_request(char request[], char **method, char **resource,
                  char **protocol) {
  const char *whitespace = " ";
  const char *line_terminator = "\r";
  *method = strsep(&request, whitespace);
  if (*method == NULL) {
    fprintf(stderr, "%s Parsing method failed: %s\n", LOG_ERROR,
            strerror(errno));
    return -1;
  }
  *resource = strsep(&request, whitespace);
  if (*resource == NULL) {
    fprintf(stderr, "%s Parsing resource failed: %s\n", LOG_ERROR,
            strerror(errno));
    return -1;
  }
  *protocol = strsep(&request, line_terminator);
  if (*protocol == NULL) {
    fprintf(stderr, "%s Parsing protocol failed: %s\n", LOG_ERROR,
            strerror(errno));
    return -1;
  }
  return 0;
}

void (*find_handler_func(router_t *router, char *resource))(int sock) {
  route_t *current = router->routes;
  while (current != NULL) {
    if (strcmp(current->path, resource) == 0) {
      return current->handler;
    }
    current = current->next;
  }
  return NULL;
}

int handle_request(router_t *router, int new_socket) {
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
  char *method;
  char *resource;
  char *protocol;
  int parsed = parse_request(buffer, &method, &resource, &protocol);
  if (parsed < 0) {
    fprintf(stderr, "%s Error parsing request: %s\n", LOG_ERROR,
            strerror(errno));
    return -1;
  }
  printf("%s Request %s %s %s\n", LOG_INFO, method, resource, protocol);
  void (*handler_func)(int sock) = find_handler_func(router, resource);
  handler_func(new_socket);
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
  printf("%s Binding socket to port %d\n", LOG_INFO, htons(address.sin_port));
  if ((bind(server_fd, (struct sockaddr *)&address, addrlen)) < 0) {
    fprintf(stderr, "%s Binding socket failed: %s\n", LOG_ERROR,
            strerror(errno));
    exit(EXIT_FAILURE);
  }

  // listen to address on socket
  printf("%s Initializing listening on port %d\n", LOG_INFO,
         htons(address.sin_port));
  if ((listen(server_fd, REQ_QUEUE_SIZE)) < 0) {
    fprintf(stderr, "%s Listening to socket failed: %s\n", LOG_ERROR,
            strerror(errno));
    exit(EXIT_FAILURE);
  }

  printf("%s Server listening on port %d\n", LOG_INFO, htons(address.sin_port));
  return server_fd;
}

int listen_and_serve(router_t *router, struct sockaddr_in address) {
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
    if (handle_request(router, new_socket) < 0) {
      fprintf(stderr, "%s Handle request: %s\n", LOG_ERROR, strerror(errno));
    }
    close(new_socket);
  }
  return 0;
}
