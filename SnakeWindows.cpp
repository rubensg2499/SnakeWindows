// SnakeWindows.cpp : Define el punto de entrada de la aplicación.
//

#include "framework.h"
#include "SnakeWindows.h"
#include "Serpiente.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#pragma comment(lib, "comctl32.lib")

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#define MAX_LOADSTRING 100
#define DEFAULT_BUFLEN	512
#define DEFAULT_PORT	"4200"

#define ID_TIMER1	1

COMIDA com = { {0,0}, NADA };

//Funciones propias de la serpiente.
PEDACITOS * NuevaSerpiente(int, int, int);
PEDACITOS * AjustarSerpiente(PEDACITOS*, int*, int, RECT);
void DibujarSerpiente(HDC, const PEDACITOS*);
bool MoverSerpiente(PEDACITOS*, int, RECT, int);
bool Colisionar(PEDACITOS*, int);
bool Comer(PEDACITOS*, int);

//Funciones para el Servidor y Cliente.
DWORD WINAPI Servidor(LPVOID);
bool Cliente(HWND, char*, PSTR);
void ObtenerDatos(char*, HWND);
void EnviarMensaje(HWND, char*, HWND);
void ItoC(int, int, int, int, int, int, int, int, int, char*);
bool ColisionarSerpientes(PEDACITOS*, PEDACITOS*, int, int);

static PEDACITOS* serpiente_cliente = NULL;
static PEDACITOS* serpiente = NULL;
static RECT rect;
static int juego_actual = SOLO;
static HPEN plumaAzul, plumaRosa, plumaNegra;
static HBRUSH brochaAzul, brochaRosa, brochaNegra;
static int tams = 5;
static int tamsC = 5;
static int cliente_conectado = FALSE;
// Variables globales:
HINSTANCE hInst;                                // instancia actual
WCHAR szTitle[MAX_LOADSTRING];                  // Texto de la barra de título
WCHAR szWindowClass[MAX_LOADSTRING];            // nombre de clase de la ventana principal

// Declaraciones de funciones adelantadas incluidas en este módulo de código:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

char szMiIP[17] = "127.0.0.1";
char szUsuario[32] = "User";
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Colocar código aquí.
	TCHAR tchIP[17];
	if (_tcslen(lpCmdLine) > 10) {
		swscanf(lpCmdLine, L"%s", tchIP);
		wcstombs(szMiIP, tchIP, 17);
	}
    // Inicializar cadenas globales
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SNAKEWINDOWS, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Realizar la inicialización de la aplicación:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SNAKEWINDOWS));

    MSG msg;

    // Bucle principal de mensajes:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCIÓN: MyRegisterClass()
