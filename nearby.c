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
  struct entity *val;
  struct hash_record *next;
} HashRecord;

Hash *new_hashmap();
void hash_insert(Hash *H, int key, struct entity *val);
int hash_lookup(Hash *H, int key, struct entity **val);
int hash_key(Hash *H, int id);
void hash_free(Hash *H);

/* heap.h */
#define SIZE_T_MAX (size_t)(-1)
#define INITIAL_SIZE 100

typedef struct h_node {
  double key;
  int val;
} HNode;

typedef struct heap {
  int size;
  int capacity;
  HNode **arr;
} Heap;

Heap* new_heap();
void heap_insert(Heap *H, double key, int val);
int extract_min(Heap *H, double *key, int *val);
int heap_index_of(Heap *H, int val);
void heap_decrease_key(Heap* H, int index, double key);
void heap_bubble_up(Heap *H, int pos);
void heap_bubble_down(Heap *H, int pos);
void heap_free(Heap *H);
void grow_heap(Heap *H);
int heap_less(struct h_node *a, struct h_node *b);
int heap_greater(struct h_node *a, struct h_node *b);

/* kdtree.h */
typedef struct kd_tree {
  struct entity *val;
  double x;
  double y;
  struct kd_tree *lchild;
  struct kd_tree *rchild;
} KDTree;

KDTree *kdtree(struct entity **entities, int depth, int size);
void kdtree_nearest(struct kd_tree *T, double x, double y, int n, int depth,
    struct heap *H, double cur_max);
int compare_entities_x(const void *a, const void *b);
int compare_entities_y(const void *a, const void *b);
void kdtree_free(KDTree *T);

/* nearby.h */
#define MAX_TOPICS 10000
#define MAX_QUESTIONS 1000
#define MAX_TOPICS_PER_QUESTION 10
#define MAX_QUERIES 10000
#define PLANE_SIZE 1000000

typedef struct entity {
  int id;
  double x;
  double y;
} Entity;

void print_nearest(KDTree *T, double x, double y, int n_results);

/* nearby.c */
int main()
{
  int i, j, k;
  char *line = NULL;
  size_t len;
  int read, offset = 0;
  int n_topics, n_questions, n_topic_questions, n_queries, n_results;
  int topic_id, question_id;
  char query;
  double x, y;
  Entity **topics = malloc(MAX_TOPICS * sizeof(Entity *));
  Entity **questions = malloc((MAX_QUESTIONS * MAX_TOPICS_PER_QUESTION) *
      sizeof(Entity *));
  Entity *entity;
  Hash *topic_hash = new_hashmap();
  KDTree *topic_tree;
  KDTree *question_tree;

  /* Read number of topics, questions, and queries */
  getline(&line, &len, stdin);
  sscanf(line, "%d %d %d", &n_topics, &n_questions, &n_queries);

  /* Load topics and insert into hash */
  for(i=0; i<n_topics; i++) {
    getline(&line, &len, stdin);
    entity = malloc(sizeof(Entity));

    sscanf(line, "%d %lf %lf", &entity->id, &entity->x, &entity->y);

    hash_insert(topic_hash, entity->id, entity);
    topics[i] = entity;
  }

  /* build topic tree */
  topic_tree = kdtree(topics, 0, n_topics);

  /* build array of questions */
  k = 0;
  for(i=0; i<n_questions; i++) {
    getline(&line, &len, stdin);
    sscanf(line, "%d %d%n", &question_id, &n_topic_questions, &offset);

    for(j=0; j<n_topic_questions; j++) {
      sscanf(line+offset, "%d%n", &topic_id, &read);
      offset += read;

      hash_lookup(topic_hash, topic_id, &entity);

      if(entity != NULL) {
        questions[k] = malloc(sizeof(Entity));
        questions[k]->id = question_id;
        questions[k]->x = entity->x;
        questions[k]->y = entity->y;
        k++;
      }
    }
  }

  /* build question tree */
  question_tree = kdtree(questions, 0, k);
  /* no longer need topic hash; was just used to attach x,y to questions */
  hash_free(topic_hash);

  /* run queries */
  for(i=0; i<n_queries; i++) {
    getline(&line, &len, stdin);
    sscanf(line, "%c %d %lf %lf", &query, &n_results, &x, &y);

    if(query == 't') {
      print_nearest(topic_tree, x, y, n_results);
    }
    else {
      print_nearest(question_tree, x, y, n_results);
    }
  }

  kdtree_free(topic_tree);
  kdtree_free(question_tree);
  free(topics);
  free(questions);

  return 0;
}

