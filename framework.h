// header.h: archivo de inclusión para archivos de inclusión estándar del sistema,
// o archivos de inclusión específicos de un proyecto
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Excluir material rara vez utilizado de encabezados de Windows
// Archivos de encabezado de Windows
#include <windows.h>
// Archivos de encabezado en tiempo de ejecución de C
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>


struct pos {
	int x;
	int y;
};
typedef struct pos POS;

struct PedacitoS {
	POS pos;
	int tipo;
	int dir;
};
typedef struct PedacitoS PEDACITOS;

struct comida {
	POS pos;
	int tipo;
};
typedef struct comida COMIDA;
