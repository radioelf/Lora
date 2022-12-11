/* Control de modulo lora E220-PIC12F675 (remoto) 
			
Nº de pin E220 	
1 M0  entrada pull-up junto a M1 y decida los cuatro modos de funcionamiento. 
2 M1  entrada pull-up junto a M0 y decide los cuatro modos de funcionamiento. 
3 RXD entrada TTL (SOLO 3.3V) se conectan al pin de salida TXD externo (MCU, PC). 
4 TXD salida  TTL(SOLO 3.3V) conectan al pin de entrada RXD externo (MCU, PC). 
5 AUX salida para indicar el estado del funcionamiento del módulo y despertar 
    el microcontrolador durante el procedimiento de inicialización de autocomprobación,
    se mantiene a estado alto y pasa a bajo an recibir datos de la radio, al 
    transmitir datos a la radio pasa a bajo unos 2 o 3ms antes de la trasnmisión
    y se restaura a alto finalizada la tranmisión radio.
6 VCC alimentación 3V~5.5V DC
7 GND tierra

Modos 			M1 	M0 	
Normal   		0  	0   los canales UART e inalámbricos están abiertos y la transmisión transparente está activada -DEFECTO para TX-
Transmisor WOR		0 	1   transmisor/receptor WOR Wake On Radio (despertar por la radio)
Receptor WOR		1 	0   SOLO receptor WOR Wake On Radio (despertar por la radio) -DEFECTO para RX-
Modo de reposo		1	1   el módulo entra en reposo (se despierta automáticamente al configurar los parámetros)
			
	
       PIC12F675 
        ---_---          Pickit3
      -|VCC GND|-        1 -> RESET (GP3) 
  INT1-|GP5 GP0|- ADC    2 -> VCC
   TXD-|GP4 GP1|- M0     3 -> GND
   AUX-|GP3 GP2|- RXD    4 -> ICSPDAT(GP0)  
        -------          5 -> ICSPCLK(GP1)
       
GP0->ADC  INPUT-NO Pull.up
GP1->M1   OUTPUT-modo E220 0->TX, 1->RX WOR, Wake On Radio (despertar por la radio)
GP2->RXD  INPUT-NO Pull-up ISR por flaco de bajada
GP3->AUX  INPUT señal estado E220
GP4->TXD  OUPUT-NO Pull.up
GP5->INT1 IMPUT-Pull-up, ISR por cambio de estado
	
    Estado: registro STATUS (ADDRESS:03h banco 0 y 83h banco 1
    Bit 7 IRP reservado
    Bit 6 RP1 reservado
    Bit 5 RP0 selección del banco de memoria 0 = Bank 0 (00h - 7Fh) 1= Bank 1 (80h - FFh)
    Bit 4 TO pasa a 0 al desbordar el wdt, a 1 al arrancar o ejecutar clrwdt o sleep
    Bit 3 PD pasa a 0 al ejecutar la instrucción sleep, a 1 al arranca y al ejecutar clrwdt
    Bit 2 Z pasa a 1 si el resultado es 0
    Bit 1 DC pasa a 1 si hay acarreo en el bit 8, 9 ,10 y 11 en una suma y a 0 en una resta
    Bit 0 C pasa a 1 si hay acarreo en el bit 8 en una suma y a cero en una resta
       	
    Calibración oscilador interno: registro OSCCAL  (ADDRESS: 90h, banco 1)
    Bits 7-2 CALx valor de calibración de la frecuencia
    Bits 1-0 NO usados
	
    Interrupciones: registro INTCON 10001000 (ADDRESS: 0Bh o 8Bh, banco 0 y 1): 
    Bit 7 GIE: habilita o deshabilita las interrupciones globalmente
    Bit 6 PEIE: habilita interrupciones perifericos
    Bit 5 T0IE: habilita interrupción por desbordamiento del TMR0
    Bit 4 INTE: habilita interrupción por el pin GP2/INT
    Bit 3 GPIE: habilita interrupción por cambio de estado en pines GPIO (habilitar en registro IOC los GPIO a usar) 
    Bit 2 TOIF: bandera por interrupción por TMR0 (borrar por código)
    Bit 1 INTF: bandera por interrupción por GP2/INT (borrar por código)
    Bit 0 GPIF: bandera por interrupción por cambio de estado de GP0 la GP5 (borrar por código)
    registro ICO 00101010->GP0 OFF, GP1 ON, GP2 OFF, GP3 ON, GP4 OFF y GP5 ON
	
    Opciones: registro OPTION_REG 00011111(ADDRESS: 81h, banco 1):
    Bit 7 GPPU: a 0 se habilitan las resistencias GPIO Pull-up de GPIO
    Bit 6 INTEDG: Flaco para la interrupción de GP2/INT
    Bit 5 T0CS: selección de TMR0 Clock, pin GP2/T0CKI o CLKOUT
    Bit 4 T0SE: selección flanco incremento TMR0 en pin GP2/T0CKI
    Bit 3 PSA: Prescaler a WDT o TIMER0 
    Bits 2-0 PS2:PS0: Prescaler Rate
    	
    Comparador: registro CMCON 00000111 (ADDRESS: 19h, banco 0)
    Bit 7 NO usado
    Bit 6 COUT:
    Bit 5 No usado
    Bit 4 CINV:
    Bit 3 CIS:
    Bits 2-0 CMx: 111->modo comparador apagado
	
    Resistencias internas Pull-Up: registro WPU 00100000 (ADDRESS: 95h, banco 1)
    GP3 NO dispone de Pull-Up
    Bits 7-6 NO usado
    Bit 5-4 selección resistencias internas Pull-Up para WPU4 y WPU5
    Bit 3 NO usado
    Bits 2-0 selección resistencias internas Pull-Up para WPU2, WPU1 y WPU0 GP1 y GP5 ON
       
    Puertos entradas-salidas: registro TRISIO 000101111 (ADDRESS: 85h, banco 1)
    bit 7-6 NO usado
    bit 5-0 1 GPx entrada, 0 GPx salida
       
    ADC: registro ANSEL 00000001(ADDRESS: 9Fh, banco 1)
    Bit 7 NO usado
    Bit 6-4 bits de selección  del clock para  la conversión A/D 
    Bit 3-0 bits de habilitación de puertos analógicos (defecto a 1 habilitados los 4 puertos)
			
    Batería tensión máxima 12.5V - divisor  tensión = 3.82V en entrada ADC 
    Batería baja < de 10.60V - divisor  tensión = 2.24V en entrada ADC
    
    **Incluir estas lineas para definir las direcciones de inicio, main y ISR en:
      project  properties box, pic-as Global Options -> pic-as linker->custom linker options:
      -pmain=80h
      -resetVector=0h
      -pisrCode=04h
    Comprobar direcciones en LoraE220.X.production.map

V. 0.0.1  2/12/22

*/
;Microprocesador usado     
PROCESSOR 12F675
PAGEWIDTH   132
RADIX       DEC
;Directivas del microcontrolador
#include <xc.inc> 
; Configuración
CONFIG  FOSC = INTRCIO      ; Bits selección del oscilador (oscilador INTOSC: Función I/O para el pin GP4/OSC2/CLKOUT, función I/O para GP5/OSC1/CLKIN)
CONFIG  WDTE = OFF          ; Bit Watchdog Timer (WDT disabled)
CONFIG  PWRTE = ON          ; Bit Power-Up Timer (PWRT enabled-delay 72ms)
CONFIG  MCLRE = OFF         ; GP3/MCLR pin seleccionado como digital I/O (GP3 digital I/O, MCLR conectado internamente a VDD)
CONFIG  BOREN = OFF	    ; Bit detección Brown-out (BOD disabled)
CONFIG  CP = OFF            ; Bit Protección código programa (Program Memory code protection is disabled)
CONFIG  CPD = OFF	    ; Bit protección código memoria (Data memory code protection is disabled)
;
;***** Asignación valores costantes Value *************************************** 
#define ADC	GPIO,0	    ;entrada analógica
#define M1	GPIO,1      ;salida modo
#define RXD	GPIO,2	    ;entrada serial RXD (ISR)
#define AUX	GPIO,3      ;entrada AUX del modulo E220 
#define TXD	GPIO,4	    ;salida serial TXD
#define INT1	GPIO,5      ;entrada 3 (ISR)
#define	bit_K	23	    ;hex DA = 1200 bits/sec (218)
			    ;hex 6b = 2400 bits/sec (107)
			    ;hex 32 = 4800 bits/sec (50)
			    ;hex 17 = 9600 bits/sec (23)
			    ;hex 0A =19200 bits/sec (10)
