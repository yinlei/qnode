/*
 * See Copyright Notice in qnode.h
 */

#include <string.h>
#include "qactor.h"
#include "qmutex.h"
#include "qsocket.h"

static qfreelist_t free_socket_list;
static qmutex_t    free_socket_list_lock;

static int FREE_SOCKET_LIST_NUM = 100;

static int  init_socket(void *data);
static void reset_socket(qsocket_t *socket);
static void destroy_socket(void *data);

void
qsocket_init_free_list() {
  qmutex_init(&free_socket_list_lock);
  qfreelist_init(&free_socket_list, "socket free list",
                 sizeof(qsocket_t), FREE_SOCKET_LIST_NUM,
                 init_socket, destroy_socket);
}

void
qsocket_destroy_free_list() {
  qmutex_lock(&free_socket_list_lock);
  qfreelist_destroy(&free_socket_list);
  qmutex_unlock(&free_socket_list_lock);
  qmutex_destroy(&free_socket_list_lock);
}

qsocket_t*
qsocket_new(int fd, qactor_t *actor) {
  qsocket_t *socket;

  qmutex_lock(&free_socket_list_lock);
  socket = (qsocket_t*)qfreelist_alloc(&free_socket_list);
  qmutex_unlock(&free_socket_list_lock);

  socket->fd = fd;
  socket->aid = actor->aid;

  return socket;
}

void
qsocket_free(qsocket_t *socket) {
  qmutex_lock(&free_socket_list_lock);
  qfreelist_free(&free_socket_list,
                 (qfree_item_t*)socket);
  qmutex_unlock(&free_socket_list_lock);
  qbuffer_reinit(socket->in);
  qbuffer_reinit(socket->out);

  reset_socket(socket);
}

static void
reset_socket(qsocket_t *socket) {
  qlist_entry_init(&socket->entry);
  socket->accept = 0;
  socket->fd = 0;
  socket->state = 0;
  socket->aid = QINVALID_ID;
  socket->addr[0] = '\0';
  socket->peer[0] = '\0';
  socket->port    = 0;
  memset(&(socket->remote), 0, sizeof(struct sockaddr));
}

static int
init_socket(void *data) {
  qsocket_t *socket;

  socket = (qsocket_t*)data;
  socket->in = qbuffer_new();
  socket->out = qbuffer_new();
  if (socket->in == NULL || socket->out == NULL) {
    return -1;
  }
  reset_socket(socket);
  return 0;
}

static void
destroy_socket(void *data) {
  qsocket_t *socket;

  socket = (qsocket_t*)data;
  qbuffer_free(socket->in);
  qbuffer_free(socket->out);
}
