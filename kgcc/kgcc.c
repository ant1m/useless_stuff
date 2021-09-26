#include "linklist.c"
#include <cjson/cJSON.h>
#include <ctype.h>
#include <curl/curl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PATH ".kube/"
#define CONFIG "config-"
#define KCONFIG PATH "config"

char *get_home_dir() {
  char *hd = getenv("HOME");
  if (hd == NULL) {
    printf("HOME env variable must be set\n");
    exit(1);
  }
  return hd;
}

struct node *get_kubeconfigs() {
  char *rd = get_home_dir();
  char *path = malloc(sizeof(char) * (strlen(rd) + strlen(PATH) + 2));
  sprintf(path, "%s/%s", rd, PATH);
  DIR *d;
  struct dirent *dir;
  d = opendir(path);
  struct node *list = NULL;
  if (d != NULL) {
    while ((dir = readdir(d)) != NULL) {
      if (strncmp(dir->d_name, CONFIG, 7) == 0) {
        char *fn = (char *)malloc(sizeof(char) * (strlen(path) + sizeof(dir->d_name)));
        strcpy(fn, path);
        strcat(fn, dir->d_name);
        linklist_push_string(&list, fn);
      }
    }
    free(path);
    closedir(d);
  } else {
    perror("Something isn't working");
  }
  return list;
}

struct file_content {
  char *content;
  size_t size;
};

void f_readall(FILE *f, struct file_content **fc) {
  fseek(f, 0L, SEEK_END);
  struct file_content *lfc = malloc(sizeof(struct file_content));
  lfc->size = ftell(f);
  rewind(f);
  lfc->content = malloc((lfc->size + 1) * sizeof(char));
  fscanf(f, "%s", lfc->content);
  lfc->size++;
  *fc = lfc;
}

struct context_cluster {
  char *context;
  char *cluster;
  char *file;
};

int get_data_from_file(char *filename, struct node **result) {
  FILE *f = fopen(filename, "r+");
  struct file_content *fc;
  f_readall(f, &fc);
  cJSON *j = cJSON_Parse(fc->content);
  if (j == NULL) {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr == NULL) {
      fprintf(stderr, "Error before: %s\n", error_ptr);
    }
    cJSON_Delete(j);
    return 1;
  }
  const cJSON *contexts = cJSON_GetObjectItemCaseSensitive(j, "contexts");
  const cJSON *c = NULL;
  cJSON_ArrayForEach(c, contexts) {
    const cJSON *contextName = cJSON_GetObjectItemCaseSensitive(c, "name");
    const cJSON *context = cJSON_GetObjectItemCaseSensitive(c, "context");
    const cJSON *clusterName =
        cJSON_GetObjectItemCaseSensitive(context, "cluster");
    struct context_cluster *cc = malloc(sizeof(struct context_cluster));
    if (linklist_push(result, cc, sizeof(struct context_cluster *)) != LL_OK) {
      return 0;
    }
    cc->context = strdup(contextName->valuestring);
    cc->cluster = strdup(clusterName->valuestring);
    cc->file = strdup(filename);
  }
  cJSON_Delete(j);
  fclose(f);
  free(fc->content);
  free(fc);
  return 0;
}

int display_menu(struct node *cca) {
  printf("Liste des configurations:\n");
  int i = 0;
  for (struct node *c = cca; c != NULL; c = c->next) {
    struct context_cluster *cc = c->data;
    printf("\t%d) %s/%s\n", i, cc->context, cc->cluster);
    i++;
  }
  printf("Choix : ");
  return i - 1;
}

int get_menu_response(int *val, int maxval) {
  char resp[3];
  scanf("%s", resp);
  for (int i = 0; resp[i] != '\0'; i++) {
    if (!isdigit(resp[i])) {
      printf("Entrez une valeur numérique.\n");
      return 1;
    }
  }

  int intresp = atoi(resp);
  if (intresp > maxval) {
    printf("La valeur doit être comprise entre 0 et %d", maxval);
    return -1;
  }
  *val = intresp;
  return 0;
}

int create_symlink(char *file) {
  char *kconfig = KCONFIG;
  if (access(kconfig, F_OK) == 0) {
    if (remove(kconfig) != 0) {
      perror("Échec lors de la suppression du symlink: ");
      return -1;
    }
  }
  if (symlink(file, kconfig)) {
    perror("Échec lors de la création du symlink: ");
    return -1;
  }
  printf("Symlink créé.\n");
  return 0;
}

int main() {
  struct node *configs = get_kubeconfigs();
  struct node *contexts = NULL;
  for (struct node *cur = configs; cur != NULL; cur = cur->next) {
    get_data_from_file(cur->data, &contexts);
  }
  for (struct node *cur = configs; cur != NULL; cur = cur->next) {
    free(cur->data);
  }
  linklist_free(configs);
  int maxvalue = display_menu(contexts);
  int resp = 0;
  if (get_menu_response(&resp, maxvalue) != 0) {
    goto end;
    return 1;
  }
  struct node *selectedItem = linklist_item(contexts, resp);
  if (selectedItem == NULL) {
    printf("la liste des contextes n'a pas d'indice %d\n", resp);
    exit(1);
  }
  struct context_cluster *selectedCc = selectedItem->data;
  printf("Vous avez sélectionné %d : %s:%s (%s)\n", resp, selectedCc->context,
         selectedCc->cluster, selectedCc->file);
  if (create_symlink(selectedCc->file) != 0) {
    goto end;
    return 1;
  }
  goto end;
end:
  for (struct node *cur = contexts; cur != NULL; cur = cur->next) {
    struct context_cluster *c = cur->data;
    free(c->cluster);
    free(c->context);
    free(c->file);
    free(c);
  }
  linklist_free(contexts);
}
