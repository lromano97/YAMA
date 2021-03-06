#include "funcionesYAMA.h"

//GENERACION DE ESTRUCTURAS
administracionYAMA* generarAdministracion(uint32_t nroJob, uint32_t nroMaster, uint32_t operacion, char* nameFile){
	administracionYAMA* adminNuevo = malloc(sizeof(administracionYAMA));
	adminNuevo->nameFile = nameFile;
	adminNuevo->estado = EN_PROCESO;
	adminNuevo->etapa = operacion;
	adminNuevo->nroJob = nroJob;
	adminNuevo->nroMaster = nroMaster;
	return adminNuevo;
}

conexionNodo* generarConexionNodo(){
	conexionNodo* conexion = malloc(sizeof(conexionNodo));
	return conexion;
}

infoDeFs* generarInformacionDeBloque(){
	infoDeFs* informacion = malloc(sizeof(infoDeFs));
	informacion->copia1 = malloc(sizeof(copia));
	informacion->copia2 = malloc(sizeof(copia));
	return informacion;
}

nodoSistema* generarNodoSistema(){
	nodoSistema* nodo = malloc(sizeof(nodoSistema));
	return nodo;
}

//LIBERACION DE ESTRUCTURAS
void liberarDatoMaster(infoNodo* info){
	free(info->nombreTemporal);
	liberarConexion(info->conexion);
	free(info);
}

void liberarConexion(conexionNodo* conexion){
	free(conexion->ipNodo);
	free(conexion->nombreNodo);
	free(conexion);
}

void liberarInfoFS(infoDeFs* infoDeBloque){
	free(infoDeBloque->copia1->nombreNodo);
	free(infoDeBloque->copia2->nombreNodo);
	free(infoDeBloque->copia1);
	free(infoDeBloque->copia2);
	free(infoDeBloque);
}

void liberarCopia(copia* copiaAEnviar) {
	free(copiaAEnviar->nombreNodo);
	free(copiaAEnviar);
}

void liberarInfoNodo(infoNodo* info){
	free(info->nombreTemporal);
	liberarConexion(info->conexion);
	free(info);
}

void liberarDatosBalanceo(datosBalanceo* datos){
	free(datos->nombreNodo);
	list_destroy(datos->bloques);
	free(datos);
}

void liberarNodoSistema(nodoSistema* nodo){
	free(nodo->nombreNodo);
	free(nodo);
}

void liberarAdminYAMA(administracionYAMA* admin){
	free(admin->nameFile);
	free(admin->nombreNodo);
	free(admin);
}

//RANDOM NAMES
char* obtenerNombreTemporalLocal(){
	char* nombreArchivo = string_new();

	pthread_mutex_lock(&semReducLocales);
	int nro = numeroDeTemporalLocal;
	numeroDeTemporalLocal++;
	pthread_mutex_unlock(&semReducLocales);

	string_append(&nombreArchivo, "../../../tmp/tempFileLocal");
	char* numero = string_itoa(nro);
	string_append(&nombreArchivo, numero);

	free(numero);
	return nombreArchivo;
}

char* obtenerNombreTemporalGlobal(){
	char* nombreArchivo = string_new();

	pthread_mutex_lock(&semReducGlobales);
	int nro = numeroDeTemporalGlobal;
	numeroDeTemporalGlobal++;
	pthread_mutex_unlock(&semReducGlobales);

	string_append(&nombreArchivo, "../../../tmp/tempFileGlobal");
	char* numero = string_itoa(nro);
	string_append(&nombreArchivo, numero);
	free(numero);
	return nombreArchivo;
}

char* obtenerNombreTemporalTransformacion(){
	char* nombreArchivo = string_new();

	pthread_mutex_lock(&semTransformaciones);
	int nro = numeroDeTemporalTransformacion;
	numeroDeTemporalTransformacion++;
	pthread_mutex_unlock(&semTransformaciones);

	char* numero = string_itoa(nro);
	string_append(&nombreArchivo, "../../../tmp/tempFileTransformacion");
	string_append(&nombreArchivo, numero);
	free(numero);
	return nombreArchivo;
}

int obtenerNumeroDeJob(){

	pthread_mutex_lock(&semContJobs);
	contadorDeJobs++;
	int nroCont = contadorDeJobs;
	pthread_mutex_unlock(&semContJobs);

	return nroCont;
}

