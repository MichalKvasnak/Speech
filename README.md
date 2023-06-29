# **KNIHOVNA PRO GENEROVÁNÍ HLASOVÉHO VÝSTUPU**


### **Mikroprocesor:** ATMega2560
### **Autor:** Michal Kvasňák a Petr Petyovský
---

## **ÚVOD**

Knihovna realizovaná v rámci bakalářské práce slouží pro generování hlasového výstupu za pomocí piezoelektrického měniče připojeného na pin PF0 mikroprocesoru ATMega2560. Logika programu byla navržena společností VoiceSoft v roce 1985 pro počítat Sinclair ZX Spectrum v jeho strojovém kódu. Knihovna je psána na základě této logiky v jazyce C.

## **NÁVOD K POUŽITÍ**
#### **Pro generování hlasového výstupu pomocí piezoelektrického měniče:**
1) Připojte soubory 'hlas.h' a 'hlas.c' k projektu <br>
2) Pomocí direktivy připojte hlavíčkový soubor <br>
        (tzn. použít `#include "hlas.h"`)
3) Připojte piezoelektrický měnič na pin A0

Nyní po správném zavolání funkce `Speak()` (např. `Speak("TESTOVACI' R^ETE^ZEC", 'r');`) bude generován hlasový výstup. 

#### **Pro zasílání hlasového výstupu v podobě Bytů pomocí sériové komunikace USART**
1) Připojte soubory 'hlas.h' a 'hlas.c' k projektu <br>
2) Pomocí direktivy připojte hlavíčkový soubor <br>
        (tzn. použít `#include "hlas.h"`)
3) Ve svém projektu inicializujte sériovou komunikaci pomocí funkce `USART_Init(UBRR)`
    - Rychlost odesílání/přijímání dat lze měnit pomocí změny `BAUD_RATE` (viz [uživatelská makra](#uživatelská-makra))

Nyní po správném zavolání funkce `Speak()` (např. `Speak("TESTOVACI' R^ETE^ZEC", 's');`) bude odesílán hlasový výstup pomocí sériové komunikace USART v podobě Bytů. 
    
---

## **UŽIVATELSKÁ MAKRA**
- `BUFFER_SIZE` - Definuje maximálni délku vstupního řetězce
- `BAUD_RATE` - Definuje rychlost prenosu seriove komunikace
- `UBRR` - Vypoctena rychlost prenosu pro vlozeni do registru dle vzorce:

            UBRR = [ F_CPU/(16*BAUD_RATE)] - 1

---

## **GLOBÁLNÍ PROMĚNNÉ**

1) **_`bool`_** **`newString`** - indikace obdržení nového řetězce přes sériovou komunikaci pomocí funkce `WholeStringReceived()` (viz [funkce](#funkce))

2) **_`char`_** **`receivedString[BUFFER_SIZE]`** - ukládá obdržený řetězec

3) **_`size_t`_** **`receivedString_Ptr`** - ukazatel na prvky v `receivedString`

4) **_`bool`_** **`timerDone`** - Indikace dokončení pauzy 

---

## **DEFINICE TABULEK PRO GENEROVÁNÍ HLASOVÉHO VÝSTUPU**

1) **_`static const uint8_t`_** **`parameterPositions[]`**
    >   - Obsahuje ukazatele na pozici parametrů v `parametricTable[]` dle daného písmena
    >   - Pozice parametru je získávána dle vzorců:
    >       - Pro písmena bez háčku nebo čárky: **(ASCII hodnota) - 65**
    >       - Pro písmena s háčkem nebo čárkou: **(ASCII hodnota) - 71**

<br />

2) **_`static const uint8_t`_** **`parametricTable[]`**
    > - Obsahuje hodnoty parametrů pro odpovídající segmenty v daném formantu
    > - Paramter je vždy složen z 2 Bytů ve formátu:
    >   - První byte:
    >       - První 3 bity - Pozice délky segmentu ve `formantLength[]`
    >       - Čtvrtý bit - Nevyužit
    >       - Poslední 4 bity - Počet opakování daného segmentu
    >   - Druhý byte:
    >       - První bit - Indikace zda má formant další segment:
    >           - Log. '1' - Formant nemá další segment
    >           - Log. '0' - Formant má další segment
    >       - Poslední bit - Pozice daného segmentu ve `formantTable[]`
    >       (správnou pozici ukazuje až po logickém posuvu o jeden bit vlevo)