#define medio_bit bit_K/2   ;valor para perio de de medio bit	
#define OK_bat  168         ;valor mínimo para el nivel OK de batería, 167 +1 (167*0.019607843 =3.274509781V)
;
;****** Reserva de registros para variable ****Banco 0, 0x20 a 0x5F*************
PSECT udata
count:	    DS	1	    ;0x20, contador 
tick:	    DS	1	    ;0x21, tick
tick_1:     DS	1	    ;0x22, tick 1    
rxd_byte:   DS	1	    ;0x23, byte de recepción serie
delay_cntr: DS	1	    ;0x24, contador para retado en rutina serial
bit_cntr:   DS	1	    ;0x25, número de bits a TX
txd_byte:   DS	1	    ;0x26, byte de transmisión serie
estado:     DS	1           ;0x27, byte indicación estados
;ADC
;lsb_byte:   DS	1           ;parte baja lectura adc (2bits)
msb_byte:   DS	1           ;0x28, parte alta lectura adc (8bits)
level_Bat:  DS	1           ;0x29, 1->batería OK, 0->batería baja
;    
;=====================================================================
;Vector reset
PSECT resetVector, class=CODE, delta=2
resetVector:
    PAGESEL main
    goto main
;Vector interrupción ISR por RXD-GP2 y INT1-GP5
PSECT isrCode, class=CODE, delta=2
intVecLo:
    bcf     STATUS,5        ;banco 0
    clrf    estado	    ;estado->0
    btfsc   INTCON,0        ;bandera ISR por cambio de estado a 1, GPIF INT1-GP5
    goto    isrInt1         ;ISR por cambio de estado de GP5
    btfss   INTCON,1        ;bandera ISR por cambio de flanco descendente, INTF RXD-GP2
    goto    exitInt         ;ISR NO por cambio de estado de GP5 o GP2
