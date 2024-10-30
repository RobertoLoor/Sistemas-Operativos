#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h> 

void generarArbol(const int fil, const int col){ 
	for(int i = 0; i < fil; i++){
	__pid_t pid = fork();
		if(pid == 0){
			for(int j=1; j < col; j++){
				__pid_t pid = fork();
				if(pid > 0){
					break;
				}
		    }
			break;
	    }
    }
	sleep(10);
}    

int main(int argc, char *argv[]){
	int fil = 0, col = 0;
	
	if(argc != 3){
		printf("Argumentos equivocados.\n");
		return 1;
	}
	else{
		fil = atoi(argv[1]);
		col = atoi(argv[2]);
		generarArbol(fil, col);
	}
	
	return 0;
}


