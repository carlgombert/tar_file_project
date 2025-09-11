#include "list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void list_init(list_t *list) {
    list->head = NULL;
    list->size = 0;
}

void list_add(list_t *list, const char *data) {
    // declares new node and data, copies data to node
    node_t *new_node = malloc(sizeof(node_t));
    strcpy(new_node->data, data);
    new_node->next = NULL;

    node_t *curr = list->head;

    if (list->head == NULL) {
        list->head = new_node;
        list->size = 1;

    } else {    // itterates through for last node, adds
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = new_node;
        list->size++;
    }
}

// super chill i will not explain
int list_size(const list_t *list) {
    return list->size;
}

// itterates through, grabs value if its in bounds
char *list_get(const list_t *list, int index) {
    node_t *curr = list->head;
    int cur = 0;

    if (index >= list->size || index < 0)
        return NULL;

    while (cur < index) {
        curr = curr->next;
        cur++;
    }

    return curr->data;
}

// clears and free list, resets list
void list_clear(list_t *list) {
    node_t *curr = list->head;
    node_t *nex;

    while (curr != NULL) {
        nex = curr->next;
        free(curr);
        curr = nex;
    }
    list->head = NULL;
    list->size = 0;
}

// checks if contains, strcmp usage
int list_contains(const list_t *list, const char *query) {
    node_t *curr = list->head;

    while (curr->next != NULL) {
        if (!strcmp(query, curr->data))
            return 1;
        curr = curr->next;
    }

    if (!strcmp(query, curr->data))
        return 1;
    return 0;
}

// print!
void list_print(const list_t *list) {
    int i = 0;
    node_t *current = list->head;
    while (current != NULL) {
        printf("%d: %s\n", i, current->data);
        current = current->next;
        i++;
    }
}
