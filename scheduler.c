#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void PrintUsage(); // Print the correct usage


typedef struct process{
	int pid;
	char name[256];
	char state[10];
	int tiempo;
	int rafagas;
	int array_rafagas[1000];
	int raf_actual;
	int t_run;//marca el tiempo de burst actual en running en caso que fue cortado
	int t_wait;//marca el tiempo de burst actual en waiting
	int turnos_cpu; //veces que entra a la cpu
	int interrumpido; //veces que sale de la cpu por exceso de tiempo
	int t_waiting; // Tiempo total esperando
	int t_end;
	int t_first;
} Process;

typedef struct procesos{
	Process** oneprocess;
	int length;
} Procesos;

typedef struct queue{
	Process** listprocess;
	int length;
} Queue;

typedef struct cpu{
	Process** listprocess;
	int length;
	int quantum;
	int idle;
	int t_actual;
} CPU;

void printlines(Queue* lista_procesos);
void printoutput(Procesos* lista_procesos);
void writeoutput(Procesos* lista_procesos, char * filename);
int readlines(char * filename, Procesos* lista_procesos);
int simulation(Procesos* lista_procesos, int quantum);
void addtimeready(Queue* readylist);
void ChecknostartedProcess(Procesos* lista_procesos, Queue* readylist,int timer);
void CheckwaitlistProcess(Queue* waitlist, Queue* readylist,int timer);
void Checkcpu(CPU* cpuprocess, Queue* readylist, Queue* waitlist, Queue* finishlist,int timer,Procesos* lista_procesos);
void reordready(Queue* readylist);
void outcpu(CPU* cpuprocess, int timer, int r_total, int r_actual, Queue* readylist, Queue* finishlist,Queue* waitlist,Procesos* lista_procesos);
void addtoreadyfromcpu(Queue* readylist,CPU* cpuprocess);
void addtocpufromready(Queue* readylist, CPU* cpuprocess, int timer,Procesos* lista_procesos,Queue* finishlist);
void addtofinishfromcpu(Queue* finishlist,CPU* cpuprocess);
void addtowaitingfromcpu(Queue* waitlist,CPU* cpuprocess);
void addtoreadyfromwaiting(Queue* readylist,Queue* waitinglist, int move);

int main(int argc, char *argv[]){
        // Define Variables
	int quantum = 3;
	// Number of arguments less than 3, exit
	if (argc < 3 || argc > 4){
		PrintUsage();
		return 1;
	}
	/*printf("%s\n",argv[0]);
	printf("%s\n",argv[1]);
	printf("%s\n",argv[2]);
	printf("%d\n",quantum);*/
	if(argc == 4){
		quantum = atoi(argv[3]);
	}
	Procesos* lista_procesos = malloc(sizeof(Procesos));
	readlines(argv[1], lista_procesos);
	simulation(lista_procesos, quantum);
	//printoutput(lista_procesos);
	writeoutput(lista_procesos,argv[2]);
	for(int i = 0; i < lista_procesos -> length; i++){
		free(lista_procesos -> oneprocess[i]);
	}
	free(lista_procesos -> oneprocess);
	free(lista_procesos);
	return 0;
}

// Print the correct usage
void PrintUsage(){
	printf("Usage: ./scheduler <file> <output> <quantum>\n");
	printf("<file> nombre de archivo input.\n");
	printf("<output> nombre archivo csv output.\n");
	printf("<quantum> parametro quantum. Por defecto q = 3.\n");
}

void printlines(Queue* lista_procesos){
	if(lista_procesos -> length > 0){
		printf("Cola Ready: \n");
	}
	for(int i = 0; i < lista_procesos -> length; i++){
		//printf("p %d |",lista_procesos -> listprocess[i] -> pid);
		//printf("t %d |",lista_procesos -> listprocess[i] -> tiempo);
		//printf("r %d |",lista_procesos -> listprocess[i] -> rafagas);
		printf("Lugar %d | ",i);
		printf("Process: %s | ",lista_procesos -> listprocess[i] -> name);
		printf("State %s \n",lista_procesos -> listprocess[i] -> state);
	}
}
void printoutput(Procesos* lista_procesos){
	printf("Estadisticas\n");
	for(int i = 0; i < lista_procesos -> length; i++){
		int inicio = lista_procesos -> oneprocess[i] -> tiempo;
		int final = lista_procesos -> oneprocess[i] -> t_end;
		int first = lista_procesos -> oneprocess[i] -> t_first;
		printf("Nombre %s | ", lista_procesos -> oneprocess[i] -> name);
		printf("Turnos cpu: %d | ", lista_procesos -> oneprocess[i] -> turnos_cpu);
		printf("Interrumpido: %d | ", lista_procesos -> oneprocess[i] -> interrumpido);
		printf("Turnaround: %d | ", final - inicio);
		printf("Response: %d | ", first - inicio);
		printf("Waiting time: %d | \n", lista_procesos -> oneprocess[i] -> t_waiting);
	}
}

