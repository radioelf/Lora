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
comenzado a recibir el primer byte, también pasa a nivel bajo en la transmisión unos 2 ms antes de que
se inicie la transmisión vía radio, y vuelve a nivel alto finalizada la trasmisión de radio.

Si ponemos a 0 pin M1 y M0 se entra en modo “Normal”, puede recibir y transmitir
todos los datos del dispositivo A al B; esta modalidad se define como “Transmisión transparente”.
* E220       <----> ESP8266
* M0         <----- PIN GPIO4-D2
* M1         <----- PIN GPIO5-D1
* TX         -----> PIN GPIO13-D7 (PullUP)
* RX         <----- PIN GPIO12-D6 (PullUP)
* AUX        -----> PIN GPIO14-D5
* VCC        ----- 3.3v/5v
* GND        ----- GND
* consumo de unos  550mA en TX radio
Configuración: 9600Bps 8N1 puerto serie<->E220, 2400Bps a través del aire, byte RSSI habilitado
tamaño máximo del paquete 32Bytes
 9600Bps uart, 1 bit 104 * (1+8+1) = 1040us
 2400Bps en aire, 1 bit 417uS * (1+8+1) = 4170uS

leer configuración en modo sleep M0 y M1 a 1
comando->C1+dirección inicial->00+longitud->0b
TX->config: c1 00 0b
RX->config: c1 00 0b 72 e0 62 c0 12 83 00 00 21 0b 1e
byte 6: canal de radio
byte 9 y 10: dirección

Banderas error E220: errorE220
bit 0 transmisión
bit 1 recepción
bit 2 recepción sin datos
bit 3 trama RX No valida
bit 4 Timeout
bit 5 sin respuesta (update data)
bit 6 sin uso
bit 7 sin respuesta del modulo
* 

    Creative Commons License Disclaimer

  UNLESS OTHERWISE MUTUALLY AGREED TO BY THE PARTIES
  AND MAKES NO REPRESENTATIONS OR WARRANTIES OF ANY
  STATUTORY OR OTHERWISE, INCLUDING, WITHOUT LIMITAT
  FITNESS FOR A PARTICULAR PURPOSE, NONINFRINGEMENT,
  ACCURACY, OR THE PRESENCE OF ABSENCE OF ERRORS, WH
  DO NOT ALLOW THE EXCLUSION OF IMPLIED WARRANTIES,
  EXCEPT TO THE EXTENT REQUIRED BY APPLICABLE LAW, I
  ON ANY LEGAL THEORY FOR ANY SPECIAL, INCIDENTAL, C
  ARISING OUT OF THIS LICENSE OR THE USE OF THE WORK
  POSSIBILITY OF SUCH DAMAGES.

  http://creativecommons.org/licenses/by-sa/3.0/

  Author: Radioelf  http://radioelf.blogspot.com.es/

*/
#include <Arduino.h>
#include <SoftwareSerial.h>

#define E220Txd 12 																      // pin ESP3286 TXD-->RXD E220 modulo E220
#define E220Rxd 13 																      // pin ESP3286 RXD<--TXD E220 modulo E220
#define E220M0 4
#define E220M1 5
#define E220AUX 14

#define BAUD_RATE 9600 															      // velocidad del puerto serie E220 (defecto)
#define CHANNEL 18	   															      // canal radio 18 (cambiar según se use)
#define MAX_Byte 32	   															      // número máximo de byte para la trama recibida remotamente (Lora)

uint8_t errorE220 = 0;

SoftwareSerial swSerE220;

