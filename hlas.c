/*******************************************************************
**                                                                **
**	KNIHOVNA PRO GENEROVANI HLASOVEHO VYSTUPU					  **
**  Autor: Michal Kvasnak a Petr Petyovsky                        **
**                                                                **
*******************************************************************/

#include "hlas.h"

// Obsluha preruseni pri "Compare match" ( citac/casovac1 dosahne hodnoty nastavene v OCR1A viz 'Pause()')
ISR(TIMER1_COMPA_vect) 
	{
 	TCCR1B &= ~(1 << CS12);	// Zastaveni citani
 	timerDone = true;	// Indikace dokonceni vykonavane pauzy
	}

// Obsluha preruseni pri prijimani dat pomoci seriove komunikace USART
ISR(USART0_RX_vect)
	{
	uint8_t receivedChar = UDR0;
	
	if( receivedChar == '\0' || receivedString_Ptr >= BUFFER_SIZE-1)
		{
		receivedString[receivedString_Ptr] = '\0';
		receivedString_Ptr = 0;
		newString = true;
		}
	else
		receivedString[receivedString_Ptr++] = receivedChar;
	}
	
void USART_Init(const uint16_t ubrr)
	{
	// Nastaveni rychlosti prenosu
	UBRR0H = (uint8_t)ubrr >> 8;
	UBRR0L = (uint8_t)ubrr;
	
	// Povoleni prijimani/odesilani dat a uplneho preruseni pri prijimani
	UCSR0B |= ( 1 << RXEN0 ) | ( 1 << TXEN0 ) | ( 1 << RXCIE0 );
	// Nastaveni formatu: 8 data bitu, 1 stop bit
	UCSR0C |= (1 << UCSZ01 ) | ( 1 << UCSZ00 );
		
	USART_Receive();
	}
	
uint8_t USART_Receive( void )
	{
	while( !(UCSR0A & (1 << RXC0 )) );
	
	return UDR0;
	}	
	
void USART_Transmit(const uint8_t dataToSend)
	{
	// Cekani az bude moct byt psano do UDR0 pomoci UDRE0 flag
	while( !(UCSR0A & (1 << UDRE0)) );
	
	// Zapis odesilaneho Bytu do UDR0 
	UDR0 = dataToSend;
	}
	
bool WholeStringReceived( void )
	{
	if( newString )
		{
		newString = false;
		return true;
		}
	return false;
	}

void Timer1_Init( void )
	{
	// 16 bitovy timer/counter1:
	TCCR1A = 0;
	TCCR1B |= (1 << WGM12);		// CTC mod
	TCCR1C = 0;
	TIMSK1 |= (1 << OCIE1A);	// Povolit preruseni pri "Compare match"
	}

