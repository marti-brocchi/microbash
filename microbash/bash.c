#include "microbash.h"

//-----------------------------------------------------------
//          Funzioni ausiliarie per gestione errori
//-----------------------------------------------------------

void fail_errno(const char * const msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

void fail(const char *const msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(EXIT_FAILURE);
}

//-----------------------------------------------------------
//          Funzioni ausiliarie per liberare memoria
//-----------------------------------------------------------

//Funzione freeMem - Dealloca memoria associata ad un array di puntatori
void freeMem(char* arr[], int n)
{
    for(int i=0; i<n && arr[i]!=NULL; i++)
        free(arr[i]);
}

//-----------------------------------------------------------
//          Funzioni per gestione struct
//-----------------------------------------------------------

//Funzione initCmd - Inizializza array cmd[]
void initCmd(Command cmd[], int* numOfCommands)
{
    for (int i=0; i<MAXNUMARGS; i++)
    {
        for (int j=0; j<MAXNUMARGS; j++)
        {
            cmd[i].command[j]= NULL;
            cmd[i].numOfArgs=0;
        }
    }

    *numOfCommands=0;
}

//Funzione deleteCmd - Svuota array cmd liberando spazi di memoria allocati dinamicamente
void deleteCmd(Command cmd[], int* numOfCommands)
{
    for (int i=0; i<MAXNUMARGS; i++)
    {
        freeMem(cmd[i].command, cmd[i].numOfArgs);
        cmd[i].numOfArgs=0;
    }

    *numOfCommands=0;
}

//-----------------------------------------------------------
//           Funzioni lettura e parsing linea
//-----------------------------------------------------------

//Funzione printPrompt - Stampa prompt dei comandi
void printPrompt()
{
    char *cwd = getcwd(NULL, 0);
    if (cwd==NULL)
        fail_errno("getcwd - error getting cwd");

    fprintf(stdout, "%s:~$ ", cwd);
    free(cwd);
}

//Funzione getCommand - Effettua lettura della linea di comando da stdin
int getCommand(Command cmd[], int* numOfCommands)
{
    char *line = readline(NULL);

    if(line==NULL)
        return 0;
    
    splitLine(line, cmd, numOfCommands);
    free(line);
    return 1;
}

//Funzione splitLine - Divide i comandi separati da pipe
void splitLine(char* line, Command cmd[], int* numOfCommands)
{
    char *command, *str, *saveptr;
    int i;

    //Lettura della linea e divisione dei comandi separati da pipe 
    for(str= line, i=0; i<MAXNUMCOMMANDS ; str=NULL, i++)
    {
        command= strtok_r(str, "|", &saveptr);
        if (command==NULL)
            break;
        
        splitCommand(command, cmd, i);
        (*numOfCommands)++;
    }
}

//Funzione splitCommand - Divide i comandi separati da spazi o tab e li salva all'interno dell'array cmd[]
void splitCommand(char *command, Command cmd[], const int cmdNum)
{
    char *arg, *str, *saveptr;
    int i;

    //Lettura del comando e divisione degli argomenti separati da uno spazio
    for(str=command, i=0; i<MAXNUMARGS ; str=NULL, i++)
    {
        arg= strtok_r(str, " " "\t", &saveptr);
        if (arg==NULL)
            break;

        //Salvo l'argomento letto in command
        saveArg(cmd, cmdNum, i, arg);
    }
}

//Funzione saveArg - Salva un nuovo argomento all'interno della struttura dati
void saveArg(Command cmd[], int i, int j, char *arg)
{
    cmd[i].command[j]= (char*)malloc(strlen(arg)+1);

    if (!cmd[i].command[j])
        fail("Malloc error- not enough free space");

    memcpy(cmd[i].command[j], arg, strlen(arg)+1);
    cmd[i].numOfArgs++;
}

//Funzione chechCmdLine - Effettua controlli di correttezza sulla linea di comandi
int checkCmdLine(Command cmd[], const int* numOfCommands)
{
    //Scandisco lista dei comandi
    for(int i=0; i<(*numOfCommands); i++)
    {
        //Se c'è un comando vuoto segnalo errore
        if (cmd[i].numOfArgs==0)
        {
            fprintf(stderr, "Error: empty command in pipe\n");
            return 0;
        }
        
        //Se troviamo cd
        else if (strncmp(cmd[i].command[0], "cd", strlen("cd"))==0)
        {
            //Non devono essere presenti altri comandi e non devono essere presenti redirezioni
            if (i==0 && *numOfCommands==1 && cmd[i].numOfArgs<=2)
            {
                if (cmd[i].numOfArgs==2 && strncmp(cmd[i].command[1], ">", strlen(">"))!=0 && strncmp(cmd[i].command[1], "<", strlen("<"))!=0)
                    return 1;
                
                if (cmd[i].numOfArgs==1)
                {
                    saveArg(cmd, i, 1, "$HOME");
                    return 1;
                }
            }
                
            //Altrimenti errore    
            fprintf(stderr, "Error: incorrect use of cd\n");
            return 0;
        }
        
        else
        {
            int inputRed=0, outputRed=0;
            
            //Scandisco lista degli argomenti
            for (int j=0; j<cmd[i].numOfArgs; j++)
            {
                //Controllo che siano utilizzati correttamente comandi per redirezioni
                if (strncmp(cmd[i].command[j], ">", strlen(">"))==0)
                {
                    //Se viene rediretto stdout su comandi all'inizio o al centro di pipe o se viene inserito uno spazio segnalo errore
                    if (outputRed==1 || i!=(*numOfCommands)-1 || strcmp(cmd[i].command[j], ">")==0)
                    {
                        fprintf(stderr, "Redirect error\n");
                        return 0;
                    }

                    outputRed++;
                } 

                if (strncmp(cmd[i].command[j], "<", strlen("<"))==0)
                {
                    //Se viene rediretto stdin su comandi alla fine o al centro di pipe o se viene inserito uno spazio segnalo errore
                    if (inputRed==1 || i!=0 || strcmp(cmd[i].command[j], "<")==0)
                    {
                        fprintf(stderr, "Redirect error\n");
                        return 0;
                    }
                    
                    inputRed++;
                }
            }
        }
    }

    return 1;
}

//-----------------------------------------------------------
//               Funzioni esecuzione comandi
//-----------------------------------------------------------

//Funzione execCmdLine - Esegue la lista di comandi
void execCmdLine(Command cmd[], int *numOfCommands)
{   
    extractVar(cmd, numOfCommands); //Sostituzione variabili di ambiente

    //Se c'è un solo comando lo eseguo
    if (*numOfCommands==1)
    {
        //Comando cd (built-in)
        if(strncmp(cmd[0].command[0], "cd", strlen("cd"))==0)
        {
            if (cd(cmd[0].command[1])==-1)
            {
                deleteCmd(cmd, numOfCommands);
                fail_errno("Error");
            }
        }

        //Altri comandi (esterni)
        else
        {
            pid_t pid = fork();
	        if (pid<0)
	            fail_errno("Can't create child process");

            if (pid==0)
                execExternCmd(cmd, numOfCommands, 0);
            else
                waitChildren(cmd, numOfCommands, &pid);
        }
    }

    //Se ci sono più argomenti
    else
    {
        //Creo pipe
        int pipedes[(*numOfCommands-1)*2]; //array per salvare fd delle diverse pipe
        createPipe(cmd, numOfCommands, pipedes);

        //Eseguo una fork per ogni processo
        pid_t pid[*numOfCommands]; //array per salvare i pid di tutti i processi figli
        int i, j, z;
        for (i=0, j=1, z=-2; i< *numOfCommands; i++, j+=2, z+=2)
        {
            pid[i] = fork();
            if (pid[i]<0)
                fail_errno("Can't create child process");

            if (pid[i]>0)
                continue;

            else if (pid[i]==0)
            {
                adjust_fd(cmd, numOfCommands, pipedes, i, j, z);
                execExternCmd(cmd, numOfCommands, i);
            }
        }
    
        //Chiusura dei file descriptor (da parte del processo padre)
        for (int x=0; x<(*numOfCommands-1)*2; x++)
            close(pipedes[x]);
        
        //Wait dei processi figli
        waitChildren(cmd, numOfCommands, pid);  
    }
}

//Funzione execExternChild - Funzione ausiliaria per esecuzione comandi esterni alla basH (non buit-in)
void execExternCmd(Command cmd[], int *numOfCommands, const int cmdToExec)
{
    //Effettuo eventuali redirezioni
    redirect(cmd, numOfCommands, cmdToExec);

    //Eseguo comando e segnalo eventuali errori
    if(execvp(cmd[cmdToExec].command[0], cmd[cmdToExec].command)==-1)
    {
        deleteCmd(cmd, numOfCommands);
        fail_errno("exec error");
    }
}

//Funzione waitChildren - Aspetta tuttti i figli di un processo padre
void waitChildren(Command cmd[], int *numOfCommands,pid_t pid[])
{
    //Effettuo wait dei processi figlii
    for (int i=0; i<*numOfCommands; i++)
    {
        int status;
        if(waitpid(pid[i], &status, 0)==-1)
        {
            deleteCmd(cmd, numOfCommands);
            fail("wait() error");
        }

        //Stampo causa di terminazione del figlio
        else 
        {
            if (WIFEXITED(status))
            {
                if (WEXITSTATUS(status)!=0)
                    printf("Process n° %d terminated by exit with exit status %d\n\n", i, WEXITSTATUS(status));
            }

            else if(WIFSIGNALED(status))
                printf("Process n° %d terminated by signal %d\n\n", i, WTERMSIG(status));
        }
    }
}

//Funzione extractVar - Scansiona lista comendi e argomenti e sostituisce variabili di ambiente con il relativo valore
void extractVar(Command cmd[], int *numOfCommands)
{
    //Scorro lista dei comandi
    for (int i=0; i<*numOfCommands; i++)
    {
        //Scorro lista degli argomenti
        for (int j=0; j< cmd[i].numOfArgs; j++)
        {
            //Se un argomento inizia per $ sostituisco con valore var ambiente
            if (strncmp(cmd[i].command[j], "$", strlen("$"))==0)
            {
                char *value= getenv(cmd[i].command[j]+1);
                if (value==NULL)
                {
                    deleteCmd(cmd, numOfCommands);
                    fail("Error: no such variable");
                }

                char *temp= realloc(cmd[i].command[j], strlen(value)+1);
                if(!temp)
                    fail("realloc: not enough space");

                cmd[i].command[j]=temp;
                memcpy(cmd[i].command[j], value, strlen(value)+1);
            }
        }
    }
}

//Funzione cd - Esegue comando cd (interno alla bash)
int cd(char *directory)
{
    return chdir(directory);
}

//-----------------------------------------------------------
//               Funzioni redirezione I/O
//-----------------------------------------------------------

//Funzione createPipe - crea il numero di pipe necessarie ad eseguire linea di comando. Salva i relativi file descriptors in pipedes
void createPipe(Command cmd[], int *numOfCommands, int pipedes[])
{
    for (int i=0; i< *numOfCommands-1; i++) 
    {
        int pipefd[2];
        if (pipe(pipefd)==-1)
        {
            deleteCmd(cmd, numOfCommands);
            fail_errno("pipe error");
        }

        pipedes[i*2]= pipefd[0];
        pipedes[(i*2)+1]= pipefd[1];
    }
}

//Funzione adjust_fd - Imposta i fd di un comando per farli coincidere con quelli della pipe
void adjust_fd(Command cmd[], int *numOfCommands, int pipedes[], int i, int j, int z)
{
    if(i==0)
        makeDup2(cmd, numOfCommands, pipedes[j], 1);
                
    else if(i==*numOfCommands-1)
        makeDup2(cmd, numOfCommands, pipedes[z], 0);
    
    else
    {
        makeDup2(cmd, numOfCommands, pipedes[j], 1);
        makeDup2(cmd, numOfCommands, pipedes[z], 0);
    }
    
    //Chiusura dei file descriptor (da parte del processo figlio)
    for (int x=0; x<(*numOfCommands-1)*2; x++)
        close(pipedes[x]);
}

//Funzione redirect - Scansiona lista argomenti di un comando ed effettua eventuali redirezioni di I/O
void redirect(Command cmd[], int *numOfCommands, const int i)
{
    int fdIn=-1, fdOut=-1;

    for (int j=0; j<cmd[i].numOfArgs; j++)
    {
        //Se un argomento inizia per > redirigo stdout
        if (strncmp(cmd[i].command[j], ">", strlen(">"))==0)
        {
            fdOut= openFileOut(cmd, numOfCommands, i, j);
            j--; continue;
        }

        //Se un argomento inizia per < redirigo stdin
        if (strncmp(cmd[i].command[j], "<", strlen("<"))==0)
        {
            fdIn= openFileIn(cmd, numOfCommands, i, j);
            j--; continue;
        }
    }

    //Se viene fatta redirezione stdin
    if (fdIn!=-1)
    {
        makeDup2(cmd, numOfCommands, fdIn, 0);
        close(fdIn);
    }

    //Se viene fatta redirezione stdout
    if (fdOut!=-1)
    {
        makeDup2(cmd, numOfCommands, fdOut, 1);
        close(fdOut);
    }
}

//Funzione openFileOut - Apre file specificato in lettura e restituisce file descriptor. Aggiorna lista argomenti
int openFileOut(Command cmd[], int *numOfCommands, int i, int j)
{
    int fd= open(cmd[i].command[j]+1, O_WRONLY | O_CREAT, 0666);
    if(fd==-1)
    {
        deleteCmd(cmd, numOfCommands);
        fail_errno("Open error");
    }
    
    updateArgs(cmd, i, j);
    return fd;
}

//Funizone openFileIn - Apre file specificato in scrittura e restituisce file descriptor. Aggiorna lista argomenti
int openFileIn(Command cmd[], int *numOfCommands, int i, int j)
{
    //Apro file specificato
    int fd= open(cmd[i].command[j]+1, O_RDONLY);
    if(fd==-1)
    {
        deleteCmd(cmd, numOfCommands);
        fail_errno("Open error");
    }

    updateArgs(cmd, i, j);
    return fd;
}

//Funzione updateArgs - Aggiorna la lista degli argomenti in seguito a redirezioni
void updateArgs(Command cmd[], int i, int j)
{
    //Libero memoria puntata dal comando che indicava redirezione
    free(cmd[i].command[j]);
            
    //Aggiorno i puntatori        
    for (int z=j; z<cmd[i].numOfArgs-1; z++)
    {
        cmd[i].command[z]= cmd[i].command[z+1];
    }

    //Tolgo ultimo comando dalla lista dei comandi
    cmd[i].command[cmd[i].numOfArgs-1]=NULL;     
    cmd[i].numOfArgs--;
}

//Funzione makeDup2 - Esegue dup2 controllando valore di ritorno
void makeDup2(Command cmd[], int *numOfCommands, int oldFd, int newFd)
{
    if (dup2(oldFd, newFd)==-1)
    {
        deleteCmd(cmd, numOfCommands);
        fail_errno("dup2");
    }
}