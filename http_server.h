#include <netinet/in.h>

#ifndef REQ_QUEUE_SIZE
#define REQ_QUEUE_SIZE 10
#endif

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 1024
#endif

typedef struct route_t {
  const char *path;
  void (*handler)(int sock);
  struct route_t *next;
} route_t;

typedef struct router_t {
  route_t *routes;
} router_t;

router_t *new_router();

int listen_and_serve(router_t *router, struct sockaddr_in address);

void free_router(router_t *router);

void register_route(router_t *router, char *path, void (*handler)(int sock));
