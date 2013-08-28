#include "lista.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define BUFFER_SIZE 1024
/*Cria a lista, de estruturas do tipo person*/
person * cria_lista(){
  person * novo;
  novo = (person *) malloc(sizeof(person));
  if (novo == NULL){
    exit(-1);
  }
  novo->seg = NULL;
  return novo;
}

void mostra_lista(person * base){
  person * aux;

  aux = base-> seg;
  printf("participants list:\n");
  while(aux != NULL){
      printf("Name:%s\tIP:%s\tport:%d\n", aux->name, aux->ip, aux->portno);
      aux = aux-> seg;
  }
}


/*Cria uma estrutura do tipo person, preenche os campos e adiciona à lista*/
void insere_pessoa(person * base, char*name, char* ip, int porto, int fd){
  person * aux;
  person * seguinte;
  person * novo;

  aux = base;
  seguinte = aux->seg;
  
  while(seguinte != NULL){
    if(strcmp(aux->name, name) == 0 || strcmp(seguinte->name, name) == 0)
        return;
    aux = aux->seg;
    seguinte = seguinte->seg;
  }
  novo = (person*)malloc(sizeof(person));
  if (novo == NULL){
      puts("erro a alocar memoria\n");
      exit(-1);
  }
  strcpy(novo->name, name);
  strcpy(novo->ip, ip);
  novo->portno = porto;
  novo->fd = fd;
  novo->seg = seguinte;
  aux->seg = novo;
}

/*Remove uma pessoa da lista com base no nome, que é unívoco*/
void remove_pessoa(person * base, char * name){
  person *anterior;
  person * aux;

  anterior = base;
  aux = anterior->seg;

  while(aux != NULL && strcmp(name, aux->name) != 0){
    anterior = anterior->seg;
    aux = aux->seg;
  }
  if (aux != NULL){
    anterior -> seg = aux -> seg;
    free(aux);
  }
}

/*Verifica pelo nome, se aquele user é vizinho interno*/
int verifica_interno(person * base, char* name){
    int ver = 0;
    person *aux;
    aux = base;
    
    while(aux != NULL){
        if(strcmp(name, aux->name) == 0)
            ver = 1;
        aux = aux->seg;
    }
    return ver;
}

/*Retorna a estrutura da pessoa com base no descritor*/
person * retorna_pessoa_por_descriptor(person * base, int descriptor){
    person *aux;
    aux = base;
    
    while(aux != NULL){
        if(aux->fd==descriptor){
            return aux;}
        aux = aux->seg;
    }
    return (person*) NULL;
}



/*Apaga a lista*/
person * apaga_lista(person * base){
    
    
    while(base->seg != NULL){
        remove_pessoa(base->seg, base->seg->name);
        base->seg = base->seg->seg;
    }
    
    return base;
    
}


