#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>

#include "server.h"
#include "tcp.h"
#include "debug.h"
#include "global.h"
#include "errno.h"

void clientProccess(const int clientSocket);

int startServer(const unsigned int port) {

	int serverSocket;
	int clientSocket;
	char clientIP[16];
	unsigned int clientPort;

	int pid;

	serverSocket = newTCPServerSocket4("127.0.0.1",port,5);
	if(serverSocket == -1) {
		error(errno,"Can't create Socket");
		return FALSE;
	}
	debug(4,"Server Socket Created");

	while(TRUE) {
		clientSocket = waitConnection4(serverSocket,clientIP,&clientPort);
		debug(2,"Connected Client %s:%u",clientIP,clientPort);

		pid = fork();
		if(pid == -1) {
			debug(0,"Can't Fork!");
			close(clientSocket);
			continue;
		} else if(pid == 0 ) {
			// Soy el Hijo.
			clientProccess(clientSocket);
		} else if (pid > 0 ) {
			// Soy el Padre
			close(clientSocket);
		}
    }

	return TRUE;

}


void clientProccess(const int clientSocket) {

	char *buffer;
	char *firstLine;
  	char *ptr;
   	char html[250];
   	int file;
   	int readBytes;
   	int firstFlag;
   	FILE *in;
   	FILE *out;

	/**INICIO DEL PROCESO DEL CLIENTE**/
	debug(2,"Inicio del proceso del Cliente\n");

    in =fdopen(clientSocket, "r");  //Lectura del Socket
	out=fdopen(clientSocket, "w");  //Escritura en el Socket
	buffer = calloc(255,1);
	firstLine = calloc(255,1);
	firstFlag = TRUE;   


	while(fgets(buffer, 255, in)!=NULL) {
			debug(4,"%s",buffer);
			if(strcmp(buffer,"\r\n")==0) {
				break;
			}
			if(firstFlag) {
				strcpy(firstLine,buffer);
				firstFlag = FALSE;
			}
			bzero(buffer,255);
		}

	/**PROCESAR EL GET**/

	firstLine = strtok( firstLine, " " );    
	firstLine = strtok( NULL, " " );    	 
	firstLine++;
	debug(5,"REQUEST: %s\n",firstLine);
			if(strlen(firstLine)==0){
				file = open("test.html",O_RDONLY); 
			}
			else{
				file = open(firstLine,O_RDONLY);
			}

	/**RESPONDER CON 200 OK SI EXISTE**/ 
   		if(file == -1) {
			error(errno, "No se pudo abrir el archivo");
			strcpy(html, "<html><head><title>404 NOT FOUND</title></head><body><h1>ERROR 404 NOT FOUND<h1></body></html>");
			sendTCPLine4(clientSocket, html,strlen(html));
			}
		else {
			strcpy(html, "HTTP/1.1 200 OK\r\n");
			sendTCPLine4(clientSocket, html,strlen(html));

			if(strlen(firstLine)!=0){
				//Se envia informaciosn del archivo
				strcpy(html, "Accept-Ranges: bytes\r\n");
				sendTCPLine4(clientSocket, html,strlen(html));
				debug(3,"SEND: %s",html);
				strcpy(html, "Content-Length: ");

					//Se calcula el tamaño del archivo a enviar
					struct stat *st = (struct stat *)malloc(sizeof(struct stat));          
					stat(firstLine, st);
					int size = st->st_size;
					free(st);
					ptr = (char*)malloc(255);
					sprintf(ptr, "%d", size);
					strcat (html, ptr);
					strcat (html, "\r\n");
				sendTCPLine4(clientSocket, html,strlen(html));
				debug(3,"SEND: %s",html);
				strcpy(html, "Content-Type: application/octet-stream\r\n");
				sendTCPLine4(clientSocket, html,strlen(html));
			} 


			/**Fin del header de envio de archivo**/
			strcpy(html, "\r\n");
			sendTCPLine4(clientSocket, html,strlen(html));
      
		/**ENVIAR EL ARCHIVO**/
		while((readBytes = read(file,buffer,255))>0) {
			debug(6,"readBytes: %d\n",readBytes);
			sendTCPLine4(clientSocket,buffer,readBytes);
		} 
	}

	/**CERRAMOS LA COMUNICACIÓN**/
  	fclose(in);
  	fclose(out);
   	close(clientSocket);
   	free(buffer);

	return;
}
