/*****************************************************
**													**
**	KNIHOVNA PRO GENEROVANI HLASOVEHO VYSTUPU		**
**													**
**  Autor: Michal Kvasnak a Petr Petyovsky          **
**                                                  **
*****************************************************/

/* Logika programu byla vytvorena spolecnosti VoiceSoft
** pro Sinclair ZX Spectrum. Tato knihovna je vytvorena
** na zaklade tohoto programu.
*/

/* NAVOD K POUZITI:
**	1)	Pripojit soubory 'hlas.h' a 'hlas.c' k projektu
**	2)	Pomoci '#include "hlas.h"' pripojit hlavickovy soubor
**
**	Pozn.:	Pokud je zamerem pouziti knihovny zasilat hlasovy vystup do PC,
**			pak je potreba v ramci Vaseho projektu inicializovat seriovou
**			komunikaci USART. Doporucena hodnota baud rate je 57600.
**
**	UZIVATELSKA MAKRA:
**		· BUFFER_SIZE - definuje maximalni delku zpracovavaneho retezce
*/

#ifndef __HLAS_H__
#define __HLAS_H__

#define _CRT_SECURE_NO_WARNINGS

/****************************************
*	KNIHOVNY POTREBNE PRO FUNKCNOST		*
****************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

/****************************************
*	DEFINICE EMBEDDED DATOVYCH TYPU		*
****************************************/
#ifndef  _STDINT_H
	#define uint8_t unsigned char
	#define uint16_t unsigned short	
	#define uint32_t unsigned int
#endif

/****************************************
*			DEFINICE MAKER				*
****************************************/
#define F_CPU 16000000

#ifndef BUFFER_SIZE
	#define BUFFER_SIZE 516
#endif
#ifndef BAUD_RATE
	#define BAUD_RATE 57600
#endif

#define UBRR F_CPU/16/BAUD_RATE-1

/****************************************
*			GLOBALNI PROMENNE			*
****************************************/

/* Indikace prijmuti noveho retezce*/
bool newString;

/* Obdrzeny retezec*/
char receivedString[BUFFER_SIZE];

/* Ukazatel na prvky obdrzeneho retezce*/
size_t receivedString_Ptr;

/* Indikace dokonceni pauzy */
bool timerDone;

/****************************************
*		DEFINICE TABULEK PRO			*
*		GENEROVANI HLASOVEHO VYSTUPU	*
****************************************/

/* Hodnoty ukazujici na pozici parametru v 'parametricTable[]' dle daneho pismena
**		- Hodnota je ziskavana az po prevodu retezce pomoci funkce 'convertToSpeech[]'
**		- Hodnota z tabulky je ziskana dle vzorce:
**			· Pro pismena bez hacku a carek:	( ASCII_hodnota - 65 )
**			· Pro pismena s hackem/carkou:		( ASCII_hodnota - 71 )
*/
static const uint8_t parameterPositions[] = {
	0x00,	// A
	0x02,	// B
	0x06,	// C
	0x0A,	// D
	0x0E,	// E
	0x10,	// F
	0x12,	// G
	0x16,	// H
	0x1A,	// I
	0x1C,	// J
	0x22,	// K
	0x26,	// L	
	0x2A,	// M
	0x2E,	// N
	0x32,	// O
	0x34,	// P
	0x38,	// Q
	0x42,	// R
	0x48,	// S
	0x4A,	// T
	0x4E,	// U
	0x50,	// V
	0x50,	// W
	0x56,	// X
	0x1A,	// Y
	0x5C,	// Z
	
	0x64,	// A'
	0x66,	// B'
	0x70,	// C'
	0x74,	// D'
	0x7A,	// E'
	0x7C,	// F'
	0xC2,	// G'
	0x84,	// CH
	0x86,	// I'
	0xC2,	// J'
	0xC2,	// K'
	0xC2,	// L'
	0x88,	// M'
	0x8C,	// N'
	0x92,	// O'
	0x94,	// P'
	0xC2,	// Q'
	0x9E,	// R'
	0xA6,	// S'
	0xA8,	// T'
	0xAE,	// U'
	0xB0,	// V'
	0xC2,	// W'
	0xC2,	// X'
	0x86,	// Y'
	0xBC,	// Z'
};