int obtenerNumeroDeMaster(){

	pthread_mutex_lock(&semContMaster);
	contadorDeMasters++;
	int nroCont = contadorDeMasters;
	pthread_mutex_unlock(&semContMaster);

	return nroCont;
}

//OBTENER DATOS DE CONEXION CON NODO

void deserializarIPYPuerto(conexionNodo* conexion){
	if(recibirUInt(socketFS) != DATOS_NODO){
		conexion->puertoNodo = 0;
		conexion->ipNodo = NULL;
	}else{
		conexion->puertoNodo = recibirUInt(socketFS);
		conexion->ipNodo = recibirString(socketFS);
	}
}

void obtenerIPYPuerto(conexionNodo* conexion){
	void* mensaje = malloc(sizeof(int)+string_length(conexion->nombreNodo));
	uint32_t tamanio = string_length(conexion->nombreNodo);
	memcpy(mensaje, &tamanio, sizeof(int));
	memcpy(mensaje + sizeof(int), conexion->nombreNodo, tamanio);
	sendRemasterizado(socketFS, DATOS_NODO, sizeof(int)+tamanio, mensaje);
	free(mensaje);
	deserializarIPYPuerto(conexion);
}

//GETTERS
//GETTERS NODOS
int obtenerJobDeNodo(t_list* listaDelNodo){
	administracionYAMA* admin = list_get(listaDelNodo, 0);
	return admin->nroJob;
}

t_list* obtenerListaDelNodo(int nroMaster, int socketMaster, char* nombreNodo){
	bool esDeNodo(administracionYAMA* admin){
		return (!strcmp(nombreNodo, admin->nombreNodo) && admin->nroMaster == nroMaster && admin->etapa == TRANSFORMACION && admin->estado != FALLO);
	}
	pthread_mutex_lock(&semTablaEstados);
	t_list* listaDelNodo = list_filter(tablaDeEstados, (void*)esDeNodo);
	pthread_mutex_unlock(&semTablaEstados);
	return listaDelNodo;
}

char* obtenerNombreNodo(t_list* listaDelNodo){
	administracionYAMA* admin = list_get(listaDelNodo, 0);
	char* nombreNodo = string_new();
	string_append(&nombreNodo, admin->nombreNodo);
	return nombreNodo;
}

//PUESTA EN MARCHA DE YAMA
void cargarYAMA(t_config* configuracionYAMA){
    if(!config_has_property(configuracionYAMA, "FS_IP")){
        log_error(loggerYAMA, "ERROR - NO SE ENCUENTRA FS_IP EN CONFIG");
        exit(-1);
    }
    FS_IP = string_new();
	string_append(&FS_IP, config_get_string_value(configuracionYAMA, "FS_IP"));
    if(!config_has_property(configuracionYAMA, "FS_PUERTO")){
        log_error(loggerYAMA, "ERROR - NO SE ENCUENTRA FS_PUERTO EN CONFIG");
        exit(-1);
    }
    FS_PUERTO = config_get_int_value(configuracionYAMA, "FS_PUERTO");
    if(!config_has_property(configuracionYAMA, "RETARDO_PLANIFICACION")){
        log_error(loggerYAMA, "ERROR - NO SE ENCUENTRA RETARDO_PLANIFICACION EN CONFIG");
        exit(-1);
    }
    RETARDO_PLANIFICACION = config_get_int_value(configuracionYAMA, "RETARDO_PLANIFICACION");
    if(!config_has_property(configuracionYAMA, "ALGORITMO_BALANCEO")){
    	log_error(loggerYAMA, "ERROR - NO SE ENCUENTRA ALGORITMO_BALANCEO EN CONFIG");
    	exit(-1);
    }
    ALGORITMO_BALANCEO = string_new();
    string_append(&ALGORITMO_BALANCEO, config_get_string_value(configuracionYAMA, "ALGORITMO_BALANCEO"));
    if(!config_has_property(configuracionYAMA, "PUERTO_MASTERS")){
    	log_error(loggerYAMA, "ERROR - NO SE ENCUENTRA PUERTO_MASTERS EN CONFIG");
    	exit(-1);
    }
    PUERTO_MASTERS = config_get_int_value(configuracionYAMA, "PUERTO_MASTERS");
    if(!config_has_property(configuracionYAMA, "BASE_AVAILABILITY")){
    	log_error(loggerYAMA, "ERROR - NO SE ENCUENTRA BASE_AVAILABILITY EN CONFIG");
    	exit(-1);
    }
    BASE_AVAILABILITY = config_get_int_value(configuracionYAMA, "BASE_AVAILABILITY");
    config_destroy(configuracionYAMA);
}


