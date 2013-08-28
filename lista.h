#ifndef LISTA_H
#define LISTA_H

typedef struct person_t{
        char name[20];
        char ip[16];
        int  portno;
        int fd;
        struct person_t * seg;
} person;



person * cria_lista();
void insere_pessoa(person * base, char*name, char* ip, int porto, int fd);
void remove_pessoa(person * base, char * name);
int lista_vazia(person * base);
void mostra_lista(person * base);
int verifica_interno(person * base, char* nome);
person * apaga_lista(person * base);
person * retorna_pessoa_por_descriptor(person * base, int descriptor);
#endif