bool ConvertToSpeech( const char* stringToConvert, char* convertedString )
	{
	if( !stringToConvert )
		return false;

	size_t convertedString_Pos = 0;
	for( size_t stringToConvert_Pos = 0; stringToConvert[stringToConvert_Pos] != '\0' && convertedString_Pos < BUFFER_SIZE; stringToConvert_Pos++, convertedString_Pos++ )
		{
		// Prevod na velke pismena 
		if( stringToConvert[stringToConvert_Pos] >= 'a' && stringToConvert[stringToConvert_Pos] <= 'z' )
			convertedString[convertedString_Pos] = stringToConvert[stringToConvert_Pos] - 32;
		else if( (stringToConvert[stringToConvert_Pos] >= 'A' && stringToConvert[stringToConvert_Pos] <= 'Z')
				|| stringToConvert[stringToConvert_Pos] == ' ' || stringToConvert[stringToConvert_Pos] == ','
				|| stringToConvert[stringToConvert_Pos] == '.' || stringToConvert[stringToConvert_Pos] == '!'
				|| stringToConvert[stringToConvert_Pos] == '?' )
			convertedString[convertedString_Pos] = stringToConvert[stringToConvert_Pos];
		else
			return false;

		// Pismena s carkou
		if( stringToConvert[stringToConvert_Pos + 1] == '\'' )
			{
			switch( convertedString[convertedString_Pos] )
				{
				case 'A':
				case 'E':
				case 'I':
				case 'Y':
				case 'O':
				case 'U':
					convertedString[convertedString_Pos] += 32;
					stringToConvert_Pos++;
					break;
				}
			}

		// Pismena s hackem
		else if( stringToConvert[stringToConvert_Pos + 1] == '^' )
			{
			switch( convertedString[convertedString_Pos] )
				{
				case 'C':
				case 'D':
				case 'N':
				case 'R':
				case 'S':
				case 'T':
				case 'Z':
					convertedString[convertedString_Pos] += 32;
					stringToConvert_Pos++;
					break;
				case 'E': // Specialni pripad -> bude prevedeno zpatky
					convertedString[convertedString_Pos - 1] += 32;
					stringToConvert_Pos++;
					break;
				}
			}

		// Pismena ovlivnena pismena 'I'
		else if( stringToConvert[stringToConvert_Pos + 1] == 'I' || stringToConvert[stringToConvert_Pos + 1] == 'i' )
			{
			switch( convertedString[convertedString_Pos] )
				{
				case 'D':
				case 'N':
				case 'T':
					convertedString[convertedString_Pos] += 32;
					break;
				}
			}

		// Pismeno 'CH'
		else if( convertedString[convertedString_Pos] == 'C' && (stringToConvert[stringToConvert_Pos + 1] == 'H' || stringToConvert[stringToConvert_Pos + 1] == 'h' ) )
			{
			convertedString[convertedString_Pos] = 'h';
			stringToConvert_Pos++;
			}
		}
	convertedString[convertedString_Pos] = '\0';
	return true;
}

void Pause(const uint16_t time, const char functionMode, const uint8_t byteValue)
	{
	timerDone = false; // Resetovani indikace dokonceni pauzy
	
	OCR1A = time;	// Nastaveni delky pauzy
	TCNT1 ^= TCNT1;		// Vynulovani citace (pozn. Casovac se nuluje i pri obsluze 'ISR(TIMER1_COMPA_vect)')
	
	uint16_t timer_mem = 0;
	
	TCCR1B |= (1 << CS12); // clk_io/256 a zaroven spusteni citace (Pozn. zastaveni probiha v obsluze preruseni)
		
	while(!timerDone)
		{
		if(functionMode == 's')
			if(timer_mem != TCNT1)
				{
				timer_mem = TCNT1;
				USART_Transmit(byteValue);
				}
		
		// Zde muzete pridat kod, ktery se ma vykonavat behem cekani na Citac/Casovac1
		}
	}