void writeoutput(Procesos* lista_procesos, char * filename){
	FILE *fp;
	fp = fopen(filename, "w+");
	for(int i = 0; i < lista_procesos -> length; i++){
		int inicio = lista_procesos -> oneprocess[i] -> tiempo;
		int final = lista_procesos -> oneprocess[i] -> t_end;
		int first = lista_procesos -> oneprocess[i] -> t_first;
		fprintf(fp,"%s,", lista_procesos -> oneprocess[i] -> name);
		fprintf(fp,"%d,", lista_procesos -> oneprocess[i] -> turnos_cpu);
		fprintf(fp,"%d,", lista_procesos -> oneprocess[i] -> interrumpido);
		fprintf(fp,"%d,", final - inicio);
		fprintf(fp,"%d,", first - inicio);
		fprintf(fp,"%d\n", lista_procesos -> oneprocess[i] -> t_waiting);
	}
	fclose(fp);
}

int readlines(char * filename, Procesos* lista_procesos){
	char rafagas1[5];
	char tiempo1[5];
	char number[5];
	char name[30];
	int *array_process[4];
	FILE *fp = fopen(filename,"r");
	int count = 1;
	char ready[10] = "READY";
	lista_procesos -> length = count;
	lista_procesos -> oneprocess = malloc(sizeof(Process*)*(lista_procesos -> length));
	while (!feof(fp)){
		Process* nuevo_proceso = malloc(sizeof(Process));
		nuevo_proceso -> raf_actual = 0;
		nuevo_proceso -> t_run = 0;
		nuevo_proceso -> t_wait = 0;
		nuevo_proceso -> turnos_cpu = 0;
		nuevo_proceso -> interrumpido = 0;
		nuevo_proceso -> t_waiting = 0;
		nuevo_proceso -> t_first = -1;
		fscanf(fp,"%s", name);
		strcpy(nuevo_proceso -> state, ready);
		strcpy(nuevo_proceso -> name, name);
		nuevo_proceso -> pid = count;
		fscanf(fp,"%s\n", tiempo1);
		nuevo_proceso -> tiempo = atoi(tiempo1);
		fscanf(fp,"%s\n", rafagas1);
		nuevo_proceso -> rafagas = (2*atoi(rafagas1))-1;
		int array_tiempos[(2*atoi(rafagas1))-1];
		for (int i=0; i < (2*atoi(rafagas1))-1; i++){
			fscanf(fp,"%s\n", number);
			array_tiempos[i] = atoi(number);
		}
		memcpy(nuevo_proceso -> array_rafagas, array_tiempos,sizeof array_tiempos);
		lista_procesos -> length = count;
		lista_procesos -> oneprocess = realloc(lista_procesos -> oneprocess, sizeof(Process*)*(lista_procesos -> length));
		lista_procesos -> oneprocess[count-1] = nuevo_proceso;
		count++;
	}
	fclose(fp);
	return 2;
}

int simulation(Procesos* lista_procesos, int quantum){
	Queue* readylist = malloc(sizeof(Queue));
	Queue* waitlist = malloc(sizeof(Queue));
	Queue* finishlist = malloc(sizeof(Queue));
	readylist -> length = 0;
	readylist -> listprocess = malloc(sizeof(Process*)*(1));
	waitlist -> length = 0;
	waitlist -> listprocess = malloc(sizeof(Process*)*(1));
	finishlist -> length = 0;
	finishlist -> listprocess = malloc(sizeof(Process*)*(1));
	CPU* cpuprocess = malloc(sizeof(CPU));
	cpuprocess -> listprocess = malloc(sizeof(Process*)*(1));
	cpuprocess -> length = 0;
	cpuprocess -> quantum = quantum;
	cpuprocess -> idle = 0;
	cpuprocess -> t_actual = 0;
	int termino = 1;
	int timer = 0;
	while(termino != 0 && timer != 70){
		//printf("tiempo actual CPU: %d\n",timer);
		//reviso si un proceso nuevo tiene que pasar a ready
		ChecknostartedProcess(lista_procesos, readylist, timer);
		//reviso si termina un proceso en waiting para que pase a ready
		CheckwaitlistProcess(waitlist, readylist, timer);
		//Reviso si cpu libera un proceso, si es asi entra uno de ready
		Checkcpu(cpuprocess, readylist, waitlist, finishlist, timer, lista_procesos);
	/*	printlines(readylist);
		printf("Cola Waiting: \n");
		printlines(waitlist);
		printf("Cola FINISH: \n");
		printlines(finishlist);
	*/addtimeready(readylist);
		if(lista_procesos -> length == finishlist -> length){
			//printf("Terminaron todos\n");
			termino = 0;
		}
		timer++;
	}
	//printlines(readylist);
	free(readylist -> listprocess);
	free(readylist);
	free(waitlist -> listprocess);
	free(waitlist);
	free(finishlist -> listprocess);
	free(finishlist);
	free(cpuprocess -> listprocess);
	free(cpuprocess);
	return termino;
}
void addtimeready(Queue* readylist){
	for(int i = 0; i < readylist -> length; i++){
		readylist -> listprocess[i] -> t_waiting ++;
	}
}
void ChecknostartedProcess(Procesos* lista_procesos, Queue* readylist,int timer){
	for(int i = 0; i < lista_procesos -> length; i++){
		if(lista_procesos -> oneprocess[i] -> tiempo == timer){
			char name_p[256];
			strcpy(name_p, lista_procesos -> oneprocess[i] -> name);
			printf("[t = %d] El proceso %s ha sido creado.\n", timer, name_p);
			readylist -> length += 1;
			readylist -> listprocess = realloc(readylist -> listprocess, sizeof(Process*)*(readylist -> length));
			readylist -> listprocess[readylist -> length - 1] = lista_procesos -> oneprocess[i];
		}
	}
}

