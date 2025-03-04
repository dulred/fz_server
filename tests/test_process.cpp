#include <stdio.h>
#include <unistd.h>

int gloabl_var = 10; 

int main(int argc, char const *argv[])
{
    int local_var = 10;

    pid_t pid = fork();
    if (pid == 0)
    {
        // child process
        gloabl_var++;
        local_var++;
        printf("child: globale_var = %d, local_var = %d \n", gloabl_var, local_var);
        
    } else if (pid > 0 )
    {
        // father process
        gloabl_var--;
        local_var--;
        printf("child: globale_var = %d, local_var = %d \n", gloabl_var, local_var);
    }
    else
    {
        printf("fork fail");
    }
    
    
    return 0;
}