<br />

3) **_`static const uint8_t`_** **`formantTable[]`**
    > - Obsahuje jednotlivé formanty
    > - Formant je často složen z více různých segmentů, které mají:
    >   - Rozdílnou délku vyzvedávanou v `segmentLength[]`
    >   - Rozdílný počet opakování vyzvedávaný v `parametricTable[]`
    > - Pro lepší orientaci je zaveden popis ve formátu:
    >       
    >       [Odpovídající písmeno]( [Pořadí segmentu], [z/k/o] )
    >
    >   z (začátek) = Segment zde začíná (Odkazuje na první Bytu v řádku)<br>
    >   k (konec) = Segment zde končí (Odkazuje na poslední Bytu v řádku)<br>
    >   o (obojí) = Segment začíná na prvním Bytu v řádku a končí na posledním Bytu v řádku

<br />

4) **_`static const uint8_t`_** **`segmentLength[]`**
    > - Obsahuje délky jednotlivých segmentů
    > - Pozice délky segmentu je vyzvedávána v `parametricTable[]`
    > - Pro lepší orientaci je zaveden popis ve formátu:
    >
    >           [Odpovídající písmeno]( [Pořadí segmentu/c] )
    >
    >   c (celý) = celý formant má segmenty o stejné délce

---

## **FUNKCE**

1) **_`void`_** **`USART_Init( const uint16_t ubrr)`**
    > - Inicializuje sériovou komunikaci USART
    > - **Parametry funkce:**
    >   - `ubrr` - vypočtená rychlost přenosu (viz [uživatelská makra](#uživatelská-makra))
    > - Funkce nastavuje:
    >   - Rychlost přenosu dle parametru `ubrr`
    >   - Povolení přijímání/odesílání dat
    >   - Povolení úplného přerušení při přijímání dat
    >   - Formát dat:
    >       - 1 start bit
    >       - 8 datových bitů
    >       - 1 stop bit
    >       - Žádná parita

<br />

2) **_`uint8_t`_** **`USART_Receive( void )`**
    > - Funkce pro přijímání dat z registru `UDR0`
    > - Funkce čeká na vlajku `RXC0` a pak vyzvedne hodnoty z 8bitového registru `UDR0`

<br />

3) **_`void`_** **`USART_Transmit( cosnt uint8_t dataToSend)`**
    > - Funkce pro zasílání dat pomocí registru `UDR0`
    > - Funkce čeká, než bude moct být zapisováno do registru `UDR0` za pomocí vlajky `UDRE0`. Následně vloží `dataToSend` do registru `UDR0`

<br />

4) **_`bool`_** **`WholeStringReceived( void )`**
    > - Funkce pro zjištění, zda byl obdržen kompletní řetězec pomocí proměnné `newString` (viz [globální proměnné](#globální-proměnné)). Tato proměnná je nastavena na hodnotu `true` jestliže byl obdržen kompletní řetězec.

<br />

5) **_`static inline char*`_** **`GetReceivedString( void )`**
    > - Funkce pro předání obdrženého řetězce.
    > - Funkce vrací `receivedString` (viz [globální proměnné](#globální-proměnné))

<br />

6) **`void TimerInit()`**
    > - Inicializuje 16 bitový Čítač/Časovač1
    > - Funkce nastavuje:
    >   - CTC (Clear Timer on Compare) režim
    >   - Povolení přerušení při shodě (při compare match)

<br />

7) **`bool ConvertToSpeech( const char* stringToConvert, char* convertedString )`**
    > - Funkce převádí vstupní řetězec `stringToConvert` do podoby vhodné pro generování hlasového výstupu a převedený řetězec ukládá do `convertedString`
    > - **Parametry funkce:**
    >   - `stringToConvert` - Vstupní řetězec, který má být převeden
    >   - `convertedString` - Výstupní již převedený řetězec
    > - Řetězec je převáděn tak, že:
    >   - Písmena neovlivněná háčkem, čárkou, písmenem 'I' nebo 'Ě' jsou reprezentovány jako velká písmena
    >   - Písmena ovlivněná háčkem, čárkou, písmenem 'I' nebo 'Ě' jsou reprezentovány jako malá písmena
    >   - Písmeno 'CH' je reprezentováno jako 'h'

<br />

9) **`void Pause(const uint16_t time, const char functionMode, const char byteValue)`**
    > - Funkce realizuje všechny pauzy během generování hlasového výstupu pomocí 16 bitového Čítače/Časovače1. Zároveň umožňuje zasílat data pomocí sériové komunikace USART během vykonávání pauzy.
    > - **Parametry funkce:**
    >   - `time` - Délka pauzy
    >   - `functionMode` - Režim, ve kterém má funkce pracovat. Hodnota je určována při volání funkce `Speak()`
    >       - `functionMode = 'r'` - Funkce vykonává pauzu a nezasílá žádné data
    >       - `functionMode = 's'` - Funkce během vykonávání pauzy zasílá Byty o hodnotě `byteValue`
    >   - `byteValue` - Hodnota Bytů, které jsou zasílány

