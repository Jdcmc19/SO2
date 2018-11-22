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
#define MAXIMAS_CONEXIONES 69
#define TIEMPO_REVISAR_CHECKSUM 15




void *hiloServidor(void *arg);
char *obtenerRecursoSolicitado(char* httpHeader);
void responderHTML(int cliente);
void responderIcono(int cliente);
void responderImagen(char *recurso, int cliente);
char* parseDir(char * recurso);
void escribirParteImagenes(FILE *html, DIR *dr);
int generarHTML();
int cargarPartesHTML();
void *hiloBuscarCambios();
char *actualizarCheckSum();




char *htmlParteSuperior;
char *htmlParteSuperiorDir;
char *htmlParteInferior;
char *htmlParteInferiorDir;
char *templateVideo = 
	"<div class=\"w3-third w3-container w3-margin-bottom\">\n"
	"\t<img src=\"images/%s\"  alt=\"TONACO\" style=\"width:100%%\""
	"class=\"w3-hover-opacity\" onclick=\"\">\n"
	"\t<div class=\"w3-container w3-white\">\n"
	"\t\t<p><b>Texto descripcion</b></p>\n"
	"\t\t<p>Texto de descripcion del archivo</p>\n"
	"\t\t<p>Tamano: en bytes</p>\n"
	"\t\t<p>Fecha y hora del archivo</p>\n"
	"\t</div>\n"
	"</div>\n";
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

	//Partes estaticas a memoria - Las fijas
	/*if(cargarPartesHTML()){
		exit(-1);
	}*/
	//Generar el html del medio
	if(generarHTML()){
		exit(-1);
	}

	//Generar el checksum 1st time
	/*checkSum = actualizarCheckSum();*/

	//Thread para regenerar el html
	/*pthread_t hiloGeneradorHTML;
	pthread_create(&hiloGeneradorHTML,NULL,&hiloBuscarCambios,NULL);*/

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

	listen(socketServidor, MAXIMAS_CONEXIONES);

	while(1){
		largoDireccion = sizeof(servidorStorage);
		socketTemporal = accept(socketServidor, (struct sockaddr*)&servidorStorage, &largoDireccion);

		//hilo para atender socket
		pthread_t nuevoHilo;
		pthread_create(&nuevoHilo, NULL, &hiloServidor, (void *)&socketTemporal);

		if(chingarmeElServidor){
			break;
		}
	}
	return 0;
}

int generarHTML(){


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

void *hiloServidor(void *arg){
	char *buffer = (char *)malloc(4098);
	
	int socketCliente = *((int *)arg);

	ssize_t datosLeidos = read(socketCliente, buffer, 4098);
	

	if(datosLeidos<=0){
		printf("helpp\n");
		close(socketCliente);
		pthread_exit(NULL);
	}
	printf("DatosLeidos: %s\n",buffer);

	char *recurso = obtenerRecursoSolicitado(buffer); //HACER FUNCION - QUE DEVUELVA LO DE ABAJO
	printf("recurso %s\n", recurso);

	if(strcmp(recurso, "/") == 0){
		printf("responderHTML\n");
		responderHTML(socketCliente);
		printf("htmlrespondido\n");
	}
	else if(strcmp(recurso,"/favicon.ico") == 0){
		printf("responderIcono\n");
		responderIcono(socketCliente);
	}
	else{
		printf("responderImagen\n");
		responderImagen(recurso,socketCliente);
	}

	close(socketCliente);
	pthread_exit(NULL);
}

char *obtenerRecursoSolicitado(char *httpHeader){
	if (httpHeader[0] == 'G' &&
	    httpHeader[1] == 'E' &&
	    httpHeader[2] == 'T') {
		int i;
		for(i = 4; httpHeader[i] != ' '; i++);
		if (i == 4){
			char *recurso = {0};
			return recurso;
		}
		int largo = i - 4;
		char *recurso = (char *)malloc(largo + 1);
		int j=0;
		for(int k = 4; k < i; k++){
			recurso[j] = httpHeader[k];
			j++;
		}
		recurso[largo] = 0;
		return recurso;
	} else {
		fprintf(stderr, "%s\n", "NO ERA UN GET");
		return 0;
	}
}

void responderHTML(int cliente){


	write(cliente, htmlHeader, strlen(htmlHeader));



	pthread_mutex_lock(&mutex);
	htmlLargo = strlen(html);
	write(cliente,html,htmlLargo);
	pthread_mutex_unlock(&mutex);

}







void escribirParteImagenes(FILE * html, DIR *dr){
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

}












































int cargarPartesHTML() {
	size_t largoArchivo;


	FILE * htmlSuperior = fopen(htmlParteSuperiorDir, "r");

	if(htmlSuperior == 0){
		printf("No se pudo encontrar %s", htmlParteSuperiorDir);
		return 1;
	}

	fseek(htmlSuperior, 0, SEEK_END);
	largoArchivo = (size_t)ftell(htmlSuperior);
	fseek(htmlSuperior, 0, SEEK_SET);


	htmlParteSuperior = (char *)malloc(largoArchivo + 1);
	fread(htmlParteSuperior, 1, largoArchivo, htmlSuperior);
	fclose(htmlSuperior);


	FILE *htmlInferior = fopen(htmlParteInferiorDir, "r");

	if(htmlInferior == 0){
		printf("NO se pudo encontrar %s", htmlParteInferiorDir);
		return 1;
	}

	fseek(htmlInferior, 0, SEEK_END);
	largoArchivo = (size_t)ftell(htmlInferior);
	fseek(htmlInferior, 0 ,SEEK_SET);


	htmlParteInferior = (char *)malloc(largoArchivo + 1);
	fread(htmlParteInferior, 1, largoArchivo, htmlInferior);
	fclose(htmlInferior);
}

void responderIcono(int cliente){
	printf("respondio icono\n");
	responderImagen("/icon.png", cliente);

}

char* parseDir(char * recurso){
	char * where = "/home/iworth/Escritorio/Proyecto2"; 
	char* result = (char*)malloc(strlen(where)+strlen(recurso)+1);
	strcpy(result,where);
	strcat(result,recurso);
	printf("resiult%s\n",result );
	return result;
}

void responderImagen(char *recurso, int cliente){
	printf("recurso %s\n", recurso);
	char *direccion = parseDir(recurso);
	printf("prueba1\n");
	FILE *image = fopen(direccion,"rb");
	printf("prueba2\n");
	fseek(image,0,SEEK_END);
	printf("prueba3\n");
	size_t largoArchivo = (size_t)ftell(image);
	printf("largo archivo %ld\n", largoArchivo);
	fseek(image,0,SEEK_SET);


	char *sendbuf = (char *)malloc( largoArchivo + 1);
	size_t result = fread(sendbuf, 1, largoArchivo , image);
	printf("result %ld\n", result);
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
	
	printf("largo archivo ultimo %ld\n", largoArchivo);
	write(cliente, header, strlen(header));
	printf("%ld\n", sizeof(sendbuf));
	write(cliente, sendbuf, largoArchivo);

}