//
//  PROPÓSITO: Registra la clase de ventana.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SNAKEWINDOWS));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_SNAKEWINDOWS);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCIÓN: InitInstance(HINSTANCE, int)
//
//   PROPÓSITO: Guarda el identificador de instancia y crea la ventana principal
//
//   COMENTARIOS:
//
//        En esta función, se guarda el identificador de instancia en una variable común y
//        se crea y muestra la ventana principal del programa.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Almacenar identificador de instancia en una variable global
   int iWidth = 516;
   int iHeight = 620;
   int iLeft = (GetSystemMetrics(SM_CXSCREEN) / 2) - (iWidth / 2);
   int iTop = (GetSystemMetrics(SM_CYSCREEN) / 2) - (iHeight / 2);
   HWND hWnd = CreateWindowW(szWindowClass, L"Snake Windows Ruben", WS_OVERLAPPEDWINDOW,
      iLeft, iTop, iWidth, iHeight, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCIÓN: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PROPÓSITO: Procesa mensajes de la ventana principal.
//
//  WM_COMMAND  - procesar el menú de aplicaciones
//  WM_PAINT    - Pintar la ventana principal
//  WM_DESTROY  - publicar un mensaje de salida y volver
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	char buffer[64] = "";
	static int cuenta = 0;
	static HANDLE hHiloServidor;
	static HANDLE hHiloCliente;
	static DWORD idHiloServidor;
	static DWORD idHiloCliente;
	static HWND hIP;

	switch (message)
	{
	case WM_CREATE: {
		hIP = CreateWindowExW(0, L"EDIT", L"", ES_LEFT | WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
			10, 530, 200, 20,
			hWnd, (HMENU)IDC_EDITIP, hInst, NULL);
		plumaRosa = CreatePen(PS_SOLID, 1, RGB(255, 20, 147));
		plumaAzul = CreatePen(PS_SOLID, 1, RGB(30, 144, 255));
		plumaNegra = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
		brochaRosa = CreateSolidBrush(RGB(255, 20, 147));
		brochaAzul = CreateSolidBrush(RGB(30, 144, 255));
		brochaNegra = CreateSolidBrush(RGB(0, 0, 0));
		ShowWindow(hIP, TRUE);
		serpiente = NuevaSerpiente(5, 3, 3);
		serpiente_cliente = NuevaSerpiente(5, 3, 10);
		break;
	}
	case WM_TIMER: {
		switch (wParam)
		{
		case ID_TIMER1: {
			GetClientRect(hWnd, &rect);
			//Ambas serpientes se mueven con el timer del servidor.
			//Este timer no está activo en ambos programas, solo en el
			//servidor, el cliente solo copia los datos del servidor.
			//Por lo que dentro de este timer solamente el servidor
			//envía sus mensajes al cliente cada se vence este timer.
			//Todo esto para actualizar visualmente al cliente.
			if (juego_actual == SERVIDOR && cliente_conectado == TRUE) {
				strcpy(buffer, "");
				ItoC(
					serpiente[tams - 1].dir,
					tams, SERVIDOR,
					serpiente[tams - 1].pos.x,
					serpiente[tams - 1].pos.y,
					com.tipo, com.pos.x, com.pos.y,
					TRUE, buffer
				);
				EnviarMensaje(hWnd, buffer, hIP);
			}
			//Se genera la comida solamente si hay un cliente conectado.
			if (cliente_conectado == TRUE) {
				cuenta++;
				if (cuenta == 25) {
					if (rand() % 100 < 80) {
						com.tipo = CRECE;
					}
					else {
						com.tipo = ACHICA;
					}
					com.pos.x = rand() % rect.right / TAMSERP;
					com.pos.y = rand() % rect.bottom / TAMSERP;
					cuenta = 0;
				}
			}
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		}
		}
		break;
	}
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Analizar las selecciones de menú:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case IDM_JUGARSOLO:
			juego_actual = SOLO;
			//Se juega cuando el modo de juego es SOLO
			if (serpiente != NULL) {
				juego_actual = SOLO;
				KillTimer(hWnd, ID_TIMER1);
				free(serpiente);
				tams = 5;
				cuenta = 0;
				serpiente = NuevaSerpiente(tams, 3, 3);
				SetTimer(hWnd, ID_TIMER1, 500, NULL);
				InvalidateRect(hWnd, NULL, TRUE);
			}
			break;
		case IDM_JUGARMULTI: {
			CloseHandle(hHiloServidor);
			//Se levanta el hilo Servidor
			juego_actual = SERVIDOR;
			free(serpiente);
			free(serpiente_cliente);
			tams = 5;
			tamsC = 5;
			serpiente = NuevaSerpiente(tams, 3, 3);
			serpiente_cliente = NuevaSerpiente(tamsC, 3, 10);
			SetTimer(hWnd, ID_TIMER1, 500, NULL);
			hHiloServidor = CreateThread(
				NULL,
				0, Servidor,
				(LPVOID)hWnd,
				0, &idHiloServidor
			);
			if (hHiloServidor == NULL) {
				MessageBox(hWnd, L"Error al crear el hilo servidor", L"Error", MB_OK | MB_ICONERROR);
			}
			InvalidateRect(hWnd, NULL, TRUE);
			}
			SetFocus(hWnd);
			break;
		case IDM_CONECTARSE:
			//Se levanta el hilo cliente, aunque es solo una forma de llamarlo
			//dado que también ejecuta código de servidor.
			//Se dice que es cliente porque se levanta del lado del "cliente"
			CloseHandle(hHiloCliente);
			juego_actual = CLIENTE;
			free(serpiente);
			free(serpiente_cliente);
			tams = 5;
			tamsC = 5;
			serpiente = NuevaSerpiente(tams, 3, 3);
			serpiente_cliente = NuevaSerpiente(tamsC, 3, 10);
			hHiloCliente = CreateThread(
				NULL,
				0, Servidor,
				(LPVOID)hWnd,
				0, &idHiloCliente
			);

			if (hHiloCliente == NULL) {
				MessageBox(hWnd, L"Error al crear el hilo para el cliente", L"Error", MB_OK | MB_ICONERROR);
			}
			strcpy(buffer, "");
			ItoC(
				serpiente_cliente[tamsC - 1].dir,
				tamsC, CLIENTE,
				serpiente_cliente[tamsC - 1].pos.x,
				serpiente_cliente[tamsC - 1].pos.y,
				com.tipo, com.pos.x, com.pos.y,
				TRUE, buffer
			);
			EnviarMensaje(hWnd, buffer, hIP);
			SetFocus(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_KEYDOWN: {
		GetClientRect(hWnd, &rect);
		switch (wParam)
		{
		/*
			* En la parte de las teclas, se optó por no avanzar con ellas sino solamente
			* cambiar la dirección de las serpientes dependiendo del caso.
			* 
			* El servidor por ejemplo, no tiene que enviar mensajes de cambio así mismo,
			* simplemente tiene que cambiar la dirección de la serpiente según ciertas 
			* reglas.
			* 
			* El cliente por otro lado, depende de lo que esté pasando con el servidor, pero
			* como tiene que tener autonomía de movimiento, lo que hace es cambiar la
			* dirección de su serpiente, su tamaño, su posción en x,y 
			* y enviar los nuevos datos al servidor, quien procesará
			* la información de tal modo que en este se harán los movimientos y respondiendo
			* al cliente con nueva información para visualizar.
			* Más abajo se dan más detalles.
		*/
		case VK_RIGHT: {
			if (juego_actual == SOLO) {
				if (!MoverSerpiente(serpiente, DER, rect, tams)) {
					KillTimer(hWnd, ID_TIMER1);
					MessageBox(hWnd, L"Ya se murio F", L"Fin del juego", MB_OK | MB_ICONINFORMATION);
				}
				if (Comer(serpiente, tams)) {
					serpiente = AjustarSerpiente(serpiente, &tams, com.tipo, rect);
					com.tipo = NADA;
				}
			}
			if (juego_actual == SERVIDOR) {
				/*Ya no se avanza simplemente, se cambia la dirección de la serpiente.*/
				// Si soy servidor no me envío todos los datos
				// solamente cambio la dirección de mi serpiente.
				if (serpiente[tams - 1].dir != IZQ) {
					serpiente[tams - 1].dir = DER;
				}
			}
			if (juego_actual == CLIENTE) {
				// Si soy cliente, dependo de un servidor, por lo que solo puedo mandar
				// solicitudes de cambio de dirección y esperar a que el servidor me
				// responda con nueva información para actualizar mi serpiente.
				if (serpiente_cliente[tamsC - 1].dir != IZQ) {
					serpiente_cliente[tamsC - 1].dir = DER;
				}
				strcpy(buffer, "");
				ItoC(
					serpiente_cliente[tamsC - 1].dir,
					tamsC, CLIENTE,
					serpiente_cliente[tamsC - 1].pos.x,
					serpiente_cliente[tamsC - 1].pos.y,
					com.tipo, com.pos.x, com.pos.y,
					TRUE, buffer
				);
				EnviarMensaje(hWnd, buffer, hIP);
			}
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		}
		case VK_LEFT: {
			if (juego_actual == SOLO) {
				if (!MoverSerpiente(serpiente, IZQ, rect, tams)) {
					KillTimer(hWnd, ID_TIMER1);
					MessageBox(hWnd, L"Ya se murio F", L"Fin del juego", MB_OK | MB_ICONINFORMATION);
				}
				if (Comer(serpiente, tams)) {
					serpiente = AjustarSerpiente(serpiente, &tams, com.tipo, rect);
					com.tipo = NADA;
				}
			}
			if (juego_actual == SERVIDOR) {
				if (serpiente[tams - 1].dir != DER) {
					serpiente[tams - 1].dir = IZQ;
				}
			}
			if (juego_actual == CLIENTE) {
				if (serpiente_cliente[tamsC - 1].dir != DER) {
					serpiente_cliente[tamsC - 1].dir = IZQ;
				}
				strcpy(buffer, "");
				ItoC(
					serpiente_cliente[tamsC - 1].dir,
					tamsC, CLIENTE,
					serpiente_cliente[tamsC - 1].pos.x,
					serpiente_cliente[tamsC - 1].pos.y,
					com.tipo, com.pos.x, com.pos.y,
					TRUE, buffer
				);
				EnviarMensaje(hWnd, buffer, hIP);
			}
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		}
		case VK_UP: {
			if (juego_actual == SOLO) {
				if (!MoverSerpiente(serpiente, ARR, rect, tams)) {
					KillTimer(hWnd, ID_TIMER1);
					MessageBox(hWnd, L"Ya se murio F", L"Fin del juego", MB_OK | MB_ICONINFORMATION);
				}
				if (Comer(serpiente, tams)) {
					serpiente = AjustarSerpiente(serpiente, &tams, com.tipo, rect);
					com.tipo = NADA;
				}
			}
			if (juego_actual == SERVIDOR) {

				if (serpiente[tams - 1].dir != ABA) {
					serpiente[tams - 1].dir = ARR;
				}
			}
			if (juego_actual == CLIENTE) {
				if (serpiente_cliente[tamsC - 1].dir != ABA) {
					serpiente_cliente[tamsC - 1].dir = ARR;
				}
				strcpy(buffer, "");
				ItoC(
					serpiente_cliente[tamsC - 1].dir,
					tamsC, CLIENTE,
					serpiente_cliente[tamsC - 1].pos.x,
					serpiente_cliente[tamsC - 1].pos.y,
					com.tipo, com.pos.x, com.pos.y,
					TRUE, buffer
				);
				EnviarMensaje(hWnd, buffer, hIP);
			}
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		}
		case VK_DOWN: {
			if (juego_actual == SOLO) {
				if (!MoverSerpiente(serpiente, ABA, rect, tams)) {
					KillTimer(hWnd, ID_TIMER1);
					MessageBox(hWnd, L"Ya se murio F", L"Fin del juego", MB_OK | MB_ICONINFORMATION);
				}
				if (Comer(serpiente, tams)) {
					serpiente = AjustarSerpiente(serpiente, &tams, com.tipo, rect);
					com.tipo = NADA;
				}
			}
			if (juego_actual == SERVIDOR) {
				if (serpiente[tams - 1].dir != ARR){
					serpiente[tams - 1].dir = ABA;
				}
			}
			if (juego_actual == CLIENTE) {
				if (serpiente_cliente[tamsC - 1].dir != ARR) {
					serpiente_cliente[tamsC - 1].dir = ABA;
				}
				strcpy(buffer, "");
				ItoC(
					serpiente_cliente[tamsC - 1].dir,
					tamsC, CLIENTE,
					serpiente_cliente[tamsC - 1].pos.x,
					serpiente_cliente[tamsC - 1].pos.y,
					com.tipo, com.pos.x, com.pos.y,
					TRUE, buffer
				);
				EnviarMensaje(hWnd, buffer, hIP);
			}
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		}
		}
	}
	case WM_PAINT:
	{
		HPEN plumaTemp;
		HBRUSH brochaTemp;
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Agregar cualquier código de dibujo que use hDC aquí...
		//Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom+TAMSERP);
		GetClientRect(hWnd, &rect);
		MoveToEx(hdc, 0, 0, NULL);
		LineTo(hdc, rect.right, 0);
		LineTo(hdc, rect.right, rect.bottom - 40);
		LineTo(hdc, 0, rect.bottom - 40);
		LineTo(hdc, 0, 0);

		// Ambas serientes se dibujan tanto en el servidor como en el cliente.
		// Pero el cliente depende directamente del servidor para los datos de
		// posición y de dirección.
		plumaTemp = (HPEN)SelectObject(hdc, plumaNegra);
		brochaTemp = (HBRUSH)SelectObject(hdc, brochaRosa);
		DibujarSerpiente(hdc, serpiente);
		SelectObject(hdc, brochaTemp);
		SelectObject(hdc, plumaTemp);

		plumaTemp = (HPEN)SelectObject(hdc, plumaNegra);
		brochaTemp = (HBRUSH)SelectObject(hdc, brochaAzul);
		DibujarSerpiente(hdc, serpiente_cliente);
		SelectObject(hdc, brochaTemp);
		SelectObject(hdc, plumaTemp);

		if (com.tipo == CRECE) {
			RoundRect(hdc,
				com.pos.x * TAMSERP,
				com.pos.y * TAMSERP,
				com.pos.x * TAMSERP + TAMSERP,
				com.pos.y * TAMSERP + TAMSERP,
				7, 7);
		}
		else if (com.tipo == ACHICA) {
			Ellipse(hdc, com.pos.x * TAMSERP,
				com.pos.y * TAMSERP,
				com.pos.x * TAMSERP + TAMSERP,
				com.pos.y * TAMSERP + TAMSERP);
		}
		if (juego_actual == SERVIDOR && cliente_conectado == FALSE) {
			TextOut(hdc, 250, 530, L"Estado: esperando conexion...", sizeof("Estado: esperando conexion..."));
		}
		else if (cliente_conectado == TRUE){
			TextOut(hdc, 250, 530, L"Estado: conectado.", sizeof("Estado: conectado."));
		}
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		free(serpiente);
		free(serpiente_cliente);
		CloseHandle(hHiloServidor);
		CloseHandle(hHiloCliente);
		free(plumaAzul);
		free(plumaNegra);
		free(plumaRosa);
		free(brochaAzul);
		free(brochaNegra);
		free(brochaRosa);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Controlador de mensajes del cuadro Acerca de.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

PEDACITOS * NuevaSerpiente(int tams, int x, int y) {
	PEDACITOS* serpiente = NULL;
	int i;
	if (tams < 2)
		tams = 2;
	serpiente = (PEDACITOS*)malloc(sizeof(PEDACITOS) * tams);
	if (serpiente == NULL) {
		MessageBox(NULL, L"Sin memoria", L"Error", MB_OK | MB_ICONERROR);
		exit(0);
	}
	serpiente[0].tipo = COLA;
	serpiente[0].pos.x = x;
	serpiente[0].pos.y = y;
	serpiente[0].dir = DER;

	for (i = 1; i < tams - 1; i++) {
		serpiente[i].tipo = CUERPO;
		serpiente[i].pos.x = i + x;
		serpiente[i].pos.y = y;
		serpiente[i].dir = DER;
	}

	serpiente[i].tipo = CABEZA;
	serpiente[i].pos.x = tams + x - 1;
	serpiente[i].pos.y = y;
	serpiente[i].dir = DER;

	return serpiente;
}
void DibujarSerpiente(HDC hdc, const PEDACITOS* serpiente) {
	int i = 1;
	//Dibujar cola
	switch (serpiente[0].dir)
	{
	case DER:
		MoveToEx(hdc, serpiente[0].pos.x * TAMSERP + TAMSERP, 
					  serpiente[0].pos.y * TAMSERP, NULL);
		LineTo(hdc, serpiente[0].pos.x * TAMSERP, 
					serpiente[0].pos.y * TAMSERP + TAMSERP / 2);
		LineTo(hdc, serpiente[0].pos.x * TAMSERP + TAMSERP, 
					serpiente[0].pos.y * TAMSERP + TAMSERP);
		LineTo(hdc, serpiente[0].pos.x * TAMSERP + TAMSERP, 
					serpiente[0].pos.y * TAMSERP);
		break;
	case IZQ:
		MoveToEx(hdc, 
			serpiente[0].pos.x * TAMSERP,
			serpiente[0].pos.y * TAMSERP, NULL);
		LineTo(hdc, 
			serpiente[0].pos.x * TAMSERP + TAMSERP,
			serpiente[0].pos.y * TAMSERP + TAMSERP / 2);
		LineTo(hdc, 
			serpiente[0].pos.x * TAMSERP,
			serpiente[0].pos.y * TAMSERP + TAMSERP);
		LineTo(hdc, 
			serpiente[0].pos.x * TAMSERP,
			serpiente[0].pos.y * TAMSERP);
		break;
	case ARR:
		MoveToEx(hdc,
			serpiente[0].pos.x * TAMSERP,
			serpiente[0].pos.y * TAMSERP, NULL);
		LineTo(hdc,
			serpiente[0].pos.x * TAMSERP + TAMSERP / 2,
			serpiente[0].pos.y * TAMSERP + TAMSERP);
		LineTo(hdc,
			serpiente[0].pos.x * TAMSERP + TAMSERP,
			serpiente[0].pos.y * TAMSERP);
		LineTo(hdc,
			serpiente[0].pos.x * TAMSERP,
			serpiente[0].pos.y * TAMSERP);
		break;
	case ABA:
		MoveToEx(hdc,
			serpiente[0].pos.x * TAMSERP,
			serpiente[0].pos.y * TAMSERP + TAMSERP, NULL);
		LineTo(hdc,
			serpiente[0].pos.x * TAMSERP + TAMSERP / 2,
			serpiente[0].pos.y * TAMSERP);
		LineTo(hdc,
			serpiente[0].pos.x * TAMSERP + TAMSERP,
			serpiente[0].pos.y * TAMSERP + TAMSERP);
		LineTo(hdc,
			serpiente[0].pos.x * TAMSERP,
			serpiente[0].pos.y * TAMSERP + TAMSERP);
		break;
	}
	//Dibujar cuerpo
	while (serpiente[i].tipo != CABEZA) {
		RoundRect(hdc, 
			serpiente[i].pos.x * TAMSERP,
			serpiente[i].pos.y * TAMSERP, 
			serpiente[i].pos.x * TAMSERP + TAMSERP, 
			serpiente[i].pos.y * TAMSERP + TAMSERP, 
			5, 5);
		i++;
	}
	//Dibujar cabeza
	RoundRect(hdc, 
		serpiente[i].pos.x * TAMSERP, 
		serpiente[i].pos.y * TAMSERP, 
		serpiente[i].pos.x * TAMSERP + TAMSERP,
		serpiente[i].pos.y * TAMSERP + TAMSERP, 
		5, 5);
	
	HPEN plumaTemp = (HPEN)SelectObject(hdc, plumaNegra);
	HBRUSH brochaTemp = (HBRUSH)SelectObject(hdc, brochaNegra);
	switch (serpiente[i].dir)
	{
	case DER:
		Ellipse(hdc,
			serpiente[i].pos.x * TAMSERP,
			serpiente[i].pos.y * TAMSERP,
			serpiente[i].pos.x * TAMSERP + TAMSERP / 2,
			serpiente[i].pos.y * TAMSERP + TAMSERP / 2);
		Ellipse(hdc,
			serpiente[i].pos.x * TAMSERP,
			serpiente[i].pos.y * TAMSERP + TAMSERP/2,
			serpiente[i].pos.x * TAMSERP + TAMSERP / 2,
			serpiente[i].pos.y * TAMSERP + TAMSERP);
		break;
	case IZQ:
		Ellipse(hdc,
			serpiente[i].pos.x * TAMSERP + TAMSERP / 2,
			serpiente[i].pos.y * TAMSERP,
			serpiente[i].pos.x * TAMSERP + TAMSERP,
			serpiente[i].pos.y * TAMSERP + TAMSERP / 2);
		Ellipse(hdc,
			serpiente[i].pos.x * TAMSERP + TAMSERP / 2,
			serpiente[i].pos.y * TAMSERP + TAMSERP / 2,
			serpiente[i].pos.x * TAMSERP + TAMSERP,
			serpiente[i].pos.y * TAMSERP + TAMSERP);
		break;
	case ARR:
		Ellipse(hdc,
			serpiente[i].pos.x * TAMSERP,
			serpiente[i].pos.y * TAMSERP + TAMSERP / 2,
			serpiente[i].pos.x * TAMSERP + TAMSERP / 2,
			serpiente[i].pos.y * TAMSERP + TAMSERP);
		Ellipse(hdc,
			serpiente[i].pos.x * TAMSERP + TAMSERP / 2,
			serpiente[i].pos.y * TAMSERP + TAMSERP / 2,
			serpiente[i].pos.x * TAMSERP + TAMSERP,
			serpiente[i].pos.y * TAMSERP + TAMSERP);
		break;
	case ABA:
		Ellipse(hdc,
			serpiente[i].pos.x * TAMSERP,
			serpiente[i].pos.y * TAMSERP,
			serpiente[i].pos.x * TAMSERP + TAMSERP / 2,
			serpiente[i].pos.y * TAMSERP + TAMSERP / 2);
		Ellipse(hdc,
			serpiente[i].pos.x * TAMSERP + TAMSERP / 2,
			serpiente[i].pos.y * TAMSERP,
			serpiente[i].pos.x * TAMSERP + TAMSERP,
			serpiente[i].pos.y * TAMSERP + TAMSERP / 2);
		break;
	}
	SelectObject(hdc, brochaTemp);
	SelectObject(hdc, plumaTemp);
}

bool MoverSerpiente(PEDACITOS* serpiente, int dir, RECT rect, int tams) {
	int i = 0;
	while (serpiente[i].tipo != CABEZA) {
		serpiente[i].dir = serpiente[i + 1].dir;
		serpiente[i].pos = serpiente[i + 1].pos;
		i++;
	}

	switch (serpiente[i].dir)
	{
	case DER:
		if (dir != IZQ)
			serpiente[i].dir = dir;
		break;
	case IZQ:
		if (dir != DER)
			serpiente[i].dir = dir;
		break;
	case ARR:
		if (dir != ABA)
			serpiente[i].dir = dir;
		break;
	case ABA:
		if (dir != ARR)
			serpiente[i].dir = dir;
		break;
	}

	switch (serpiente[i].dir)
	{
	case DER:
		serpiente[i].pos.x = serpiente[i].pos.x + 1;
		if (serpiente[i].pos.x >= rect.right / TAMSERP)
			serpiente[i].pos.x = 0;
		break;
	case IZQ:
		serpiente[i].pos.x = serpiente[i].pos.x - 1;
		if (serpiente[i].pos.x < 0)
			serpiente[i].pos.x = rect.right / TAMSERP;
		break;
	case ARR:
		serpiente[i].pos.y = serpiente[i].pos.y - 1;
		if (serpiente[i].pos.y < 0)
			serpiente[i].pos.y = (rect.bottom-60) / TAMSERP;
		break;
	case ABA:
		serpiente[i].pos.y = serpiente[i].pos.y + 1;
		if (serpiente[i].pos.y > (rect.bottom-60) / TAMSERP)
			serpiente[i].pos.y = 0;
		break;
	}
	return !Colisionar(serpiente, tams);
}

bool Colisionar(PEDACITOS* serpiente, int tams) {
	int i = 0;
	while (serpiente[i].tipo != CABEZA) {
		if (serpiente[i].pos.x == serpiente[tams - 1].pos.x && serpiente[i].pos.y == serpiente[tams - 1].pos.y) {
			return true;
		}
		i++;
	}
	return false;
}

bool ColisionarSerpientes(PEDACITOS* serpiente1, PEDACITOS* serpiente2, int tams1, int tams2) {
	int i = 0;
	while (serpiente1[i].tipo != CABEZA) {
		if (serpiente1[i].pos.x == serpiente2[tams2 - 1].pos.x && serpiente1[i].pos.y == serpiente2[tams2 - 1].pos.y) {
			return true;
		}
		i++;
	}
	return false;
}

PEDACITOS* AjustarSerpiente(PEDACITOS* serpiente, int* tams, int comida, RECT rect) {
	int i;
	PEDACITOS cabeza = serpiente[*tams - 1];
	switch (comida)
	{
	case CRECE: {
		(*tams)++;
		
		serpiente = (PEDACITOS *) realloc(serpiente, sizeof(PEDACITOS) * (*tams));
		serpiente[*tams - 2].tipo = CUERPO;
		serpiente[*tams - 1] = cabeza;
		i = *tams - 1;
		switch (serpiente[i].dir)
		{
		case DER:
			serpiente[i].pos.x = serpiente[i].pos.x + 1;
			if (serpiente[i].pos.x >= rect.right / TAMSERP)
				serpiente[i].pos.x = 0;
			break;
		case IZQ:
			serpiente[i].pos.x = serpiente[i].pos.x - 1;
			if (serpiente[i].pos.x < 0)
				serpiente[i].pos.x = rect.right / TAMSERP;
			break;
		case ARR:
			serpiente[i].pos.y = serpiente[i].pos.y - 1;
			if (serpiente[i].pos.y < 0)
				serpiente[i].pos.y = (rect.bottom-60) / TAMSERP;
			break;
		case ABA:
			serpiente[i].pos.y = serpiente[i].pos.y + 1;
			if (serpiente[i].pos.y > (rect.bottom-60) / TAMSERP)
				serpiente[i].pos.y = 0;
			break;
		}
		break;
	}
	case ACHICA: {
		if (*tams > 2) {
			
			i = 0;
			while (serpiente[i].tipo != CABEZA) {
				serpiente[i] = serpiente[i + 1];
				i++;
			}
			(*tams)--;
			serpiente = (PEDACITOS*)realloc(serpiente, sizeof(PEDACITOS) * (*tams));
			serpiente[*tams - 1] = cabeza;
		}
		break;
	}
	}
	return serpiente;
}

bool Comer(PEDACITOS* serpiente, int tams) {
	if (serpiente[tams - 1].pos.x == com.pos.x && serpiente[tams - 1].pos.y == com.pos.y) {
		return true;
	}
	return false;
}

//Código para el Servidor y el Cliente
bool Cliente(HWND hwnd, char* szDirIP, PSTR pstrMensaje) {
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo* result = NULL,
		* ptr = NULL,
		hints;
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;
	char szMsg[256];
	char localhost[] = "localhost";
	char chat[] = "chat";
	TCHAR msgFalla[256];

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		wsprintf(msgFalla, L"WSAStartup failed with error: %d\n", iResult);
		MessageBox(NULL, msgFalla, L"Error en cliente", MB_OK | MB_ICONERROR);
		return true;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo(szDirIP, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		wsprintf(msgFalla, L"getaddrinfo failed with error: %d\n", iResult);
		MessageBox(NULL, msgFalla, L"Error en cliente", MB_OK | MB_ICONERROR);
		WSACleanup();
		return true;
	}

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			wsprintf(msgFalla, L"socket failed with error: %d\n", WSAGetLastError());
			MessageBox(NULL, msgFalla, L"Error en cliente", MB_OK | MB_ICONERROR);
			WSACleanup();
			return true;
		}

		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}
	freeaddrinfo(result);
	if (ConnectSocket == INVALID_SOCKET) {
		MessageBox(NULL, L"Unable to conect to server!\n", L"Error en cliente", MB_OK | MB_ICONERROR);
		WSACleanup();
		return true;
	}
	sprintf_s(szMsg, "%s %s ", szMiIP, szUsuario);
	iResult = send(ConnectSocket, szMsg, sizeof(char) * 256, 0);
	iResult = recv(ConnectSocket, szMsg, sizeof(char) * 256, 0);

	strcpy_s(szMsg, pstrMensaje);

	iResult = send(ConnectSocket, szMsg, sizeof(char) * 256, 0);
	iResult = shutdown(ConnectSocket, SD_SEND);
	iResult = recv(ConnectSocket, szMsg, sizeof(char) * 256, 0);
	ObtenerDatos(szMsg, hwnd);
	closesocket(ConnectSocket);
	WSACleanup();
	return true;
}

DWORD WINAPI Servidor(LPVOID argumento) {
	HWND args = (HWND)argumento;
	WSADATA wsaData;
	int iResult;
	TCHAR msgFalla[256];

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo* result = NULL;
	struct addrinfo hints;

	int iSendResult;
	int recvbuflen = DEFAULT_BUFLEN;
	char szBuffer[256], szIP[16], szNN[32];

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		wsprintf(msgFalla, L"WSAStartup failed with error: %d\n", iResult);
		MessageBox(NULL, msgFalla, L"Error en servidor", MB_OK | MB_ICONERROR);
		return true;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		wsprintf(msgFalla, L"getaddrinfo failed with error: %d\n", iResult);
		MessageBox(NULL, msgFalla, L"Error en servidor", MB_OK | MB_ICONERROR);
		WSACleanup();
		return true;
	}

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		wsprintf(msgFalla, L"socket failed with error: %d\n", WSAGetLastError());
		MessageBox(NULL, msgFalla, L"Error en servidor", MB_OK | MB_ICONERROR);
		freeaddrinfo(result);
		return true;
	}

	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		wsprintf(msgFalla, L"bind failed with error: %d\n", WSAGetLastError());
		MessageBox(NULL, msgFalla, L"Error en servidor", MB_OK | MB_ICONERROR);
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return true;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		wsprintf(msgFalla, L"listen failed with error: %d\n", WSAGetLastError());
		MessageBox(NULL, msgFalla, L"Error en servidor", MB_OK | MB_ICONERROR);
		closesocket(ListenSocket);
		WSACleanup();
		return true;
	}

	while (TRUE)
	{
		ClientSocket = accept(ListenSocket, NULL, NULL);

		if (ClientSocket == INVALID_SOCKET) {
			wsprintf(msgFalla, L"accept failed with error: %d\n", WSAGetLastError());
			MessageBox(NULL, msgFalla, L"Error en servidor", MB_OK | MB_ICONERROR);
			closesocket(ListenSocket);
			WSACleanup();
			return true;
		}

		iResult = recv(ClientSocket, szBuffer, sizeof(char) * 256, 0);
		int aux = sscanf(szBuffer, "%s %s ", szIP, szNN);
		sprintf_s(szBuffer, "Ok");
		iSendResult = send(ClientSocket, szBuffer, sizeof(char) * 256, 0);
		iResult = recv(ClientSocket, szBuffer, sizeof(char) * 256, 0);
		iSendResult = send(ClientSocket, szBuffer, sizeof(char) * 256, 0);
		
		ObtenerDatos(szBuffer, args);
		iResult = shutdown(ClientSocket, SD_SEND);
	}

	closesocket(ClientSocket);
	WSACleanup();
	return true;
}

