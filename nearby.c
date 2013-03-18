#include<stdio.h>
#include<stdlib.h>
#include<math.h>

#define MAX_TOPICS 10000
#define MAX_QUESTIONS 1000

struct point {
  double lat;
  double lng;
};

struct topic {
  int id;
  struct point location;
};

typedef struct TreeNode {
  int topic_id;
  struct point location;
  struct TreeNode *left;
  struct TreeNode *right;
} TreeNode;

typedef struct ResultNode {
  int topic_id;
  double distance;
} ResultNode;

TreeNode* kd_tree(struct topic topics[], int n_topics, int depth);
int nearest_neighbors(TreeNode* node, struct point search, int depth, ResultNode *results);
int square_distance(struct point a, struct point b);
int compare_nodes(const void *a, const void *b);
int compare_topics_lat(const void *a, const void *b);
int compare_topics_lng(const void *a, const void *b);

int main()
{
  int n_topics, n_questions, n_queries, n_question_topics;
  int topic_id, question_id;
  char query_type;
  int n_results;
  int n_found;
  int i,j;
  struct topic topics[MAX_TOPICS];
  struct point point;
  TreeNode *kdt;
  TreeNode *found;
  ResultNode *results;

  scanf("%d %d %d\n", &n_topics, &n_questions, &n_queries);
  printf("%d topics, %d questions, %d queries\n", n_topics, n_questions, n_queries);

  for(i=0; i<n_topics; i++) {
    scanf("%d %lf %lf\n", &topic_id, &point.lat, &point.lng);
    topics[i].id = topic_id;
    topics[i].location = point;
  }

  kdt = kd_tree(topics, n_topics, 0);

  for(i=0; i<n_questions; i++) {
    scanf("%d %d", &question_id, &n_question_topics);

    for(j=0; j<n_question_topics; j++) {
      scanf("%d", &topic_id);
    }

    scanf("\n");
  }

  for(i=0; i<n_queries; i++) {
    scanf("%1c %d %lf %lf\n", &query_type, &n_results, &point.lat, &point.lng);

    if(query_type == 't') {
      n_found = nearest_neighbors(kdt, point, 0, &results);
      qsort(results, n_found, sizeof(TreeNode), compare_nodes);

      for(j=0; j < n_found; j++) {
        printf("%d", results[j].topic_id);
        if(n_found - 1 > j) printf(" ");
      }

      printf("\n");
    }
  }
}

TreeNode* kd_tree(struct topic topics[], int n_topics, int depth)
{
  int i;
  int median;
  TreeNode *node;

  if(n_topics == 0) {
    return NULL;
  }

  node = malloc(sizeof(TreeNode));

  if(depth % 2 == 0)
    qsort(topics, n_topics, sizeof(struct topic), compare_topics_lat);
  else
    qsort(topics, n_topics, sizeof(struct topic), compare_topics_lng);

  median = n_topics/2;

  node->topic_id = topics[median].id;
  node->location = topics[median].location;
  node->left = kd_tree(topics, median, depth+1);
  node->right = kd_tree(topics+(median+1), n_topics-(median+1), depth+1);

  return node;
}

int nearest_neighbors(TreeNode* node, struct point search, int depth, ResultNode *results)
{
  TreeNode* best;
  TreeNode* other_best;
  int distance;

  if(node->left == NULL || node->right == NULL)
    return 0;

  if(depth % 2 == 0) { /* by latitude */
    if(search.lat < node->location.lat) {
      best = nearest_neighbors(node->left, search, depth+1, results);
      distance = square_distance(best->location, search);

      /* If there's a possibility that the other side contains a closer point,
       * search it as well */
      if(distance > abs(search.lat - node->location.lat)) {
        other_best = nearest_neighbors(node->right, search, depth+1, results);
        if(square_distance(other_best->location, search) < distance)
          best = other_best;
      }

      return best;
    } else {
      best = nearest_neighbors(node->right, search, depth+1, results);
      distance = square_distance(best->location, search);

      /* If there's a possibility that the other side contains a closer location,
       * search it as well */
      if(distance > abs(search.lat - node->location.lat)) {
        other_best = nearest_neighbors(node->left, search, depth+1, results);
        if(square_distance(other_best->location, search) < distance)
          best = other_best;
      }

      return best;
    }
  } else { /* by longitude */
    if(search.lng < node->location.lng) {
      best = nearest_neighbors(node->left, search, depth+1, results);
      distance = square_distance(best->location, search);

      /* If there's a possibility that the other side contains a closer location,
       * search it as well */
      if(distance > pow((double)abs(search.lng - node->location.lng), 2)) {
        other_best = nearest_neighbors(node->right, search, depth+1, results);
        if(square_distance(other_best->location, search) < distance)
          best = other_best;
      }

      return best;
    } else {
      best = nearest_neighbors(node->right, search, depth+1, results);
      distance = square_distance(best->location, search);

      /* If there's a possibility that the other side contains a closer location,
       * search it as well */
      if(distance > pow((double)abs(search.lng - node->location.lng), 2)) {
        other_best = nearest_neighbors(node->left, search, depth+1, results);
        if(square_distance(other_best->location, search) < distance)
          best = other_best;
      }

      return best;
    }
  }
}

int square_distance(struct point a, struct point b)
{
  return pow((double)abs(a.lat - b.lat), 2) + pow((double)abs(a.lng - b.lng), 2);
}

int compare_nodes(const void *a, const void *b)
{
  const ResultNode *res_a = a;
  const ResultNode *res_b = b;

  if(res_a->distance < res_b->distance)
    return -1;
  else if(res_a->distance > res_b->distance)
    return 1;
  else
    return res_a->topic_id - res_b->topic_id;
}

int compare_topics_lat(const void *a, const void *b)
{
  const struct topic *topic_a = a;
  const struct topic *topic_b = b;

  if(topic_a->location.lat < topic_b->location.lat)
    return -1;
  else if(topic_a->location.lat > topic_b->location.lat)
    return 1;
  else
    return 0;
}

int compare_topics_lng(const void *a, const void *b)
{
  const struct topic *topic_a = a;
  const struct topic *topic_b = b;

  if(topic_a->location.lng < topic_b->location.lng)
    return -1;
  else if(topic_a->location.lng > topic_b->location.lng)
    return 1;
  else
    return 0;
}
