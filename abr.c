#include "abr.h"
#include <stdlib.h>

node *abr_insert(node *root, int val)
{
    int k = shuffle(val);
    node *new_node = malloc(sizeof(*new_node));
    if (new_node == NULL)
    {
        fprintf(stderr, "Error: memory allocation failed for new_node");
        exit(1);
    }
    new_node->val_node = k;
    new_node->left = NULL;
    new_node->right = NULL;
    if (root == NULL) return new_node;
    node *curr = root;
    while (1)
    {
        if (k == curr->val_node)
        {
            free(new_node);
            return root;
        }
        else if (k < curr->val_node)
        {
            if (curr->left == NULL)
            {
                curr->left = new_node;
                return root;
            }
            curr = curr->left;
        }
        else 
        {
            if (curr->right == NULL)
            {
                curr->right = new_node;
                return root;
            }
            curr = curr->right;
        }
    }
}

int abr_search(node *root, int val)
{
    int k = shuffle(val);
    while (root != NULL)
    {
        if (k == root->val_node) return 1;
        if (k < root->val_node) root = root->left;
        else root = root->right;
    }
    return 0;
}

void free_abr(node *root)
{
    if (root == NULL) return;
    free_abr(root->left);
    free_abr(root->right);
    free(root);
}

int shuffle(int n) {
  return ((((n & 0x3F) << 26) | ((n >> 6) & 0x3FFFFFF)) ^ 0x55555555);
}

int unshuffle(int n) {
  return ((((n >> 26) & 0x3F) | ((n & 0x3FFFFFF) << 6)) ^ 0x55555555);
}

