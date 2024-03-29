#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/shm.h>

#include <time.h>

#define SHMSZ 3 // Tamaño de la memoria compartida
#define FILAS 15
#define COLUMNAS 10000
#define RANGO_MAX 301



int descriptores[2];

// Señal enviada desde el padre al hijo para que se finalize este
void signal_fin_h2(int signum) {
	// printf("fin proceso : %d", getpid());
		printf("Proceso hijo: %d finaliza\n", getpid());
		exit(0);
}
int es_Primo(int numero) {
	// Los números menores o iguales a 1 no son primos
	if (numero <= 1)  {
		return 0;
	}

	// Verificar si el número es divisible por algún número del 2 hasta la raíz cuadrada de 'numero'
	for (int num = 2; num * num <= numero; num++) {
		if (numero % num == 0) {
			// Si es divisible, no es primo
			return 0;
		}
	}

	// Si no se encontró ningún divisor, es primo
	return 1;
}
// señal enviada desde el hijo al padre,
// plara que desde la tuberia vea cual es sus:
// pid ( del hijo para que el padre pueda enviarle la señal singal_fin_h2 al hijo correspondiente)
// su i para saber de que posicion de memoria compartida tiene que leeer
// y asi poder ver desde memoria compartida lo que ha encontrado tal hijo
void signal_avisarPadre(int singnum) {
		FILE *archivoN1;
		int dataTuberia[2];
		int pidHijo;
		int iHijo;

		// Leer datos de la tubería directamente al array
		ssize_t bytesLeidos = read(descriptores[0], dataTuberia, sizeof(dataTuberia));

		if (bytesLeidos < 0) {
			perror("Error al leer de la tubería");
		}
		else if (bytesLeidos == 0)  {
			// El extremo de escritura de la tubería se ha cerrado
			// Esto podría indicar que el proceso de escritura ha terminado
			// o que no hay datos disponibles para leer
			// Maneja esto según tus necesidades
		}
		else if (bytesLeidos == sizeof(dataTuberia))  {
			// Procesar los datos leídos según sea necesario
			// dataTuberia[0] contiene el primer entero, dataTuberia[1] contiene el segundo entero
			char nombreArchivoN1[20];
			sprintf(nombreArchivoN1, "N1_%d.primos", getpid());
			archivoN1 = fopen(nombreArchivoN1, "a");
			// padre lee los datos de la tuberia
			// y atraves de estos ve que ha escrito el hio en la memoria compartida
			// y en la pos 4 los suma
			pidHijo = dataTuberia[0];
			iHijo = dataTuberia[1];
			fprintf(archivoN1, "Padre recibe señal de finalización del hijo: %d\n", pidHijo);
			fclose(archivoN1);
			// El padre el dice al hijo que puede terminar
			sprintf(nombreArchivoN1, "N1_%d.primos", getpid());
			archivoN1 = fopen(nombreArchivoN1, "a");
			fprintf(archivoN1, "Padre envia señal al hijo con pid: %d para que termine \n", pidHijo);
			fclose(archivoN1);
			kill(pidHijo, SIGINT);
		}
		else
		{
			perror("Error al leer de una tuberia");
		}
}

