#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>

#define PORT 8080
#define MAXCO 69

// gcc server.c -o server.out -lpthread



void *mainThread(void *arg);
char *getSolicitud(char* httpHeader);
void sendHTML(int cliente);
void sendIcon(int cliente);
void sendImage(char *recurso, int cliente);
void sendVideo(char *recurso, int cliente);
char* parseDir(char * recurso);
void createMidHTML(FILE *html, DIR *dr);
int generateHTML();
int getTOPDOWNhtml();
int isMP4(char* recurso);
int isPNG(char* recurso);
int getCantFrags(int largo, int tamanoFrag);
char * getFragmento(char * buff,int page, int largoArchivo, int tamanoFrag);




char *htmlTOP;
char *htmlTOPDir;
char *htmlBOT;
char *htmlBOTDir;
/*char *templateVideo = 
	"<div class=\"w3-third w3-container w3-margin-bottom\">\n"
	"\t<img src=\"images/%s\"  alt=\"TONACO\" style=\"width:100%%\""
	"class=\"w3-hover-opacity\" onclick=\"\">\n"
	"\t<div class=\"w3-container w3-white\">\n"
	"\t\t<p><b>Texto descripcion</b></p>\n"
	"\t\t<p>Texto de descripcion del archivo</p>\n"
	"\t\t<p>Tamano: en bytes</p>\n"
	"\t\t<p>Fecha y hora del archivo</p>\n"
	"\t</div>\n"
	"</div>\n";*/
char *htmlHeader = 
	"HTTP/1.1 403 Forbidden\r\n"
	"Server: localHost:8080\r\n"
	"Content-Type: text/html; charset=iso-8859-1\r\n"
	"\r\n";
char *html;// = "<!DOCTYPE html><html><body><h1>HelloWorld Tutorial</h1><img src=\"/home/iworth/Escritorio/Proyecto2/dado.png\"></body></html>";

size_t htmlLargo = 0;





char *checksum;


int chingarmeElServidor = 0;
int chingarmeHiloBuscaCamios = 0;


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char const *argv[]){
	
	//Datos necesarios la primera vez


	/*if(getTOPDOWNhtml()){
		exit(-1);
	}*/
	//Generar el html del medio
	if(generateHTML()){
		exit(-1);
	}



	int socketServidor, socketTemporal;
	struct sockaddr_in direccionServidor;
	struct sockaddr_storage servidorStorage;
	socklen_t largoDireccion;

	//socket del servidor
	socketServidor = socket(PF_INET, SOCK_STREAM,0);

	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = INADDR_ANY;
	direccionServidor.sin_port = htons(PORT);

	bind(socketServidor, (struct sockaddr*)&direccionServidor, sizeof(direccionServidor));

	listen(socketServidor, MAXCO);

	while(1){
		largoDireccion = sizeof(servidorStorage);
		socketTemporal = accept(socketServidor, (struct sockaddr*)&servidorStorage, &largoDireccion);

		//hilo para atender socket
		pthread_t nuevoHilo;
		pthread_create(&nuevoHilo, NULL, &mainThread, (void *)&socketTemporal);

		if(chingarmeElServidor){
			break;
		}
	}
	return 0;
}

int generateHTML(){


	FILE * htmlFile = fopen("/home/iworth/Escritorio/Proyecto2/index.html", "r");

	if(htmlFile == 0){
		printf("No se pudo encontrar %s", "htmlFile");
		return 1;
	}

	fseek(htmlFile, 0, SEEK_END);
	htmlLargo = (size_t)ftell(htmlFile);
	fseek(htmlFile, 0, SEEK_SET);


	html = (char *)malloc(htmlLargo + 1);
	fread(html, 1, htmlLargo, htmlFile);
	fclose(htmlFile);

	return 0;
}