void ObtenerDatos(char* mensaje, HWND hwnd) {
	/*
		* ObtenerDatos es la función que permite la recepción de la información
		  y la procesa según ciertos parámetrso.

		* La lógica del programa es la siguiente:
			  A pesar de existir dos hilos servidores (uno del lado del cliente y uno del lado del servidor) 
			  en realidad la mayor parte de la información (sino es que toda), es procesada por el programa
			  servidor.

			  El cliente solo hace la función de "espejo", pues replica todas las acciones que hayan en el 
			  servidor.
			  Tanto la serpiente servidor como la serpiente cliente, ambas están siendo tratadas por el servidor
			  y solamente se ve reflejado todo del lado del cliente.

			  Por tanto cuando si se quiere mover a la serpiente servidor, el mismo servidor procesa la acción.
			  El cliente por otro lado, envía la dirección de la serpiente cliente al servidor
			  para que este actualice ese valor en la serpiente cliente que vive en el servidor*
			  y después devuelva el nuevo conjunto de valores para la serpiente cliente del lado del
			  cliente*. (El cliente solo replica los valores del servidor.)

		*Todo esto puede apreciarse en el código siguiente.
		
		* El protocolo es el siguiente:
			* La información que viaja entre el servidor y el cliente es:
			actual -> indica quién es el que está enviando el mensaje al servidor y dependiendo de quién
					  se procesarán acciones diferentes.
			dir -> es la dirección que debe tener mi serpiente servidor del lado del servidor o
				   mi serpiente cliente del lado del cliente.
			tam -> es el tamaño de la serpiente servidor o cliente.
			x -> es la posición en x de la serpiente servidor o cliente.
			y -> es la posición en y de la serpiente servidor o cliente.
			comida_tipo -> es tipo de comida tiene actualmente el tablero del lado del servidor.
						   el cliente solo replica esta información.
			comida_x -> es la posicó en x de la comida de lado del servidor.
						el cliente solo replica esta información.
			comida_y -> es la posicó en y de la comida de lado del servidor.
						el cliente solo replica esta información.
			conectado -> es una bandera que permite evaluar si existe algún cliente conectado al servidor.
						 el servidor no inicia ningún juego si no hay cliente con quien jugar.
	*/
	int actual = 0, dir = 0, tam = 0, x = 0, y = 0, comida_tipo = 0, comida_x = 0, comida_y = 0, conectado;
	sscanf(mensaje, "%d %d %d %d %d %d %d %d %d", &dir, &tam, &actual, &x, &y, &comida_tipo, &comida_x, &comida_y, &conectado);
	cliente_conectado = conectado;
	if (actual == SERVIDOR)	{
		//El servidor procesa todos los mensajes
		//En este caso el servidor esta mandando la información al cliente

		/*
			Servidor -> Cliente
			Esta información es enviada por el servidor cada 500 milisegundos a través de
			su timer activo.
			El cliente recupera la información en este bloque de código y actualiza su información.
			Empenzado por los datos de la serpiente servidor y los datos de la comida del servidor.
			Después verfica que exista conexión entre ambos (Cliente-Servidor) y de ser así
			procede a ejecutar las acciones que replican al servidor.
			Mueve ambas serpientes, verifica que no hayan colisionado y también verifica si comió alguna
			de ellas.
		*/
		tams = tam;
		serpiente[tams - 1].dir = dir;
		serpiente[tams - 1].pos.x = x;
		serpiente[tams - 1].pos.y = y;
		com.tipo = comida_tipo;
		com.pos.x = comida_x;
		com.pos.y = comida_y;
		if (cliente_conectado) {
			if (!MoverSerpiente(serpiente, serpiente[tams - 1].dir, rect, tams)) {
				KillTimer(hwnd, ID_TIMER1);
				MessageBox(hwnd, L"El servidor ha muerto, choco con su cuerpo.", L"Fin del juego", MB_OK | MB_ICONINFORMATION);
			}
			if (!MoverSerpiente(serpiente_cliente, serpiente_cliente[tamsC - 1].dir, rect, tamsC)) {
				KillTimer(hwnd, ID_TIMER1);
				MessageBox(hwnd, L"El cliente ha muerto, choco con su cuerpo.", L"Fin del juego", MB_OK | MB_ICONINFORMATION);
			}
			if (Comer(serpiente, tams)) {
				serpiente = AjustarSerpiente(serpiente, &tams, com.tipo, rect);
				com.tipo = NADA;
			}
			if (Comer(serpiente_cliente, tamsC)) {
				serpiente_cliente = AjustarSerpiente(serpiente_cliente, &tamsC, com.tipo, rect);
				com.tipo = NADA;
			}
			if (ColisionarSerpientes(serpiente, serpiente_cliente, tams, tamsC)) {
				KillTimer(hwnd, ID_TIMER1);
				MessageBox(hwnd, L"El servidor ha muerto, choco con el cliente", L"Fin del juego", MB_OK | MB_ICONINFORMATION);
			}
			if (ColisionarSerpientes(serpiente_cliente, serpiente, tamsC, tams)) {
				KillTimer(hwnd, ID_TIMER1);
				MessageBox(hwnd, L"Cliente ha muerto, choco con el servidor", L"Fin del juego", MB_OK | MB_ICONINFORMATION);
			}
		}
	}
	if (actual == CLIENTE){
		//El cliente no procesa, solo cambia de dirección a la serpiente cliente para que el servidor
		//pueda actulizar su información y devolver la nueva información al cliente.
		/*
			Cliente - Servidor.
			El cliente solo actuliza la dirección, el tamaño y la posición
			para que servidor actualice su información.
		*/
		tamsC = tam;
		serpiente_cliente[tamsC - 1].dir = dir;
		serpiente_cliente[tamsC - 1].pos.x = x;
		serpiente_cliente[tamsC - 1].pos.y = y;
	}

	InvalidateRect(hwnd, NULL, TRUE);
}

