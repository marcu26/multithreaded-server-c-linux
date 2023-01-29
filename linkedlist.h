#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>

struct Node
{
    char filename[100];
    char operation[8];
    struct Node *next;
    int index;
};

struct Node *CreateNode(char *filename, char *operation, int _index)
{
    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    memset(newNode->filename, '\0', 100);
    memset(newNode->operation, '\0', 8);
    strcpy(newNode->filename, filename);
    strcpy(newNode->operation, operation);
    newNode->index = _index;
    newNode->next = NULL;
    return newNode;
}

void Append(struct Node **headRef, char *filename, char *operation, int _index)
{
    struct Node *newNode = CreateNode(filename, operation, _index);
    if (*headRef == NULL)
    {
        *headRef = newNode;
        return;
    }
    struct Node *last = *headRef;
    while (last->next != NULL)
    {
        last = last->next;
    }
    last->next = newNode;
}

void DeleteNode(struct Node **headRef, int index)
{
    struct Node *temp = *headRef, *prev;
    if (temp != NULL && temp->index == index)
    {
        *headRef = temp->next;
        free(temp);
        return;
    }
    while (temp != NULL && temp->index != index)
    {
        prev = temp;
        temp = temp->next;
    }
    if (temp == NULL)
        return;
    prev->next = temp->next;
    free(temp);
}

struct Node *SearchInList(struct Node *head, char *filename, char *operation, int index)
{
    struct Node *current = head;
    while (current != NULL)
    {
        if (strcmp(current->filename, filename) == 0 && strcmp(current->operation, operation) == 0 && current->index!=index && current->index<index)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}