/* Hodnoty jednotlivych parametru. Vzdy jsou cteny 2 Byty pricemz:
**	- Prvni byte:
**		· Prvni 3 bity => Pozice delky segmentu ve 'segmentLength[]'
**		· Ctvrty bit => Nic
**		· Posledni 4 bity => Pocet opakovani daneho segmentu
**		
**	- Druhy byte:
**		· Prvni bit => Indikace dalsich parametru (log. '1' - nema dalsi parametry)
**		· Posledni bit => Pozice daneho segmentu ve 'phonemeTable[]'
**			(Pozn.: Spravnou hodnotu ukazuje az po logickem posuvu vlevo o 1 bit)
*/
static const uint8_t parametricTable[] = {
	0x36,0x81,											// A
	0x34,0x19,0x31,0xAB,								// B
	0x18,0x19,0x91,0xC3,								// C
	0x34,0x19,0x31,0xE0,								// D
	0x36,0x84,											// E
	0x92,0xE3,											// F
	0x35,0x19,0x51,0x9C,								// G
	0x31,0x31,0x34,0x96,								// H
	0x36,0x87,											// I + Y
	0x33,0x3A,0x32,0x3D,0x32,0xC0,						// J
	0x18,0x19,0x51,0x9C,								// K
	0x33,0x22,0x31,0xB1,								// L
	0x31,0x31,0x36,0xA5,								// M
	0x31,0x31,0x36,0xA8,								// N
	0x36,0x8A,											// O
	0x18,0x19,0x31,0xAB,								// P
	0x18,0x19,0x51,0x1C,0x34,0x31,0x32,0x34,0x32,0xB7,	// Q
	0x22,0x10,0x13,0x19,0x21,0xAE,						// R
	0x92,0xC3,											// S
	0x18,0x19,0x31,0xE0,								// T
	0x36,0x8D,											// U
	0x34,0x31,0x32,0x34,0x32,0xB7,						// V
	0x18,0x19,0x71,0x1C,0x92,0xC3,						// X
	0x32,0x31,0x32,0x43,0x32,0x44,0x32,0xC5,			// Z
	
	0x3F,0x81,											// A'
	0x34,0x19,0x31,0x2B,0x33,0x3A,0x32,0x3D,0x32,0xC0,	// B'	(Pozn. BE^)
	0x18,0x19,0x91,0xD3,								// C'
	0x33,0x19,0x71,0x6D,0x32,0x93,						// D'
	0x3E,0x84,											// E'
	0x92,0x63,0x33,0x3A,0x32,0x3D,0x32,0xC0,			// F'	(Pozn. FE^)
	0x92,0xF3,											// CH' + H'
	0x3E,0x87,											// I'
	0x31,0x31,0x36,0x25,0x31,0x31,0x35,0x25,0x32,0x93,	// M' + N'	(Pozn. ME^)
	0x3E,0x8A,											// O'
	0x18,0x19,0x31,0x2B,0x33,0x3A,0x32,0x3D,0x32,0xC0,	// P'	(Pozn. PE^)
	0x13,0x19,0x32,0x60,0x13,0x19,0x71,0xDD,			// R'
	0x92,0xD3,											// S'
	0x18,0x19,0x71,0x6D,0x32,0x93,						// T'
	0x3E,0x8D,											// U'
	0x34,0x31,0x32,0x34,0x32,0x37,0x33,0x3A,0x32,0x3D,0x32,0xC0, // V'	(Pozn. VE^)
	0x32,0x53,0x32,0x54,0x32,0xD5,						// Z'
};