void EnviarMensaje(HWND hwnd, char * mensaje, HWND hIP)
{
	TCHAR tchDirIP[16];
	char szDirIP[16];
	int tam = 0;
	size_t i;

	GetWindowText(hIP, tchDirIP, 16);
	tam = GetWindowTextLength(hIP);
	wcstombs(szDirIP, tchDirIP, tam);
	szDirIP[tam] = '\0';

	long iLength;
	PSTR pstrBuffer;
	TCHAR* ptchBuffer;

	iLength = (long)strlen(mensaje);
	if (NULL == (pstrBuffer = (PSTR)malloc(sizeof(char) * (iLength + 2))) ||
		NULL == (ptchBuffer = (TCHAR*)malloc(sizeof(TCHAR) * (iLength + 2))))
		MessageBox(NULL, L"Error al reservar memoria", L"Error", MB_OK || MB_ICONERROR);
	else
	{
		swprintf(ptchBuffer, 64, L"%hs", mensaje);
		wcstombs_s(&i, pstrBuffer, (iLength + 1), ptchBuffer, (iLength + 1));
		pstrBuffer[iLength + 1] = '\0';

		Cliente(hwnd, szDirIP, pstrBuffer);

		free(pstrBuffer);
		free(ptchBuffer);
	}
}

void ItoC(
	int dir, int tam, int actual, int x, int y, 
	int comida_tipo, int comida_x, int comida_y,
	int conectado, char* buffer
) {
	/*Procedimiento que permite empaquetar los datos en un buffer para su envío*/
	/*
		* int dir -> indica la dirección de la serpiente.
		* int tam -> indica el tamaño de la serpiente.
		* int actual -> indica quien es el que está enviando un mensaje, puede tomar los valores de
			* CLIENTE o SERVIDOR
		* int x -> indica la posición de la serpiente en x.
		* int y -> indica la posición de la serpiente en y.
		* int comida_tipo -> indica el tipo de comida a dibujar.
		* int comida_x -> indica la posición en x de la comida.
		* int comida_y -> indica la posición en y de la comida.
		* int conectado -> indica si hay un cliente conectado al servidor, puede tomar los valores de
			* TRUE o FALSE
		* char* buffer -> variable donde se almacenan todos los datos anteriores para su envío 
		  al servidor o al cliente.
	*/
	sprintf(buffer, "%d %d %d %d %d %d %d %d %d", dir, tam, actual, x, y, comida_tipo, comida_x, comida_y, conectado);
}