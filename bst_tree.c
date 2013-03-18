TreeNode* new_bst_node(int topic_id, struct point location);
TreeNode* bst_insert(TreeNode* node, int topic_id, struct point location);
struct point* bst_find(TreeNode* node, int topic_id);

TreeNode* new_bst_node(int topic_id, struct point location)
{
  TreeNode *node;

  node = malloc(sizeof(TreeNode));
  node->topic_id = topic_id;
  node->location = location;
  node->left = NULL;
  node->right = NULL;

  return node;
}

TreeNode* bst_insert(TreeNode *node, int topic_id, struct point location)
{
  if(node == NULL) {
    return new_bst_node(topic_id, location);
  }
  else {
    if(topic_id <= node->topic_id) {
      node->left = bst_insert(node->left, topic_id, location);
      return node;
    }
    else {
      node->right = bst_insert(node->right, topic_id, location);
      return node;
    }
  }
}

struct point* bst_find(TreeNode* node, int topic_id)
{
  if(node == NULL) {
    return NULL;
  }
  if(node->topic_id == topic_id) {
    return &node->location;
  }

  if(topic_id <= node->topic_id)
    return bst_find(node->left, topic_id);
  else
    return bst_find(node->right, topic_id);
}

