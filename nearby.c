#include<stdio.h>
#include<stdlib.h>
#include<math.h>

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
int hash_lookup(Hash *H, int key, struct topic **val);
int hash_contains(Hash *H, int id);
int hash_key(Hash *H, int id);
struct topic **hash_to_a(Hash *H);
void hash_free(Hash *H);

/* heap.h */
#define SIZE_T_MAX (size_t)(-1)
#define INITIAL_SIZE 100

typedef struct h_node {
  double key;
  struct topic *val;
} HNode;

typedef struct heap {
  int size;
  int capacity;
  HNode **arr;
} Heap;

Heap* new_heap();
void heap_insert(Heap *H, double key, struct topic *val);
int extract_min(Heap *H, double *key, struct topic **val);
int heap_index_of(Heap *H, struct topic *val);
void heap_decrease_key(Heap* H, int index, double key);
void heap_bubble_up(Heap *H, int pos);
void heap_bubble_down(Heap *H, int pos);
void heap_free(Heap *H);
void grow_heap(Heap *H);
int heap_less(struct h_node *a, struct h_node *b);
int heap_greater(struct h_node *a, struct h_node *b);

/* kdtree.h */
typedef struct kd_tree {
  struct topic *val;
  double x;
  double y;
  struct kd_tree *lchild;
  struct kd_tree *rchild;
} KDTree;

KDTree *kdtree(struct topic **topics, int depth, int size);
void kdtree_nearest(struct kd_tree *T, double x, double y, int n, int depth,
    struct heap *topics, double cur_max);
int compare_topics_x(const void *a, const void *b);
int compare_topics_y(const void *a, const void *b);
void kdtree_free(KDTree *T);

/* nearby.h */
#define MAX_TOPICS 10000
#define MAX_QUESTIONS 1000
#define MAX_QUERIES 10000
#define PLANE_SIZE 1000000

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
int compare_question_ids(const void *a, const void *b);

