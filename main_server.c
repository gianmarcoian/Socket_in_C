#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include "utils.h"
#include "rxb.h"

#define MAX_REQUEST_SIZE 4096

void handler(int signo){
    int status;

    (void)signo;
    //gestisco tutti i figli terminati
    while(waitpid(-1,&status,WNOHANG)>0) {   continue;   }

}

int main(int argc, char **argv){
    struct addrinfo hints,*res;
    int err,sd,ns,pid;
    struct sigaction sa;
    char *porta=argv[1];
    int optval=1;

    if(argc!=2){
        perror("Errore nell'uso del server. Utilizzo corretto:  ./server_spese <porta>");
        exit(EXIT_FAILURE);
    }

    memset(&sa,0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_flags= SA_RESTART;
    sa.sa_handler= handler;

    if(sigaction(SIGCHLD,&sa,NULL)==-1){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    memset(&hints,0, sizeof(hints));
    hints.ai_family=AF_UNSPEC;  //IPV4 e IPV6 VALIDO
    hints.ai_socktype=SOCK_STREAM;  //comunicazione affidabile con le socketstream 
    hints.ai_flags=AI_PASSIVE;    //server (socket passiva)

    //chiamata a DNS
    if((err= getaddrinfo(NULL,porta,&hints,&res))!=0){
    
        fprintf(stderr,"errore setup indirizzo bind: %s",gai_strerror(err));
        exit(EXIT_FAILURE);

    }

    if((sd=socket(res->ai_family,res->ai_socktype,res->ai_protocol))<0){
        perror("socket");
        exit(EXIT_FAILURE);
    }


    if(setsockopt(sd,SOL_SOCKET,SO_REUSEADDR, &optval, sizeof(optval))<0){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }


    if(bind(sd,res->ai_addr,res->ai_addrlen)<0){
        perror("bind");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);
    if(listen(sd,SOMAXCONN)<0){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    for(;;){
        if((ns=accept(sd,NULL,NULL))<0){   
            perror("accept");
            exit(EXIT_FAILURE);
        }

        if((pid=fork())<0){   
            perror("fork");
            exit(EXIT_FAILURE);
        }else if(pid==0){

            //nel processo figlio
            rxb_t rxb;
            int pid_n1, pid_n2, pipe_n1n2[2],status;
            const char *end_request="---END REQUEST---";
            
            close(sd);
            //disabilito gestore sigchild
            memset(&sa,0,sizeof(sa));
            sigemptyset(&sa.sa_mask);
            sa.sa_handler= SIG_DFL;
            if(sigaction(SIGCHLD,&sa,NULL)==-1){   
                perror("sigaction");
                exit(EXIT_FAILURE);
            }

            rxb_init(&rxb,MAX_REQUEST_SIZE);


            //avvio ciclo richieste
            for(;;){
                
                int err;
                char file[MAX_REQUEST_SIZE*3];
                char mese[MAX_REQUEST_SIZE];
                char categoria[MAX_REQUEST_SIZE];
                char numero[MAX_REQUEST_SIZE];
                size_t cat_len;
                size_t mese_len;
                size_t num_len;

                memset(mese,0,sizeof(mese));
                mese_len= sizeof(mese)-1;
                err=rxb_readline(&rxb,ns,mese,&mese_len);             //prima mese poi categoria poi numero
                if(err<0){
                    rxb_destroy(&rxb);
                    break;
                }

                memset(categoria,0,sizeof(categoria));
                cat_len= sizeof(categoria)-1;
                err=rxb_readline(&rxb,ns,categoria,&cat_len);             //prima mese poi categoria poi numero
                if(err<0){
                    rxb_destroy(&rxb);
                    break;
                }

                memset(numero,0,sizeof(numero));
                num_len= sizeof(numero)-1;
                err=rxb_readline(&rxb,ns,numero,&num_len);             //prima mese poi categoria poi numero
                if(err<0){
                    rxb_destroy(&rxb);
                    break;
                }

#ifdef USE_LIBUNISTRING
                //VERIFICA UFT-8 VALID 
                if(u8_check((uint8_t*)categoria,cat_len)!=NULL){
                    //not valid string
                    fprintf(stderr,"not UFT-8");
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif
                //CREO PIPE
                if(pipe(pipe_n1n2)<0){
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }

                if((pid_n1=fork())<0){
                    perror("primo processo nipote");
                    exit(EXIT_FAILURE);
                }else if(pid_n1==0){

                    close(ns);
                    close(pipe_n1n2[0]);

                    close(1);
                    if(dup(pipe_n1n2[1])<0){
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(pipe_n1n2[1]);

                    //uso di un path fac-simile per esame, dovrebbe essere /var/local/expenses/%s/%s.txt, mese,categoria
                    sprintf(file,"./%s/%s.txt",mese,categoria);
                    if(access(file,F_OK)!=-1){
                        execlp("head","head","-n",numero,file,(char*)NULL);
                    } else {printf("Non ci sono spese con questa combinazione categoria mese");}
                    perror("execlp HEAD");
                    exit(EXIT_FAILURE);

                }
                //fine primo nipote(head), sono nel figlio di nuovo
                if((pid_n2=fork())<0){
                    perror("secondo nipote");
                }else if(pid_n2==0){
                    //secondo nipote

                    close(pipe_n1n2[1]);
                    close(0);
                    if(dup(pipe_n1n2[0])<0){
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(pipe_n1n2[0]);

                    close(1);
                    if(dup(ns)<0){
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(ns);
                    execlp("sort","sort", "-n",(char*)NULL);
                    perror("execlp sort");
                    exit(EXIT_FAILURE);




                }

                close(pipe_n1n2[0]);
                close(pipe_n1n2[1]);
                waitpid(pid_n1,&status,0);
                waitpid(pid_n2,&status,0);

                if(write_all(ns,end_request,strlen(end_request))<0){
                    perror("write");
                    exit(EXIT_FAILURE);
                }              

                close(ns);
                exit(EXIT_SUCCESS);

            }
            close(ns);

        }
        


    }





        close(sd);
        return 0;

}

