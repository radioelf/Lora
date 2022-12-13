/*
ESP8266<---->E220(lora)--->MQTT-->Home Assistant

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
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h> // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266HTTPClient
#include <ESP8266WebServer.h>
#include <PubSubClient.h>

//#define DebugSerial 1

#include "ESP8266_OTA.h"

// ESP8266 RX GPIO13<-- TX E220, ESP8266 TX GPIO12--> RX E220, M0 ->GPIO4, M1 ->GPIO5, AUX->GPIO14
// pin led estado (gpio16)
#define LedPin 16

// Configuración red WiFi
const char *ssid = "xxxxx";
const char *password = "xxxxxx";
//const char *ssid = "yyyyyy";
//const char *password = "yyyyyyy";
//  configuración IP estática
IPAddress staticIP(192, 168, 0, xx);
IPAddress gateway(192, 168, 0, xx);
IPAddress dnServer(192, 168, 0, xx);
IPAddress subnet(255, 255, 255, 0);
// Nombre del punto de acceso si falla la conexión a la red WiFi
#define nameAp "LoraEsp"
#define passAp "xxxxxxx"

// Uptime overflow
#define UPTIME_OVERFLOW 4294967295UL

const char *sofVersion = "0.1.3";
const String Compiler = String(__DATE__);                                             // Obtenemos la fecha de la compilación

bool APmode = false;
bool okMqtt = false;
bool locate = false;

// valor del error para mostrar en pantalla
uint8_t errorDisplay = 0;

// 4.5 minutos (<MQTT_KEEPALIVE)

HTTPClient http;
// servidor HTTP run
WiFiClient clientHttp;

String statusBatt = "?";
String statusDoor = "?";
uint8_t battRx = 0;
uint8_t rssiRx = 0;
uint16_t contMqtt = 89;
uint16_t updateE220 = 1520;
uint16_t TxUpdate = 0;
bool doorRx = false;
bool loraOKdata = false;
bool updateLora = false;

#include "mqtt.h"
#include "e220.h"
#include "http.h"

ADC_MODE(ADC_VCC)

void getUptime();
void espReset();

//////////////////////////////////////////////////////////////////////////////////////
// Configuración
//////////////////////////////////////////////////////////////////////////////////////
void setup()
{
  // configuramos GPIOs
  pinMode(LedPin, OUTPUT);
  digitalWrite(LedPin, LOW);

  delay(250);
#if DebugSerial
  Serial.begin(115200);
  delay(500);
  debugPrintln(String(F("\n\n<-------\n\n")));
  // 1=normal boot, 4=watchdog, 2=reset pin, 3=software reset
  debugPrintln(String(F("SYSTEM: Reinicio por: ")) + String(ESP.getResetInfo()));
#else
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#endif
#if DebugSerial == 1
  debugPrintln(String(F("SYSTEM: ID ESP: ")) + String(ESP.getChipId()));
  debugPrintln(String(F("SYSTEM: CPU frecuencia: ")) + String(ESP.getCpuFreqMHz()) + "MHz");
  debugPrintln(String(F("SYSTEM: Versión Core: ")) + String(ESP.getCoreVersion()));
  debugPrintln(String(F("SYSTEM: Versión SDK: ")) + String(ESP.getSdkVersion()));
  debugPrintln(String(F("SYSTEM: Versión: ")) + String(sofVersion));
  debugPrintln(String(F("SYSTEM: Compilado: ")) + Compiler);
#endif

  // Tamaño máximo de paquete MQTT
  client.setBufferSize(1024);
  client.setKeepAlive(300);
  // Después de reiniciar aún se podría conserva la conexión antigua
  WiFi.disconnect();
  // No se guardar SSID y contraseña
  WiFi.persistent(false);
  // Reconectar si se pierde la conexión
  WiFi.setAutoReconnect(true);
  // Intentamos conectarnos a la red WIFI y luego conectarse al servidor MQTT
  reConnect();
  InitServer();
  if (InitOTA())
  {
#if DebugSerial
    debugPrintln(F("[OTA] Inicializado"));
  }
  else
  {
    debugPrintln(F("[OTA] ERROR!!"));
#endif
  }
  if (IniE220())
  {
    digitalWrite(LedPin, LOW);
    ModeE220(1);                                                                      // TX-RX WOD, pasamos a TX + código despertar
#if DebugSerial
    debugPrintln(F("[E220] Inicializado"));
  }
  else
  {
    debugPrintln(F("[E220] ERROR!!"));
#endif
  }
}
uint8_t timeAPmode = 0;
uint16_t contCicle = 0;
//************************************************************************************
// Principal
//************************************************************************************
void loop()
{
  // Reconectar si se perdió la conexión wifi o mqtt
  if (!client.connected() || WiFi.status() != 3)
  {
    reConnect();
  }

  if (updateLora)
  {
    TxE220("U");                                                                      // TX->despertar modulo remoto + 0x85 + RSSI
    updateLora = false;
    if (++TxUpdate > 1)
      bitSet(errorE220, 5);
  }
  if (RxE220())
  {
    locate = true;
    loraOKdata = true;
    publicMqtt();
    contCicle = 0;
    TxUpdate = 0;
    loraOKdata = false;
    locate = false;
    if (bitRead(errorE220, 5))
      bitClear(errorE220, 5);
    digitalWrite(LedPin, LOW);
  }

  delay(10);
  if (bitRead(errorE220, 7) && (contCicle % 20 == 0))
  {
      digitalWrite(LedPin, !digitalRead(LedPin));
  }
  // algo más de 40 segundos
  if (contCicle == 2000)
  {
      getUptime();
  }
  // algo más de 40 segundos
  if (++contCicle == 4000)
  {
      contCicle = 0;
      publicMqtt();
  }
  // Mantenemos activa la conexión MQTT
  client.loop();
  // escuchamos las conexiones http entrantes
  server.handleClient();
  ArduinoOTA.handle();
}
//////////////////////////////////////////////////////////////////////////////////////
// Reseteamos ESP
//////////////////////////////////////////////////////////////////////////////////////
void espReset()
{
#if DebugSerial
  debugPrintln(F("RESET ESP8266"));
#endif
  if (client.connected())
    client.disconnect();
  WiFi.disconnect();
  ESP.restart();
  delay(5000);
}
//////////////////////////////////////////////////////////////////////////////////////
// Gestión modo AP
//////////////////////////////////////////////////////////////////////////////////////
void wifiAP(bool OnOff)
{
  if (OnOff == false)
  {
    delay(5000);
    espReset();
  }
  WiFi.mode(WIFI_AP);
  // Canal RF 6, ISSD ON, 1 conexión
  while (!WiFi.softAP(nameAp, passAp, 6, 0, 1))
  {
    delay(100);
  }
  APmode = true;
#if DebugSerial
  debugPrintln(String(F("Configurado modo AP, nombre: ")) + String(nameAp) + " Pass: " + String(passAp));
  debugPrintln(String(F("IP: ")) + WiFi.softAPIP().toString());                       // Dirección para el AP
#endif
}
//////////////////////////////////////////////////////////////////////////////////////
// Gestión conexión/reconexión Wifi + MQTT
//////////////////////////////////////////////////////////////////////////////////////
void reConnect()
{
  uint8_t conectOK = 0;
#ifndef DebugSerial
  digitalWrite(LED_BUILTIN, LOW);
#endif
  // Configuración en modo cliente
  WiFi.mode(WIFI_STA);
  WiFi.config(staticIP, dnServer, gateway, subnet);
  WiFi.begin(ssid, password);
  // Re-intentamos conectarse a wifi si se pierde la conexión
  if (WiFi.status() != WL_CONNECTED)
  {
    locate = false;
#if DebugSerial
    debugPrintln(String(F("Conectando a: ")) + String(ssid));
#endif

    if (APmode)
    {
      WiFi.softAPdisconnect();
    if (++timeAPmode == 100)
      espReset();
    }
    APmode = false;
    // permanecemos mientras esperamos la conexión
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
#if DebugSerial == 1
      debugPrintln(F("."));
#endif
      if (++conectOK == 100)
      {
        wifiAP(false);
        errorDisplay = 11;
        return;
      }
    }
    timeAPmode = 0;
    // Creamos el nombre del cliente basado en la dirección MAC y los últimos 3 bytes
    uint8_t mac[6];
    WiFi.macAddress(mac);
    clientId = "E220";
    clientId += "-" + String(mac[3], 16) + String(mac[4], 16) + String(mac[5], 16);
    clientId.toUpperCase();
#if DebugSerial == 1
    debugPrintln(String(F("WIFI: Conexión OK con IP: ")) + WiFi.localIP().toString());
    debugPrintln(String(F("WIFI: mascara de subred: ")) + WiFi.subnetMask().toString());
    debugPrintln(String(F("WIFI: gateway: ")) + WiFi.gatewayIP().toString());
    debugPrintln(String(F("WIFI: DNS: ")) + WiFi.dnsIP().toString());
    debugPrintln(String(F("WIFI: MAC ESP: ")) + WiFi.macAddress().c_str());
    debugPrintln(String(F("WIFI: HOST  http://")) + WiFi.hostname().c_str() + ".local");
    debugPrintln(String(F("WIFI: BSSID: ")) + WiFi.BSSIDstr().c_str());
    debugPrintln(String(F("WIFI: CH: ")) + WiFi.channel());
    debugPrintln(String(F("WIFI: RSSI: ")) + WiFi.RSSI());
#endif
  }
  // Conexión al broker MQTT
  // Cada mensaje MQTT puede ser enviado como un mensaje con retención (retained), en este caso cada
  // nuevo cliente que conecta a un topic recibirá el último mensaje retenido de ese tópico.
  // Cuando un cliente conecta con el Broker puede solicitar que la sesión sea persistente, en ese
  // caso el Broker almacena todas las suscripciones del cliente.
  // Un mensaje MQTT CONNECT contiene un valor keepAlive en segundos donde el cliente establece el
  // máximo tiempo de espera entre intercambio de mensajes
  // QOS
  // 0: El broker/cliente entregará el mensaje una vez, sin confirmación. (Baja/rápido)
  // 1: El broker/cliente entregará el mensaje al menos una vez, con la confirmación requerida. (Media)
  // 2: El broker/cliente entregará el mensaje exactamente una vez. (Alta/lento)
  okMqtt = false;
  conectOK = 25;
  if (WiFi.status() == WL_CONNECTED)
  {
#if DebugSerial == 1
    debugPrintln(String(F("Config MQTT: ")) + "clientID: " + clientId.c_str() + +" Broker: " + String(MQTT_SERVER) + " username: " + userMQTT + " password: " + passMQTT + " willTopic: " + String(willTopic) + " MQTT_QOS: " + String(MQTT_QOS) + " MMQTT_RETAIN: " + String(MQTT_RETAIN) + " willMessage: " + String(willMessage));
    debugPrintln("Intentando conectar a MQTT...");
#endif
    client.disconnect();
    client.setClient(espClient);
    client.setServer(MQTT_SERVER, MQTT_PORT);
    // Permanecemos mientras NO estemos conectados al servidor MQTT
    while (!client.connected())
    {
      //                              clientID          username  password  willTopic  willQoS,  willRetain,  willMessage,  cleanSession =1 (default)
      okMqtt = client.connect((char *)clientId.c_str(), userMQTT, passMQTT, willTopic, MQTT_QOS, MQTT_RETAIN, willMessage, 0);
      if (!okMqtt)
      {
        if (conectOK-- == 0)
          break;
        delay(2500);
        /* Respuesta a client.state()
          -4: MQTT_CONNECTION_TIMEOUT- el servidor no respondió dentro del periodo esperado
          -3: MQTT_CONNECTION_LOST- la conexión de red se interrumpió
          -2: MQTT_CONNECT_FAILED- la conexión de red falló
          -1: MQTT_DISCONNECTED- el cliente está desconectado limpiamente
          0: MQTT_CONNECTED el cliente está conectado
          1: MQTT_CONNECT_BAD_PROTOCOL el servidor no admite la versión solicitada de MQTT
          2: MQTT_CONNECT_BAD_CLIENT_ID el servidor rechazó el identificador del cliente
          3: MQTT_CONNECT_UNAVAILABLE- el servidor no pudo aceptar la conexión
          4: MQTT_CONNECT_BAD_CREDENTIALS- el nombre de usuario/contraseña NO validos
          5: MQTT_CONNECT_UNAUTHORIZED- el cliente no estaba autorizado para conectarse
        */