/* nearby.c */
int main(int argc, char *argv[])
{
  char *line = NULL;
  size_t len;
  int i;
  int n_topics, n_questions, n_topic_questions, n_queries;
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
    sscanf(line, "%d %d%n", &question_id, &n_topic_questions, &offset);

    while(sscanf(line+offset, "%d%n", &topic_id, &read) == 1) {
      offset += read;

      hash_lookup(H, topic_id, &topic);

      if(topic != NULL) {
        /* Add question to linked list */
        if(topic->question == NULL) {
          topic->question = malloc(sizeof(Question));
          topic->question->id = question_id;
          topic->question->next = NULL;
        }
        else {
          next = topic->question;
          while(next->next != NULL) next = next->next;
          next->next = malloc(sizeof(Question));
          next = next->next;
          next->id = question_id;
          next->next = NULL;
        }
      }
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
  Topic *topic;
  Heap *topics = new_heap();
  int i;
  double dist;

  kdtree_nearest(T, x, y, n_results, 0, topics, PLANE_SIZE);

  if(topics->size < n_results)
    n_results = topics->size;

  /* Print the first n_results entries in the heap */
  for(i=0; i < n_results; i++) {
    extract_min(topics, &dist, &topic);
    printf("%d", topic->id);
    if(i < n_results-1) printf(" ");
  }
  printf("\n");

  heap_free(topics);
}

void print_nearest_questions(KDTree *T, double x, double y, int n_results)
{
  Topic *topic;
  Heap *topics = new_heap();
  Question **questions = malloc(2 * n_results * sizeof(Question *));
  Question *next;
  int i = 0;
  int found = 0;
  int last_step = 0;
  double dist;
  double cur_dist = 0;
  Hash *H = new_hashmap();

  kdtree_nearest(T, x, y, n_results*2, 0, topics, PLANE_SIZE);

  if(topics->size > 0) {
    while(extract_min(topics, &dist, &topic) == 1) {
      if(dist > cur_dist && found > 0) {
        qsort(questions+last_step, found-last_step, sizeof(Question *), compare_question_ids);

        /* once we're finished with all topics at a given distance, we're done
         * if we've found enough questions */
        if(found >= n_results)
          break;

        last_step += found;
        cur_dist = dist;
      }

      /* add all questions to the array that haven't already been seen */
      for(next = topic->question; next != NULL; next = next->next) {
        if(!hash_contains(H, next->id)) {
          questions[found] = next;
          hash_insert(H, next->id, topic);
          found++;
        }
      }
    }

    for(i=0; i < found; i++) {
      printf("%d", questions[i]->id);
      if(i < found-1) printf(" ");
    }
  }

  heap_free(topics);
  free(questions);
  hash_free(H);
}

int compare_question_ids(const void *a, const void *b)
{
  const struct question *question_a = (*(Question **)a);
  const struct question *question_b = (*(Question **)b);
  return question_b->id - question_a->id;
}

/* kdtree.c */
KDTree *kdtree(Topic **topics, int depth, int size)
{
  if(size == 0) return NULL;

  int median;
  int axis = depth % 2;
  KDTree *node = malloc(sizeof(KDTree));

  if(axis == 0)
    qsort(topics, size, sizeof(Topic *), compare_topics_x);
  else
    qsort(topics, size, sizeof(Topic *), compare_topics_y);

  median = (size-1) / 2;

  node->val = topics[median];
  node->x = node->val->x;
  node->y = node->val->y;
  node->lchild = kdtree(topics, depth+1, median);
  node->rchild = kdtree(topics+median+1, depth+1, size-(median+1));

  return node;
}

int compare_topics_x(const void *a, const void *b)
{
  const struct topic *topic_a = (*(Topic **)a);
  const struct topic *topic_b = (*(Topic **)b);

  if(topic_a->x < topic_b->x)
    return -1;
  else if(topic_a->x > topic_b->x)
    return 1;
  else
    return 0;
}

int compare_topics_y(const void *a, const void *b)
{
  const struct topic *topic_a = a;
  const struct topic *topic_b = b;

  if(topic_a->y < topic_b->y)
    return -1;
  else if(topic_a->y > topic_b->y)
    return 1;
  else
    return 0;
}

void kdtree_nearest(KDTree *T, double x, double y, int n, int depth,
    Heap *topics, double cur_max)
{
  if(T == NULL)
    return;

  double d;
  double a, b;
  Topic *topic;
  int axis = depth % 2;
  double dist = pow(T->x - x, 2) + pow(T->y - y, 2);
  int existing;

  if(axis == 0) {
    a = x; b = T->x;
  }
  else {
    a = y; b = T->y;
  }

  /* Follow tree down in appropriate direction */
  if(a <= b)
    kdtree_nearest(T->lchild, x, y, n, depth+1, topics, cur_max);
  else
    kdtree_nearest(T->rchild, x, y, n, depth+1, topics, cur_max);

  /* Add or swap out a value in the heap, if necessary */
  if(topics->size < n || dist < cur_max) {
    if(dist > cur_max)
      cur_max = dist;

    heap_insert(topics, dist, T->val);
  }

  /* If there might be a point closer than our current max on the other side */
  if(topics->size < n || pow(a-b, 2) < cur_max) {
    if(a <= b)
      kdtree_nearest(T->rchild, x, y, n, depth+1, topics, cur_max);
    else
      kdtree_nearest(T->lchild, x, y, n, depth+1, topics, cur_max);
  }
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

int hash_lookup(Hash *H, int id, Topic **val)
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

  return *val != NULL;
}

int hash_contains(Hash *H, int id)
{
  Topic *question;
  int in_hash;
  in_hash = hash_lookup(H, id, &question);
  return in_hash;
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
  /* bootleg, but for our purposes it should do an adequate job */
  return id % H->size;
}

void hash_free(Hash *H)
{
  free(H->arr);
  free(H);
}

/* heap.c */
Heap* new_heap()
{
  Heap* H = malloc(sizeof(Heap));
  H->size = 0;
  H->capacity = INITIAL_SIZE;
  H->arr = malloc(sizeof(HNode*) * INITIAL_SIZE);
  return H;
}

void heap_insert(Heap *H, double key, Topic *val)
{
  int parent;
  int pos = H->size;

  /* Make sure the heap has space to hold the new value */
  grow_heap(H);

  H->arr[pos] = malloc(sizeof(HNode));
  H->arr[pos]->key = key;
  H->arr[pos]->val = val;
  H->size++;

  heap_bubble_up(H, pos);
}

int extract_min(Heap *H, double *key, Topic **val)
{
  if(H->size > 0) {
    *key = H->arr[0]->key;
    *val = H->arr[0]->val;

    H->arr[0] = H->arr[H->size-1];
    H->size--;
    heap_bubble_down(H, 0);

    return 1;
  }
  else {
    return 0;
  }
}

int heap_index_of(Heap *H, Topic *val)
{
  int i;
  int index = -1;
  for(i=0; i < H->size; i++) {
    if(H->arr[i]->val == val) {
      index = i;
      break;
    }
  }

  return index;
}

void heap_decrease_key(Heap* H, int index, double key)
{
  if(key < H->arr[index]->key) {
    H->arr[index]->key = key;
    heap_bubble_up(H, index);
  }
}

void heap_bubble_up(Heap *H, int pos)
{
  int parent;
  HNode *tmp;
  HNode **arr = H->arr;

  if(pos % 2 == 0)
    parent = (pos+1)/2;
  else
    parent = pos/2;

  /* bubble up */
  while(heap_greater(arr[parent], arr[pos])) {
    tmp = arr[parent];
    arr[parent] = arr[pos];
    arr[pos] = tmp;

    pos = parent;

    if(pos % 2 == 0)
      parent = (pos+1)/2;
    else
      parent = pos/2;
  }
}

void heap_bubble_down(Heap *H, int pos)
{
  int child, l_child, r_child, swap;
  HNode *tmp;
  HNode **arr = H->arr;

  if(pos == 0) {
    l_child = 1;
    r_child = 2;
  }
  else {
    l_child = pos*2;
    r_child = pos*2+1;
  }

  /* bubble down */
  while(1) {
    if(r_child >= H->size) {
      if(l_child >= H->size)
        break;
      else
        swap = l_child;
    }
    else {
      if(heap_less(arr[l_child], arr[r_child]))
        swap = l_child;
      else
        swap = r_child;
    }

    if(heap_greater(arr[pos], arr[swap])) {
      tmp = arr[pos];
      arr[pos] = arr[swap];
      arr[swap] = tmp;

      pos = swap;
      l_child = pos*2;
      r_child = pos*2+1;
    }
    else {
      break;
    }
  }
}

int heap_greater(HNode *a, HNode *b)
{
  int a_int = (int)(a->key * 1000);
  int b_int = (int)(b->key * 1000);

  if(a_int == b_int) {
    /* note that the comparison is flipped here, because larger ids are
     * actually "lower" (considered closer) with distance comparisons */
    return a->val->id < b->val->id;
  }
  else {
    return a_int > b_int;
  }
}

int heap_less(HNode *a, HNode *b)
{
  int a_int = (int)(a->key * 1000);
  int b_int = (int)(b->key * 1000);

  if(a_int == b_int) {
    /* note that the comparison is flipped here, because larger ids are
     * actually "lower" (considered closer) with distance comparisons */
    return a->val->id > b->val->id;
  }
  else {
    return a_int < b_int;
  }
}

void heap_free(Heap *H)
{
  free(H->arr);
  free(H);
}

void grow_heap(Heap *H)
{
  if(H->size == H->capacity) {
    int new_capacity = H->capacity * 2;

    if(new_capacity < SIZE_T_MAX/sizeof(int)) {
      HNode **new_array = realloc(H->arr, new_capacity*sizeof(HNode*));

      if(new_array != NULL) {
        H->arr = new_array;
        H->capacity = new_capacity;
      }
      else {
        fputs("Error: out of memory", stderr);
      }
    }
    else {
      fputs("Error: overflow\n", stderr);
    }
  }
}