void print_nearest(KDTree *T, double x, double y, int n_results)
{
  int id;
  Heap *H = new_heap();
  int i;
  double dist;

  kdtree_nearest(T, x, y, n_results, 0, H, PLANE_SIZE);

  if(H->size < n_results)
    n_results = H->size;

  /* Print the first n_results entries in the heap */
  for(i=0; i < n_results; i++) {
    extract_min(H, &dist, &id);
    printf("%d", id);
    if(i < n_results-1) printf(" ");
  }
  printf("\n");

  heap_free(H);
}

/* kdtree.c */
KDTree *kdtree(Entity **entities, int depth, int size)
{
  if(size == 0) return NULL;

  int median;
  int axis = depth % 2;
  KDTree *node = malloc(sizeof(KDTree));

  if(axis == 0)
    qsort(entities, size, sizeof(Entity *), compare_entities_x);
  else
    qsort(entities, size, sizeof(Entity *), compare_entities_y);

  median = (size-1) / 2;

  node->val = entities[median];
  node->x = node->val->x;
  node->y = node->val->y;
  node->lchild = kdtree(entities, depth+1, median);
  node->rchild = kdtree(entities+median+1, depth+1, size-(median+1));

  return node;
}

void kdtree_nearest(KDTree *T, double x, double y, int n, int depth,
    Heap *H, double cur_max)
{
  if(T == NULL)
    return;

  double d;
  double a, b;
  Entity *topic;
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
    kdtree_nearest(T->lchild, x, y, n, depth+1, H, cur_max);
  else
    kdtree_nearest(T->rchild, x, y, n, depth+1, H, cur_max);

  if(H->size < n || dist < cur_max) {
    if((existing = heap_index_of(H, T->val->id)) != -1) {
      heap_decrease_key(H, existing, dist);
    }
    else {
      heap_insert(H, dist, T->val->id);

      if(dist > cur_max)
        cur_max = dist;
    }
  }

  /* If there might be a point closer than our current max on the other side */
  if(H->size < n || pow(a-b, 2) < cur_max) {
    if(a <= b)
      kdtree_nearest(T->rchild, x, y, n, depth+1, H, cur_max);
    else
      kdtree_nearest(T->lchild, x, y, n, depth+1, H, cur_max);
  }
}

int compare_entities_x(const void *a, const void *b)
{
  const struct entity *topic_a = (*(Entity **)a);
  const struct entity *topic_b = (*(Entity **)b);

  if(topic_a->x < topic_b->x)
    return -1;
  else if(topic_a->x > topic_b->x)
    return 1;
  else
    return 0;
}

int compare_entities_y(const void *a, const void *b)
{
  const struct entity *topic_a = a;
  const struct entity *topic_b = b;

  if(topic_a->y < topic_b->y)
    return -1;
  else if(topic_a->y > topic_b->y)
    return 1;
  else
    return 0;
}

void kdtree_free(KDTree *T)
{
  free(T);
}

/* hash.c */
Hash *new_hashmap()
{
  Hash *H = malloc(sizeof(Hash));
  H->size = MAX_TOPICS;
  H->arr = malloc(H->size * sizeof(HashRecord *));
  return H;
}

void hash_insert(Hash *H, int id, Entity *val)
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

int hash_lookup(Hash *H, int id, Entity **val)
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

void heap_insert(Heap *H, double key, int val)
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

int extract_min(Heap *H, double *key, int *val)
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

int heap_index_of(Heap *H, int val)
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
    return a->val < b->val;
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
    return a->val > b->val;
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