#if DebugSerial
        debugPrintln(String(F("Fallo al conectar al broker, rc=")) + String(client.state()) + " intentando conectar en 5 segundos, " + String(conectOK));
#endif
      }
    }
    if (okMqtt)
    {
      publicMqtt();
      const String topicCmd = clientId + "/cmd/#";
      client.subscribe((char *)topicCmd.c_str(), 0);
      // para eliminar suscripción
      //const String topicClear = clientId + "/door/#";
      // client.unsubscribe((char *)topicClear.c_str());
      // Mensaje a enviar en caso de:
      // Un error I/O o fallo de red detectado por el servidor.
      // Un cliente falla al comunicarse dentro del intervalo Keep Alive configurado.
      // Un cliente cierra la conexión de red sin primero enviar el paquete DISCONNECT.
      // El servidor cierra la conexión de red debido a un error de protocolo.
      //  RETAIN ON
      const String topicLWT = clientId + "/LWT";
      client.publish((char *)topicLWT.c_str(), (char *)"Online", true);
#if DebugSerial
      debugPrintln("Conectado a MQTT");
      debugPrintln("Publicación de Topics enviada");
      debugPrintln(String(F("Subscrito a:")) + topicCmd);
      debugPrintln("Suscripción a topics enviada");
#else
      digitalWrite(LED_BUILTIN, HIGH);
#endif
    }
    else
    {
      espReset();
    }
  }
}
//////////////////////////////////////////////////////////////////////////////////////
// Obtenemos el periodo en marcha en segundos
//////////////////////////////////////////////////////////////////////////////////////
void getUptime()
{
  static unsigned long last_uptime = 0;
  static unsigned long ToUpdate = 0;
  static unsigned char uptime_overflows = 0;

  if (millis() < last_uptime)
    ++uptime_overflows;
  last_uptime = millis();
  unsigned long uptime_seconds = uptime_overflows * (UPTIME_OVERFLOW / 1000) + (last_uptime / 1000);
  // Reset cada dos dias
   if (uptime_seconds > 172799) espReset();
  String GetT = String(uptime_seconds);
  // 2 minutos (40seg. * 3 = 120Seg.)
  if (++contMqtt % 3 == 0)
  {
    String message;
    uint16_t messageLeng;
    message = String(F("/sensor/uptime/state"));
    messageLeng = message.length();
    char msg_t[messageLeng];
    message.toCharArray(msg_t, (messageLeng + 1));
    mqttSend(msg_t, (char *)GetT.c_str());
  }
  // 6 veces al día (40seg. * 360 =14400Seg.->240 minutos)
  if (contMqtt >= 360)
  {
    contMqtt = 1;
    contCicle = 0;
    updateLora = true;
    // comprobamos que uptime_seconds no desborde
    if (uptime_seconds + 1520 == 0)
    {
      ToUpdate = 1520;
    }
    else
    {
      ToUpdate = uptime_seconds + 1520;
    }
#if DebugSerial
      debugPrintln(F("[E220] Actualización periódica Lora"));
#endif
  }
  if (uptime_seconds <= ToUpdate && ToUpdate != 0)
  {
    updateE220 =  ToUpdate - uptime_seconds;
#if DebugSerial
    debugPrintln(String(F("Actualización E220 en: ")) + String (updateE220));
#endif
  }
}
#if DebugSerial
//////////////////////////////////////////////////////////////////////////////////////
// Enviamos trama de debugger
//////////////////////////////////////////////////////////////////////////////////////
void debugPrintln(String debugText)
{
  yield();
  noInterrupts();
#if DebugSerial == 1
  String debugTimeText = "[+" + String(float(millis()) / 1000, 3) + "s] " + debugText;
  Serial.println(debugTimeText);
#else
  Serial.println(debugText);
#endif
  interrupts();
  yield();
  // Esperar hasta que el uart emita la interrupción TXComplete
  Serial.flush();
}

#endif
