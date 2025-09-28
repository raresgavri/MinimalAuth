#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;

int main (int argc, char *argv[])
{
  int sd;			// descriptorul de socket
  struct sockaddr_in server;	// structura folosita pentru conectare 
  		// mesajul trimis
  char token[13];
  char refresh[13];
  char buff[1000];
  char sendBuf[100];

  /* exista toate argumentele in linia de comanda? */
  if (argc != 3)
    {
      printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
      return -1;
    }

  /* stabilim portul */
  port = atoi (argv[2]);

  /* cream socketul */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("Eroare la socket().\n");
      return errno;
    }

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons (port);
  

  //pregatim socketul si umplem structura pt res_server
    int sd_res;
    struct sockaddr_in res_server;
    if ((sd_res = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("Eroare la socket() auth_server");
      return errno;
    }

    res_server.sin_family=AF_INET;
    res_server.sin_addr.s_addr = inet_addr("127.0.0.1");
    int port_res = atoi("2909");
    res_server.sin_port = htons(port_res);

    int pid = getpid();


    /* start */
    printf("[client] Se autorizeaza client %d\n", pid);
    printf ("Redirecting you to auth_server...\nIntroduceti codul primit de la [auth_server]\n");
    fflush (stdout);

    /* ne conectam la server */
    if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
      {
        perror ("[client] Eroare la connect() auth_server");
        return errno;
      }

    if (write (sd, &pid, sizeof(int)) <= 0)
    {
      perror ("[client] Eroare la write() pid spre auth_server");
      return errno;
    }

    int code;
    scanf("%d", &code);
    if (write (sd, &code, sizeof(int)) <= 0)
    {
      perror ("[client] Eroare la write() code spre auth_server");
      return errno;
    }

    /* citirea raspunsului dat de server 
     (apel blocant pana cind serverul raspunde) */
    if (read (sd, &token, 13) < 0)
    {
      perror ("[client] Eroare la read() de la auth_server");
      return errno;
    }
    
    if (read (sd, &refresh, 13) < 0)
    {
      perror ("[client] Eroare la read() de la auth_server");
      return errno;
    }
    
    /* afisam token-ul primit */
    printf ("[client] Token-ul primit este: %s, refresh: %s\n", token, refresh);

    if (connect (sd_res, (struct sockaddr *) &res_server,sizeof (struct sockaddr)) == -1)
      {
        perror ("[client] Eroare la connect() server resurse");
        return errno;
      }
    
    const char * pick_file() 
    {
      const char* file[5];
      file[0] = "1_users.txt";
      file[1] = "2_history.txt";
      file[2] = "3_configuration.txt";
      file[3] = "4_code.c";
      file[4] = "5_loginTime.txt";

      const char* random;
      srand(time(NULL));
      random = file[rand() % 5];
      return random;
    }

    void receiveFile(const char* fileName, int sd_res)
    {
      unsigned char recvBuff[256]={0};
      FILE *fp;

      /* pregatim locatia unde vom salva fisierul solicitat */
          char pid[5];
          sprintf(pid, "%d", getpid());
          struct stat st = {0};
          char dpath [10]; strcpy(dpath, "./"); strcat(dpath, pid);
          if (stat(dpath, &st) == -1) 
          {
            mkdir(dpath, 0700);
          }

          char file_received[30];
          strcpy(file_received, dpath);
          strcat(file_received, "/received_");
          strcat(file_received, fileName);

      

      fp = fopen(file_received ,"w");
      if(NULL == fp)
      {
        printf("Error opening file");
      }
      int bytesReceived=0;
      int sum=0;
      while((bytesReceived = read(sd_res, recvBuff, 256)) > 0)
       {
         sum=sum+bytesReceived;
         fwrite(recvBuff, 1,bytesReceived,fp);
         //printf("%s\n", recvBuff);
         if(bytesReceived<256)
            break;
       }
       printf("[client] Bytes received %d. File is ready!\n", sum);
       

    }


  do
  {
    
    const char * fileName = pick_file();
    sprintf(sendBuf, "%s %s", token, fileName);
    int len =strlen(sendBuf);
    sendBuf[len]='\0';
    printf("==============================\n");
    printf("[client] Conectare la server resurse: %s\n", sendBuf);
    if (write (sd_res, sendBuf, strlen(sendBuf)+1) <= 0)
    {
      perror ("[client] Eroare la write() spre server resurse\n");
      return errno;
    }

    if(read(sd_res, buff, 15) < 0)
    {
      perror ("[client] Eroare la read() de la res_server");
      return errno;
    }
    else
    {
      printf("[client] %s\n",buff);
      if(strstr(buff, "Token expirat!")==0)
          receiveFile(fileName, sd_res);
    }
    

    
    
    
    sleep(5);
  }
  while(strstr(buff, "Token expirat!")==0);
    
  
    /* inchidem conexiunea, am terminat */
    close (sd);
    close(sd_res);

}

    