//CHEQUEO DE SIGNAL
void chequeameLaSignal(int signal){
	//PASO A RECARGAR EL ARCHIVO
	log_info(loggerYAMA, "SIGNAL - SIGUSR1");
	sigusr1Activa = true;
	t_config* configuracionNueva = generarTConfig("off_yama.ini", 6);
	if(!config_has_property(configuracionNueva, "RETARDO_PLANIFICACION")){
		log_error(loggerYAMA, "ERROR - NO SE ECONTRO RETARDO_PLANIFICACION EN CONFIG");
		exit(-1);
	}else{
		RETARDO_PLANIFICACION = config_get_int_value(configuracionNueva, "RETARDO_PLANIFICACION");
		log_info(loggerYAMA, "CONFIG - RETARDO_PLANIFICACION %d", RETARDO_PLANIFICACION);
	}
	if(!config_has_property(configuracionNueva, "ALGORITMO_BALANCEO")){
		log_error(loggerYAMA, "ERROR - NO SE ENCONTRO ALGORITMO_BALANCEO EN CONFIG");
		exit(-1);
	}else{
		ALGORITMO_BALANCEO = string_new();
		string_append(&ALGORITMO_BALANCEO, config_get_string_value(configuracionNueva, "ALGORITMO_BALANCEO"));
		log_info(loggerYAMA, "CONFIG - ALGORITMO_BALANCEO %s", ALGORITMO_BALANCEO);
	}
	config_destroy(configuracionNueva);
}

void laParca(int signal){
	//Se prosigue a morir elegantemente
	log_trace(loggerYAMA, "SIGNAL - SIGINT");
	log_info(loggerYAMA, "MURIENDO CON ESTILO");
	list_destroy_and_destroy_elements(nodosSistema, (void*)liberarNodoSistema);
	list_destroy_and_destroy_elements(tablaDeEstados, (void*)liberarAdminYAMA);
	free(FS_IP);
	free(ALGORITMO_BALANCEO);
	log_trace(loggerYAMA, "ESTRUCTURAS LIBERADAS");
	log_trace(loggerYAMA, "CERRANDO SOCKETS");
	close(socketEscuchaMasters);
	close(socketFS);
	log_debug(loggerYAMA, "CIERRA YAMA");
	log_destroy(loggerYAMA);
	exit(0);
}

//ENVIO DE MENSAJES
//void enviarCopiaAMaster(int socket, copia* copiaAEnviar){
//	conexionNodo* conection = generarConexionNodo();
//	string_append(&conection->nombreNodo, copiaAEnviar->nombreNodo);
//	obtenerIPYPuerto(conection);
//	void* copiaSerializada = serializarCopia(copiaAEnviar, conection);
//	sendRemasterizado(socket, REPLANIFICAR, obtenerTamanioCopia(copiaAEnviar, conection), copiaSerializada);
//	liberarCopia(copiaAEnviar);
//}

//HANDSHAKE CON FS (RECIBO LOS NODOS DEL SISTEMA)
void handshakeFS(){
	sendDeNotificacion(socketFS, ES_YAMA);
	if(recibirUInt(socketFS) != ES_FS){
		log_error(loggerYAMA, "ERROR - CONEXION CON FILESYSTEM ERRONEA");
		exit(-1);
	}
	uint32_t cantidadDeNodos = recibirUInt(socketFS);
	log_info(loggerYAMA, "CANTIDAD DE NODOS DEL SISTEMA %d", cantidadDeNodos);
	uint32_t i;
	for(i = 0; i<cantidadDeNodos; i++){
		nodoSistema* nuevoNodo = generarNodoSistema();
		nuevoNodo->nombreNodo = recibirString(socketFS);
		nuevoNodo->wl = 0;
		list_add(nodosSistema, nuevoNodo);
	}
	log_debug(loggerYAMA, "INFORMACION DE NODOS RECIBIDA CORRECTAMENTE");
}

