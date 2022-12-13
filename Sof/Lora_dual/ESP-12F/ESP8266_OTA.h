/*
Configuración OTA

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

#ifndef ESP8266_OTA_H
#define ESP8266_OTAP_H

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

bool InitOTA() {
  // ArduinoOTA.setPort(8266);                                            // Puerto por defecto 8266
  ArduinoOTA.setHostname("Lora_E220");                                    // Nombre para el módulo ESP8266 al descubrirlo por el Ide de Arduino
  ArduinoOTA.setPassword("xxxxxxxx");                                     // 

  ArduinoOTA.onStart([]() {
  });

  // ArduinoOTA.onEnd([]() {});

  // ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) { });

  ArduinoOTA.onError([](ota_error_t error) {
    return 0;
  });

  ArduinoOTA.begin();
  return 1;
}

#endif