isrRx:
    movlw   0b10000100	    ;valor para indicar ISR en RXD-GP2, bit 2 y 7 ON  (flaco de bajada)
    movwf   estado	    ;estado->136, indicando E220 despertado por RX datos radio
    goto    exitInt         ;saltamos a exitInt
isrInt1:
    movlw   0b10100000	    ;valor para indicar ISR en INT1-GP5, bit 5 y 7 ON (cambio de estado)
    movwf   estado	    ;estado->160, movemos el valor de W a estado
exitInt:
    clrf    INTCON	    ;borramos INTCON->00000000
    retfie		    ;salimos de ISR, retfie pondrá el bit 7 de INTCON a 1
;
;=====================================================================
;Inicialización
PSECT main,class=CODE, delta=2   
main:
    BANKSEL OSCCAL	    ;banco 1
    call    0x3FF	    ;valor de calibración fabrica
    movwf   OSCCAL	    ;calibración
;Configuración registros
    movlw   0b00011111	    ;valor configuración, Pull-UP ON, preescale a WDT, clock interno (bit5=0), ISR  GP2 por flaco descendente
    movwf   OPTION_REG      ;00011111->OPTION_REG
    clrf    VRCON	    ;VRCON->00000000
    movlw   0b01010001      ;00000001->W
    movwf   ANSEL	    ;GP0->ADC, 16TOSC (4us), ADC en AN0 GP0
    movlw   0b00101101	    ;pin 0-ADC,2-RXD,3-AUX y 5-INT1 como inputs y pin 1-LED y 4-TXD como output
    movwf   TRISIO          ;00101111->TRISIO
    movlw   0b00100000	    ;00100000->W
    movwf   WPU		    ;resistencias pul-up GP5-INT1
    movlw   0b00100000	    ;00100000->W	
    movwf   IOC		    ;GPIO con interrupción por cambio de estado (ISR para GP5)
    BANKSEL GPIO	    ;banco 0  
    movlw   0b00010010	    ;00010010->W
    movwf   GPIO	    ;GPI->00010010, GP1-LED y GP4-TXD a 1
    movlw   0b00000111	    ;0000111->W
    movwf   CMCON	    ;Comparador Off 
    movlw   0b00000001	    ;bit 7->0 izquierda, bit 6 0-> Vdd, bit 3-2 00->canal 0,bit 1 0->aun no inicia ADC, bit 0 1->ADC operativo
    movwf   ADCON0	    ;W->ADCON0