/* Tabulka s formanty
**  - Formant je casto slozen z vice segmentu
**  - Pozice a pocet opakovani segmentu je vyzvedavana v 'parametricTable[]'
**  - Delka segmentu je vyzvedavana v 'segmentLength[]'
**  - Pro snazsi orientaci je zde zaveden popis ve formatu:
**
**          [Odpovidajici pismeno]( [Poradi segmentu], [z/k/o] )
**
**      Pozn.:  z (zacatek) -> Segment zde zacina (Odkazuje se na prvni byte v radku)
**              k (konec) -> Segment zde konci (Odkazuje na posledni byte v radku)
**              o (oboji) -> Segment zacina prvnim bytem a konci poslednim bytem v radku
**
**      Pozn.: Pismena jsou reprezentovana jako:
**              Velke pismena -> Pismena bez hacku, carek a neovlivnena pismeny 'I'/'E^'
**              Male pismena -> Pismena s hackem/carkou nebo ovlivnena pismeny 'I'/'E^' (Pozn. h = 'CH')
**                  (napr. I' = i, C^ = c)
*/
static const uint8_t phonemeTable[] = {
	0x1A,0x99,	//Nevyuzite hodnoty

	/* VOKALY */
	0xE1,0xC3,0xE1,0xC7,0x8F,0x0F,  //  A(1, o) | a(1, o)
	0xF8,0x03,0x0F,0x07,0xC1,0xE3,  //  E(1, o) | e(1, o)
	0xFF,0x40,0x17,0xFF,0x00,0x03,	//  I(1, o) | i(1, o)
	0xF8,0x7C,0xC1,0xF1,0xF8,0x03,  //  O(1, o) | o(1, o)
	0xFE,0x00,0x7F,0xFC,0x00,0x03,	//  U(1, o) | u(1, o)

	/* FRIKATIVY, PLOZIVY, NAZALY A AFRIKATY*/
	0xF8,0x0F,0x09,0xF1,0xFE,0x03,	//  R(1, o)
	0xEF,0x40,0x17,0xFF,0x00,0x03,	//  d(3, o) | m(5, o) | n(5, o) | t(2, o)
	0xE1,0x5C,0x35,0xC5,0xAA,0x35,	//  H(2, o)
	0x00,0x00,0x00,0x00,0x00,0x00,	//  B(1, o) | b(1, o) | D(1, o) | d(1, o) | G(1, o)
	0x3E,0x8E,0x38,0x73,0xCF,0xF8,  //  G(2, z) | K(1, z) | Q(1, z) | X(1, z)
	0x78,0xC3,0xDF,0x1C,0xF1,0xC7,  //  G(2, k) | K(1, k) | Q(1, k) | X(1, k)
	0xFE,0x03,0xC0,0xFF,0x00,0x00,	//  L(1, o)
	0xFF,0xF8,0x00,0x7F,0xF8,0x03,	//  M(2, o) | m(2, o) | m(4, o) | n(2, o) | n(4, o)
	0xFF,0xF0,0x01,0xFF,0xE0,0x03,	//  N(2, o)
	0xAA,0xCA,0x5A,0xD5,0x21,0x3D,	//  B(2, o) | b(2, o) | P(1, o) | p(1, o)
	0xFE,0x1F,0xF8,0x00,0x00,0x1F,	//  R(2, o)
	0xFF,0xFC,0x20,0x20,0x00,0x00,  /*  H(1, o) | L(2, o) | M(1, o) | m(1, o) | m(3, o) | N(1, o)
                                    n(1, o) | n(3, o) | Q(2, o) | V(1, o) | v(1, o) | Z(1, o) */
	0x03,0xFF,0xFF,0x08,0x79,0x00,	//  Q(3, o) | V(2, o) | v(2, o)
	0x02,0xFF,0xE1,0xC7,0x1F,0xE0,	//  Q(4, o) | V(3, o) | v(3, o)
	0x03,0xFF,0xD0,0x01,0xFF,0xF0,	//  b(2, o) | f(2, o) | J(1, o) | p(2, o) | v(4, o)
	0x03,0x7F,0x01,0xFA,0x5F,0xC0,  //  b(3, o) | f(2, o) | J(2, o) | p(3, o) | v(5, o)
	0x07,0xF8,0x0F,0xC0,0xFF,0x00,	//  b(4, o) | f(3, o) | J(3, o) | p(4, o) | v(6, o)
	0x42,0xAA,	                    //  C(1, z) | S(1, z) | X(2, z) | Z(2, z)
	0xA5,0x55,	                    //  Z(3, z)
	0x5A,0xAA,	                    //  Z(2, k) | Z(4, z)
	0xAA,0x5A,                      //  Z(3, k)
	0xA5,0x5A,                      //  Z(4, k)
	0xAA,0x55,0x55,0xAA,0xAA,0xA5,
	0x55,0xAA,0x5A,0xAA,0xA5,0x55,
	0xAA,0xAA,0xA5,0x55,0xAA,0xAA,
	0x55,0xA5,0xA5,0xAA,            //  C(1, k) | S(1, k) | X(2, k)
	0xA5,0xB7,	                    //  c(1, z) | s(1, z) | z(1, z)
	0x66,0x6C,	                    //  z(2, z)
	0xD8,0xF9,                      //  z(1, k) |  z(3, z)
	0xB3,0x6C,                      //  z(2, k)
	0xAD,0x37,                      //  z(3, k)
	0x37,0x66,0xFC,0x9B,0x87,0xF6,
	0xC0,0xD3,0xB6,0x60,          
	0xF7,0xF7,0x3E,0x4D,0xFB,0xFE,  //  r(3, z)
	0x5D,0xB7,0xDE,0x46,0xF6,0x96,  /*  D(2, z) | r(1, z) | T(1, z) | c(1, k) | D(2, k)
	                                r(1, k) | r(3, k) | s(1, k) | T(1, k) */
	0xB4,0x4F,0xAA,0xA9,0x55,0xAA,  //  F(1, z) | f(1, z)
	0xAA,0xA5,0x69,0x59,0x9A,0x6A,
	0x95,0x55,0x95,0x55,0x6A,0xA5,
	0x55,0xA9,0x4D,0x66,0x6A,0x92,
	0xEC,0xA5,0x55,0xD2,0x96,0x55,  //  d(2, z) | t(1, z)
	0xA2,0xBA,                     	//  d(2, k) | F(1, k) | f(1, k) | t(1, k)
	/* PISMENO 'CH' */
	0xCD,0x00,0x66,0x99,0xCC,0x67,	//  h(1, z)
	0x31,0x8E,0x66,0x39,0xA6,0x6B,
	0x19,0x66,0x59,0xC6,0x71,0x09,
	0x67,0x19,0xCB,0x01,0x71,0xCC,
	0x73,0x19,0x99,0xCC,0xC6,0x67,
	0x19,0x9A,                  	//  h(1, k)

	0xC6,0x59	// Nevyuzite data
	};

