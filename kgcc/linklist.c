#include <criterion/criterion.h>
#include <stdio.h>
#include <stdlib.h>

struct node {
  void *data;
  size_t size;
  struct node *next;
};

typedef enum {
  LL_OK,
  LL_OOM,
  LL_INDEX_BEYOND_LENGTH,
} ll_status;

struct node *linklist_node(void *value, size_t size) {
  struct node *l = malloc(sizeof(struct node));
  l->data = value;
  l->size = size;
  return l;
}

ll_status linklist_push(struct node **list, void *value, size_t size) {
  struct node *l = linklist_node(value, size);
  if (l == NULL)
    return LL_OOM;
  l->next = *list;
  if (list != NULL)
    *list = l;
  return LL_OK;
}

struct node *linklist_node_str(char *str) {
  return linklist_node(str, strlen(str) * sizeof(char));
}

ll_status linklist_push_string(struct node **list, char *str_to_push) {
  return linklist_push(list, str_to_push, strlen(str_to_push) * sizeof(char));
}

ll_status linklist_append(struct node *list, void *value, size_t size) {
  struct node *l = linklist_node(value, size);
  if (l == NULL)
    return LL_OOM;
  struct node *cur = NULL;
  for (cur = list; list->next != NULL; cur = cur->next) {
  }
  cur->next = l;
  return LL_OK;
}

ll_status linklist_append_string(struct node *list, char *str_to_append) {
  return linklist_append(list, str_to_append,
                         strlen(str_to_append) * sizeof(char));
}

void linklist_print(struct node *list) {
  for (struct node *cur = list; cur != NULL; cur = cur->next) {
    if (cur->data != NULL)
      printf("%s\n", (char *)cur->data);
  }
}

void *linklist_item(struct node *list, const int index) {
  int i = 0;
  while (i < index) {
    if (list == NULL) {
      return NULL;
    }
    list = list->next;
    i++;
  }
  return list;
}

ll_status linklist_free(struct node *list) {
  struct node *tmp;
  while (list != NULL) {
    tmp = list;
    list = list->next;
    free(tmp);
  }
  return LL_OK;
}