;Estados inicial
    clrf    INTCON	    ;INTCON->00000000
    clrf    estado          ;estado->00000000
    call    delay_4m	    ;llamamos a esperar 4
    btfss   AUX		    ;si GP3 es 1 saltamos
    goto    $-1		    ;salta una posición, esperamos GP4->AUX a 1
    call    delay_1s	    ;esperamos 1 segundo a que inicialice el modulo E220
;TX rs232 Lora
    movlw   0b01010110	    ;86=V->W
    movwf   txd_byte	    ;W->txd_byte	
    call    xmit232	    ;llamamos a rutina enviar por RS-232 valor byte txd_byte
    movlw   0b00111010      ;58=:->W
    movwf   txd_byte	    ;W->txd_byte
    call    xmit232	    ;llamamos a rutina enviar por RS-232 valor byte txd_byte
    movlw   0b00110000	    ;48=0->W
    movwf   txd_byte	    ;W->txd_byte
    call    xmit232	    ;llamamos a rutina enviar por RS-232 valor byte txd_byte
    movlw   0b00110001      ;49=1->W
    movwf   txd_byte	    ;W->txd_byte	
    call    xmit232	    ;llamamos a rutina enviar por RS-232 valor byte txd_byte
    btfss   AUX		    ;si GP3 es 1 saltamos
    goto    $-1		    ;salta una posición, esperamos GP3-AUX a 1
    call    delay_50m	    ;llamamos a esperar 50ms
;=====================================================================
;Bucle principal...
loop:
    bcf	    M1		    ;M0->GND, M1->VCC (RX WOR, transmisor remoto en modo RX-RX WOR)
    movlw   0b10011000	    ;valor configuración ISR, GIEO bit7 permitimos ISR, bit 3 GPIE ON y bit 4 GP2/INT
    movwf   INTCON          ;10001000->INTCON, habilitamos ISR por cambio de estado
    nop			    ;4 ciclos de reloj 0.25us*4=1us
    sleep		    ;pasamos a reposo, esperando ISR.. 
    nop			    ;4 ciclos de reloj 0.25us*4=1us
    clrf    INTCON          ;deshabilitamos ISRs, bit 7-GIE a 0
    bcf     STATUS,5        ;banco 0
    btfss   estado,7	    ;bit 7 a 1 ISR por cambio de estado de GP5-INT1 o GP2-RXD
    goto    iniciar         ;NO ISR INT1-RXD   
    btfsc   estado,5        ;bit 5 a 1 INT1, salta si es 0 
    goto    TX              ;bit 5 de estado a 1
    btfss   estado,2        ;bit 2 (RXD) a 1 salta->ISR por RX datos 
    goto    iniciar         ;saltamos a iniciar
    call    RxE220          ;llamamos a leer el byte recibido
    btfss   estado,1        ;bit 1 a 1 salta, petición actualizar->envio trama
    goto    iniciar         ;NO ISR INT1, NO RX caracter actualizar
TX:
    bsf	    M1		    ;M0->GND, M1->GND (modo transparente TX-RX)
    call    delay_50m	    ;llamamos a esperar 50ms
    call    AdcRead	    ;leer puerto ADC 
    btfss   AUX		    ;si GP3 es 1 saltamos
    goto    $-1		    ;salta una posición, esperamos GP3-AUX a 1
    call    delay_4m	    ;llamamos a esperar 4ms
    call    TxE220          ;llamanos a enviar trama por radio
    nop			    ;4 ciclos de reloj 0.25us*4=1us
iniciar:
    btfss   AUX		    ;si GP3 es 1 saltamos 
    goto    $-1		    ;salta una posición, esperamos GP3->AUX a 1
    call    delay_50m	    ;llamamos a esperar 50ms
    goto    loop	    ;iniciamos bucle
;=====================================================================
;********************** Subrutinas ***********************************
;********************* RX byte E220 **********************************
RxE220:
    bcf     STATUS,5        ;banco 0
Rx_U:
    ;call    Receive232     ;llamamos a leer byte RX serial
    ;movlw   0x55	    ;primer byte rx "U", registro W a 85
    ;xorwf   rxd_byte,W     ;XOR lo leído con W->85
    ;btfss   STATUS,2	    ;si es 0 el bit Z (Zero bit), rxd_byte es = 0x55, salta
    ;goto    Rx_U           ;esperamos caracter "U"
    ;bsf     estado,1       ;bit 1 a 1, indicación actualizar
