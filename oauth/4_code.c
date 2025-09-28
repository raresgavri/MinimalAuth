/* EXAMPLE CODE FILE */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <utmp.h>


    int main()
    {
        char cmd[100];
        int len, fd;
        bool loggedin = false;
        

      
        if( -1 == mknod("comenzi.txt", S_IFIFO | 0666, 0) )
        {
            if(errno == EEXIST) // errno=17 for "File already exists"
                fprintf(stdout, "Canalul fifo 'comenzi' exista deja!\n");
            else
            {
                perror("Eroare la crearea canalului fifo. Cauza erorii");
                exit(1);
            }
        }

        //Fifo transmitere output catre client
        if( -1 == mknod("output.txt", S_IFIFO | 0666, 0) )
        {
            if(errno == EEXIST) // errno=17 for "File already exists"
                fprintf(stdout, "Canalul fifo 'output' exista deja!\n");
            else
            {
                perror("Eroare la creare fifo output. Cauza erorii");
                exit(2);
            }
        }
        

        printf("Astept sa scrie cineva...\n");
        fd = open("comenzi.txt", O_RDONLY);
        printf("A venit cineva:\n");


        int fd_out;
        if ( -1 == (fd_out = open("output.txt", O_WRONLY)) )
        {
            perror("Eroare la deschiderea canalului fifo output. Cauza erorii");
            exit(3);
        }
        printf("Sunt gata de transmis\n");



        do {
            if ((len = read(fd, cmd, 100)) == -1)
                perror("Eroare la citirea din FIFO!");
            else {
                cmd[len] = '\0';
                printf("S-au citit din FIFO %d bytes: \"%s\"\n", len, cmd);



                char buffer[2000]; //buffer de transmis inapoi la client
                //procesez comanda
                if(strstr(cmd, "login")!=0)
                {
                    int pipe_fds[2];

                    if( -1 == pipe(pipe_fds))
                       {
                          perror("Eroare la crearea canalului intern. Cauza erorii");
                          exit(1);
                       }


                    pid_t pid = fork();
                    if(pid!=0)
                    {
                        printf("Parent process : %d\n", getpid());
                        close(pipe_fds[1]);

                        int len_read = read(pipe_fds[0], buffer, 2000);
                        buffer[len_read] = '\0';
                        printf("-----Bufferul primit de 'login'-----\n%s\n", buffer);
                        if(strcmp(buffer, "Logged in!")==0)
                            loggedin = true;
                        else
                            loggedin = false;

                    }

                    if(pid==0)
                    {
                        printf("Sunt procesul copil %d al parintelui %d\n", getpid(), getppid());
                        close(pipe_fds[0]);
                        char *user;
                        user=strtok(cmd+8, "\n");

                        FILE *users = fopen("users.txt", "r");
                        char id[50];
                        while (fscanf(users, "%s", id) == 1)
                        {
                            if(strcmp(user, id)==0)
                            {
                                write(pipe_fds[1], "Logged in!", 10);
                                exit(0);
                            }
                        }
                        write(pipe_fds[1], "User not found!", 15);
                        exit(1);
               

                    }

                }
                printf("Comanda: %s, %d\n", cmd, loggedin==true);
                if(strcmp(cmd, "get-logged-users")==0 && loggedin==true)
                {
                    int sockets[2], child;

                    socketpair(AF_UNIX,SOCK_STREAM,0,sockets);

                    child = fork();

                    if(child)
                    {
                        //parinte
                        close(sockets[0]); //inchid socket copil
                        int len_read = read(sockets[1], buffer, 2000);
                        buffer[len_read] = '\0';
                        printf("-----Buferul primit de 'get-logged-users'-----\n%s\n", buffer);

                        
                    }

                    else
                    {
                        //copil
                        close(sockets[1]); //inchid socket parinte
                        int file_utmp;
                        if(-1 == (file_utmp = open("/var/run/utmp", O_RDONLY) ))
                        {
                            perror("Eroare la deschidere utmp_file");
                            exit(4);
                        }



                        struct utmp data[20];

                        void utmp_to_buffer(struct utmp *data) 
                        {
                            char entry[1024];
                            sprintf(entry, "username: %s\nhost: %s\ntime entry was made: %d\n", data->ut_user, data->ut_host, data->ut_time);
                            strcat(buffer,entry);
                            strcat(buffer,"====================\n");
                        }

                        bool last_user(struct utmp *data)
                        {
                            if(strcmp(data->ut_user, "")!=0)
                                return true;
                            else
                                return false;
                        }


                        int len_read = read(file_utmp, &data, 20*sizeof(struct utmp));

                        int i=0;
                        buffer[0]='\0';
                        while(last_user(&data[i])) 
                        {
                            utmp_to_buffer(&data[i]);
                            i++;
                        }
                        write(sockets[0], buffer, strlen(buffer));
                        exit(9);


                    }
                }
                else if(strcmp(cmd, "get-logged-users")==0 && loggedin==false)
                {
                    sprintf(buffer, "Nu sunteti autentificat!");
                }

                //comanda get-proc-info
                if(strstr(cmd, "get-proc-info")!=0 && loggedin==true)
                {
                    int sockets[2], child;
                    

                    socketpair(AF_UNIX,SOCK_STREAM,0,sockets);

                    child = fork();

                    if(child)
                    {
                        //parinte
                        close(sockets[0]); //inchid socket copil
                        int len_read = read(sockets[1], buffer, 2000);
                        buffer[len_read] = '\0';
                        printf("-----Buferul primit de 'get-proc-info'-----\n%s\n", buffer);

                    }
                    else
                    {
                        //copil
                        close(sockets[1]); //inchid socket parinte

                        char *pid;
                        pid = strtok(cmd+16," ");
                        printf("PIDUL ESTE %s\n", pid);
                        char path[50];
                        sprintf(path, "/proc/%s/status", pid);
                        printf("path : %s\n", path);

                        FILE * file_status;
                        file_status=fopen(path, "r");

                        char line[99];
                        while(fscanf(file_status, "%[^\n] ", line)!=EOF)
                        {
                            
                            if(strstr(line, "Name")!=0 || strstr(line, "State")!=0 || strstr(line, "PPid")!=0 || strstr(line, "Uid")!=0 || strstr(line, "VmSize")!=0)
                            {
                                printf("%s\n", line);
                                strcat(buffer, line);
                                strcat(buffer, "\n");
                            }

                        }

                        //scriem buffer ul in socket
                        write(sockets[0], buffer, strlen(buffer));
                        exit(10);


                    }
                }
                else if(strstr(cmd, "get-proc-info")!=0 && loggedin==false)
                {
                    sprintf(buffer, "Nu sunteti autentificat!");
                }

                if(strcmp(cmd, "logout")==0)
                {
                    loggedin=false;
                    sprintf(buffer, "Disconnected successfully!");
                }

                if(strcmp(cmd, "quit")==0)
                {
                    write(fd_out, "Have a nice day :)", 18);
                    exit(12);
                }

                //transmitere buffer inapoi la client
                if ((len = write(fd_out, buffer, strlen(buffer))) == -1)
                            perror("Problema la scriere in FIFO_out!");
                        else
                            printf("S-au scris in FIFO_out %d bytes\n", len);
                buffer[0]='\0'; // golim bufferul dupa ce transmitem clientului output ul fiecarei comenzi
            }
        } 
        while (len > 0);

        return 0; //
    }