void Speak( const char* originalString, const char functionMode )
	{
	if(!originalString)
		return;
		
	size_t originalString_len = strlen(originalString);
	if (originalString_len >= BUFFER_SIZE)
		return;
	
	char stringForSpeech[BUFFER_SIZE] = {0};
	if(!ConvertToSpeech(originalString, stringForSpeech))
		return;
	
	// Promenne odkazujici na tabulku 'parameterPositions[]'	
	uint8_t positionOfParameter = 0;	// Pozice prvniho parametru v 'parametricTable[]'
	
	// Promenne odkazujici na tabulku 'parametricTable[]'
	uint8_t parameterValue = 0;	/* Hodnoty jednotlivych paramametru z 'parametricTable[]':
									� Nejvyssi bit -> Zda existuje navazujici segment
									� Nejnizsi 4 bity -> Pocet opakovani segmentu
								*/
	uint8_t nextParameter_pos = 0; 		// Pomocny inkrement pro zisk navazujicich parametru (pokud je dane pismeno ma)

	// Promenne odkazujici na tabulku 'phonemeTable[]'
	uint8_t segment_pos = 0;  			// Pozice aktualniho segmentu daneho formantu
	
	// Promenne odkazujici na tabulku 'segmentLength[]'
	uint8_t segmentLength_val = 0;		// Celkova delka formantu
	uint8_t segmentLength_pos = 0;		// Pozice hodnoty delky formantu

	// Inicializace Citace/Casovace1
	Timer1_Init();
	
	// Pruchod celym retezcem, ktery ma byt precten/odeslan
	for(size_t speechString_ptr = 0; stringForSpeech[speechString_ptr] != '\0'; speechString_ptr++)
		{
		switch( stringForSpeech[speechString_ptr] )
			{
			case ' ':
				Pause( 7581, functionMode, 0x00); // Pauza pro mezeru
				break;
			case ',':
				Pause( 14550, functionMode, 0x00); // Pauza pro carku
				break;
			case '.':
				Pause( 28613, functionMode, 0x00); // Pauza pro tecku
				break;
			default:
				{
				// Zisk pozice parametru v 'parametricTable[]'
				if( stringForSpeech[speechString_ptr] >= 'A' && stringForSpeech[speechString_ptr] <= 'Z')
					positionOfParameter = parameterPositions[stringForSpeech[speechString_ptr] - 65];
				else
					positionOfParameter = parameterPositions[stringForSpeech[speechString_ptr] - 71];
				
				// Zisk poctu opakovani daneho segmentu			
				parameterValue = parametricTable[positionOfParameter + nextParameter_pos] & 0x0F;
				
				/* Zisk indikace o dalsich segmentech a zaroven slouceni s poctem opakovani daneho segmentu ve formatu:
					- Prvni bit:	� Log. '1' => Formant nema dalsi segment
									� Log. '0' => Formant ma dalsi segment
					- Posledni 4 bity => pocet opakovani daneho segmentu	*/
				parameterValue |= parametricTable[positionOfParameter + 1 + nextParameter_pos]  & 0x80; 
				
				// Zisk pozice delky aktualniho segmentu (ve 'segmentLength[]')
				segmentLength_pos = (parametricTable[positionOfParameter + nextParameter_pos] & 0xE0) >> 5;

				// Opakovani aktualniho segmentu z formantu NEBO vysloveni celeho formanu v pripade, ze neni slozen z vice segmentu
				while( parameterValue & 0x0F )
					{
					// Zisk delky (poctu bitu) daneho segmentu (nebo formantu pokud neni slozen z vice segmentu)
					segmentLength_val = segmentLength[segmentLength_pos];
					// Zisk pozice formantu
					segment_pos = parametricTable[positionOfParameter + 1 + nextParameter_pos] << 1;
					
					// Cteni jednoho segmentu o delce 'segmentLength_val'
					for(uint8_t outputMask = 0x80; segmentLength_val != 0; segment_pos++, outputMask = 0x80)
						{						
						// Cteni jednoho Bytu z daneho segmentu. Segment je cten bit po bitu, dokud nebylo precteno 'segmentLength_val' bitu
						for(; outputMask != 0 && segmentLength_val != 0; outputMask = outputMask >> 1)
							{
							switch(functionMode)
								{
								case 'r':
									{
									if( outputMask & phonemeTable[segment_pos] )
										PORTF ^= (1 << 0);
									else
										PORTF ^= PORTF;
									
									Pause( 9, functionMode, NULL);	// Pauza mezi jednotlivymi bity									
									break;
									}
								case 's':
									{
									if( outputMask & phonemeTable[segment_pos] )
										Pause(9, functionMode, 0xFF);	// Po dobu pauzy mezi bity odesila '0xFF'	
									else
										Pause(9, functionMode, 0x00);	// Po dobu pauzy mezi bity odesila '0x00'
									break;
									}
								}
							--segmentLength_val;
							} // Konec cteni 1 Bytu ze segmentu
						} // Konec cteni jedne casti segmentu
					
					Pause(479, functionMode, 0x00); // Pauza mezi jednotlivymi segmenty
					
					// Dekrementace poctu opakovani
					parameterValue--;
					
					// Jestlize bylo dokonceno opakovani ( (parameterValue & OxOF) = 0 ) daneho segmentu pak:
					if( !(parameterValue & 0x0F) )
						{
						/* - Pokud ma formant vice segmentu -> Program zustane na stejnem pismenu a zaroven ses posune
							na dalsi parametr ukazujici na nasledujici segment */
						if( !(parameterValue & 0x80))
							{
							speechString_ptr--;
							nextParameter_pos += 2;
							}
						// - Pokud formant nema dalsi segment -> Presun na dalsi pismeno a jeho prvni parametr
						else
							nextParameter_pos = 0;
						}
					} // Konec cteni segmentu (nebo formantu pokud neni slozen z vice segmentu)
				break;
				}
			}
		}	
	}