Rx:
    btfss   AUX		    ;si GP3 es 1 saltamos
    goto    Rx		    ;NO es 0, continuamos recibiendo datos serie
    call    delay_4m        ;esperamos, más bytes RX?
    bsf     estado,1        ;bit 1 a 1, indicación actualizar
    return		    ;retornamos
;
;******Enviamos trama con el valor leído en ADC y estdo GP5 (puerta)***
;TX-> Bat:O/L,Door:1/0,rssi:x 19 bytes, RS232 8N1-> 10 bits por byte
;190bits, a 9600Bps unos 104us *190 =19.78ms, (radio 2400Bps 416us *190 =45.35ms)
TxE220:
    bcf     STATUS,5        ;banco 0
    movlw   66		    ;valor para carácter "B" ascii->W
    movwf   txd_byte	    ;W->txd_byte	
    call    xmit232	    ;llamamos a rutina enviar por RS-232 valor byte txd_byte
    movlw   97		    ;valor para carácter "a" ascii->W
    movwf   txd_byte	    ;W->txd_byte	
    call    xmit232	    ;llamamos a rutina enviar por RS-232 valor byte txd_byte
    movlw   116		    ;valor para carácter "t" ascii->W
    movwf   txd_byte	    ;W->txd_byte	
    call    xmit232	    ;llamamos a rutina enviar por RS-232 valor byte txd_byte
    call    dosPuntos	    ;llamamos a dosPuntos
valueAdc:    
    btfss   level_Bat,0	    ;bit 0 a 1 saltamos, nivel batería OK 
    goto    batLow          ;saltamos a batería baja
    movlw   0b01001111	    ;O 79->ASCII->W
    movwf   txd_byte	    ;W->txd_byte	
    goto    batOk	    ;saltamos batería OK
batLow:
    movlw   0b01001100	    ;L 76->ASCII->W
    movwf   txd_byte	    ;W->txd_byte		
batOk:    
    call    xmit232	    ;llamamos a rutina enviar por RS-232 valor byte txd_byte
    call    coma	    ;llamamos a coma
estatusDoor:
    movlw   68		    ;valor para carácter "D" ascii->W
    movwf   txd_byte	    ;W->txd_byte	
    call    xmit232	    ;llamamos a rutina enviar por RS-232 valor byte txd_byte
    movlw   111		    ;valor para carácter "o" ascii->W
    movwf   txd_byte	    ;W->txd_byte	
    call    xmit232	    ;llamamos a rutina enviar por RS-232 valor byte txd_byte
    movlw   111		    ;valor para carácter "o" ascii->W
    movwf   txd_byte	    ;W->txd_byte	
    call    xmit232	    ;llamamos a rutina enviar por RS-232 valor byte txd_byte
    movlw   114		    ;valor para carácter "r" ascii->W
    movwf   txd_byte	    ;W->txd_byte	
    call    xmit232	    ;llamamos a rutina enviar por RS-232 valor byte txd_byte
    call    dosPuntos	    ;llamamos a dosPuntos
    movlw   0b00110001	    ;valor para carácter "1"(ascii)->W
    movwf   estado	    ;movemos el valor de W a estado
    btfss   INT1	    ;si GP5 es 1 saltamos
    bcf	    estado,0	    ;estado->00110000 = "0"(ascii)
    movf    estado,W	    ;registro estado 0 o 1 ascii->W
    movwf   txd_byte	    ;W->txd_byte	
    call    xmit232	    ;llamamos a rutina enviar por RS-232 valor byte txd_byte
    call    coma	    ;llamamos a coma
    movlw   114		    ;valor para carácter "r" ascii->W
    movwf   txd_byte	    ;W->txd_byte	
    call    xmit232	    ;llamamos a rutina enviar por RS-232 valor byte txd_byte
    movlw   115		    ;valor para carácter "s" ascii->W
    movwf   txd_byte	    ;W->txd_byte	
    call    xmit232	    ;llamamos a rutina enviar por RS-232 valor byte txd_byte
    movlw   115		    ;valor para carácter "s" ascii->W
    movwf   txd_byte	    ;W->txd_byte	
    call    xmit232	    ;llamamos a rutina enviar por RS-232 valor byte txd_byte	
    movlw   105		    ;valor para carácter "i" ascii->W
    movwf   txd_byte	    ;W->txd_byte
    call    xmit232	    ;llamamos a rutina enviar por RS-232 valor byte txd_byte
    call    dosPuntos	    ;llamamos a dosPunto
    return		    ;retornamos
