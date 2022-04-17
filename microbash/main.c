#include "microbash.h"

int main(int argc, char *argv[])
{   
    const char initMessage[]= "*****************************\nHi, welcome to our microbash!\n*****************************\n";
    printf("%s\n", initMessage);

    int numOfCommands;
    Command cmd[MAXNUMCOMMANDS];

    for(;;)
    {

        printPrompt();
        initCmd(cmd, &numOfCommands);

        //Se viene letto EOF esco
        if (!getCommand(cmd, &numOfCommands))
        {
            printf("\nBye, thanks for choosing us!\n");
            deleteCmd(cmd, &numOfCommands);
            exit(0);
        }

        //Se la sequenza letta non è corretta ricomincio
        if (!checkCmdLine(cmd, &numOfCommands))
        {
            deleteCmd(cmd, &numOfCommands);
            continue;
        }

        execCmdLine(cmd, &numOfCommands);
        deleteCmd(cmd, &numOfCommands);
    }
}
