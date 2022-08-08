#include <stdio.h>
#include <stdlib.h>
#define MAX_FUTURES 100

/* Stolen from:
 * https://gist.github.com/ryankurte/61f95dc71133561ed055ff62b33585f8 */

#include <assert.h>
#include <stdlib.h>

typedef struct {
  size_t head;
  size_t tail;
  size_t size;
  void **data;
} FIFO;

void *Fpop(FIFO *queue) {
  if (queue->tail == queue->head) {
    return NULL;
  }
  void *handle = queue->data[queue->tail];
  queue->data[queue->tail] = NULL;
  queue->tail = (queue->tail + 1) % queue->size;
  return handle;
}

int Fpush(FIFO *queue, void *handle) {
  if (((queue->head + 1) % queue->size) == queue->tail) {
    return -1;
  }
  queue->data[queue->head] = handle;
  queue->head = (queue->head + 1) % queue->size;
  return 0;
}
#define box(type) malloc(sizeof(type));

enum future_state_e { PEDING, READY };

struct future_s {
  void *Data;
  enum future_state_e State;
  struct future_s *(*Pool)(struct future_s *);
};

typedef struct future_s *future_t;

future_t fut() {
  future_t fut = box(future_t);
  return fut;
};

struct read_s {
  FILE *file;
  char *buffer;
  size_t size;
  size_t now;
};

typedef struct read_s *read_t;

future_t read_pool(future_t this) {
  FILE *f = this->Data;
  read_t read = this->Data;
  if (read->now == read->size) {
    this->State = READY;
    return this;
  } else {
    read->now +=
        fread(read->buffer + read->now, 1, read->size - read->now, read->file);
    return this;
  }
  return this;
}

future_t AsyncRead(FILE *f, char *buffer, size_t size) {
  future_t this = fut();
  this->Data = box(struct read_s);

  read_t read = this->Data;
  read->file = f;
  read->buffer = buffer;
  read->size = size;
  read->now = 0;

  this->State = PEDING;
  this->Pool = read_pool;
  return this;
}

struct future_runtime_s {
  FIFO *futures;
};

typedef struct future_runtime_s *future_runtime_t;

future_runtime_t new_rt() {
  future_runtime_t this = box(future_runtime_t);
  this->futures = box(FIFO);
  this->futures->head = 0;
  this->futures->tail = 0;
  this->futures->size = MAX_FUTURES;
  this->futures->data = malloc(sizeof(future_t) * MAX_FUTURES);

  return this;
}

void pushF(future_runtime_t rt, future_t fut) {
  if (fut == NULL) {
    puts("tried to await a null future");
    abort();
  }
  Fpush(rt->futures, fut);
}
void run_rt(future_runtime_t rt) {
  future_t current;
  while (1) {
    current = Fpop(rt->futures);
    if (current == NULL) {
      break;
    }
    if (current->State == READY) {
      free(current);
    } else {
      current = current->Pool(current);
      Fpush(rt->futures, current);
    }
  }
}
int main() {
  future_runtime_t rt = new_rt();
  FILE *f = fopen("Hello.txt", "r");
  char data[256] = {};
  pushF(rt, AsyncRead(f, data, 12));
  run_rt(rt);
  puts(data);
  fclose(f);
}
