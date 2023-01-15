/*************************************************************************************
Gestión de 4 entradas, 4 salidas y 4 entradas analógicas por un Arduino Nano y modulo Lora E220


V 0.0.2  15/1/23

 *     Creative Commons License Disclaimer
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
 ************************************************************************************/
#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/power.h>

#define E220Rxd 0                                                                     // pin nano RXD<--TXD E220
#define E220Txd 1                                                                     // pin nano TXD-->RXD E220
#define E220AUX 2
#define E220M0 3
#define INPUT1 4                                                                      // D4
#define INPUT2 5                                                                      // D5
#define INPUT3 6                                                                      // D6
#define INPUT4 7                                                                      // D7
#define E220M1 8
#define OUTPUT1 9
#define OUTPUT2 10
#define OUTPUT3 11
#define OUTPUT4 12
#define ADC1 A0
#define ADC2 A1
#define ADC3 A2
#define ADC4 A3
#define POWER A6

volatile uint8_t ISR = 0;

#include "e220.h"

void ReadAdc(void);
void DataJson(void);
uint8_t updateInput(void);

bool StatusD4 = 0, StatusD5 = 0, StatusD6 = 0, StatusD7 = 0;
String statusPower = "?", ReadADC1 = "?", ReadADC2 = "?", ReadADC3 = "?", ReadADC4 = "?";

