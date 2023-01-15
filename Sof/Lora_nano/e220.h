#ifndef _E220_H_
#define _E220_H_
/*
 E220-900TBH-01 IC LLCC68
https://github.com/xreef/EByte_LoRa_E220_Series_Library
https://github.com/xreef/EByte_LoRa_E220_Series_Library/tree/master/examples
https://www.mischianti.org/category/my-libraries/ebyte-lora-e22-devices/

EU863 de 863 a 870 MHz
867.1 MHz Opcional
867.3 MHz Opcional
867.5 MHz Opcional
867.7 MHz Opcional
867.9 MHz Opcional
868.1 MHz/125 primario
868.3 MHz/125-250 obligatorio
868.5 MHz obligatorio
868.9 MHz/125 Opcional
869.5 MHz/125 para RX2 respuesta SF9(10%)
Canales modulo E220 (850.125 + CH):
13 -> 863.125Mhz
14 -> 864.125Mhz
15 -> 865.125Mhz
16 -> 866.125Mhz
17 -> 867.125Mhz
18 -> 868.125Mhz
19 -> 869.125Mhz
20 -> 870.125Mhz
Número máximo de trasmisiones por día: cabecera + datos 30bytes a 2400Bps = unos 125ms por paquete
un máximo de 10 trasmisiones por hora.

Modos:
modo 0 - Normal-transparente M0->GND, M1->GND
Transmisión: los datos recibidos por el puerto serie son transmitidos por el modulo.
Recepción: La recepción inalámbrica está activada, y después de recibir los datos se envían por TXD

modo 1 - RX y TX,  incluye el código para despertar, WOD TX M0->VCC, M1->GND
Transmisión: se añade automáticamente un preámbulo antes de transmitir.
Recepción: Puede recibir datos normalmente, la función de recepción es la misma que la del modo normal.

modo 2 - SOLO RX  y necesario código despertar enviado por el transmisor, WOD RX M0->GND M1->VCC
Transmisión: TX por RF desactivada
Recepción: Sólo puede recibir datos de un transmisor en modo WOR

modo 3 - Configuración/Sleep, M0->VCC, M1->VCC
Transmisión: TX por RF desactivada
Recepción: RX por RF desactivada
Al pasar del modo de reposo a otros modos, el módulo re configurará los parámetros.
Durante el proceso de configuración, AUX permanecerá en nivel bajo; después de la configuración,
pasa a nivel alto, comprobar flaco de subida

Configuración:
ADDH	    Byte de dirección alto del módulo (el valor predeterminado 00H)	00H
ADDL	    Byte de dirección bajo del módulo (el valor predeterminado 00H)	01H
SPED      	Información sobre el bit de paridad de la tasa de datos y la tasa de datos en el aire	02H
OPTION	    Tipo de transmisión, configuración de pull-up, hora de activación, FEC, potencia de transmisión	03H
CAN	        Canal de comunicación (850.125 + CHAN *1M), predeterminado 17H (873.125MHz),  válido solo para dispositivos de 868MHz	04H
TRANSMISSION_MODE	Todos los parámetros de transmisión	05H
CRTYPT_H	Cifrado de usuario	06H
CRTYPT_L	Cifrado de usuario	07H

el Pin AUX del modulo E220 se mantiene a 1 a excepción del arranque, salida del modo de configuración y
en la recepción y transmisión de datos vía radio, en la recepción se pasa a nivel bajo cuando ya se a
comenzado a recibir el primer byte, unos 1,5ms después del paso a bajo se comienza a recibir los datos,
también pasa a nivel bajo en la transmisión unos 2 ms antes de que se inicie la transmisión vía radio,
y vuelve a nivel alto finalizada la trasmisión de radio.

Si ponemos a 0 pin M1 y M0 se entra en modo “Normal”, puede recibir y transmitir
todos los datos del dispositivo A al B; esta modalidad se define como “Transmisión transparente”.
------------- Arduino Nano  ------------
E220		  <----> ArduinoUNO
 * M0         <----- 3 3.3v
 * M1         <----- 8 3.3v
 * TX         -----> 0 3.3V (Pull-UP)
 * RX         <----- 1 3.3V (Pull-UP)
 * AUX        -----> 2 (Pull-UP)
 * VCC        ------ 3.3v/5v
 * GND        ------ GND
 *
consumo de unos  550mA en TX radio para 30dBm
Configuración: 9600Bps 8N1 puerto serie<->E220, 2400Bps a través del aire, byte RSSI habilitado
tamaño máximo del paquete 200Bytes
 9600Bps uart, 1 bit 104 * (1+8+1) = 1040us
 2400Bps en aire, 1 bit 417uS * (1+8+1) = 4170uS

leer configuración en modo sleep M0 y M1 a 1
comando->C1+dirección inicial->00+longitud->0b
TX->config: c1 00 0b
RX->config: c1 00 0b 72 df 62 00 11 83 00 00 20 0b 16
byte 6: canal de radio
byte 9 y 10: dirección

Banderas error E220: errorE220
bit 0 ->1 transmisión (gpio AUX)
bit 1 ->2 transmisión
bit 2 ->4 longitud recepción trama
bit 3 ->8 trama RX No valida
bit 4 ->16 Timeout
bit 5 ->32 
bit 6 ->64 
bit 7 ->128 sin respuesta del modulo Lora
*/

