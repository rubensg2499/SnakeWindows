#pragma once
#define TAMSERP 20

#define CUERPO	1
#define CABEZA	2
#define COLA	3

#define IZQ		1
#define DER		2
#define ARR		3
#define ABA		4

#define CRECE	1
#define ACHICA	2
#define NADA	3

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

struct informacion {
	POS pos;
	int dir;
	int tam;
};
typedef struct informacion INFO;