void CheckwaitlistProcess(Queue* waitlist, Queue* readylist,int timer){
	char ready[10] = "READY";
	if(waitlist -> length > 0){
		int moves[waitlist -> length];
		int order = 0;
		int w_length = waitlist -> length;
		for(int i = 0; i < w_length; i++){
			char name_p[256];
			strcpy(name_p, waitlist -> listprocess[i] -> name);
			int r_total = waitlist -> listprocess[i] -> rafagas;
			int r_actual = waitlist -> listprocess[i] -> raf_actual;
			int tmp_actual = waitlist -> listprocess[i] -> t_wait;
			int t_rafaga = waitlist -> listprocess[i] -> array_rafagas[r_actual];
			if(t_rafaga == tmp_actual){
				waitlist -> listprocess[i] -> t_wait = 0;
				waitlist -> listprocess[i] -> raf_actual ++;
				strcpy(waitlist -> listprocess[i] -> state, ready);
				printf("[t = %d] El proceso %s ha pasado a estado READY.\n", timer, name_p);
				addtoreadyfromwaiting(readylist, waitlist, i);
				moves[i] = 1;
				order = 1;
				waitlist -> length --;
			}
			else{
				waitlist -> listprocess[i] -> t_wait ++;
				waitlist -> listprocess[i] -> t_waiting ++;
				moves[i] = 0;
			}
		}
		if (order == 1){
			if(w_length > 2){
				for(int i = 0; i < w_length; i++){
					int flag = 0;
					if(moves[i] == 1){
						for(int x = i + 1; x < w_length; x++){
							if(flag == 0){
								if (moves[x] == 0){
									waitlist -> listprocess[i] = waitlist -> listprocess[x];
									moves[x] = 1;
									flag = 1;
								}
							}
						}
					}
				}
			}
			else{ // Lugar del error, no existia este else para el caso de igual a 2
				waitlist -> listprocess[0] = waitlist -> listprocess[1];
			}
		}
	}
}

void Checkcpu(CPU* cpuprocess, Queue* readylist, Queue* waitlist, Queue* finishlist, int timer, Procesos* lista_procesos){
	char running[10] = "RUNNING";
	int quantum1 = cpuprocess -> quantum;
	if(cpuprocess -> length == 0){
		addtocpufromready(readylist, cpuprocess, timer,lista_procesos,finishlist);
	}
	else{
		char name_p[256];
		strcpy(name_p, cpuprocess -> listprocess[0] -> name);
		int r_total = cpuprocess -> listprocess[0] -> rafagas;
		int r_actual = cpuprocess -> listprocess[0] -> raf_actual;
		int tmp_actual = cpuprocess -> listprocess[0] -> t_run;
		int t_rafaga = cpuprocess -> listprocess[0] -> array_rafagas[r_actual];
		if(cpuprocess -> t_actual == quantum1){
			//cuenta como interrupcion
			cpuprocess -> listprocess[0] -> interrumpido ++;
			if(t_rafaga > tmp_actual + quantum1){
				cpuprocess -> listprocess[0] -> t_run = tmp_actual + quantum1;
				addtoreadyfromcpu(readylist, cpuprocess);
				printf("[t = %d] El proceso %s ha pasado a estado READY.\n", timer, name_p);
				addtocpufromready(readylist, cpuprocess, timer,lista_procesos,finishlist);
			}
			else{
				outcpu(cpuprocess,timer, r_total, r_actual,readylist,finishlist,waitlist,lista_procesos);
			}
		}
		else if(cpuprocess -> t_actual + tmp_actual == t_rafaga){
			outcpu(cpuprocess,timer, r_total, r_actual,readylist,finishlist,waitlist,lista_procesos);
		}
		else{//actualizar tiempos
			cpuprocess -> t_actual ++;
			//printf("Se actualiza tiempo %d de proceso: %s\n",cpuprocess -> t_actual, name_p);
		}
	}

}

