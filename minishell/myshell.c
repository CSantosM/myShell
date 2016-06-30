#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include "parser.h"

int esCD(tline * linea){
	if((linea->ncommands>0)&&(strcmp(linea->commands[0].argv[0],"cd")==0)){
		return 1;
	}else{
		return 0;
	}
}
void ejecutarCD(tline * linea){
	if(linea->commands[0].argc==1){
		//printf("estoy en cambiar directorio a HOME");
		if(chdir(getenv("HOME"))<0){	//cambia el directorio a /home
			perror("cd");
		}
	}else{
		//printf("estoy en cambiar directorio a ruta");
		if(chdir(linea->commands[0].argv[1])<0){	//cambia el directorio a la ruta de destino
			perror("cd");
		}
	}
	
}
int redirecciones(tline * linea, int posicion){
	int file;
		if ((linea->redirect_output!=NULL) && (posicion==(linea->ncommands-1))){ 
			file=open(linea->redirect_output,O_WRONLY+O_CREAT,0664);//crea el fichero y lo abre en modo escritura
			if(file==-1){
				perror("Error al abrir/crear el fichero");
				exit(1);
			}
			dup2(file,1);
			close(file);
		}
		
		if ((linea->redirect_input!=NULL) && (posicion==0)){
			file=open(linea->redirect_input,O_RDONLY,0644);//abre el fichero en modo lectura
			if(file==-1){
				perror("Error al abir archivo");
				exit(1);
			}
			dup2(file,0);
			close(file);
		}
		
		if ((linea->redirect_error!=NULL) && (posicion==(linea->ncommands-1))){
			file=open(linea->redirect_error,O_WRONLY+O_CREAT,0664);
			if(file==-1){
				perror("Error al abrir/crear archivo");
				exit(1);
			}
			dup2(file,2);
			close(file);
		}
	return 0;
}

int
main(void) {
	char buf[1024];
	tline * line;
	int i,j;
	int **pipes;
	typedef struct {	
		int hijo;
	} tpid;
	tpid *pids;
	int fichero;
	
	//quito señales para no poder matar al padre
	signal(SIGINT,SIG_IGN);
	signal(SIGQUIT,SIG_IGN);
						
						
	printf("minishell~~$ ");	
	while (fgets(buf, 1024, stdin)) {
		
		line = tokenize(buf);
		
		if (line==NULL) {
			continue;
		}
		if(esCD(line)){ //si el comando es cd entonces ejecuto cd
			ejecutarCD(line);
		}else{
			if(line->ncommands>1){
				pipes=malloc(sizeof(int*)*line->ncommands-1);//array de pipes
				for(i=0; i<line->ncommands-1; i++){
					pipes[i]=malloc(sizeof(int*)*2);// en cada posicion del array un nuevo array de 2 posiciones
					pipe(pipes[i]);//convierto el array de 2 posiciones en un pipe
				}
			}
			pids=malloc(sizeof(int*)*line->ncommands);
			
		
			for (i=0; i<line->ncommands; i++) {
				
				pids[i].hijo=fork();
					
				if(pids[i].hijo==0){//entra el hijo
					if(!line->background){
						//reactivo señales del hijo
						signal(SIGINT,SIG_DFL);
						signal(SIGQUIT,SIG_DFL);
					}
					
					if (line->redirect_output!=NULL||line->redirect_input!=NULL||line->redirect_error!=NULL){
						redirecciones(line,i);
					}
					if(line->ncommands>1){//primero
						if(i==0){
							dup2(pipes[i][1],1);//cierro la pantalla de la tabla de descriptores y duplico la entrada de escritura del pipe
						}
						if(i==line->ncommands-1){//ultimo
							dup2(pipes[i-1][0],0);//cierro la entrada estandar (teclado) de la tabla de descriptores y duplico la del pipe para lectura
						}
						if((i!=0)&&(i!=line->ncommands-1)){//del medio
							dup2(pipes[i][1],1);
							dup2(pipes[i-1][0],0);
						}
					}
					for(j=0; j<line->ncommands-1; j++){//cierro todos los pipes del hijo
						close (pipes[j][0]);
						close (pipes[j][1]);
					}
					execvp(line->commands[i].filename,line->commands[i].argv);
					perror(line->commands[i].argv[i]);
					exit(1);
				}
			
				
			}
			for(i=0; i<line->ncommands-1; i++){//cierro todos los pipes del padre
				close (pipes[i][0]);
				close (pipes[i][1]);
			}
			
			if(!line->background){//espera por todos los hijos si esta en &
				for (i=0; i<line->ncommands; i++) {
					waitpid(pids[i].hijo,NULL,0);
				}
			}else {
				for (i=0; i<line->ncommands; i++) {
					printf ("%d\n", pids[i].hijo);
				}
			}
			free(pids);//libero memoria de pids de hijos
			if(line->ncommands>1){//libero memoria de pipes
				for(i=0; i<line->ncommands-1; i++){
					free (pipes[i]);
				}
				free(pipes);
			}
				
							
		}
	
		printf("minishell~~$ ");	
	}
	return 0;
}