int main() {
	// Total de primos encontrados
	int totalPrimos = 0;
	// Esctructura para el manejador de señales
	struct sigaction saP;

	// Configurar el manejador de señales
	saP.sa_handler = signal_avisarPadre;
	sigemptyset(&saP.sa_mask);
	saP.sa_flags = 0;

	// Establecer el manejador de señales usando sigaction
	if (sigaction(SIGUSR1, &saP, NULL) == -1)  {
		perror("Error en sigaction en el Padre");
		exit(EXIT_FAILURE);
	}
	// signal(SIGQUIT, signal_avisarPadre); // Manejar la seña SIGINT (Ctrl+C)

	//////////// CREACION DE LA MATRIZ CON LOS NUMEROS ALETORIOS //////////////
	// Semilla para los números aleatorios
	srand(time(NULL));

	// Declaración de la matriz
	int matriz[FILAS][COLUMNAS];

	// Generación de números aleatorios y asignación a la matriz
	for (int fil = 0; fil < FILAS; fil++)  {
		for (int col = 0; col < COLUMNAS; col++) {
			matriz[fil][col] = rand() % RANGO_MAX;
		}
	}

	//////////// Comienzo del programa //////////////
	FILE *archivoN1;
	FILE *archivoN2;
	FILE *archivoN3;

	int estado;
	key_t key = ftok("/bin/man", 136); // Clave para la memoria compartida
	int numeroTotalPrimos = 0;

	// Para la gestion de las tuberias
	int valoresHijosIds[3];
	pipe(descriptores);

	// Crear la memoria compartida
	int shmid; // ID de la memoria compartida
	int *shm;  // Puntero a la memoria compartida
	// Crea un espacio de 3 por que va generar 3 procesos hijos
	// que cada uno guardara cuantos numeros primos ha encontrado
	if ((shmid = shmget(key, 4 * sizeof(int), 0777 | IPC_CREAT)) < 0)  {
		perror("shmget");
		exit(1);
	}

	// Adjuntar la memoria compartida al espacio de direcciones del proceso
	if ((shm = shmat(shmid, (char *)0, 0)) == (int *)-1)  {
		perror("shmat");
		exit(1);
	}

	// Inicializar la memoria compartida
	for (int mc = 0; mc < SHMSZ; ++mc)  {
		shm[mc] = 0;
	}

	///////  GUARDAR INICIO DE LA EJECUCION DEL PROCESO PADRE EN EL FICHERO ////////
	char nombreArchivoN1[20];
	sprintf(nombreArchivoN1, "N1_%d.primos", getpid());
	archivoN1 = fopen(nombreArchivoN1, "a");
	fprintf(archivoN1, "Inicio de la ejecucion\n");
	fclose(archivoN1);

	// Crear tres procesos hijos
	//for (int i = 0; i < 3; i++){
	for (int i = 0; i < 3; i++)
	{
		//////// --> Guardar la crecion de un proceso en el fichero del padre //////////
		//printf("padre creando hijo: %d \n", i);
		archivoN1 = fopen(nombreArchivoN1, "a");
		fprintf(archivoN1, "Padre %d crea el proceso hijo: %d \n", getpid(), (i + 1));
		fclose(archivoN1);
		// se crea la virfucación del codigo
		pid_t pid = fork();
		switch (pid){
			case -1:
				perror("fork");
				exit(1);
				break;
			case 0:
				// Armado de señal para que termine cuando el padre se lo diga
				// sigaction en vez de signal

				// Configurar el manejador de señales
				saP.sa_handler = signal_fin_h2;
				sigemptyset(&saP.sa_mask);
				saP.sa_flags = 0;

				// Establecer el manejador de señales usando sigaction
				if (sigaction(SIGINT, &saP, NULL) == -1)
				{
					perror("Error en sigaction en el Hijo: ");
					exit(EXIT_FAILURE);
				}
				// signal(SIGINT, signal_fin_h2); // Manejar la seña SIGINT (Ctrl+C)
				//  avisa de que se ha creado el hijo
				printf("Proceso hijo-%d con pid: %d con padre %d creado correctamente \n", i, getpid(), getppid());

				// Se crea el nombre del archivo
				char nombreArchivoN2[20];
				sprintf(nombreArchivoN2, "N2_%d.primos", getpid());
				// Se guarda el inicio de la ejecucion
				archivoN2 = fopen(nombreArchivoN2, "a");
				fprintf(archivoN2, "Inicio de la ejecucion\n");
				fclose(archivoN2);

				// Se iniciliza el numero de hijos
				int numPrimosHijos = 0;
				// Crea los 5 procesos hijos (nietos) correspondientes
				for (int e = 0; e < 5; e++) {
					// Se guarda la idenficacion de los procesos hijos que se estan creando
					// empezando por el 1
					archivoN2 = fopen(nombreArchivoN2, "a");
					fprintf(archivoN2, "Creando el proceso nieto : %d del hijo %d \n", (e + 1), (i + 1));
					fclose(archivoN2);
					// se crea la virfucación del codigo
					pid_t pid2 = fork();
					switch (pid2){
						case -1:
							perror("fork 2");
							exit(1);
						case 0:
							// printf("Proceso nieto%d con pid: %d con padre %d creado correctamente \n", e + 1, getpid(), getppid());
							// se le asisgna la fila con la que va trabajar
							int fila = e + i;
							int numPrimos = 0;
							// recorre sus 1000 posiciones en busca de los numeros primos
							for (int colum = 0; colum < COLUMNAS; colum++)
							{
								int valor = matriz[fila][colum];
								int esPrimo = es_Primo(valor);

								// Si no se encontró ningún divisor, es primo
								if (esPrimo == 1)  {
									// Se guarda en el fichero
									// Espacio para el nombre del archivo
									char nombreArchivoN3[20];
									// Construir el nombre del archivo con getpid()
									sprintf(nombreArchivoN3, "N3_%d.primos", getpid());
									// Se abre el archivo en modo escritura
									archivoN3 = fopen(nombreArchivoN3, "a");

									if (archivoN3 != NULL) {
										// Escribir en el archivo con el formato deseado
										fprintf(archivoN3, "%d:%d:%d\n", 3, getpid(), valor);

										// Cerrar el archivo
										fclose(archivoN3);
									}

									// Se actuliza el numero de primos
									numPrimos++;
								}
							}
							// Hace un exit con el numero de primos como pide el enunciado
							exit(numPrimos);
							break;
						default:
							break;
					}
				}
				// Se hacen 5 esperas wait, una por cada hijo,
				// Se suma al numPrimosHijos el numero de primos que encontro el hijo
				// el cual devuelve el valor en un exit(numeroPrimos)
				for (int x2 = 0; x2 < 5; x2++) {
					int estado;
					pid_t wpid = wait(&estado);
					if (wpid > 0) {
						// Se comprueba la finalizacion del nieto
						if (WIFEXITED(estado)) {
							int exit_status = WEXITSTATUS(estado);
							// Su exit es el numero de primos encontrados
							// Si es mayor a cero se actuliza el numero de primos encontrados
							if (exit_status > 0) {
								numPrimosHijos += exit_status; // Suma la cantidad de números primos encontrados por cada hijo
							}
						}
						else  {
							// printf("Proceso nieto con PID %d finalizado anormalmente\n", wpid);
						}
					}
				}
				// Se guarda el resultado total enviado por sus hijos.
				archivoN2 = fopen(nombreArchivoN2, "a");
				fprintf(archivoN2, "Total de primos encontrados: %d \n", numPrimosHijos);
				fclose(archivoN2);
				// Se guarda la información en la memoria compartida
				shm[i] = numPrimosHijos;
				// Escribe en la tuberia para avisar de que el padre ya puede leer.
				int dataTuberia[2] = {getpid(), i};
				write(descriptores[1], &dataTuberia, sizeof(dataTuberia));
				// Envia la señal al padre
				// En la tuberia tiene su ID y su i
				// El padre en base a esto lee de memoria compartida el dato
				kill(getppid(), SIGUSR1);
				usleep(150);
				// se pasa ya que finaliza cuando el padre le envia una señal
				pause();
				break;
			default:
				////
				// guarda el pid del hijo para enviarle luego la señal de que finalice
				valoresHijosIds[i] = pid;
				break;
		}
	}

	// esperamos a que los tres hijos acaben
	for (int o = 0; o < 3; o++) {
		int estado;
		//pid_t wpid = waitpid(valoresHijosIds[o], &estado, 0);
		pid_t wpid = wait(&estado);
		if (wpid > 0)
		{
			// Se comprueba la finalización del proceso hijo
			if (WIFEXITED(estado)){
				int exit_status = WEXITSTATUS(estado);
				if (exit_status == 0) {
					printf("Proceso hijo: %d con PID %d finalizado normalmente con un exit(0) su padre: %d\n", o, wpid, getpid());
				}
				else {
					printf("Proceso hijo: %d con PID %d finalizado con un exit(%d)\n", o, wpid, exit_status);
				}
			}
			else {
				printf("Proceso hijo: %d con PID %d finalizado anormalmente\n", o, wpid);
			}
		}

	}

	printf("El padre con pid: %d ha finalizado los procesos hijos \nY empiza a leer de memoria compartida\n", getpid());
	// Padre cuando los hijos acaban lee de memoria compartida y hace la suma total de primos encontrados
	totalPrimos = shm[0] + shm[1] + shm[2];
	// Padre guarda en el fichero el total de hiijos encontrados
	archivoN1 = fopen(nombreArchivoN1, "a");
	fprintf(archivoN1, "Numero total de primos: %d\n", totalPrimos);
	fclose(archivoN1);
	// Padre cierra los descriptores
	close(descriptores[0]);
	close(descriptores[1]);
	// liberar puntero memoria compartida
	// eliminar memoria compartida
	// Padre termina
	exit(0);
	return 0;
}