/************************************************************************************/
// ISR para pines D4, D5, D6, y D7
/************************************************************************************/
ISR(PCINT2_vect)
{
  ISR = 1;
}
//////////////////////////////////////////////////////////////////////////////////////
void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(INPUT1, INPUT_PULLUP);
  pinMode(INPUT2, INPUT_PULLUP);
  pinMode(INPUT3, INPUT_PULLUP);
  pinMode(INPUT4, INPUT_PULLUP);
  pinMode(ADC1, INPUT);
  pinMode(ADC2, INPUT);
  pinMode(ADC3, INPUT);
  pinMode(ADC4, INPUT);
  pinMode(POWER, INPUT);
  pinMode(OUTPUT1, OUTPUT);
  pinMode(OUTPUT2, OUTPUT);
  pinMode(OUTPUT3, OUTPUT);
  pinMode(OUTPUT4, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(9600);
  delay(500);

  do
  {
    IniE220();                                                                        // inicializamos y modo WOR_RX
  }
  while (bitRead(errorE220, 7));
  // bits de habilitación de interrupción cambio estado del pines
  // Bit0 = 1 -> "PCIE0" enabeled (PCINT0 to PCINT7)
  // Bit1 = 1 -> "PCIE1" enabeled (PCINT8 to PCINT14)
  // Bit2 = 1 -> "PCIE2" enabeled D0-D7(PCINT16 to PCINT23)
  PCICR |= B00000100;
  // Bit4 = 1 -> "PCINT2" enabeled -> activará la interrupción de D4
  // Bit5 = 1 -> "PCINT2" enabeled -> activará la interrupción de D5
  // Bit6 = 1 -> "PCINT2" enabeled -> activará la interrupción de D6
  // Bit7 = 1 -> "PCINT2" enabeled -> activará la interrupción de D7
  PCMSK2 |= B11110000;
  cleanUARTBuffer();
  StatusD4 = digitalRead(INPUT1);
  StatusD5 = digitalRead(INPUT2);
  StatusD6 = digitalRead(INPUT3);
  StatusD7 = digitalRead(INPUT4);
  ReadAdc();
  digitalWrite(LED_BUILTIN, LOW);
}
//////////////////////////////////////////////////////////////////////////////////////
void sleepNow()
{
  digitalWrite(LED_BUILTIN, LOW);
  // Modos sleep:
  // SLEEP_MODE_IDLE - el modo de ahorro de energía más bajo, UART ON (unos 10mA)
  // SLEEP_MODE_ADC
  // SLEEP_MODE_PWR_SAVE
  // SLEEP_MODE_STANDBY
  // SLEEP_MODE_PWR_DOWN - el modo de ahorro de energía más alto (6 ciclos de reloj)
  set_sleep_mode(SLEEP_MODE_IDLE);

  sleep_enable(); 
// deshabilitamos periféricos NO usados en reposo
  power_adc_disable();
  power_spi_disable();
  power_timer0_disable();
  power_timer1_disable();
  power_timer2_disable();
  power_twi_disable();

  sleep_mode(); 

  sleep_disable(); 
// habilitamos todos los periféricos
  power_all_enable();
  
}
//////////////////////////////////////////////////////////////////////////////////////
void loop()
{ 
  // Gestión RX serial
  if (Serial.available())                                                             // tenemos datos serie?
  {
    digitalWrite(LED_BUILTIN, HIGH);
    if (RxE220() == false)                                                            // gestionamos trama RX y error en trama RX?
    {
      if (errorE220)                                                                  // error?
      {
        if (TxDataWOR())
        {
          Serial.print("RX_Error:" + String (errorE220));
          Serial.flush();
          delay(100);
          ModeE220(2);                                                                // WOR_RX
          errorE220 = 0;
        }
      }
      cleanUARTBuffer();
    }
    else                                                                              // OK RX-> Update o orden salidas
    {
      updateInput();
      if (TxDataWOR())
      {
        DataJson();
        delay(100);
        ModeE220(2);                                                                  // WOR_RX
      }
    }
  }
  // Gestión ISR
  if (ISR == 1)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);                                                                       // filtro rebotes
    if (updateInput() > 3)
    {
      if (TxDataWOR())
      {
        DataJson();
        delay(100);
        ModeE220(2);                                                                  // WOR_RX
      }
    }
    ISR = 0;
  }
  else
  {
    delay(1000);
    sleepNow();
  }
}
//////////////////////////////////////////////////////////////////////////////////////
uint8_t updateInput()
{
  ReadAdc();
  if (StatusD4 != digitalRead(INPUT1))
  {
    StatusD4 = digitalRead(INPUT1);
    ISR = 4;
  }
  if (StatusD5 != digitalRead(INPUT2))
  {
    StatusD5 = digitalRead(INPUT2);
    ISR = 5;
  }
  if (StatusD6 != digitalRead(INPUT3))
  {
    StatusD6 = digitalRead(INPUT3);
    ISR = 6;
  }
  if (StatusD7 != digitalRead(INPUT4))
  {
    StatusD7 = digitalRead(INPUT4);
    ISR = 7;
  }
  return ISR;
}
//////////////////////////////////////////////////////////////////////////////////////
float fmap(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
//////////////////////////////////////////////////////////////////////////////////////
void ReadAdc()
{
  char str[4];                                                                        // string 3 posiciones + \0
  uint16_t value = analogRead(ADC1);
  Serial.println(value);
  float x = fmap(value, 0, 1023, 0.0, 5.0);                                           
  dtostrf(x, 1, 1, str);
  ReadADC1 = String(str);

  value = analogRead(ADC2);
  x = fmap(value, 0, 1023, 0.0, 5.0);
  dtostrf(x, 1, 1, str);
  ReadADC2 = String(str);
  
  value = analogRead(ADC3);
  x = fmap(value, 0, 1023, 0.0, 5.0);
  dtostrf(x, 1, 1, str);
  ReadADC3 = String(str);

  value = analogRead(ADC4);
  x = fmap(value, 0, 1023, 0.0, 5.0);
  dtostrf(x, 1, 1, str);
  ReadADC4 = String(str);

  value = analogRead(POWER);
  x = fmap(value, 0, 1023, 0.0, 5.0);  
  x = x * 3.23;                                                                       // corregimos tensión según divisor resistivo      
  if (x < 10)                                   
    dtostrf(x, 1, 1, str);
  else
    dtostrf(x, 2, 1, str);
  statusPower = String(str);
}
//////////////////////////////////////////////////////////////////////////////////////
// Máximo 200 Bytes (de 172 a 181bytes, máximo 180 + rssi)
//////////////////////////////////////////////////////////////////////////////////////
void DataJson()
{
  Serial.print("{\"E220\":[{\"In1\":\"" + String(StatusD4 ? "off" : "on") +
               "\",\"In2\":\"" + String(StatusD5 ? "off" : "on") +
               "\",\"In3\":\"" + String(StatusD6 ? "off" : "on") +
               "\",\"In4\":\"" + String(StatusD7 ? "off" : "on") +
               "\",\"Out1\":\"" + String(digitalRead(OUTPUT1) ? "on" : "off") +
               "\",\"Out2\":\"" + String(digitalRead(OUTPUT2) ? "on" : "off") +
               "\",\"Out3\":\"" + String(digitalRead(OUTPUT3) ? "on" : "off") +
               "\",\"Out4\":\"" + String(digitalRead(OUTPUT4) ? "on" : "off") +
               "\",\"ADC1\":\"" + ReadADC1 + "\",\"ADC2\":\"" + ReadADC2 +
               "\",\"ADC3\":\"" + ReadADC3 + "\",\"ADC4\":\"" + ReadADC4 +
               "\",\"Power\":\"" + statusPower + "\"}]}");
  Serial.flush();
}
