#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>


/* portul folosit */
#define PORT 2908

/* codul de eroare returnat de anumite apeluri */
extern int errno;

typedef struct thData
{
	int idThread; //id-ul thread-ului tinut in evidenta de acest program
	int cl; //descriptorul intors de accept
	int fd_tokens;
	int fd_refresh;
}thData;

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
void raspunde(void *, char*, char*);

int main ()
{
  struct sockaddr_in server;	// structura folosita de server
  struct sockaddr_in from;	
  int nr;		//mesajul primit de trimis la client 
  int sd;		//descriptorul de socket 
  int pid;
  pthread_t th[100];    //Identificatorii thread-urilor care se vor crea
	int i=0;
  

	/* fifo-ul in care fiecare thread va inregistra fiecare token generat si nr. de incercari */
	int fd_tokens;
	if ( -1 == (fd_tokens = open("valid_tokens.txt", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) )
        {
            perror("Eroare la deschiderea canalului fifo. Cauza erorii");
            exit(1);
        }

  int fd_refresh;
  if ( -1 == (fd_refresh = open("refresh_tokens.txt", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) )
        {
            perror("Eroare la deschiderea canalului fifo. Cauza erorii");
            exit(1);
        }

  /* crearea unui socket */
  	if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror ("[auth server] Eroare la socket() ");
	  	return errno;
	}

  /* utilizarea optiunii SO_REUSEADDR */
  	int on=1;
  	setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
  
  /* pregatirea structurilor de date */
	bzero (&server, sizeof (server));
	bzero (&from, sizeof (from));
  
  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
	server.sin_family = AF_INET;	
  /* acceptam orice adresa */
	server.sin_addr.s_addr = htonl (INADDR_ANY);
  /* utilizam un port utilizator */
	server.sin_port = htons (PORT);
  
  /* atasam socketul */
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
	{
	  perror ("[auth server] Eroare la bind() ");
	  return errno;
	}

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen (sd, 2) == -1)
	{
	  perror ("[auth server] Eroare la listen() ");
	  return errno;
	}

  /* servim in mod concurent clientii...folosind thread-uri */
  while (1)
	{
	  int client;
	  thData * td; //parametru functia executata de thread     
	  int length = sizeof (from);

	  printf ("[auth server] Asteptam la portul %d...\n",PORT);
	  fflush (stdout);

	  /* acceptam un client (stare blocanta pana la realizarea conexiunii) */
	  if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
		{
		  perror ("[auth server] Eroare la accept() ");
		  continue;
		}
	
		/* s-a realizat conexiunea, se astepta mesajul */
	
		// int idThread; //id-ul threadului
		// int cl; //descriptorul intors de accept

		td=(struct thData*)malloc(sizeof(struct thData));	
		td->idThread=i++;
		td->cl=client;
		td->fd_tokens=fd_tokens;
		td->fd_refresh=fd_refresh;

		pthread_create(&th[i], NULL, &treat, td);	      

	}//while    
};	

char* generate_token()
{
	srand(rand());
    int i;
    char pass[13]; //token de 12 caractere + caracterul null

    for (i = 0; i < 4; i++) 
    {
        //revised logic to generate random characters at all positions (0 - 11)
        pass[ 3 * i ] = '0' + (rand() % 10); //generating numeric character
        char capLetter = 'A' + (rand() % 26); //generating upper case alpha character
        pass[(3 * i) + 1] = capLetter;
        char letter = 'a' + (rand() % 26); //generating lower case alpha character
        pass[(3 * i) + 2] = letter;
    }
    pass[3 * i] = '\0'; //placing null terminating character at the end
    char *arr=malloc(13*sizeof(char));
    strcpy(arr, pass);
    return arr;

    
}

int generate_attempts()
{
	srand((unsigned int)(time(NULL)));
	return (rand() % (4 - 1)) + 1;
}


int grant_acces(void *arg)
{
	int pid; int auth_code, cl_code=0;
	char buf[100];

	struct thData tdL; 
	tdL= *((struct thData*)arg);

	if (read (tdL.cl, &pid, sizeof(int)) < 0)
    {
      perror ("[client] Eroare la read() de la client");
      return errno;
    }


  printf("[thread %d] Clientul cu pid: %d solicita acces la resurse\n Confirm? (y/n)\n", tdL.idThread, pid);
  gets(buf);
  if(strcmp(buf, "y")==0)
  {
  	auth_code=rand()/1000000;
  	printf("[thread %d] Tastati acest cod in client(%d) : %d\n", tdL.idThread, pid, auth_code);
  	read(tdL.cl, &cl_code, sizeof(int));
  	if(auth_code==cl_code)
  		return pid;
  	else
  	{
  		printf("Client refuzat. Nu coincide codul primit: %d\n", cl_code);
  		return 0;
  	}

  }
  else
  {
  	printf("Client refuzat.\n");
  	return 0;
  }
}

void validate_token (void* arg, char* token, char* refresh, int nr_utilizari_token, int pid)
{
	struct thData tdL; 
	tdL= *((struct thData*)arg);
	char *string=malloc(16*sizeof(char));
	char *string2=malloc(16*sizeof(char));

	sprintf(string, "%s:%d\n", token, nr_utilizari_token);
	sprintf(string2, "%s:%d\n", refresh, pid);

	if ((write(tdL.fd_tokens, string, strlen(string))) == -1)
                perror("Problema la scriere in FIFO!");
  if ((write(tdL.fd_refresh, string2, strlen(string2))) == -1)
                perror("Problema la scriere in FIFO!");

	free(string);
	free(string2);
}

static void *treat(void * arg)
{		
	struct thData tdL; 
	tdL= *((struct thData*)arg);	
	printf ("[thread %d] Asteptam mesajul...\n", tdL.idThread);
	fflush (stdout);		 
	pthread_detach(pthread_self());
	int pid=0;

	if((pid = grant_acces((struct thData*)arg))!=0)
	{
		char * token=generate_token();
		char * refresh=generate_token();
		int nr_utilizari_token=generate_attempts();

		validate_token((struct thData*)arg, token, refresh, nr_utilizari_token, pid);
		printf ("[thread %d] Token generat: %s - %d attempts - refresh: %s\n",tdL.idThread, token, nr_utilizari_token, refresh);
		raspunde((struct thData*)arg, token, refresh);

		free(token);
		free(refresh);
	}
	
	

	/* am terminat cu acest client, inchidem conexiunea */
	close ((intptr_t)arg);
	
	return(NULL);	
};




void raspunde(void *arg, char* token, char* refresh)
{
	struct thData tdL; 
	tdL= *((struct thData*)arg);
			  

			  /* returnam mesajul clientului */
	if (write (tdL.cl, token, 13) <= 0)
		{
		 printf("[thread %d] ",tdL.idThread);
		 perror ("Eroare la write() token catre client ");
		}
	if(write(tdL.cl, refresh, 13) <= 0)
	{
		printf("[thread %d] ",tdL.idThread);
		 perror ("Eroare la write() refresh token catre client ");
	}	

}