comillas:
    movlw   34		    ;valor para carácter " ascii->W
    movwf   txd_byte	    ;W->txd_byte	
    call    xmit232	    ;llamamos a rutina enviar por RS-232 valor byte txd_byte
    return		    ;retornamos
coma:
    movlw   44		    ;valor para carácter , ascii->W
    movwf   txd_byte	    ;W->txd_byte	
    call    xmit232	    ;llamamos a rutina enviar por RS-232 valor byte txd_byte
    return		    ;retornamos
dosPuntos:
    movlw   58		    ;valor para carácter : ascii->W
    movwf   txd_byte	    ;W->txd_byte	
    call    xmit232	    ;llamamos a rutina enviar por RS-232 valor byte txd_byte
    return		    ;retornamos	
;
;********** Leemos ADC GP0 y obtenemos el valor en un byte ***********
;El resultado de la lectura se almacena en dos registros ADRESH y ADRESL.
;Se ha seleccionado justificado hacia a la izquierda, por lo que los 8bits
;MSB están en el registro ADRESH mientras que LSB están 2bis en el registro ADRESL
;	MSB	   LSB
;     76543210   76543210
;     xxxxxxxx   xx000000
;      ADRESH     ADRESL
AdcRead:
    BANKSEL ADCON0	    ;banco 0
    bsf	    ADCON0,1	    ;inicializamos lectura ADC, run conversión ADC, ADON->1
    btfsc   ADCON0,1	    ;termino la conversión ADC?, GO a 0
    goto    $-1		    ;salta una posición, esperamos fin conversión
    ;BANKSEL ADRESL	    ;banco 1
    ;movf    ADRESL,W       ;registro ADRESL 2 bits a W
    ;movwf   lsb_byte       ;W->lsb_byte, resultado de la conversión
    ;BANKSEL ADRESH	    ;banco 0
    movf    ADRESH,W	    ;registro ADRESH byte a W
    movwf   msb_byte	    ;W->msb_byte, resultado de la conversión
;Se desprecián los 2 bits de menos peso de ADRESL, para obtener la tensión en AN0
;multiplicaremos el valor de ADRESH x 0.019607843
levelBat:
    movlw   0b00000001	    ;valor OK batería
    movwf   level_Bat	    ;movemos el valor de W a level_Bat 
    movlw   OK_bat          ;colocamos el valor mínimo del nivel batería
    subwf   msb_byte,W      ;restamos lo leído con W
    btfss   STATUS,0	    ;si es 0 el bit C (acarreo), msb_byte es menor que OK_bat
    clrf    level_Bat       ;valor leído menor que el valor mínimo
    return		    ;retornamos
;
;********************** TX byte RS-232 ************************************
;9600 8 N 1 -> 10 bits por palabra (1 start bit, 8 data bits, y 1 stop bit)
;duracion 10/9600 = 1041.66 microsegundos. TX byte xmt_byte
xmit232:		    ;RS-232C salida serie, un byte
    bcf     STATUS,5        ;banco 0
again:
    movlw   8		    ;valor 8->W
    movwf   bit_cntr	    ;contador de bits a 8	
    bcf	    TXD		    ;GPIO,4 a 0	 
    call    bit_delay	    ;llamamos a la rutina de espera
xmit:
    rrf	    txd_byte,f	    ;rotamos a derecha un bit del registro xmt_byte, el acarreo pasa al bit C de STATUS
    btfss   STATUS,0	    ;STATUS,0 se encuentra a 1 saltamos			
    bcf	    TXD		    ;GPIO,4 a 0	
    btfsc   STATUS,0	    ;STATUS,0 se encuentra a 0, salta	
    bsf	    TXD		    ;GPIO,4 a 1
    call    bit_delay	    ;llamamos a la rutina de espera
    decfsz  bit_cntr,f	    ;decrementamos el valor de bit_cntr y lo colocamos en bit_cntr
    goto    xmit	    ;saltamos a la rutina xmit
    bsf	    TXD		    ;GPIO,4 a 1		
    call    bit_delay	    ;llamanos a la rutina de espera
    return		    ;retornamos
; espera entre bits
bit_delay:
    movlw   bit_K	    ;valor Bps, 9600Bps->23, 104uS
    movwf   delay_cntr	    ;bit_K->W->del
loopTx:
    nop			    ;4 ciclos de reloj 0.25us*4=1us
    decfsz  delay_cntr,f    ;decrementamos el valor de delay_cntr y lo colocamos en delay_cntr
    goto    loopTx	    ;iniciamos ciclo espera
    return		    ;retornamos
