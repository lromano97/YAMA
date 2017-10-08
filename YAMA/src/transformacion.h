/*
 * transformacion.h
 *
 *  Created on: 22/9/2017
 *      Author: utnso
 */
#include "estructuras.h"
#include "funcionesYAMA.h"
#include "serializaciones.h"
#include "balanceoDeCargas.h"


#ifndef TRANSFORMACION_H_
#define TRANSFORMACION_H_

void solicitarArchivo(char*);
t_list *recibirInfoArchivo();
char* recibirNombreArchivo(int);
void cargarTransformacion(int, int, t_list*);
void terminarTransformacion(int, int);

#endif /* TRANSFORMACION_H_ */
