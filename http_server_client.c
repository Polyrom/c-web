#include "http_server.h"
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LOG_ERROR "ERROR:"

void handle_home(int sock) {
  const char *response = "HTTP/1.1 200 OK\r\nContent-Type: "
                         "text/html\r\n\r\n<h1>Welcome to the page!</h1>\n";
  write(sock, response, strlen(response));
}

int main(void) {
  router_t *router = new_router();
  register_route(router, "/", handle_home);
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(8003);
  if (listen_and_serve(router, address) < 0) {
    fprintf(stderr, "%s Server error: %s\n", LOG_ERROR, strerror(errno));
    free_router(router);
    exit(EXIT_FAILURE);
  }
  free_router(router);
  return 0;
}