void reordready(Queue* readylist){
	for(int i = 0;i < readylist -> length - 1; i++){
        	readylist -> listprocess[i]=readylist -> listprocess[i+1];
	}
	readylist -> length --;
	readylist -> listprocess = realloc(readylist -> listprocess, sizeof(Process*)*(readylist -> length));
}

void outcpu(CPU* cpuprocess, int timer, int r_total, int r_actual, Queue* readylist, Queue* finishlist,Queue* waitlist,Procesos* lista_procesos){
	char name_p[256];
	strcpy(name_p, cpuprocess -> listprocess[0] -> name);
	//Sale hacia Finish o Waiting
	cpuprocess -> listprocess[0] -> t_run = 0;
	if(r_total == r_actual + 1){
		cpuprocess -> listprocess[0] -> t_end = timer;
		addtofinishfromcpu(finishlist, cpuprocess);
		printf("[t = %d] El proceso %s ha pasado a estado FINISHED.\n", timer, name_p);
		addtocpufromready(readylist, cpuprocess, timer,lista_procesos,finishlist);
	}
	else{
		cpuprocess -> listprocess[0] -> t_wait = 1;
		cpuprocess -> listprocess[0] -> t_waiting ++;
		cpuprocess -> listprocess[0] -> raf_actual ++;
		addtowaitingfromcpu(waitlist, cpuprocess);
		printf("[t = %d] El proceso %s ha pasado a estado WAITING.\n", timer, name_p);
		addtocpufromready(readylist, cpuprocess, timer,lista_procesos,finishlist);
	}
}

void addtoreadyfromcpu(Queue* readylist,CPU* cpuprocess){
	char ready[10] = "READY";
	readylist -> length += 1;
	readylist -> listprocess = realloc(readylist -> listprocess, sizeof(Process*)*(readylist -> length));
	readylist -> listprocess[readylist -> length - 1] = cpuprocess -> listprocess[0];
	strcpy(readylist -> listprocess[readylist -> length -1] -> state, ready);
}

void addtocpufromready(Queue* readylist, CPU* cpuprocess, int timer, Procesos* lista_procesos,Queue* finishlist){
	char running[10] = "RUNNING";
	if(readylist -> length > 0){
		cpuprocess -> listprocess[0] = readylist -> listprocess[0];
		cpuprocess -> length = 1;
		cpuprocess -> t_actual = 1;
		cpuprocess -> listprocess[0] -> turnos_cpu ++;
		if(cpuprocess -> listprocess[0] -> t_first < 0){
			cpuprocess -> listprocess[0] -> t_first = timer;
		}
		strcpy(cpuprocess -> listprocess[0] -> state, running);
		char name_p[256];
		strcpy(name_p, cpuprocess -> listprocess[0] -> name);
		printf("[t = %d] El proceso %s ha pasado a estado RUNNING.\n", timer, name_p);
		reordready(readylist);
	}
	else{
		cpuprocess -> length = 0;
		if(lista_procesos -> length != finishlist -> length){
			cpuprocess -> idle ++;
			printf("[t = %d] La CPU esta en estado idle.\n", timer);
		}
	}
}

void addtofinishfromcpu(Queue* finishlist,CPU* cpuprocess){
	char finish[10] = "FINISH";
	finishlist -> length += 1;
	finishlist -> listprocess = realloc(finishlist -> listprocess, sizeof(Process*)*(finishlist -> length));
	finishlist -> listprocess[finishlist -> length - 1] = cpuprocess -> listprocess[0];
	strcpy(finishlist -> listprocess[finishlist -> length -1] -> state, finish);
}

void addtowaitingfromcpu(Queue* waitlist,CPU* cpuprocess){
	char waiting[10] = "WAITING";
	waitlist -> length += 1;
	waitlist -> listprocess = realloc(waitlist -> listprocess, sizeof(Process*)*(waitlist -> length));
	waitlist -> listprocess[waitlist -> length - 1] = cpuprocess -> listprocess[0];
	strcpy(waitlist -> listprocess[waitlist -> length -1] -> state, waiting);
}

void addtoreadyfromwaiting(Queue* readylist,Queue* waitlist, int move){
	char ready[10] = "READY";
	readylist -> length += 1;
	readylist -> listprocess = realloc(readylist -> listprocess, sizeof(Process*)*(readylist -> length));
	readylist -> listprocess[readylist -> length - 1] = waitlist -> listprocess[move];
	strcpy(readylist -> listprocess[readylist -> length -1] -> state, ready);
}