<br />

10) **`void Speak( const char* originalString, const char functionMode )`**
    > - Funkce generující hlasový výstup pomocí piezoelektrického měniče, který je zapojen na pin A0 (uvnitř funkce je možno změnit pin, na kterém je připojen piezoelektrický měnič)
    > - **Parametry funkce:**
    >   - `originalString` - Vstupní řetězec, dle kterého má být generován hlasový výstup. Pro generování hlasového výstupu musí řetězec splňovat určité parametry:
    >       - Písmena s čárkou jsou reprezentovány jako: **[Písmeno]'** (Znak **'** je náhradou za čárku např. Á = A')
    >       - Písmena s háčkem jsou reprezentovány jako: **[Písmeno]^** (Zank **^** je náhradou za háček např. Č = C^)
    >   - `functionMode` - Určuje v jakém režimu je funkce volána.
    >       - `functionMode = 'r'` - Je generován hlasový výstup pomocí piezoelektrického měniče připojeného na pin A0
    >       - `functionMode = 's'` - Hlasový výstup je pomocí funkce `Pause()` zasílán pomocí sériové komunikace USART v podobě Bytů (vhodné pro generování '.wav' souborůl). Byty mají hodnotu:
    >           - `0x00` - Simulace vypnutého piezoelektrického měniče (log. '0')
    >           - `0xFF` - Simulace aktivního piezoelektrického měniče (log. '1')

### **OBSLUHA PŘERUŠENÍ**

1) **`ISR(TIMER1_COMPA_vect)`**
    > - Obsluha přerušení generovaného Čítačem/Časovačem1 při compare match (Čítač/Časovač dosáhne hodnoty nastavené v registru `OCR1`, která je nastavována ve funkci `Pause()` (viz [funkce](#funkce)))
    > - Obsluha vynuluje (automaticky) a zastaví čítání čítače. Následně nastaví proměnnou `timerDone` (viz [globální proměnné](#globální-proměnné)) na hodnotu `true` čímž se signalizuje dokončení pauzy.

<br />

2) **`ISR(USART0_RX_vect)`**
    > - Obsluha přerušení generovaného sériovou komunikací `USART0` při dokončení přijímání dat
    > - Využívá se pro čtení prijímaného řetězce.
    > - Obsluha inicializuje proměnnou `receivedChar` na hodnotu obdrženého znaku v registru `UDR0` pomocí funkce `USART_Receive()` (viz [funkce](#funkce)). Následně znak otestuje na znak `\0` a zároveň zda ukazatel `receivedChar_Ptr` nepřesáhl hodnotu `BUFFER_SIZE-1`. Pokud je testování:
    >   - Platné -> Nastaví `receivedString[receivedString_Ptr] = '\0'`, vynuluje ukazatel `receivedString_Ptr` a nastaví indikaci dokončení přijímání řetězce `newString` (viz [globální proměnné](#globální-proměnné))na hodnotu `true` 
    >   - Neplatné -> Nastaví aktuální hodnotu znaku `receivedString[receivedStringPtr] = receivedChar` a inkrementuje hodnotu ukazatele `receivedStringPtr`.   