void *mainThread(void *arg){
	char *buffer = (char *)malloc(4098);
	
	int webClient = *((int *)arg);

	ssize_t datosLeidos = read(webClient, buffer, 4098);
	

	if(datosLeidos<=0){
		printf("helpp\n");
		close(webClient);
		pthread_exit(NULL);
	}
	printf("DatosLeidos: %s\n",buffer);

	char *recurso = getSolicitud(buffer); //HACER FUNCION - QUE DEVUELVA LO DE ABAJO
	printf("recurso %s\n", recurso);

	if(strcmp(recurso, "/") == 0){
		printf("sendHTML\n");
		sendHTML(webClient);
		printf("htmlrespondido\n");
	}
	else if(strcmp(recurso,"/favicon.ico") == 0){
		printf("sendIcon\n");
		sendIcon(webClient);
	}
	else{
		if(isPNG(recurso)){
			printf("sendImage\n");
			sendImage(recurso,webClient);
		}
		else if(isMP4(recurso)){
			printf("sendVIdeo\n");
			sendVideo(recurso,webClient);
		}else{
			printf("NO POS NO\n");
		}

	}

	close(webClient);
	pthread_exit(NULL);
}
int isPNG(char* recurso){
	int i;
	for(i=0;recurso[i] != '.';i++);
	if(recurso[i+1]=='p' && recurso[i+2]=='n')
		return 1;
	return 0;
}
int isMP4(char* recurso){
	int i;
	for(i=0;recurso[i] != '.';i++);
	if(recurso[i+1]=='m' && recurso[i+2]=='p')
		return 1;
	return 0;
}
char *getSolicitud(char *solicitud){
	if (solicitud[0] == 'G' && solicitud[1] == 'E' && solicitud[2] == 'T') {
		int i;
		for(i = 4; solicitud[i] != ' '; i++);
		if (i == 4){
			char *recurse = {0};
			return recurse;
		}
		int largo = i - 4;
		char *recurse = (char *)malloc(largo + 1);
		int j=0;
		for(int k = 4; k < i; k++){
			recurse[j] = solicitud[k];
			j++;
		}
		recurse[largo] = 0;
		return recurse;
	} else {
		return 0;
	}
}

void sendHTML(int cliente){
	write(cliente, htmlHeader, strlen(htmlHeader));
	pthread_mutex_lock(&mutex);
	htmlLargo = strlen(html);
	write(cliente,html,htmlLargo);
	pthread_mutex_unlock(&mutex);

}

/*
void createMidHTML(FILE * html, DIR *dr){
	struct dirent *de;


	char *abrirContenedor = "<div class=\"w3-row-padding\">";
	fwrite(abrirContenedor, 1, strlen(abrirContenedor), html);


	for (int i = 0; (de = readdir(dr)) != NULL; i++){
		if(strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) {
			i--;
			continue;
		}

		fprintf(html, templateVideo, de->d_name);
	}


	char *cerrarContenedor = "</div>";
	fwrite(cerrarContenedor, 1, strlen(cerrarContenedor), html);

}*/

int getTOPDOWNhtml() {
	size_t largoArchivo;


	FILE * htmlSuperior = fopen(htmlTOPDir, "r");

	if(htmlSuperior == 0){
		printf("No se pudo encontrar %s", htmlTOPDir);
		return 1;
	}

	fseek(htmlSuperior, 0, SEEK_END);
	largoArchivo = (size_t)ftell(htmlSuperior);
	fseek(htmlSuperior, 0, SEEK_SET);


	htmlTOP = (char *)malloc(largoArchivo + 1);
	fread(htmlTOP, 1, largoArchivo, htmlSuperior);
	fclose(htmlSuperior);


	FILE *htmlInferior = fopen(htmlBOTDir, "r");

	if(htmlInferior == 0){
		printf("NO se pudo encontrar %s", htmlBOTDir);
		return 1;
	}

	fseek(htmlInferior, 0, SEEK_END);
	largoArchivo = (size_t)ftell(htmlInferior);
	fseek(htmlInferior, 0 ,SEEK_SET);


	htmlBOT = (char *)malloc(largoArchivo + 1);
	fread(htmlBOT, 1, largoArchivo, htmlInferior);
	fclose(htmlInferior);
}

