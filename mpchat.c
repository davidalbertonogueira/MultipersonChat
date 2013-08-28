/* RCI Project - Multi-Chat
 * Grupo 1
 * 67537 - André Leitão
 * 67569 - David Nogueira
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include "lista.h"

#define BUFFER_SIZE 1024
#define MAX_SIZE_PARTICIPANTS 60

/*Funcao para escrever no socket fd, o conteudo do buffer, de tamanho size. Se nao conseguir escrever os bytes todos, fica em ciclo ate' completar*/
int escreve(int fd, char*buffer, int size){
    char *ptr;
    int nleft, nwritten;
    nleft=size;
    ptr=buffer;

    while(nleft>0){
        nwritten=write(fd,ptr,nleft);
        if(nwritten<=0)return -1;/*error*/
        nleft-=nwritten;
        ptr+=nwritten;
    }
    return nwritten;

}


int main(int argc, char * argv[]){
    void (*old_handler)(int);/*interrupt handler*/
    char bell=7;
    int fd, addrlen, peeraddrlen,newsockfd, udpfd,prport=58000, lnkport=9000,i,j,k, x,y,z, aux, existe_ext=0;
    fd_set descriptorlist_master , descriptorlist;
    int maxfd, ext_fd;
    int counter;
    char stdin_buffer[BUFFER_SIZE];
    int stdin_buffer_position_read=0;
    int stdin_buffer_position_write=0;
    char participants_buffer[MAX_SIZE_PARTICIPANTS][BUFFER_SIZE];
    int participants_buffer_position_read[MAX_SIZE_PARTICIPANTS];
    int participants_buffer_position_write[MAX_SIZE_PARTICIPANTS];    
    struct in_addr *a_UDP;
    struct hostent *h_UDP;
    
    struct sockaddr_in addr; /*server para escutar*/
    struct sockaddr_in peeraddr; /*novas ligacoes no accept*/
    struct sockaddr_in UDP_addr; /*ligacao ao servidor de nomes    */
    struct sockaddr_in ext_addr; /*ligacao a um externo */
    int n;                                                                      
    char buffer[BUFFER_SIZE];
    char buffer2[BUFFER_SIZE],portaux[6];
    char buffer3[BUFFER_SIZE], comando1[20], comando2[20];
      
    person externalneighbour;
    person user; /*myself*/
    person backup;
    person participant,internalneighbour;
    person * participants, * internalneighbours, *aux_p;
    
    
    participants = cria_lista();
    internalneighbours = cria_lista();
    
     memset((void*)&stdin_buffer,(int)'\0',BUFFER_SIZE*sizeof(char));
    for(i=0;i<MAX_SIZE_PARTICIPANTS;i++){
        memset((void*)&participants_buffer[i],(int)'\0',BUFFER_SIZE*sizeof(char));
    }
      
    memset((void*)&(user.name),(int)'\0',20*sizeof(char));
    memset((void*)&(user.ip),(int)'\0',16*sizeof(char));
    memset((void*)&(portaux),(int)'\0',6*sizeof(char));
    
    memset((void*)&(user),(int)'\0',sizeof(user));
    memset((void*)&(externalneighbour),(int)'\0',sizeof(externalneighbour));
    memset((void*)&( backup),(int)'\0',sizeof( backup));
    puts("welcome\n");
    
    if((old_handler=signal(SIGPIPE,SIG_IGN))==SIG_ERR)exit(1);/*error*/
    
    
    h_UDP=gethostbyname("tejo.ist.utl.pt");
    a_UDP=(struct in_addr*)h_UDP->h_addr_list[0];
    
    if (argc%2==0 || argc>7){printf("Wrong sintax. mpchat [-s addr] [-p prport] [-l lnkport]\n");exit(-1);}
    if (argc == 3 ||argc == 5 ||argc == 7) {
        for(i=3; i<=argc; i++,i++){
            if (strcmp("-s", argv[i-2])==0){
                if((h_UDP=gethostbyname(argv[i-1]))==NULL)exit(1);
                a_UDP=(struct in_addr*)h_UDP->h_addr_list[0];
            }else
                if (strcmp("-p", argv[i-2])==0){
                    prport=atoi(argv[i-1]);
                }else
                    if (strcmp("-l", argv[i-2])==0){
                        lnkport=atoi(argv[i-1]);
                    }else{
                        printf("Wrong sintax. mpchat [-s addr] [-p prport] [-l lnkport]\n");exit(-1);   
                    }
                    
        }
    }

    
    if((fd=socket(AF_INET,SOCK_STREAM,0))==-1)exit(1);/*error*/

    if((udpfd=socket(AF_INET,SOCK_DGRAM,0))==-1)exit(1);/*error*/

    /*inicializacao do user*/
    user.portno=lnkport;
    user.fd=fd;

    /*Preenche estruturas*/
    memset((void*)&addr,(int)'\0',sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    addr.sin_port=htons(lnkport);
     /*Preenche estruturas*/
    memset((void*)&UDP_addr,(int)'\0',sizeof(UDP_addr));
    UDP_addr.sin_family=AF_INET;
    UDP_addr.sin_addr = *a_UDP;
    UDP_addr.sin_port=htons(prport);

    if(bind(fd,(struct sockaddr*)&addr,sizeof(addr))==-1){puts("Error on bind\n");exit(1);}/*error*/

    if(listen(fd,20)==-1)exit(1);/*error*/
    FD_ZERO(&descriptorlist );
    FD_ZERO(&descriptorlist_master );
    FD_SET(fd,&descriptorlist_master );
    FD_SET(fileno( stdin ),&descriptorlist_master );
    maxfd=fd;

    /*Fica indefinidamente neste while cycle 'a espera de dados nos descriptores, e quando detecta, processa*/
     while(1){   
        descriptorlist=descriptorlist_master;
        /*o master guarda os dados sempre. O descriptorlist e' o que e' usado no select e que portanto, e' alterado aquando da saida da funcao. A cada nova iteracao, volta a ser inicializado.*/
        counter=select(maxfd+1,&descriptorlist , (fd_set*)NULL,(fd_set*)NULL,(struct timeval *)NULL);
        if(counter<=0)exit(1);/*error*/
        
        for(k=0;k<=maxfd;k++){
            if(FD_ISSET(k,&descriptorlist )){
                
                /*descritor que esta a aceitar novas ligacoes e atribui um novo descritor a cada uma que chega*/
                if(k==fd){
                    memset((void*)&peeraddr,(int)'\0',sizeof(peeraddr));
                    peeraddrlen=sizeof(peeraddr);
                    if((newsockfd=accept(fd,(struct sockaddr*)&peeraddr,&peeraddrlen))==-1)exit(1);/*error*/
                    printf("New connection\n");
            
                    FD_SET(newsockfd, &descriptorlist_master); /*adiciona ao master */
                    if(newsockfd > maxfd)
                    { /* compara e ve se e' maior */
                        maxfd = newsockfd;
                    }
                    
            
                }
                
                else if(k==fileno( stdin )){/*se veio do teclado*/ 
                    
                    memset((void*)&buffer3,(int)'\0',BUFFER_SIZE*sizeof(char));
                    
                    if((n=read(k,buffer3,BUFFER_SIZE))!=0){
                        if(n==-1)exit(1);/*error*/
                        for(z=0;z<n;z++){/*escreve os dados lidos no buffer circular, e posiciona o ponteiro de escrita*/
                            stdin_buffer[stdin_buffer_position_write%BUFFER_SIZE]=buffer3[z];
                            stdin_buffer_position_write++;
                        }

                        for(x=stdin_buffer_position_read;x<(stdin_buffer_position_write);x++){
                            /*analisa desde a posicao da ultima leitura ate' ao da ultima escrita, se ja' temos algum comando, procurando pela terminacao \n*/
                            if(stdin_buffer[x%BUFFER_SIZE]=='\n'){
                                /*se houver, pegamos nesse comando e vamos processar*/
                                memset((void*)&buffer3,(int)'\0',BUFFER_SIZE*sizeof(char));
                                for(z=stdin_buffer_position_read,y=0;z<=x;z++,y++){
                                    buffer3[y]=stdin_buffer[z%BUFFER_SIZE];
                                }
                                /*a posicao de leitura e' redefinida*/
                                stdin_buffer_position_read=x+1;
                                
                                bzero(comando1,20);
                                bzero(comando2,20);
                                sscanf(buffer3, "%s", comando1);
                    
                                if(strcmp(comando1,"join")==0){/*Genericamente, corresponde a fazer o REG*/
                                    sscanf(buffer3, "%s %s", comando1, comando2);
                                    strcpy(user.name,comando2);
                                    
                                    sprintf(buffer,"REG %s;%d\n",comando2,user.portno); 
                                    n= sendto(udpfd, buffer,strlen(buffer),0,(struct sockaddr*)&UDP_addr, sizeof(UDP_addr));
                                    if(n== -1){
                                        puts("JOIN: Error on send\n"); exit(1);
                                    }
                                    memset((void*)&buffer,(int)'\0',BUFFER_SIZE*sizeof(char));
                                    addrlen=sizeof(UDP_addr);
                                    n=recvfrom(udpfd,buffer,BUFFER_SIZE,0,(struct sockaddr*)&UDP_addr,&addrlen);
                                    if(n== -1){
                                        puts("JOIN: Error on receive\n"); exit(1);
                                    }
                                    sscanf(buffer, "%s", comando1);
                                    if(strcmp(comando1,"ROK")==0){
                                        printf("JOIN: successful command\n");
                                        sprintf(buffer,"QRY %s %s\n",user.name, user.name); 
                                        n= sendto(udpfd, buffer,strlen(buffer),0,(struct sockaddr*)&UDP_addr, sizeof(UDP_addr));
                                        /*comunica com o servidor de nomes para obter os dados do user*/
                                        if(n== -1){
                                            puts("JOIN: Error on send\n"); exit(1);
                                        }
                                        memset((void*)&buffer,(int)'\0',BUFFER_SIZE*sizeof(char));
                                        addrlen=sizeof(UDP_addr);
                                        n=recvfrom(udpfd,buffer,BUFFER_SIZE,0,(struct sockaddr*)&UDP_addr,&addrlen);
                                        
                                        if(n== -1){
                                            puts("JOIN: Error on receive\n"); exit(1);
                                        }
                                        sscanf(buffer, "%s",comando1);
                                        
                                        j=0;
                                        aux=0;
                                        for(i=4;buffer[i] != '\n'; i++){
                                            if(buffer[i]==';'){
                                                j++;
                                                aux=0;
                                            }
                                            else{
                                                if(j==1){
                                                    user.ip[aux]=buffer[i];
                                                    aux++;
                                                }
                                            }
                                        }
                                        fflush(stdout);
                                        printf("JOIN: user %s registed with IP: %s port %d\n",user.name,user.ip,user.portno); 
                                        /*fica backup de si proprio*/
                                        strcpy(backup.name, user.name);
                                        strcpy(backup.ip, user.ip);
                                        backup.portno = user.portno;
                                    }
                                    if(strcmp(comando1,"RNOK")==0)
                                        printf("JOIN: unsuccessful command\n");
                                    insere_pessoa(participants , user.name , user.ip , user.portno , -1);
                                }
                             
                                
                                else if(strcmp(comando1,"leave")==0){/*Genericamente, corresponde a fazer o UNR*/
                                    memset((void*)&buffer,(int)'\0',BUFFER_SIZE*sizeof(char));
                                    sprintf(buffer,"UNR %s\n",user.name); 
                                    n= sendto(udpfd, buffer,strlen(buffer),0,(struct sockaddr*)&UDP_addr, sizeof(UDP_addr));
                                    if(n== -1){
                                        puts("LEAVE: Error on send\n"); exit(1);
                                    }
                                    memset((void*)&buffer,(int)'\0',BUFFER_SIZE*sizeof(char));
                                    addrlen=sizeof(UDP_addr);
                                    n=recvfrom(udpfd,buffer,BUFFER_SIZE,0,(struct sockaddr*)&UDP_addr,&addrlen);
                                    if(n== -1){
                                        puts("LEAVE: Error on receive\n"); exit(1);
                                    }
                                    sscanf(buffer, "%s",comando1);
                                    if(strcmp(comando1,"UOK")==0)
                                            printf("LEAVE: command sent\n");
                                }

                                else if(strcmp(comando1,"show")==0){
                                    sscanf(buffer3, "%s %s", comando1, comando2);
                                    if(strcmp(comando2,"users") != 0 && strcmp(comando2,"participants") != 0 && strcmp(comando2,"inter") != 0 && strcmp(comando2,"exter") != 0){
                                        puts("SHOW: Wrong sintax.\nCorrect: show users or show participants\n");
                                    }
                                    else if(strcmp(comando2,"users") ==0 ){/*Genericamente, corresponde a fazer o USERS*/
                                        memset((void*)&buffer,(int)'\0',BUFFER_SIZE*sizeof(char));
                                        sprintf(buffer,"USERS %s\n",user.name); 
                                        n= sendto(udpfd, buffer,strlen(buffer),0,(struct sockaddr*)&UDP_addr, sizeof(UDP_addr));
                                        if(n== -1){
                                                puts("SHOW: Error on send\n"); exit(1);
                                        }
                                        memset((void*)&buffer,(int)'\0',BUFFER_SIZE*sizeof(char));
                                        addrlen=sizeof(UDP_addr);
                                        n=recvfrom(udpfd,buffer,BUFFER_SIZE,0,(struct sockaddr*)&UDP_addr,&addrlen);
                                        if(n== -1){
                                                puts("SHOW: Error on receive\n"); exit(1);
                                        }
                                        sscanf(buffer, "%s",comando1);
                                        if(strcmp(comando1,"ANOK")==0){
                                                printf("SHOW: user not registed\n");
                                        }else{
                                                puts("USERS:");
                                        printf("%s\n",buffer);
                                        }
                                    }   
                                    else if(strcmp(comando2,"participants") ==0 ){
                                            /*percorre a lista de participants e imprimir os nomes*/
                                            mostra_lista(participants);
                                    }
                                    else if(strcmp(comando2,"inter") ==0 ){
                                            /*percorre a lista de internos e imprimir os nomes*/
                                            mostra_lista(internalneighbours);
                                    }
                                    else if(strcmp(comando2,"exter") ==0 ){
                                             printf("Name:%s\tIP:%s\tport:%d\n", externalneighbour.name, externalneighbour.ip, externalneighbour.portno);
      
                                    }
                                        
                                }
                    
                                else if(strcmp(comando1,"connect")==0){/*Genericamente, corresponde a fazer o uma QRY com o nome, preencher a estrutura, fazer connect, adicionar descritor ao select e enviar NEW*/
                                    
                                    if (existe_ext==1){printf("CONNECT: You already have an external and you're active in a conversation. To join another, disconnect from this and join the other.\n");}
                                    else{
                                        sscanf(buffer3, "%s %s", comando1, comando2);
                                        if(user.name[0]=='\0' || strcmp(user.name,comando2) == 0){
                                            puts("CONNECT: invalid command");
                                        }
                                        else{
                                            memset((void*)&buffer,(int)'\0',BUFFER_SIZE*sizeof(char));
                                            sprintf(buffer,"QRY %s %s\n",user.name, comando2); 
                                            n= sendto(udpfd, buffer,strlen(buffer),0,(struct sockaddr*)&UDP_addr, sizeof(UDP_addr));
                                            /*comunica com o servidor de nomes para obter os dados do user*/
                                            if(n== -1){
                                                puts("CONNECT: Error on send\n"); exit(1);
                                            }
                                            memset((void*)&buffer,(int)'\0',BUFFER_SIZE*sizeof(char));
                                            addrlen=sizeof(UDP_addr);
                                            n=recvfrom(udpfd,buffer,BUFFER_SIZE,0,(struct sockaddr*)&UDP_addr,&addrlen);
                                            if(n== -1){
                                                puts("CONNECT: Error on receive\n"); exit(1);
                                            }
                                            sscanf(buffer, "%s",comando1);
                                            if(strcmp(comando1,"ANOK")==0){
                                                printf("CONNECT: user not registed\n");
                                            }
                                            else{
                                                memset((void*)&(externalneighbour.name),(int)'\0',20*sizeof(char));
                                                memset((void*)&(externalneighbour.ip),(int)'\0',16*sizeof(char));
                                                memset((void*)&(portaux),(int)'\0',6*sizeof(char));
                                                j=0;
                                                aux=0;
                                                for(i=4;buffer[i] != '\n'; i++){
                                                    
                                                    if(buffer[i]==';'){
                                                        j++;
                                                        aux=0;
                                                    }
                                                    else{
                                                        if(j==0){
                                                            externalneighbour.name[aux]=buffer[i];
                                                            aux++;
                                                        }
                                                        if(j==1){
                                                            externalneighbour.ip[aux]=buffer[i];
                                                            aux++;
                                                        }
                                                        if(j==2){
                                                            portaux[aux]=buffer[i];
                                                            aux++;
                                                        }
                                                    }
                                                        
                                                }
                                                externalneighbour.portno = atoi(portaux);
                                            
                                            
                                                if((ext_fd=socket(AF_INET,SOCK_STREAM,0))==-1){
                                                    puts("CONNECT:error creating socket\n");
                                                    exit(1);
                                                }
                                                externalneighbour.fd = ext_fd;
                                                
                                                ext_addr.sin_family=AF_INET;
                                                inet_aton(externalneighbour.ip, &ext_addr.sin_addr);
                                                ext_addr.sin_port=htons(externalneighbour.portno);
                                                
                                                printf("CONNECT:trying to connect %s at %s : %d\n",externalneighbour.name,externalneighbour.ip,externalneighbour.portno);
                                                n=connect(ext_fd,(struct sockaddr*)&ext_addr,sizeof(ext_addr));
                                                if(n==-1){
                                                        printf("CONNECT:error on connect %s at %s : %d\n",externalneighbour.name,externalneighbour.ip,externalneighbour.portno);
                                                }
                                                else{
                                                    memset((void*)&buffer,(int)'\0',BUFFER_SIZE*sizeof(char));
                                                    sprintf(buffer,"NEW %s;%s;%d\n",user.name, user.ip, user.portno); 
                                                    
                                                    n= escreve(ext_fd,buffer,strlen(buffer));
                                                    if(n<=0){
                                                            puts("CONNECT:Couldn't write. \nConnection aborted. Please try again later or try to connect to a different user.\n");
                                                    }else{
                                                        existe_ext = 1;
                                                        insere_pessoa(participants,externalneighbour.name,externalneighbour.ip,externalneighbour.portno, ext_fd);
                                                        FD_SET(ext_fd, &descriptorlist_master);  /* adiciono ao master*/
                                                        if(ext_fd > maxfd)
                                                        { /* comparo para ver se e' maior */
                                                            maxfd = ext_fd;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                    
                                else if(strcmp(comando1,"disconnect")==0){/*Genericamente, corresponde a enviar SUB 'a sua vizinhanca*/
                                    //enviar um "SUB user.name" para vizinhos internos e externo
                                    memset((void*)&buffer,(int)'\0',BUFFER_SIZE*sizeof(char));
                                    sprintf(buffer,"SUB %s\n",user.name); 
                                    n= escreve(externalneighbour.fd,buffer,strlen(buffer));
                                    if(n<=0)
                                        puts("DISCONNECT: Couldn't write SUB to externalneighbour\n");

                                    aux_p= internalneighbours->seg;
                                    while(aux_p != NULL){
                                        if(aux_p->fd != externalneighbour.fd){
                                            n= escreve(aux_p->fd,buffer,strlen(buffer));
                                            if(n<=0)
                                            { printf("DISCONNECT: Couldn't write to internalneighbour %s\n",aux_p->name);  }
                                        }
                                        aux_p=aux_p->seg;
                                        
                                    }
                                    existe_ext=0;
                                    participants = apaga_lista(participants);
                                    insere_pessoa(participants , user.name , user.ip , user.portno , -1);//volto a adicionar-me à lista
                                    internalneighbours = apaga_lista(internalneighbours);
                                    
                                    FD_ZERO(&descriptorlist );
                                    FD_ZERO(&descriptorlist_master );
                                    FD_SET(fd,&descriptorlist_master );
                                    FD_SET(fileno( stdin ),&descriptorlist_master );
                                    maxfd=fd;

                                }
                                
                                else if(strcmp(comando1,"message") == 0){/*Genericamente, corresponde a enviar um "MSG user.name string" para vizinhos internos e externo*/
                                        
                                    memset((void*)&buffer2,(int)'\0',BUFFER_SIZE*sizeof(char));
                                    memset((void*)&buffer,(int)'\0',BUFFER_SIZE*sizeof(char));
                                    aux=0;
                                    for(i=8;buffer3[i] != '\n'; i++){
                                        buffer2[aux] = buffer3[i];
                                        aux++;
                                    }
                                    
                                    sprintf(buffer,"MSS %s %s\n",user.name, buffer2); 
                                    n = escreve(externalneighbour.fd,buffer,strlen(buffer));
                                    if(n<=0)
                                        puts("MESSAGE:Couldn't write message to externalneighbour\n");
                                    
                                    
                                    aux_p = internalneighbours->seg;
                                    while(aux_p != NULL){
                                        if(aux_p->fd != externalneighbour.fd){
                                            n = escreve(aux_p->fd, buffer, strlen(buffer));
                                            if(n == -1){
                                                printf("MESSAGE:Error on write to internalneighbour %s\n",aux_p->name);
                                            }
                                        }
                                        aux_p = aux_p->seg;
                                    }
                                }         
                                
                                else if(strcmp(comando1,"exit")==0){
                                        exit(0);
                                }
                    
                                else{
                                         printf("Command from stdin : %s -- not recognized\n", buffer3);
                                }

                            }  
                        }
                       
                        
            
                    } // end of if((n=read(k,buffer3,BUFFER_SIZE))!=0)
                } //end of else if(k==fileno( stdin ))
                else{
                    
                
                    //qualquer outro descritor, mensagens podem ser ADD, SUB, MSS, NEW, BCKP
                    memset((void*)&buffer,(int)'\0',BUFFER_SIZE*sizeof(char));
                    if((n=read(k,buffer,BUFFER_SIZE))>0)
                    {
                    
                    
                        for(z=0;z<n;z++){/*escreve os dados lidos no buffer circular, e posiciona o ponteiro de escrita*/
                            participants_buffer[k][participants_buffer_position_write[k]%BUFFER_SIZE]=buffer[z];
                            participants_buffer_position_write[k]++;
                        }
                        for(x=participants_buffer_position_read[k];x<(participants_buffer_position_write[k]);x++){
                            /*analisa desde a posicao da ultima leitura ate' ao da ultima escrita, se ja' temos algum comando, procurando pela terminacao \n*/
                            if( participants_buffer[k][x%BUFFER_SIZE]=='\n'){
                                /*se houver, pegamos nesse comando e vamos processar*/
                                memset((void*)&buffer,(int)'\0',BUFFER_SIZE*sizeof(char));
                                for(z=participants_buffer_position_read[k],y=0;z<=x;z++,y++){
                                    buffer[y]= participants_buffer[k][z%BUFFER_SIZE];
                                }
                                /*a posicao de leitura e' redefinida*/
                                participants_buffer_position_read[k]=x+1;
                                                                
                                memset((void*)&comando1,(int)'\0',20*sizeof(char));
                                memset((void*)&comando2,(int)'\0',20*sizeof(char));
                                sscanf(buffer, "%s",comando1);
                                
                                
                                
                                //recebe ADD
                                if(strcmp(comando1,"ADD")==0){/*Genericamente, corresponde a fazer a fazer QRY com o nome do user, a adicionar 'a lista de participantes e a espalhar a mensagem*/
                                    
                                    sscanf(buffer, "%s %s",comando1, comando2);
                                    memset((void*)&buffer2,(int)'\0',BUFFER_SIZE*sizeof(char));
                                    sprintf(buffer2,"QRY %s %s\n",user.name, comando2); 
                                    n= sendto(udpfd, buffer2,strlen(buffer2),0,(struct sockaddr*)&UDP_addr, sizeof(UDP_addr));
                                    /*comunica com o servidor de nomes para obter os dados do user*/
                                    if(n== -1){
                                        puts("ADD: Error on send\n"); exit(1);
                                    }                                
                                    memset((void*)&buffer2,(int)'\0',BUFFER_SIZE*sizeof(char));
                                    addrlen=sizeof(UDP_addr);
                                    n=recvfrom(udpfd,buffer2,BUFFER_SIZE,0,(struct sockaddr*)&UDP_addr,&addrlen);
                                    if(n== -1){
                                        puts("ADD: Error on receive\n"); exit(1);
                                    }      
                                    sscanf(buffer2, "%s",comando1);
                                    if(strcmp(comando1,"ANOK")==0){
                                        printf("ADD: user not registed\n");
                                    }
                                    else{
                                        memset((void*)&(participant.name),(int)'\0',20*sizeof(char));
                                        memset((void*)&(participant.ip),(int)'\0',16*sizeof(char));
                                        memset((void*)&(portaux),(int)'\0',6*sizeof(char));
                                        j=0;
                                        aux=0;
                                        for(i=4;buffer2[i] != '\n'; i++){
                                            
                                            if(buffer2[i]==';'){
                                                j++;
                                                aux=0;
                                            }
                                            else{
                                                if(j==0){
                                                    participant.name[aux]=buffer2[i];
                                                    aux++;
                                                }
                                                if(j==1){
                                                    participant.ip[aux]=buffer2[i];
                                                    aux++;
                                                }
                                                if(j==2){
                                                    portaux[aux]=buffer2[i];
                                                    aux++;
                                                }
                                            }
                                                
                                        }
                                        participant.portno = atoi(portaux);
                                    }
/*   ADD
 
 1- tenho vizinhos internos e externo -> envio a mensagem que recebi a cada um deles, excepto ao que me enviou. Adiciono 'a minha lista de participants
 2- so tenho vizinho externo -> adiciono 'a minha lista de participants*/
                                    insere_pessoa(participants,participant.name,participant.ip,participant.portno,-1);
                                    
                                    /*codigo para reenviar a mensagem que recebeu*/
                                    if(k != externalneighbour.fd){
                                        n = escreve(externalneighbour.fd, buffer, strlen(buffer));
                                        if(n == -1){
                                            puts("ADD: Error on write to externalneighbour\n");
                                        }     
                                    }
                                    aux_p = internalneighbours->seg;
                                    while(aux_p != NULL){
                                        if((aux_p->fd != k) && (aux_p->fd != externalneighbour.fd)){
                                            n = escreve(aux_p->fd, buffer, strlen(buffer));
                                            if(n == -1){
                                                printf("ADD: Error on write to internalneighbour %s\n", aux_p->name);
                                            }
                                        }
                                            aux_p = aux_p->seg;
                                    }
                                    
                                }/*end of   if(strcmp(comando1,"ADD")==0){*/ 
                                
                                
                                /*recebe SUB*/ 
                                else if(strcmp(comando1,"SUB") == 0){/*Genericamente, corresponde a remover o user das listas, a espalhar a mensagem. Tem 4 configuracoes possiveis abaixo indicadas.*/
                                    /*codigo para reenviar a mensagem que recebeu*/ 
                                    if(k != externalneighbour.fd){
                                        n = escreve(externalneighbour.fd, buffer, strlen(buffer));
                                        if(n == -1){
                                            puts("SUB: Error on write to externalneighbour\n");
                                        }     
                                    }
                                    aux_p = internalneighbours->seg;
                                    while(aux_p != NULL){
                                        if((aux_p->fd != k) && (aux_p->fd != externalneighbour.fd)){
                                            n = escreve(aux_p->fd, buffer, strlen(buffer));
                                            if(n == -1){
                                                printf("SUB: Error on write to internalneighbour %s\n", aux_p->name);
                                            }
                                        }
                                        aux_p = aux_p->seg;
                                    }
                                    
                                    sscanf(buffer, "%s %s",comando1,comando2);
/* SUB

 1- não esta na minha vizinhança                 -> tirar participants
 2- é meu vizinho interno mas nao é externo      -> tiro de participants e de internalneighbour,  fecha socket, tira do select
 3- é meu vizinho externo, mas nao é interno     -> tirar participants, fecha socket, tira do select. Faço connect ao meu backup
 4- é meu vizinho externo e interno              -> tiro de participants e de internalneighbour,  fecha socket, tira do select. Adiciono um dos internos como meu externo, se houver. A haver, envio-lhe backup de si proprio
*/                                    
                                    if(verifica_interno(internalneighbours, comando2) == 0 && strcmp(comando2,externalneighbour.name) != 0){/*nao esta na minha vizinhanca*/ 
                                        remove_pessoa(participants,comando2);
                                    }
                                    else if(verifica_interno(internalneighbours, comando2) == 1 && strcmp(comando2,externalneighbour.name) != 0){
                                        remove_pessoa(internalneighbours,comando2);
                                        remove_pessoa(participants,comando2);
                                        FD_CLR(k, &descriptorlist_master);
                                        close(k);
                                    } 
                                    else if(verifica_interno(internalneighbours, comando2) == 0 && strcmp(comando2,externalneighbour.name) == 0){
                                        remove_pessoa(participants,comando2);
                                        FD_CLR(k, &descriptorlist_master);
                                        close(k);
                                        existe_ext=0;
                                        memset((void*)&(externalneighbour.name),(int)'\0',20*sizeof(char));
                                        memset((void*)&(externalneighbour.ip),(int)'\0',16*sizeof(char));
                                        memset((void*)&(portaux),(int)'\0',6*sizeof(char));
                                        strcpy(externalneighbour.name,backup.name);
                                        strcpy(externalneighbour.ip,backup.ip);
                                        externalneighbour.portno = backup.portno;
                                        
                                        if((ext_fd=socket(AF_INET,SOCK_STREAM,0))==-1){
                                            puts("SUB: error creating socket\n");
                                            exit(1);
                                        }
                                        externalneighbour.fd = ext_fd;
                                        
                                        ext_addr.sin_family=AF_INET;
                                        inet_aton(externalneighbour.ip, &ext_addr.sin_addr);
                                        ext_addr.sin_port=htons(externalneighbour.portno);
                                        
                                        printf("SUB: trying to connect %s at %s : %d\n",externalneighbour.name,externalneighbour.ip,externalneighbour.portno);
                                        n=connect(ext_fd,(struct sockaddr*)&ext_addr,sizeof(ext_addr));
                                        if(n==-1){
                                                printf("SUB: error on connect with %s at %s : %d\n",externalneighbour.name,externalneighbour.ip,externalneighbour.portno);
                                        }
                                        else{
                                            existe_ext = 1;
                                            insere_pessoa(participants,externalneighbour.name,externalneighbour.ip,externalneighbour.portno, ext_fd);
                                            memset((void*)&buffer,(int)'\0',BUFFER_SIZE*sizeof(char));
                                            sprintf(buffer,"NEW %s;%s;%d\n",user.name, user.ip, user.portno); 
                                            n= escreve(ext_fd,buffer,strlen(buffer));
                                            if(n<=0)
                                                    puts("SUB: Couldn't write to new externalneighbour\n");

                                            FD_SET(ext_fd, &descriptorlist_master); /* adiciono ao master*/
                                            if(ext_fd > maxfd)
                                            { /* compara para ver se e' maior */
                                                maxfd = ext_fd;
                                            }
                                            sprintf(buffer,"BCKP %s;%s;%d\n",externalneighbour.name, externalneighbour.ip, externalneighbour.portno); 
                                            
                                            
                                            aux_p = internalneighbours->seg;
                                            while(aux_p != NULL){
                                                if((aux_p->fd != k) && (aux_p->fd != externalneighbour.fd)){
                                                    n = escreve(aux_p->fd, buffer, strlen(buffer));
                                                    if(n == -1){
                                                        printf("SUB: Error on write to internalneighbour %s\n", aux_p->name);
                                                    }
                                                }
                                                aux_p = aux_p->seg;
                                            }

                                        }
                                    }
                                    else if(verifica_interno(internalneighbours, comando2) == 1 && strcmp(comando2,externalneighbour.name) == 0){
                                        remove_pessoa(internalneighbours,comando2);
                                        remove_pessoa(participants,comando2);
                                        FD_CLR(k, &descriptorlist_master);
                                        close(k);
                                        existe_ext=0;
                                        memset((void*)&(externalneighbour.name),(int)'\0',20*sizeof(char));
                                        memset((void*)&(externalneighbour.ip),(int)'\0',16*sizeof(char));
                                        memset((void*)&(portaux),(int)'\0',6*sizeof(char));

                                        
                                        if(internalneighbours->seg != NULL){
                                            memset((void*)&buffer2,(int)'\0',BUFFER_SIZE*sizeof(char));
                                            sprintf(buffer2,"BCKP %s;%s;%d\n",internalneighbours->seg->name, internalneighbours->seg->ip, internalneighbours->seg->portno);
                                            n = escreve(internalneighbours->seg->fd,buffer2,strlen(buffer2));
                                             if(n<=0){
                                                printf("SUB: Couldn't write to internalneighbour %s\n", internalneighbours->seg->name);
                                             }
                                             else{
                                                existe_ext = 1;
                                                strcpy(externalneighbour.name,internalneighbours->seg->name);
                                                strcpy(externalneighbour.ip,internalneighbours->seg->ip);
                                                externalneighbour.portno = internalneighbours->seg->portno;
                                                externalneighbour.fd = internalneighbours->seg->fd;
                                        
                                                aux_p = internalneighbours->seg;
                                                if(aux_p->seg != NULL){
                                                    sprintf(buffer,"BCKP %s;%s;%d\n",externalneighbour.name, externalneighbour.ip, externalneighbour.portno); 
                                                    aux_p= aux_p->seg;
                                                    while(aux_p != NULL){
                                                        if((aux_p->fd != k) && (aux_p->fd != externalneighbour.fd)){
                                                            n = escreve(aux_p->fd, buffer, strlen(buffer));
                                                            if(n == -1){
                                                                printf("SUB: Error on write to internalneighbour %s\n", aux_p->name);
                                                            }
                                                        }
                                                        aux_p = aux_p->seg;
                                                    }
                                                }
                                                
                                             }
                                        }
                                        else{
                                            puts("SUB: I don't have internal neighbours :(\n");
                                        }
                                    }
                                }//end of   if(strcmp(comando1,"SUB") == 0){
                                
                                
                                //recebe MSS
                                else if(strcmp(comando1,"MSS") == 0){/*Genericamente, corresponde a tocar a campainha, imprimir a mensagem e a espalhar a mesma*/
                                    memset((void*)&comando1,(int)'\0',20*sizeof(char));
                                    memset((void*)&comando2,(int)'\0',20*sizeof(char));
                                    memset((void*)&buffer3,(int)'\0',1024*sizeof(char));
                            
                                    for(i=4; buffer[i] != ' '; i++);
                                    for(;buffer[i] == ' '; i++);
                                    aux=0;
                                    while(buffer[i] != '\n'){
                                        buffer3[aux] = buffer[i];
                                        aux++;
                                        i++;
                                    }
                                    sscanf(buffer, "%s %s",comando1,comando2);
                                    printf("%c",bell);/*toca a campainha (bell) do sistema*/
                                    printf("MSS: received message:\n\t\t\t \"%s\" de %s\n",buffer3,comando2);
                                        
                                        
                                    
                                    if(k != externalneighbour.fd){
                                        n = escreve(externalneighbour.fd, buffer, strlen(buffer));
                                        if(n == -1){
                                            puts("MSS: Error on write to externalneighbour\n");
                                        }     
                                    } 
                                    aux_p = internalneighbours->seg;
                                    while(aux_p != NULL){
                                        if((aux_p->fd != k) && (aux_p->fd != externalneighbour.fd)){
                                            n = escreve(aux_p->fd, buffer, strlen(buffer));
                                            if(n == -1){
                                                printf("MSS: Error on write to internalneighbour %s\n", aux_p->name);
                                            } 
                                        }
                                        aux_p = aux_p->seg;
                                    }   
                                        
                                }
                                
                                
                                
                                //recebe NEW
                                else if(strcmp(comando1,"NEW") == 0){/*Genericamente, corresponde a criar estrutura para novo user, e a colocar na(s) fila(s) correspondente(s), envia backup, e avisa outros do novo user*/
                                    memset((void*)&(internalneighbour.name),(int)'\0',20*sizeof(char));
                                    memset((void*)&(internalneighbour.ip),(int)'\0',16*sizeof(char));
                                    memset((void*)&(portaux),(int)'\0',6*sizeof(char));
                                    j=0;
                                    aux=0;
                                    for(i=4;buffer[i] != '\n'; i++){
                                        
                                        if(buffer[i]==';'){
                                            j++;
                                            aux=0;
                                        }
                                        else{
                                            if(j==0){
                                                internalneighbour.name[aux]=buffer[i];
                                                aux++;
                                            }
                                            if(j==1){
                                                internalneighbour.ip[aux]=buffer[i];
                                                aux++;
                                            }
                                            if(j==2){
                                                portaux[aux]=buffer[i];
                                                aux++;
                                            }
                                        }
                                            
                                    }
                                    internalneighbour.portno = atoi(portaux);
                                    internalneighbour.fd = k;
                                    insere_pessoa(internalneighbours,internalneighbour.name,internalneighbour.ip,internalneighbour.portno,internalneighbour.fd);
                                    insere_pessoa(participants,internalneighbour.name,internalneighbour.ip,internalneighbour.portno,internalneighbour.fd);

                                    if(existe_ext == 0){/*se ainda nao estava numa ligacao, nao tinha externo, e o novo user sera ancora, tal como eu. Somos internos e externos mutuamente*/
                                        memset((void*)&(externalneighbour.name),(int)'\0',20*sizeof(char));
                                        memset((void*)&(externalneighbour.ip),(int)'\0',16*sizeof(char));
                                        memset((void*)&(portaux),(int)'\0',6*sizeof(char));
                                        
                                        strcpy(externalneighbour.name, internalneighbour.name);
                                        strcpy(externalneighbour.ip, internalneighbour.ip);
                                        externalneighbour.portno = internalneighbour.portno;
                                        externalneighbour.fd  = internalneighbour.fd;
                                        existe_ext =1;
                                    }    
                                    /*Envio-lhe o seu backup*/
                                    sprintf(buffer,"BCKP %s;%s;%d\n",externalneighbour.name,externalneighbour.ip,externalneighbour.portno);                                    
                                    n= escreve(k,buffer,strlen(buffer));
                                    if(n<=0)
                                        puts("NEW: Couldn't write backup to new internalneighbour\n");
                                    else{
                                    
                                        memset((void*)&(buffer),(int)'\0',1024*sizeof(char));
                                        sprintf(buffer,"ADD %s\n",internalneighbour.name);                                
                                        
                                        //Dá aos outros o novo elemento
                                        if(k != externalneighbour.fd){
                                            n = escreve(externalneighbour.fd, buffer, strlen(buffer));
                                            if(n == -1){
                                                puts("NEW: Error on write to externalneighbour\n");
                                            }     
                                        }
                                        aux_p = internalneighbours->seg;
                                        while(aux_p != NULL){
                                            if((aux_p->fd != k) && (aux_p->fd != externalneighbour.fd)){
                                                n = escreve(aux_p->fd, buffer, strlen(buffer));
                                                if(n == -1){
                                                    printf("NEW: Error on write to internalneighbour\n", aux_p->name);
                                                } 
                                            }
                                            aux_p = aux_p->seg;
                                        }
                                        
                                        //Dá ao novo elemento o nome de todos os outros
                                        aux_p = participants->seg;
                                        while(aux_p != NULL){
                                            if( strcmp(aux_p->name,internalneighbour.name) != 0 &&  strcmp(aux_p->name,user.name) != 0){
                                                memset((void*)&(buffer),(int)'\0',1024*sizeof(char));
                                                sprintf(buffer,"ADD %s\n",aux_p->name);                                         
                                                n = escreve(internalneighbour.fd, buffer, strlen(buffer));
                                                fflush(stdout);
                                                if(n == -1){
                                                    printf("NEW: Error on write to internalneighbour\n", internalneighbour.name);
                                                }
                                            }
                                            aux_p = aux_p->seg;
                                        }
                                    }
                                }
                                
                                //recebe BCKP
                                else if(strcmp(comando1,"BCKP") == 0){/*Genericamente, corresponde a adicionar/trocar o dado relativo ao backup*/

                                    memset((void*)&(backup.name),(int)'\0',20*sizeof(char));
                                    memset((void*)&(backup.ip),(int)'\0',16*sizeof(char));
                                    memset((void*)&(portaux),(int)'\0',6*sizeof(char));
                                    j=0;
                                    aux=0;
                                    for(i=5;buffer[i] != '\n'; i++){
                                        
                                        if(buffer[i]==';'){
                                            j++;
                                            aux=0;
                                        }
                                        else{
                                            if(j==0){
                                                backup.name[aux]=buffer[i];
                                                aux++;
                                            }
                                            if(j==1){
                                                backup.ip[aux]=buffer[i];
                                                aux++;
                                            }
                                            if(j==2){
                                                portaux[aux]=buffer[i];
                                                aux++;
                                            }
                                        }
                                            
                                    }
                                    backup.portno = atoi(portaux);                                          
                                    backup.fd=-1;
                                    printf("Backup will be %s with IP: %s and port %d\n",backup.name,backup.ip,backup.portno);
                                    if(strcmp(backup.name, user.name) == 0){
                                        puts("\nANCHOR of conversation\n");
                                        insere_pessoa(internalneighbours,externalneighbour.name,externalneighbour.ip,externalneighbour.portno,externalneighbour.fd);
                                        }
                                }
                                //recebe outro comando
                                else{
                                    printf("error: Unknown command '%s'\n", buffer);
                                }

                                
                                
                                
                            }//if( participants_buffer[k][x%BUFFER_SIZE]=='\n'){
                        }//for(x=participants_buffer_position_read[k];x<(participants_buffer_position_write[k]);x++){
                    }//if((n=read(k,buffer,BUFFER_SIZE))>0)
                    else{   
                         /*se nao ocorreu nenhum dos outros casos, e' porque o utilizador k se desligou ou nao foi possivel comunicar com ele. CTRL+C e' comtemplado aqui. O k que activou a saida do select sera removido*/   
                        if(k == externalneighbour.fd){/*ou e' externo....*/
                                sscanf(externalneighbour.name, "%s",comando2);
                        }else{/*....ou e' interno.*/
                            if((aux_p=(person*)retorna_pessoa_por_descriptor(internalneighbours, k))!=NULL)
                                sscanf(aux_p->name,"%s",comando2);
                            else
                                printf("unknown client hung up\n", comando2);
                        }
                        printf("Client '%s' hung up\n", comando2);
                        FD_CLR(k, &descriptorlist_master);
                        close(k);
                        /*espalha a mensagem de saida deste user. Semelhante ao SUB*/ 
                        sprintf(buffer,"SUB %s\n",comando2); 
                        
                        //codigo para reenviar a mensagem que recebeu
                        if(k != externalneighbour.fd){
                            n = escreve(externalneighbour.fd, buffer, strlen(buffer));
                            if(n == -1){
                                puts("CTRL+C: Error on write to externalneighbour\n");
                            }     
                        }
                        aux_p = internalneighbours->seg;
                        while(aux_p != NULL){
                            if((aux_p->fd != k) && (aux_p->fd != externalneighbour.fd)){
                                n = escreve(aux_p->fd, buffer, strlen(buffer));
                                if(n == -1){
                                    printf("CTRL+C: Error on write to internalneighbour %s\n", aux_p->name);
                                }
                            }
                            aux_p = aux_p->seg;
                        }
// 2- é meu vizinho interno mas nao é externo      -> tiro de participants e de internalneighbour,  fecha socket, tira do select
// 3- é meu vizinho externo, mas nao é interno     -> tirar participants, fecha socket, tira do select. Faço connect ao meu backup
// 4- é meu vizinho externo e interno              -> tiro de participants e de internalneighbour,  fecha socket, tira do select. Adiciono um dos internos como meu externo, se houver. A haver, envio-lhe backup de si proprio
                        
                       if(verifica_interno(internalneighbours, comando2) == 1 && strcmp(comando2,externalneighbour.name) != 0){
                            remove_pessoa(internalneighbours,comando2);
                            remove_pessoa(participants,comando2);
                            FD_CLR(k, &descriptorlist_master);
                            close(k);
                        } 
                        else if(verifica_interno(internalneighbours, comando2) == 0 && strcmp(comando2,externalneighbour.name) == 0){
                            remove_pessoa(participants,comando2);
                            FD_CLR(k, &descriptorlist_master);
                            close(k);
                            existe_ext=0;
                            memset((void*)&(externalneighbour.name),(int)'\0',20*sizeof(char));
                            memset((void*)&(externalneighbour.ip),(int)'\0',16*sizeof(char));
                            memset((void*)&(portaux),(int)'\0',6*sizeof(char));
                            strcpy(externalneighbour.name,backup.name);
                            strcpy(externalneighbour.ip,backup.ip);
                            externalneighbour.portno = backup.portno;
                            
                            if((ext_fd=socket(AF_INET,SOCK_STREAM,0))==-1){
                                puts("CTRL+C: error creating socket\n");
                                exit(1);
                            }
                            externalneighbour.fd = ext_fd;
                            
                            ext_addr.sin_family=AF_INET;
                            inet_aton(externalneighbour.ip, &ext_addr.sin_addr);
                            ext_addr.sin_port=htons(externalneighbour.portno);
                            
                            printf("trying to connect %s at %s : %d\n",externalneighbour.name,externalneighbour.ip,externalneighbour.portno);
                            n=connect(ext_fd,(struct sockaddr*)&ext_addr,sizeof(ext_addr));
                            if(n==-1){
                                    printf("CTRL+C: error on connect with %s at %s : %d\n",externalneighbour.name,externalneighbour.ip,externalneighbour.portno);
                            }
                            else{
                                    existe_ext = 1;
                                    insere_pessoa(participants,externalneighbour.name,externalneighbour.ip,externalneighbour.portno, ext_fd);
                                    memset((void*)&buffer,(int)'\0',BUFFER_SIZE*sizeof(char));
                                    sprintf(buffer,"NEW %s;%s;%d\n",user.name, user.ip, user.portno); 
                                    n= escreve(ext_fd,buffer,strlen(buffer));
                                    if(n<=0)
                                            puts("CTRL+C: Couldn't write to externalneighbour\n");
                                    
                                    FD_SET(ext_fd, &descriptorlist_master); /* adiciono ao master*/
                                    if(ext_fd > maxfd)
                                    { /* compara para ver se e' maior */
                                        maxfd = ext_fd;
                                    }
                                    sprintf(buffer,"BCKP %s;%s;%d\n",externalneighbour.name, externalneighbour.ip, externalneighbour.portno); 
                                            
                                    aux_p = internalneighbours->seg;
                                    while(aux_p != NULL){
                                        if((aux_p->fd != k) && (aux_p->fd != externalneighbour.fd)){
                                            n = escreve(aux_p->fd, buffer, strlen(buffer));
                                            if(n == -1){
                                                printf("SUB: Error on write to internalneighbour %s\n", aux_p->name);
                                            }
                                        }
                                        aux_p = aux_p->seg;
                                    }
                            }
                        }
                        else if(verifica_interno(internalneighbours, comando2) == 1 && strcmp(comando2,externalneighbour.name) == 0){
                            remove_pessoa(internalneighbours,comando2);
                            remove_pessoa(participants,comando2);
                            FD_CLR(k, &descriptorlist_master);
                            close(k);
                            existe_ext=0;
                            memset((void*)&(externalneighbour.name),(int)'\0',20*sizeof(char));
                            memset((void*)&(externalneighbour.ip),(int)'\0',16*sizeof(char));
                            memset((void*)&(portaux),(int)'\0',6*sizeof(char));

                            
                            if(internalneighbours->seg != NULL){
                                memset((void*)&buffer2,(int)'\0',BUFFER_SIZE*sizeof(char));
                                sprintf(buffer2,"BCKP %s;%s;%d\n",internalneighbours->seg->name, internalneighbours->seg->ip, internalneighbours->seg->portno);
                                n = escreve(internalneighbours->seg->fd,buffer2,strlen(buffer2));
                                if(n<=0){
                                printf("CTRL+C: Couldn't write to internalneighbour\n",internalneighbours->seg->name);
                                }else{
                                    existe_ext = 1;
                                    strcpy(externalneighbour.name,internalneighbours->seg->name);
                                    strcpy(externalneighbour.ip,internalneighbours->seg->ip);
                                    externalneighbour.portno = internalneighbours->seg->portno;
                                    externalneighbour.fd = internalneighbours->seg->fd;
                                    
                                    aux_p = internalneighbours->seg;
                                    if(aux_p->seg != NULL){
                                        sprintf(buffer,"BCKP %s;%s;%d\n",externalneighbour.name, externalneighbour.ip, externalneighbour.portno); 
                                        aux_p= aux_p->seg;
                                        while(aux_p != NULL){
                                            if((aux_p->fd != k) && (aux_p->fd != externalneighbour.fd)){
                                                n = escreve(aux_p->fd, buffer, strlen(buffer));
                                                if(n == -1){
                                                    printf("SUB: Error on write to internalneighbour %s\n", aux_p->name);
                                                }
                                            }
                                            aux_p = aux_p->seg;
                                        }
                                    }

                                }
                            }
                            else{
                                puts("CTRL+C: I don't have internal neighbours :(\n");
                            }
                        }
                    }//connection closed by peer
                                        
                }//end of  else
            }//end of  if(FD_ISSET(k,&descriptorlist )){
        }//end of  for(k=0;k<=maxfd;k++){ 
     }//end of  while(1)
     close(fd);
    return 0; 
}//end of  main 
 