//FUNCIONES PARA MANEJAR LA AVAILABILITY
uint32_t obtenerWLMax(){
	uint32_t maximo = 0;
	uint32_t posicion;
	pthread_mutex_lock(&semNodosSistema);
	for(posicion = 0; posicion < list_size(nodosSistema); posicion++){
		nodoSistema* nodo = list_get(nodosSistema, posicion);
		if(nodo->wl >= maximo){
			maximo = nodo->wl;
		}
	}
	pthread_mutex_unlock(&semNodosSistema);
	printf("maximo wl: %d", maximo);
	return maximo;
}

int calculoAvailability(char* nombreNodo){
	bool esNodo(nodoSistema* nodoAChequear){
		return strcmp(nombreNodo, nodoAChequear->nombreNodo) == 0;
	}
	uint32_t availability = 0;
	if(strcmp(ALGORITMO_BALANCEO, "Clock") == 0){
		availability = BASE_AVAILABILITY;
	}else{
		int wlMax = obtenerWLMax();
		pthread_mutex_lock(&semNodosSistema);
		nodoSistema* nodo = list_find(nodosSistema, (void*)esNodo);
		availability = BASE_AVAILABILITY + wlMax - nodo->wl;
		pthread_mutex_unlock(&semNodosSistema);
	}
	return availability;
}

//ARMO LOS DATOS DE BALANCEO A PARTIR DE LOS DATOS RECIBIDOS DE LA LISTA DE FS
datosBalanceo* obtenerDatosDeCopia(t_list* listaDeBalanceo, copia* copiaAChequear){
	bool existeEnLaLista(datosBalanceo* datos){
		return strcmp(datos->nombreNodo, copiaAChequear->nombreNodo) == 0;
	}
	return list_find(listaDeBalanceo, (void*)existeEnLaLista);
}

datosBalanceo* generarDatosBalanceo(){
	datosBalanceo* datos = malloc(sizeof(datosBalanceo));
	datos->nombreNodo = string_new();
	datos->bloques = list_create();
	return datos;
}

t_list* armarDatosBalanceo(t_list* listaDeBloques){
	bool porMayorAvailability(datosBalanceo* dato1, datosBalanceo* datos2){
		return dato1->availability > datos2->availability;
	}
	uint32_t posicion;
	t_list* listaDeBalanceo = list_create();
	for(posicion = 0; posicion < list_size(listaDeBloques); posicion++){
		infoDeFs* informacionAOrdenar = list_get(listaDeBloques, posicion);
		datosBalanceo* datoCopia1 = obtenerDatosDeCopia(listaDeBalanceo, informacionAOrdenar->copia1);
		datosBalanceo* datoCopia2 = obtenerDatosDeCopia(listaDeBalanceo, informacionAOrdenar->copia2);
		if(datoCopia1 != NULL){
			list_add(datoCopia1->bloques, informacionAOrdenar->nroBloque);
		}else{
			datosBalanceo* datosAAgregar = generarDatosBalanceo();
			string_append(&datosAAgregar->nombreNodo, informacionAOrdenar->copia1->nombreNodo);
			datosAAgregar->availability = calculoAvailability(datosAAgregar->nombreNodo);
			list_add(datosAAgregar->bloques, informacionAOrdenar->nroBloque);
			list_add(listaDeBalanceo, datosAAgregar);
		}
		if(datoCopia2 != NULL){
			list_add(datoCopia2->bloques, informacionAOrdenar->nroBloque);
		}else{
			datosBalanceo* datosAAgregar = generarDatosBalanceo();
			string_append(&datosAAgregar->nombreNodo, informacionAOrdenar->copia2->nombreNodo);
			datosAAgregar->availability = calculoAvailability(datosAAgregar->nombreNodo);
			list_add(datosAAgregar->bloques, informacionAOrdenar->nroBloque);
			list_add(listaDeBalanceo, datosAAgregar);
		}
	}
	list_sort(listaDeBalanceo, (void*)porMayorAvailability); //ORDENO LA LISTA BALANCEO A PARTIR DEL availability
	return listaDeBalanceo;
}