void sendIcon(int cliente){
	printf("respondio icono\n");
	sendImage("/icon.png", cliente);

}

char* parseDir(char * recurso){
	char * where = "/home/iworth/Escritorio/Proyecto2"; 
	char* result = (char*)malloc(strlen(where)+strlen(recurso)+1);
	strcpy(result,where);
	strcat(result,recurso);
	printf("resiult%s\n",result );
	return result;
}

void sendVideo(char *recurso, int cliente){
	printf("VIDEOrecurso %s\n", recurso);
	char *direccion = parseDir(recurso);
	FILE *image = fopen(direccion,"rb");
	fseek(image,0,SEEK_END);
	size_t largoArchivo = (size_t)ftell(image);
	fseek(image,0,SEEK_SET);


	char *sendbuf = (char *)malloc( largoArchivo + 1);
	size_t result = fread(sendbuf, 1, largoArchivo , image);
	fclose(image);

	//creacion de header image
	char header[2018];
	sprintf(header,
		"HTTP/1,1 200 OK\r\n"
		"Server: localhost:8080\r\n"
		"accept-ranges: bytes\r\n"
		"access-control-allow-origin: \r\n"
		"content-type: video/mp4\r\n"
		"x-content-type-options: nosniff\r\n"
		"x-xss-protection: 1; mode=block\r\n"
		"content-length: %ld\r\n"
		"\r\n", largoArchivo); 

	int i=0;
	write(cliente, header, strlen(header));
	write(cliente,sendbuf,largoArchivo);
	/*int cantPages = getCantFrags(largoArchivo,8000000);
	printf("CANTIDAD DE PAGINAS %d\n", cantPages);
	while(i<=cantPages){
		char * help = getFragmento(sendbuf,i,largoArchivo,8000000);
		write(cliente, help, largoArchivo);
		i++;
	}*/
	
	
	
}
char * getFragmento(char * buff,int page, int largoArchivo, int tamanoFrag){
	int inicio = page * tamanoFrag;
	int fin;
	if((page+1)*tamanoFrag>largoArchivo){
		fin=largoArchivo;
	}
	else{
		fin=(page+1)*tamanoFrag;
	}
	char * res = (char *)malloc( fin-inicio );
	printf("NUMERO DE PAG: %d\n", page);
	printf("FRAGMENTO INICIO: %d\n", inicio);
	while(inicio<fin ){
		res[inicio-(page*tamanoFrag)] = buff[inicio];
		inicio++;
	}
	printf("FRAGMENTO FIN: %d\n", fin);
	return res;
}
int getCantFrags(int largo, int tamanoFrag){
	int completas = largo/tamanoFrag;
	if(largo%tamanoFrag>0){
		completas++;
	}
	completas = completas - 1;
	return completas;
}
void sendImage(char *recurso, int cliente){
	printf("recurso %s\n", recurso);
	char *direccion = parseDir(recurso);
	FILE *image = fopen(direccion,"rb");
	fseek(image,0,SEEK_END);
	size_t largoArchivo = (size_t)ftell(image);
	fseek(image,0,SEEK_SET);


	char *sendbuf = (char *)malloc( largoArchivo + 1);
	size_t result = fread(sendbuf, 1, largoArchivo , image);
	fclose(image);

	//creacion de header image
	char header[2018];
	sprintf(header,
		"HTTP/1,1 200 OK\r\n"
		"Server: localhost:8080\r\n"
		"accept-ranges: bytes\r\n"
		"access-control-allow-origin: \r\n"
		"content-type: image/png\r\n"
		"x-content-type-options: nosniff\r\n"
		"x-xss-protection: 1; mode=block\r\n"
		"content-length: %ld\r\n"
		"\r\n", largoArchivo); 
	
	write(cliente, header, strlen(header));
	write(cliente, sendbuf, largoArchivo);

}