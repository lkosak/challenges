#include<stdio.h>
#include<stdlib.h>

/* hash.h */
typedef struct hash {
  struct hash_record **arr;
  int size;
} Hash;

typedef struct hash_record {
  int id;
  struct topic *val;
  struct hash_record *next;
} HashRecord;

Hash *new_hashmap();
void hash_insert(Hash *H, int key, struct topic *val);
void hash_lookup(Hash *H, int key, struct topic **val);
int hash_key(Hash *H, int id);
struct topic **hash_to_a(Hash *H);
void hash_free(Hash *H);

/* kdtree.h */
typedef struct kd_tree {
  struct topic *val;
  double x;
  double y;
  struct kd_tree *lchild;
  struct kd_tree *rchild;
} KDTree;

KDTree *kdtree(struct topic **topics, int depth, int size);
int compare_topics_x(const void *a, const void *b);
int compare_topics_y(const void *a, const void *b);
void kdtree_free(KDTree *T);

/* nearby.h */
#define MAX_TOPICS 10000
#define MAX_QUESTIONS 1000
#define MAX_QUERIES 10000

typedef struct topic {
  int id;
  double x;
  double y;
  struct question *question;
} Topic;

typedef struct question {
  int id;
  struct question *next;
} Question;

void print_nearest_topics(KDTree *T, double x, double y, int n_results);
void print_nearest_questions(KDTree *T, double x, double y, int n_results);

/* nearby.c */
int main(int argc, char *argv[])
{
  char *line = NULL;
  size_t len;
  int i;
  int n_topics, n_questions, n_queries;
  int topic_id, question_id;
  int read;
  double x, y;
  int n_results;
  char query;
  int offset = 0;
  Topic *topic;
  Question *next;
  Hash *H = new_hashmap();
  KDTree *T;

  /* Read number of topics, questions, and queries */
  getline(&line, &len, stdin);
  sscanf(line, "%d %d %d", &n_topics, &n_questions, &n_queries);

  /* Load topics and insert into hash */
  for(i=0; i<n_topics; i++) {
    getline(&line, &len, stdin);
    topic = malloc(sizeof(Topic));
    sscanf(line, "%d %lf %lf", &topic->id, &topic->x, &topic->y);
    hash_insert(H, topic->id, topic);
  }

  /* Build kdtree */
  T = kdtree(hash_to_a(H), 0, n_topics);

  /* Load questions and append to topic linked lists */
  for(i=0; i<n_questions; i++) {
    getline(&line, &len, stdin);
    sscanf(line, "%d", &question_id);

    while(sscanf(line+offset, "%d%n", &topic_id, &read) == 1) {
      offset += read;

      hash_lookup(H, topic_id, &topic);

      next = topic->question;
      while(next != NULL) next = next->next;

      next = malloc(sizeof(Question));
      next->id = question_id;
      next->next = NULL;
    }
  }

  /* Run queries */
  for(i=0; i<n_queries; i++) {
    getline(&line, &len, stdin);
    sscanf(line, "%c %d %lf %lf", &query, &n_results, &x, &y);

    if(query == 't') {
      print_nearest_topics(T, x, y, n_results);
    }
    else {
      print_nearest_questions(T, x, y, n_results);
    }
  }

  hash_free(H);
  kdtree_free(T);
}

void print_nearest_topics(KDTree *T, double x, double y, int n_results)
{
}

void print_nearest_questions(KDTree *T, double x, double y, int n_results)
{
}

/* kdtree.c */
KDTree *kdtree(Topic **topics, int depth, int size)
{
  if(size == 0) return NULL;

  int median;
  int axis = depth % 2;
  KDTree *node = malloc(sizeof(KDTree));

  if(depth == 0)
    qsort(topics, size, sizeof(Topic *), compare_topics_x);
  else
    qsort(topics, size, sizeof(Topic *), compare_topics_y);

  median = size / 2;

  node->val = topics[median];
  node->lchild = kdtree(topics, depth+1, median);
  node->rchild = kdtree(topics+median+1, depth+1, size-(median+1));

  return node;
}

int compare_topics_x(const void *a, const void *b)
{
  const struct topic *topic_a = a;
  const struct topic *topic_b = b;
  return topic_a->x - topic_b->x;
}

int compare_topics_y(const void *a, const void *b)
{
  const struct topic *topic_a = a;
  const struct topic *topic_b = b;
  return topic_a->y - topic_b->y;
}

void kdtree_free(KDTree *T)
{
  free(T);
}

/* hash.c */
Hash *new_hashmap()
{
  int initial_size = MAX_TOPICS*2;
  Hash *H = malloc(sizeof(Hash));
  H->arr = malloc(initial_size * sizeof(HashRecord *));
  H->size = initial_size;
  return H;
}

void hash_insert(Hash *H, int id, Topic *val)
{
  int key = hash_key(H, id);
  HashRecord *next;

  if(H->arr[key] == NULL) {
    H->arr[key] = (HashRecord *)malloc(sizeof(HashRecord *));
    H->arr[key]->id = id;
    H->arr[key]->val = val;
    H->arr[key]->next = NULL;
  }
  else {
    next = H->arr[key];
    while(next->next != NULL) next = next->next;

    next->next = (HashRecord *)malloc(sizeof(HashRecord *));
    next->next->id = id;
    next->next->val = val;
    next->next->next = NULL;
  }
}

void hash_lookup(Hash *H, int id, Topic **val)
{
  int key = hash_key(H, id);
  HashRecord *next;

  *val = NULL;

  if(H->arr[key] != NULL) {
    next = H->arr[key];

    while(next != NULL) {
      if(next->id == id) {
        *val = next->val;
        break;
      }

      next = next->next;
    }
  }
}

Topic **hash_to_a(Hash *H)
{
  Topic **ar = malloc(H->size * sizeof(Topic *));
  int i;
  int j=0;

  for(i=0; i<H->size; i++) {
    if(H->arr[i] != NULL) {
      ar[j] = (Topic *)malloc(sizeof(Topic *));
      ar[j] = H->arr[i]->val;
      j++;
    }
  }

  return ar;
}

int hash_key(Hash *H, int id)
{
  return id % H->size;
}

void hash_free(Hash *H)
{
  free(H->arr);
  free(H);
}