#define BAUD_RATE 9600                                                                // velocidad del puerto serie E220 (defecto)
#define CHANNEL 17	                                                                  // canal radio 18 (cambiar según configuración)
#define MAX_Byte 30                                                                   // número maximo de byte de RX para la trama Lora

bool update = false;
uint8_t errorE220 = 0;
uint8_t rssiRx = 0;
uint8_t c[MAX_Byte];

void ModeE220(uint8_t);
bool RxE220();
uint8_t waitCompleteResponse(unsigned int);
void cleanUARTBuffer();

//////////////////////////////////////////////////////////////////////////////////////
// Inicializamoos modulo Lora E220 y comrobamos correcta comunicación (leemos el canal de radio)
//////////////////////////////////////////////////////////////////////////////////////
bool IniE220()
{
	pinMode(E220M0, OUTPUT);
	pinMode(E220M1, OUTPUT);
	pinMode(E220AUX, INPUT);

	delay(500);
	cleanUARTBuffer();
	waitCompleteResponse(5000);
	ModeE220(3);                                                                      // pasamos a configuración
	if (digitalRead(E220AUX) == LOW)
	{
		waitCompleteResponse(1000);
	}
	Serial.write(0xc1);                                                               // comando leer registros
	Serial.write(0x04);                                                               // dirección inicial
	Serial.write(0x01);                                                               // número de registros a leer
	// RX OK = 0xc1 0x04 0x01 0x11-->193 4 4 17
	byte message[] = {0, 0, 0, 0};
	uint8_t i = 0;

	delay(10);                                                                        // 9600Bps ->104 * (1+8+1) = 1040us *4bytes = 4160Us
	waitCompleteResponse(5000);
	while (Serial.available() && i < 4)
	{
		byte x = Serial.read();                                                       // leemos carácter a carácter...
		message[i++] = x;
	}
	if (message[3] == CHANNEL)
	{
		if (bitRead(errorE220, 7))
			bitClear(errorE220, 7);

		ModeE220(2);                                                                  // pasamos a RX + código despertar
		return true;
	}
	else
	{
		bitSet(errorE220, 7); 														  // 1xxxxxxx->128
		return false;
	}
}
/*////////////////////////////////////////////////////////////////////////////////////
Modos de trabajo para el modulo E220
	MODE_0_Transparente TX-RX	= 0  M0 GND, M1 GND
	MODE_1_WOR_TX-RX        	= 1  M0 VCC, M1 GND
	MODE_2_WOR_RX           	= 2  M0 GND, M1 VCC
	MODE_3_configuración-SLEEP 	= 3  M0 VCC, M1 VCC
/////////////////////////////////////////////////////////////////////////////////////*/
void ModeE220(uint8_t mode)
{
	if (bitRead(errorE220, 7))
	{
		return;
	}
	if (mode >= 0 || mode < 4)
	{
		delay(40);
		if (mode == 0)
		{
			digitalWrite(E220M0, LOW);
			digitalWrite(E220M1, LOW);
		}
		else if (mode == 1)
		{
			digitalWrite(E220M0, HIGH);
			digitalWrite(E220M1, LOW);
		}
		else if (mode == 2)
		{
			digitalWrite(E220M0, LOW);
			digitalWrite(E220M1, HIGH);
		}
		else if (mode == 3)
		{
			digitalWrite(E220M0, HIGH);
			digitalWrite(E220M1, HIGH);
		}
		delay(40);
		waitCompleteResponse(5000);
	}
}
//////////////////////////////////////////////////////////////////////////////////////
// Comprobamos si tenemos datos del E220 a través del puerto serie y gestionamos
//////////////////////////////////////////////////////////////////////////////////////
bool RxE220()
{
	if (bitRead(errorE220, 7))
	{
		delay(250);                                                                   // nos aseguramos de tener todos los datos en el buffer para borrarlos
		cleanUARTBuffer();
		IniE220();                                                                    // reintentamos incializar modulo E220
		return false;
	}
	uint8_t x = 0;
	do
	{
		c[x] = Serial.read();                                                         // va leyendo carácter a carácter.
		delay(2);
		if (++x > MAX_Byte)
			x = MAX_Byte;
	} while (Serial.available());
	
	if (x != MAX_Byte)
	{
		bitClear(errorE220, 2);
		rssiRx = c[x-1];                                                              // en el último carácter se recibe el valor rssi, incluido por el modulo E220
	}
	else
	{
		bitSet(errorE220, 2);                                                         // xxxxx1xx->4
		delay(100);
		return false;
	}
	// Tramas validas, el último byte es el RSSI añadido por el modulo (según configuración)
	// Out1:1/0,Out2:1/0,Out3:1/0,Out4:1/0
	// Update:
	if ('U' == c[0])
	{
		if ('p' == c[1] && 'd' == c[2] && 'a' == c[3] && 't' == c[4] && 'e' == c[5] && ':' == c[6])
		{
			bitClear(errorE220, 3);
			return true;
		}
	}
	else if ('O' == c[0])
	{
		if ('u' == c[1] && 't' == c[2] && '1' == c[3] && ':' == c[4])
			digitalWrite(OUTPUT1, c[5]-48);
		if ('O' == c[7] && 'u' == c[8] && 't' == c[9] && '2' == c[10] && ':' == c[11])
			digitalWrite(OUTPUT2, c[12]-48);
		if ('O' == c[14] && 'u' == c[15] && 't' == c[16] && '3' == c[17] && ':' == c[18])
			digitalWrite(OUTPUT3, c[19]-48);
		if ('O' == c[21] && 'u' == c[22] && 't' == c[23] && '4' == c[24] && ':' == c[25])
			digitalWrite(OUTPUT4, c[26]-48);
		bitClear(errorE220, 3);
		return true;
	}
	bitSet(errorE220, 3);                                                             // xxxxx1xxx->8
	delay(100);
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////
// 9600Bps uart, 1 bit 104 * (1+8+1) = 1040us * 200 = 205ms
// 2400Bps en aire, 1 bit 417uS * (1+8+1) = 4170uS * 200 = 0,834Seg.
//////////////////////////////////////////////////////////////////////////////////////
bool TxDataWOR()
{
	ModeE220(1);                                                                      // WOR_TX-RX
	delay(3);
	if (bitRead(errorE220, 7))                                                        // 1xxxxxxx->128
	{
		delay(250);                                                                   // nos aseguramos de tener todos los datos en el buffer para borrarlos
		cleanUARTBuffer();
		IniE220();                                                                    // reintentamos incializar modulo E220
		return false;
	}
	if (digitalRead(E220AUX) == LOW)
	{
		waitCompleteResponse(1000);
	}
	else
	{
		if (digitalRead(E220AUX) == LOW)
		{
			bitSet(errorE220, 0);                                                     // xxxxxxx1->1
		}
		else
		{
			bitClear(errorE220, 0);
			bitClear(errorE220, 1);
			return true;
		}
	}
	ModeE220(2);
	bitSet(errorE220, 1);
	delay(100);
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////
// Limpiar buffer serial RX y array c[]
//////////////////////////////////////////////////////////////////////////////////////
void cleanUARTBuffer()
{
	while (Serial.available())
	{
		Serial.read();
	}
	memset(c, 0, sizeof(c));
}
//////////////////////////////////////////////////////////////////////////////////////
// Función de espera para estado OK E220
//////////////////////////////////////////////////////////////////////////////////////
uint8_t waitCompleteResponse(unsigned int waitNoAux)
{
	unsigned long t = millis();
	// comprobamos que millis() no se encuentre a puntó de desabordar
	if (((unsigned long)(t + waitNoAux)) == 0)
	{
		t = 0;
	}
	while (digitalRead(E220AUX) == LOW)
	{
		if ((millis() - t) > waitNoAux)
		{
			bitSet(errorE220, 4);                                                     // xxx1xxxx->16
			return 9;			                                                      // ERR_E220_TIMEOUT
		}
		delay(1);
	}
	bitClear(errorE220, 4);
	// El control después de que el pin aux pase a 1 es de 1-2ms
	delay(2 + (waitNoAux / 1000));
	return 1;                                                                         // E220_SUCCESS
}

#endif