/* Tabulka s delky jednotlivych segmentu
**	- Delka je udavana v bitech
**	- Pozice delky je vyzvedavana v 'paramtericTable[]'
**	- Formant je casto slozen ze segmentu o rozdilne delce
**	- Pro jednodussi orientaci je zaveden popis ve formatu:
**
**		[Odpovidajici pismeno]( [Poradi segmentu v danem formantu/c] )
**
**			Pozn. c (cele) -> Vsechny segmenty daneho pismena jsou o konstantni delce
*/
static const uint8_t segmentLength[] = {
	0x00,	//	C(1); K(1); P(1); Q(1); R(2); T(1); X(1); Z(1); c(1); p(1); r(1, 3); t(1)
	0x2E,	/*	A(c); B(c); D(c); E(c); G(1); H(c); I(c); J(c); L(c); M(c); N(c); O(c); P(2); 
				Q(3, 4, 5); T(2); U(c); V(c); Z(c); a(c); b(c); d(1, 3); e(c); f(2, 3, 4); i(c);
				m(c); n(c); o(c); p(2, 3, 4, 5); r(2); t(3); v(c); z(c);	*/	
	0x5A,	//	G(2); K(2); Q(2);
	0x5E,	//	X(2); d(2); r(4); t(2);
	0xFE	//	C(2); F(c); S(c); X(3); c(2); f(1); h(c); s(c); 
	};

/****************************************
*			DEFINICE FUNKCI				*
****************************************/

/* Inicializace seriove komunikace USART
**	· ubrr - vypoctena rychlost prenosu (viz makro UBRR)
*/
void USART_Init( const uint16_t ubrr );

uint8_t USART_Receive( void );

/* Obsluha odesilani dat
**	· dataToSend - byte, ktery ma byt odeslan
*/

/* Funkce pro zjisteni, zda byl prijmut cely novy retezec:
**	- Pokud byl prijat -> vraci `true`
**	- Pokud nebyl prijat -> vraci `false`
*/
bool WholeStringReceived( void );

/* Funkce pro predani obdrzeneho retezce
*/
static inline char* GetReceivedString( void )
	{
	return receivedString;
	}

/* 	Inicializace Citace/Casovace1 pro funkci knihovny. Nastavuje jej do:
**		· Rezimu CTC
**		· Povoluje preruseni pri "Compare match"
**	(pozn. Preddelicka signalu a samotne spusteni je az ve funkci 'Delay()')	
*/
void Timer1_Init( void );

/* Funkce vraci hodnotu obdrzeneho Bytu z UDR0
*/
uint8_t USART_Receive( void );	

/*	Funkce prevadejici ziskany retezec do tvaru vhodneho k zisku parametru. Prevadi text na:
**		· `stringToConvert` - Vstupni retezec, ktery ma byt preveden
**		· `convertedString` - Vystupni jiz prevedeny retezec
*/
bool ConvertToSpeech( const char* stringToConvert, char* convertedString );

/*	Funkce zasila data 'value' do PC za pomoci seriove komunikace. 
**		· `value` - Hodnoty Bytu, ktery ma byt zaslan
*/
void USART_Transmit(const uint8_t value);
	
/*	Funkce vykonavajici vsechny pauzy behem generovani hlasoveho vystupu.
**		· 'time' -> nastavuje delku pauzy
**		· 'functionMode' -> Urcuje zda funkce ma posilat data zpet do PC (tuto hodnotu prebira podle funkce 'Speak')
**		· 'bitValue' -> Urcuje hodnotu dat, ktere se maji zasilat 
*/
void Pause(const uint16_t time, const char functionMode, const uint8_t byteValue);

/*	Funkce generujici hlasovy vystup pomoci piezoelektrickeho menice.
**		· 'originalString' -> Retezec, ktery ma byt precten
**		· 'functionMode' -> Rozhoduje, zda se retezec ma precist nebo odeslat jakozto binarni hodnoty (umoznuje tvorbu '.wav' souboru)
**			= 'r' -> Generace reci pomoci piezoelektrickoho menice
**			= 's' -> Generovana rec bude odesilana v podobe Bytu pomoci seriove komunikace USART
*/
void Speak( const char* originalString, const char functionMode );

#endif