char* buscarNodoEncargado(uint32_t nroMaster){
	bool esReduccionFinalizada(administracionYAMA* admin){
		return admin->nroMaster == nroMaster && admin->estado == FINALIZADO && admin->etapa == REDUCCION_GLOBAL;
	}
	char* nodoEncargado = string_new();
	pthread_mutex_lock(&semTablaEstados);
	administracionYAMA* admin = list_find(tablaDeEstados, (void*)esReduccionFinalizada);
	string_append(&nodoEncargado, admin->nombreNodo);
	pthread_mutex_unlock(&semTablaEstados);
	return nodoEncargado;
}


t_list* filtrarTablaMaster(uint32_t nroMaster){
  bool esDeMaster(administracionYAMA* admin){
    return admin->nroMaster == nroMaster;
  }
  t_list* listaMaster = list_filter(tablaDeEstados, (void*)esDeMaster);
  return listaMaster;
}


void imprimirConfigs(){
	printf("-------CONFIGURACION-------\n");
	printf("FS_IP: %s\n", FS_IP);
	printf("FS_PUERTO: %d\n", FS_PUERTO);
	printf("RETARDO_PLANIFICACION: %d\n", RETARDO_PLANIFICACION);
	printf("ALGORITMO_BALANCEO: %s\n", ALGORITMO_BALANCEO);
	printf("PUERTO_MASTERS: %d\n", PUERTO_MASTERS);
	printf("BASE_AVAILABILITY: %d\n", BASE_AVAILABILITY);
	printf("---------------------------\n");
}

void imprimirWLs(){
	printf("-------WL ACTUALES-------\n");
	uint32_t posicion;
	pthread_mutex_lock(&semNodosSistema);
	for(posicion = 0; posicion < list_size(nodosSistema); posicion++){
		nodoSistema* nodo = list_get(nodosSistema, posicion);
		printf("NODO: %s - WL: %d\n", nodo->nombreNodo, nodo->wl);
	}
	pthread_mutex_unlock(&semNodosSistema);
}

void eliminarTransformaciones(t_list* listaTransfMaster){
	uint32_t posicion;
	for(posicion = 0; posicion < list_size(listaTransfMaster); posicion++){
		administracionYAMA* admin = list_get(listaTransfMaster, posicion);
		bool esNodo(nodoSistema* nodo){
			return strcmp(nodo->nombreNodo, admin->nombreNodo) == 0;
		}
		pthread_mutex_lock(&semNodosSistema);
		nodoSistema* nodo = list_find(nodosSistema, (void*)esNodo);
		nodo->wl--;
		printf("%s %d", nodo->nombreNodo, nodo->wl);
		pthread_mutex_unlock(&semNodosSistema);
	}
}

void eliminarReduccionesLocales(t_list* listaReducLTerminadas, t_list* listaTransfTerminadas){
	uint32_t posicion;
	for(posicion = 0; posicion < list_size(listaReducLTerminadas); posicion++){
		administracionYAMA* admin = list_get(listaReducLTerminadas, posicion);
		bool esNodo(nodoSistema* nodo){
			return strcmp(nodo->nombreNodo, admin->nombreNodo) == 0;
		}
		pthread_mutex_lock(&semNodosSistema);
		nodoSistema* nodo = list_find(nodosSistema, (void*)esNodo);
		nodo->wl--;
		printf("%s %d", nodo->nombreNodo, nodo->wl);
		pthread_mutex_unlock(&semNodosSistema);
		admin = list_get(listaTransfTerminadas, posicion);
		pthread_mutex_lock(&semNodosSistema);
		nodoSistema* nodo2 = list_find(nodosSistema, (void*)esNodo);
		nodo2->wl--;
		printf("%s %d", nodo->nombreNodo, nodo->wl);
		pthread_mutex_unlock(&semNodosSistema);
	}
}

uint32_t peekingNotificacion(int socketMensajero){
	uint32_t notificacion;
	uint32_t resultadoRecv = recv(socketMensajero, &notificacion, sizeof(uint32_t), MSG_WAITALL | MSG_PEEK);
	if(resultadoRecv <= 0){
		return 0;
	}
	return notificacion;
}