void ModeE220(uint8_t);
bool TxE220(String);
bool RxE220();
uint8_t waitCompleteResponse(unsigned int);
void cleanUARTBuffer();
//////////////////////////////////////////////////////////////////////////////////////
// Inicializamos modulo Lora E220 y comprobamos correcta comunicación (leemos el canal de radio)
//////////////////////////////////////////////////////////////////////////////////////
bool IniE220()
{
	pinMode(E220M0, OUTPUT);
	pinMode(E220M1, OUTPUT);
	pinMode(E220AUX, INPUT);
	// 9600Bps, 8N1 (10 bits),pin RXD,pin txd, NO invertido, 32 bytes para el buffer de RX
	swSerE220.begin(BAUD_RATE, SWSERIAL_8N1, E220Rxd, E220Txd, false, MAX_Byte);
	digitalWrite(LedPin, HIGH);
#if DebugSerial
	if (!swSerE220)
	{
		debugPrintln(F("ERROR los pines usados NO son validos"));
	}
	else
	{
		debugPrintln(F("Configuración de los pines TXD-RXD correctos"));
	}
#endif
	delay(500);
	cleanUARTBuffer();
	waitCompleteResponse(5000);
	ModeE220(3);                                                                      // pasamos a configuración
	if (digitalRead(E220AUX) == LOW)
	{
		waitCompleteResponse(1000);
	}
	swSerE220.write(0xc1); 															  // comando leer registros
	swSerE220.write(0x04); 															  // dirección inicial
	swSerE220.write(0x01); 															  // número de registros a leer
	byte message[] = {0, 0, 0, 0};
	uint8_t i = 0;
#if DebugSerial == 1
	debugPrintln(F("[E220] Comandos petición canal RF E220: 0xC1 0x04 0x01"));
#endif
	delay(10);                                                                        // 9600Bps ->104 * (1+8+1) = 1040us *4bytes = 4160Us
	waitCompleteResponse(5000);
	while (swSerE220.available() && i < 4)
	{
		byte x = swSerE220.read();                                                    // leemos carácter a carácter...
		message[i++] = x;
		yield();
	}
	if (message[3] == CHANNEL)
	{
		if (bitRead(errorE220, 7))
			bitClear(errorE220, 7);
#if DebugSerial
		debugPrintln(String(F("[E220] OK en respuesta: ")) + String(message[0], HEX) + " " + String(message[1], HEX) + " " + String(message[2], HEX) + " " + String(message[3], HEX));
#endif
		ModeE220(2);                                                                  // pasamos a RX + código despertar
		return true;
	}
	else
	{
#if DebugSerial
		debugPrintln(String(F("[E220] ERROR en respuesta: ")) + String(message[0], HEX) + " " + String(message[1], HEX) + " " + String(message[2], HEX) + " " + String(message[3], HEX));
#endif
		bitSet(errorE220, 7);
		return false;
	}
}
//////////////////////////////////////////////////////////////////////////////////////
/* Modos de trabajo para el modulo E220
	MODE_0_Transparente TX-RX	= 0  M0 GND, M1 GND
	MODE_1_WOR_TX-RX        	= 1  M0 VCC, M1 GND
	MODE_2_WOR_RX           	= 2  M0 GND, M1 VCC
	MODE_3_configuración-SLEEP 	= 3  M0 VCC, M1 VCC
*/
//////////////////////////////////////////////////////////////////////////////////////
void ModeE220(uint8_t mode)
{
	if (bitRead(errorE220, 7))
	{
#if DebugSerial == 1
		debugPrintln(F("[E220] Modo E220 NO INICIALIZADO"));
#endif
		return;
	}
	if (mode >= 0 || mode < 4)
	{
		delay(40);
#if DebugSerial == 1
		debugPrintln(F("[E220] Se ha cambiado al modo: "));
#endif
		if (mode == 0)
		{
			digitalWrite(E220M0, LOW);
			digitalWrite(E220M1, LOW);
#if DebugSerial == 1
			Serial.println(F("Transparente"));
#endif
		}
		else if (mode == 1)
		{
			digitalWrite(E220M0, HIGH);
			digitalWrite(E220M1, LOW);
#if DebugSerial == 1
			Serial.println(F("TX-RX + despertar (WOD)"));
#endif
		}
		else if (mode == 2)
		{
			digitalWrite(E220M0, LOW);
			digitalWrite(E220M1, HIGH);
#if DebugSerial == 1
			Serial.println(F("Desperar + RX (WOD)"));
#endif
		}
		else if (mode == 3)
		{
			digitalWrite(E220M0, HIGH);
			digitalWrite(E220M1, HIGH);
#if DebugSerial == 1
			Serial.println(F("Configuración-SLEEP"));
#endif
		}
		delay(40);
		waitCompleteResponse(5000);
	}
	else
	{
#if DebugSerial == 1
		debugPrintln(F("[E220] Modo E220 NO validos"));
#endif
	}
}
//////////////////////////////////////////////////////////////////////////////////////
// Comprobamos si tenemos datos del E220 a través del puerto serie y gestionamos
//////////////////////////////////////////////////////////////////////////////////////
bool RxE220()
{
	if (swSerE220.available() < 2)                                                    // mínimo 2 bytes recibidos en buffer serial
		return (false);
	if (bitRead(errorE220, 7))
	{
		delay(250); 													              // nos aseguramos de tener todos los datos en el buffer para borrarlos
		cleanUARTBuffer();
#if DebugSerial == 1
		debugPrintln(F("[E220] RX-Modo E220 NO INICIALIZADO"));
#endif
		IniE220(); 														              // re-intentamos inicializar modulo E220
		return false;
	}
	uint8_t count = MAX_Byte;
	digitalWrite(LedPin, HIGH);
	// Esperamos que el pin AUX se encuentre a 1
	do
	{
		delay(5);
		count--;
	} while (digitalRead(E220AUX) == LOW && count != 0);
	delay(2); 															              // nos aseguramos que no falte algún byte por recibir
	if (count == 0)
	{
#if DebugSerial == 1
		debugPrintln(F("[E220RX] ERROR en RX.."));
#endif
		bitSet(errorE220, 1);
		return false;
	}
	if (bitRead(errorE220, 1))
		bitClear(errorE220, 1);
	uint8_t nByte = swSerE220.available();
	String RxDataE220 = "";
	if (nByte != 0)
	{
		char c = 0;
		do
		{
			c = swSerE220.read(); 												      // va leyendo carácter a carácter.
			RxDataE220 += c;
			yield();
		} while (--nByte != 0);
		rssiRx = (uint8_t)c; 													      // en el último carácter se recibe el valor rssi, incluido por el modulo E220
	}
	else
	{
#if DebugSerial == 1
		debugPrintln(F("[E220RX] ERROR en RX SIN datos"));
#endif
		bitSet(errorE220, 2);
		return false;
	}
#if DebugSerial
	debugPrintln(String(F("[E220] RX: ")) + String(RxDataE220));
#endif
	if (bitRead(errorE220, 2))
		bitClear(errorE220, 2);
	contMqtt = 0; 																	  // borramos el contador de envío periódico de actualizar estado Lora remoto
//VVVVVVVVVVVVV CODIGO para la gestión de la trama siguiente: VVVVVVVVVVVVVVVVVVVVVVVV
	//  RX 19bytes->Bat:O/L,Door:1/0,rssi:1byte incluido por el modulo E220
	byte prevPos = RxDataE220.indexOf(':');							 	              // buscar la posición del primer dos puntos en la cadena
	String first = RxDataE220.substring(0, prevPos); 					              // primera cadena de caracteres antes del los dos puntos
	if ((String) "Bat" == first)
	{
		prevPos++;														              // desplazar el índice a siguiente posición
		byte currPos = RxDataE220.indexOf(':', prevPos);		 		              // almacenar dos posiciones.
		//battRx = RxDataE220.substring(prevPos, currPos).toInt(); 		              // extrae la subcadena y se convierte en un valor numérico
		statusBatt = RxDataE220.substring(prevPos + 1, prevPos);
		prevPos = currPos + 1;									 		              // desplazamiento de índice.
		currPos = RxDataE220.indexOf(':', prevPos);
		doorRx = (bool)RxDataE220.substring(prevPos, currPos).toInt(); 	              // extrae la subcadena y se convierte en un valor numérico
		doorRx ? statusDoor = "cerrada": statusDoor = "abierta";
//^^^^^^^^^^^^^^^^^^^^ MODIFICAR CÓDIGO para gestionar otras tramas!!^^^^^^^^^^^^^^^^^
#if DebugSerial
		// "O" ->nivel correcto (OK), "L" ->nivel bajo (Low)
		String battLevel = "baja";
		if (statusBatt == "O")
			battLevel = "OK";
		debugPrintln(String(F("[E220] datos recibidos: batería:")) + battLevel + ", puerta:" + statusDoor + ", RSSI:" + String(rssiRx));
#endif
		if (bitRead(errorE220, 3))
			bitClear(errorE220, 3);
		return true;
	}
#if DebugSerial
	debugPrintln(F("[E220] Trama RX NO valida"));
#endif
	bitSet(errorE220, 3);
	delay(100);
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
bool TxE220(String data)
{
	if (bitRead(errorE220, 7))
	{
		delay(250); 														          // nos aseguramos de tener todos los datos en el buffer para borrarlos
		cleanUARTBuffer();
#if DebugSerial == 1
		debugPrintln(F("[E220] TX-Modo E220 NO INICIALIZADO"));
#endif
		IniE220(); 															          // re-intentamos inicializar modulo E220
		return false;
	}
	bool TX = false;
	if (digitalRead(E220AUX) == LOW)
	{
		waitCompleteResponse(1000);
	}
	else
	{
		if (digitalRead(E220AUX) == LOW)
		{
#if DebugSerial
			debugPrintln(F("[E220] ERROR en TX"));
#endif
			bitSet(errorE220, 0);
		}
		else
		{
			swSerE220.print(data);
			if (bitRead(errorE220, 0))
				bitClear(errorE220, 0);
			TX = true;
#if DebugSerial
			debugPrintln(String(F("[E220] TX:")) + data);
#endif
		}
	}
	return TX;
}
//////////////////////////////////////////////////////////////////////////////////////
// Limpiar buffer serial RX
//////////////////////////////////////////////////////////////////////////////////////
void cleanUARTBuffer()
{
#if DebugSerial
	uint8_t count = 0;
#endif
	while (swSerE220.available())
	{
		swSerE220.read();
#if DebugSerial
		count++;
#endif
	}
#if DebugSerial
	debugPrintln(String(F("[E220] Borrados: ")) + count + " Bytes del buffer");
#endif
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
			bitSet(errorE220, 4);
#if DebugSerial
			debugPrintln(F("[E220] Timeout error!"));
#endif
      return 9;                                                                       // ERR_E220_TIMEOUT
    }
		yield();
	}
	if (bitRead(errorE220, 4))
		bitClear(errorE220, 4);
#if DebugSerial
	debugPrintln(F("[E220] Pin AUX a 1!"));
#endif
	// El control después de que el pin aux pase a 1 es de 1-2ms
  delay(2 + (waitNoAux / 1000));
#if DebugSerial
	debugPrintln(F("[E220] Finalizado AUX!"));
#endif
  return 1;                                                                           // E220_SUCCESS
}

#endif
