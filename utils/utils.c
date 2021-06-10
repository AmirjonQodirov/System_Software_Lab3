#include "utils.h"

void addToDo(ToDo toDo, ListElement* listRoot){
    ListElement* newItem = malloc(sizeof(ListElement));
    newItem->toDo = toDo;
    if(listRoot->next == NULL){
        newItem->next = NULL;
        listRoot->next = newItem;
    } else{
        newItem->next = listRoot->next;
        listRoot->next = newItem;
    }
}

void changeToDo(ToDo toDo, ListElement* listRoot){
    ListElement *currentItem = listRoot->next;
    while (currentItem != NULL){
        if(currentItem->toDo.id == toDo.id){
            strcpy(currentItem->toDo.list, toDo.list);
            strcpy(currentItem->toDo.title, toDo.title);
            strcpy(currentItem->toDo.description, toDo.description);
            currentItem->toDo.deadline_in_day = toDo.deadline_in_day;
            break;
        }
        currentItem = currentItem->next;
    }
}

void deleteToDo(ToDo toDo, ListElement* currentItem){
    while (currentItem->next != NULL){
        if(currentItem->next->toDo.id == toDo.id){
            ListElement * item = currentItem->next;
            currentItem->next = currentItem->next->next;
            free(item);
            break;
        }
        currentItem = currentItem->next;
    }
}

void deleteToDoList(ToDo toDo, ListElement* currentItem){
    while (currentItem->next != NULL){
        if(strcmp(currentItem->next->toDo.list, toDo.list) == 0 && strcmp(currentItem->next->toDo.user, toDo.user)==0){
            ListElement * item = currentItem->next;
            currentItem->next = currentItem->next->next;
            free(item);
        } else{
            currentItem = currentItem->next;
        }
    }
}

void ProcessingPakage(ToDoPakage toDoPakage, ListElement* mainElement){
    switch (toDoPakage.mode) {
        case ADD_MODE:{
            addToDo(toDoPakage.toDo, mainElement);
            return;
        }
        case CHANGE_MODE:{
            changeToDo(toDoPakage.toDo, mainElement);
            return;
        }
        case DELETE_MODE:{
            deleteToDo(toDoPakage.toDo, mainElement);
            return;
        }
        case DELETE_LIST_MODE:{
            deleteToDoList(toDoPakage.toDo, mainElement);
            return;
        }
        default: return;
    }
}
