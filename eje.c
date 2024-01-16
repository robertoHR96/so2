#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    int num_hijos = 3;

    for (int i = 0; i < num_hijos; i++)
    {
        pid_t pid = fork();
        if (pid == -1)
        {
            // Error al crear el proceso hijo
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            printf("Proceso hijo %d con pid : %d creado. Hijo de PID: %d\n", i + 1, getpid(), getppid());
            for (int e = 0; e< 3; e++)
            {
                pid_t pid = fork();
                if (pid == -1)
                {
                    // Error al crear el proceso hijo
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                else if (pid == 0)
                {
                    printf("Proceso nieto %d con pid : %d creado. Hijo %d de PID: %d\n", (e+1), getpid(),i+1, getppid());
                    // El proceso hijo termina aquí
                    exit(EXIT_SUCCESS);
                }
            }

            // Código ejecutado por el proceso padre
            // Espera a que todos los hijos terminen
            for (int i = 0; i < num_hijos; i++)
            {
                wait(NULL);
            }

            printf("Proceso hijo %d  con pid %d finalizado.\n", i+1, getpid());
            // El proceso hijo termina aquí
            exit(EXIT_SUCCESS);
        }
    }

    // Código ejecutado por el proceso padre
    // Espera a que todos los hijos terminen
    for (int i = 0; i < num_hijos; i++)
    {
        wait(NULL);
    }

    printf("Proceso padre finalizado.\n");

    return 0;
}