;************************ RX byte RS-232  ****************************
Receive232:
    bcf     STATUS,5        ;banco 0
start_bit:
    btfsc   RXD		    ;si tenemos un 0 en GP2, saltamos
    goto    start_bit	    ;saltamos a rutina start_bit
    call    start_delay	    ;llamamos a espera para bit start
    btfsc   RXD		    ;si tenemos un 0 en GP2, saltamos
    goto    start_bit	    ;saltamos a rutina start_bit
    movlw   8		    ;valor 8->W
    movwf   bit_cntr	    ;W->bit_cntr
    clrf    rxd_byte	    ;000000000->rcv_byte
receive:
    call    bit_delay_R	    ;pasamos a rutina espera bit
    btfss   RXD		    ;si tenemos un 1 en GP2, saltamos	
    bcf	    STATUS,0	    ;bit C de STATUS a 1
    btfsc   RXD		    ;si tenemos un 0 en GP2, saltamos	
    bsf	    STATUS,0	    ;bit C de STATUS a 0	
    rrf	    rxd_byte,f	    ;rotamos a derecha un bit del registro rcv_byte, el acarreo pasa al bit C de STATUS
    decfsz  bit_cntr,f	    ;decrementamos el valor de bit_cntr y lo colocamos
    goto    receive	    ;saltamos a la rutina de recepción
    call    bit_delay_R	    ;llamamos a espera bit
    return		    ;retornamos
bit_delay_R:
    movlw   bit_K	    ;valor Bps, 9600Bps->23, 104uS
    movwf   delay_cntr	    ;bit_K->W->delay_cntr
loop_Rx:    
    nop			    ;4 ciclos de reloj 0.25us*4=1us
    decfsz  delay_cntr,f    ;decrementamos el valor de delay_cntr y lo colocamos en delay_cntr
    goto    loop_Rx	    ;saltamos a loop_R
    return		    ;retornamos
start_delay:
    movlw   medio_bit	    ;valor para periodo 1/2bit, 23/2=11->W
    movwf   delay_cntr	    ;W->delay_cntr
loop_Rx2:
    nop			    ;4 ciclos de reloj 0.25us*4=1us
    decfsz  delay_cntr,f    ;decrementamos el valor de delay_cntr y lo colocamos en delay_cntr
    goto    loop_Rx2	    ;saltamos a loop_R2
    return		    ;retornamos
;******************** Retardo de un segundo ****************************
delay_1s:
    bcf     STATUS,5        ;banco 0
    movlw   0xFF	    ;valor 11111111->W
    movwf   count	    ;W->count
    movwf   tick	    ;W->tick
    movlw   0x05	    ;valor 00000101->W
    movwf   tick_1	    ;W->tick_1
    decfsz  count,f	    ;decrementamos count
    goto    $-1		    ;saltamos una posición, count NO 0
    decfsz  tick,f	    ;decrementamos count
    goto    $-3		    ;saltamos tres posiciones, tick NO 0
    decfsz  tick_1,f	    ;decrementamos count
    goto    $-5		    ;retorna cinco posiciones, tick_1 NO 0
    return		    ;retornamos
;******************** Retardo de 50mS **********************************   
delay_50m:      	    ;49993 ciclos
	movlw	0x0E
	movwf	tick
	movlw	0x28
	movwf	tick_1
delay_50:
	decfsz	tick,f
	goto	$+2
	decfsz	tick_1,f
	goto	delay_50    ;3 ciclos
	goto	$+1
	nop
	return		    ;4 ciclos + retorno
;******************** Retardo de 4mS **********************************
delay_4m:		    ;3994 ciclos
    bcf     STATUS,5        ;banco 0
    movlw   0x1E	    ;30, 00011110->W
    movwf   tick	    ;W->tick
    movlw   0x04	    ;4, 00000100->W
    movwf   tick_1	    ;W->tick_1
delay_4:
    decfsz  tick,f	    ;decrementamos tick
    goto    $+2		    ;salta dos posiciones, tick NO 0
    decfsz  tick_1,f	    ;decrementamos tick_1
    goto    delay_4	    ;saltamos a dalay_4, 3 ciclos
    goto    $+1		    ;salta una posición, tick_1 NO 0
    return		    ;4 ciclos + retorno
    
END resetVector
