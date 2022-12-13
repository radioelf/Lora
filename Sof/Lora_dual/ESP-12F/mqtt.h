#ifndef _MQTT_H_
#define _MQTT_H_

/*
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

// MQTT
// Dirección IP del servidor MQTT
const char *MQTT_SERVER = "192.168.0.xxx";
// MQTT puerto broker
const uint16_t MQTT_PORT = 1883;
const char *userMQTT = "xxxxxx";
const char *passMQTT = "xxxxxx";
String clientId = "";
// MQTT bandera retain
bool MQTT_RETAIN = false;
// MQTT QoS mensajes
uint8_t MQTT_QOS = 0;
// MQTT willTopic
const char *willTopic = 0;
// MQTT willMessage
const char *willMessage = 0;

void reConnect();
void debugPrintln(String debugText);
void espReset();
void callback(char *topic, byte *payload, unsigned int length);

// Instancia a objetos
WiFiClient espClient;

PubSubClient client(MQTT_SERVER, MQTT_PORT, callback, espClient);

//////////////////////////////////////////////////////////////////////////////////////
// Publicamos en el broker MQTT
//////////////////////////////////////////////////////////////////////////////////////
void mqttSend(String topic, char *topublish)
{
    if (client.connected())
    {
        topic = clientId + topic;
        client.publish((char *)topic.c_str(), topublish, MQTT_RETAIN);
#if DebugSerial == 1
        debugPrintln(String(F("Publicado topic: ")) + topic + " valor: " + topublish);
#endif
    }
    else
    {
#if DebugSerial
        debugPrintln(String(F("Fallo en conexión con el broker, rc=")) + String(client.state()) + " intentando RE-conectar");
#endif
        reConnect();
    }
}

//////////////////////////////////////////////////////////////////////////////////////
// Publicación MQTT
//////////////////////////////////////////////////////////////////////////////////////
void publicMqtt()
{
    static uint8_t contLocate = 0;
    char payload[6];
    if (contLocate % 3 == 0 || locate == false)
    {
        mqttSend("/status", (char *)"1");
        mqttSend("/app", (char *)"Radioelf");
        mqttSend("/version", (char *)sofVersion);
        mqttSend("/board", (char *)"LORA1_RADIOELF");
        mqttSend("/host", (char *)clientId.c_str());
        mqttSend("/desc", (char *)"LORA_E220");
        mqttSend("/ssid", (char *)ssid);
        mqttSend("/ip", (char *)WiFi.localIP().toString().c_str());
        mqttSend("/mac", (char *)WiFi.macAddress().c_str());
        snprintf(payload, 5, "%d", WiFi.RSSI());
        mqttSend("/rssi", payload);
        // snprintf(payload, 6, "%d", ESP.getVcc());
        String Vcc = String(ESP.getVcc() / 1023.0F);
        mqttSend("/vcc", (char *)Vcc.c_str());
        // solo obtenemos 45 caracteres de la respuesta de ESP.getResetInfo()
        String infoReset = ESP.getResetInfo().substring(0, 45);
        mqttSend("/reset", (char *)infoReset.c_str());
    }

    if (locate)
    {
        if (loraOKdata)
        {
            // door: on significa abierta, off significa cerrada
            mqttSend("/door", doorRx ? (char *)"off" : (char *)"on");
            // battery: on significa baja, off significa normal
            mqttSend("/battery", statusBatt == "O" ? (char *)"off" : (char *)"on");
            snprintf(payload, 5, "%d", rssiRx);
            mqttSend("/sensor/rssi/state", payload);
         }
        else if (TxUpdate > 2)
        {
            mqttSend("/sensor/rssi/state", (char *)"unavailable");
        }
    }
    mqttSend("/status", okMqtt ? (char *)"online" : (char *)"offline");
    if (++contLocate == 120)
    {
        contLocate = 0;
        locate = false;
    }
    if (!locate)
    {
        // Auto localización para Home Assistant <discovery_prefix>/<component>/[<node_id>/]<object_id>/config
        String message = "{";
        message += String(F("\"stat_t\":\"")) + String(clientId) + String(F("/door\",\""));
        message += String(F("name\":\"Door\",\""));
        message += String(F("unique_id\":\"")) + String(clientId) + String(F("_door\",\""));
        message += String(F("dev_cla\":\"door\",\"pl_on\":\"on\",\"pl_off\":\"off\",\""));
        message += String(F("pl_avail\":\"on\",\"pl_not_avail\":\"off\",\""));
        message += String(F("device\":{\"identifiers\":\"")) + String(clientId) + String(F("\",\""));
        message += String(F("name\":\"LoraE220\",\"sw_version\":\"E220 ")) + String(sofVersion) + "-" + Compiler + String(F("\",\"manufacturer\":\"Radioelf\",\"model\":\"E220Lora1\"}"));
        message += "}";

        String topic = ("homeassistant/binary_sensor/" + clientId + "_door/config");
        uint16_t messageLeng = message.length();
        char msg[messageLeng];
        message.toCharArray(msg, (messageLeng + 1));
        client.publish((char *)topic.c_str(), msg, messageLeng);

        message = "{";
        message += String(F("\"stat_t\":\"")) + String(clientId) + String(F("/battery\",\""));
        message += String(F("name\":\"Baterry\",\""));
        message += String(F("unique_id\":\"")) + String(clientId) + String(F("_battery\",\""));
        message += String(F("dev_cla\":\"battery\",\"pl_on\":\"on\",\"pl_off\":\"off\",\""));
        message += String(F("pl_avail\":\"on\",\"pl_not_avail\":\"off\",\""));
        message += String(F("device\":{\"identifiers\":\"")) + String(clientId) + String(F("\",\""));
        message += String(F("name\":\"LoraE220\",\"sw_version\":\"E220 ")) + String(sofVersion) + "-" + Compiler + String(F("\",\"manufacturer\":\"Radioelf\",\"model\":\"E220Lora1\"}"));
        message += "}";

        String topic_batt = ("homeassistant/binary_sensor/" + clientId + "_battery/config");
        uint16_t messageLeng_batt = message.length();
        char msg_batt[messageLeng_batt];
        message.toCharArray(msg_batt, (messageLeng_batt + 1));
        client.publish((char *)topic_batt.c_str(), msg_batt, messageLeng_batt);

        message = "{";
        message += String(F("\"stat_t\":\"")) + String(clientId) + String(F("/status\",\""));
        message += String(F("name\":\"SYS: Connectivity_")) + String(clientId) + String(F("\",\""));
        message += String(F("unique_id\":\"")) + String(clientId) + String(F("_connectivity\",\""));
        message += String(F("dev_cla\":\"connectivity\",\"pl_on\":\"online\",\"pl_off\":\"offline\",\""));
        message += String(F("pl_avail\":\"online\",\"pl_not_avail\":\"offline\",\""));
        message += String(F("device\":{\"identifiers\":\"")) + String(clientId) + String(F("\",\""));
        message += String(F("name\":\"LoraE220\",\"sw_version\":\"E220 ")) + String(sofVersion) + "-" + Compiler + String(F("\",\"manufacturer\":\"Radioelf\",\"model\":\"E220Lora1\"}"));
        message += "}";

        String topic_Conect = ("homeassistant/binary_sensor/" + clientId + "_connectivity/config");
        uint16_t messageLeng_Status = message.length();
        char msg_Status[messageLeng_Status];
        message.toCharArray(msg_Status, (messageLeng_Status + 1));
        client.publish((char *)topic_Conect.c_str(), msg_Status, messageLeng_Status);

        message = "{";
        message += String(F("\"unit_of_measurement\":\"")) + "" + String(F("\",\"icon\":\"mdi:")) + "signal-variant" + String(F("\",\""));
        message += String(F("name\":\"")) + "RSSI" + String(F("\",\""));
        message += String(F("state_topic\":\"")) + String(clientId) + String(F("/sensor/")) + "rssi" + String(F("/state\",\""));
        message += String(F("availability_topic\":\"")) + String(clientId) + String(F("/status\",\""));
        message += String(F("uniq_id\":\"")) + String(clientId) + "LoraRssi" + String(F("\",\""));
        message += String(F("device\":{\"identifiers\":\"")) + String(clientId) + String(F("\",\""));
        message += String(F("name\":\"LoraE220\",\"sw_version\":\"E220 ")) + String(sofVersion) + "-" + Compiler + String(F("\",\"manufacturer\":\"Radioelf\",\"model\":\"E220Lora1\"}"));
        message += "}";

        String topic_sensors = ("homeassistant/sensor/" + clientId + "/" + "rssi" + "/config");
        uint16_t messageLeng_sensors = message.length();
        char msg_sensors[messageLeng_sensors];
        message.toCharArray(msg_sensors, (messageLeng_sensors + 1));
        client.publish((char *)topic_sensors.c_str(), msg_sensors, messageLeng_sensors);

        message = "{";
        message += String(F("\"unit_of_measurement\":\"")) + "seg" + String(F("\",\"icon\":\"mdi:")) + "timeline-clock" + String(F("\",\""));
        message += String(F("name\":\"")) + "E220_Uptime" + String(F("\",\""));
        message += String(F("state_topic\":\"")) + String(clientId) + String(F("/sensor/")) + "uptime" + String(F("/state\",\""));
        message += String(F("availability_topic\":\"")) + String(clientId) + String(F("/status\",\""));
        message += String(F("uniq_id\":\"")) + String(clientId) + "E220 Uptime" + String(F("\",\""));
        message += String(F("device\":{\"identifiers\":\"")) + String(clientId) + String(F("\",\""));
        message += String(F("name\":\"LoraE220\",\"sw_version\":\"E220 ")) + String(sofVersion) + "-" + Compiler + String(F("\",\"manufacturer\":\"Radioelf\",\"model\":\"E220Lora1\"}"));
        message += "}";

        String topic_upTime = ("homeassistant/sensor/" + clientId + "/" + "uptime" + "/config");
        uint16_t messageLeng_upTime = message.length();
        char msg_upTime[messageLeng_upTime];
        message.toCharArray(msg_upTime, (messageLeng_upTime + 1));
        locate = client.publish((char *)topic_upTime.c_str(), msg_upTime, messageLeng_upTime);

#if DebugSerial == 1
        debugPrintln("Auto localización Home Assistant");
#endif
    }
    // client.endPublish();
}
//////////////////////////////////////////////////////////////////////////////////////
// Gestionamos la recepción de los topic recibidos
// topic:
// LORA-xxxxxx/cmd restartCmd
// LORA-xxxxxx/cmd updateCmd
// LORA-xxxxxx/cmd updateLora
//////////////////////////////////////////////////////////////////////////////////////
void callback(char *topic, byte *payload, unsigned int length)
{
    payload[length] = '\0'; // Añadimos null al final
    String recv_payload = String((char *)payload);
#if DebugSerial == 1
    debugPrintln(String(F("Topic recibido :[")) + String(topic) + "] payload " + recv_payload + " Longitud: " + String(length));
#endif
    // Comando reset?
    if (!strncmp((char *)payload, "restartCmd", length))
        espReset();
    // Comando actualizar lora
    if (!strncmp((char *)payload, "updateLora", length))
    {
        updateLora = true;
        return;
    }
    // Comando actualizar MQTT
    if (!strncmp((char *)payload, "updateCmd", length))
    {
        locate = false;
        publicMqtt();
        return;
    }
    return;
}
//////////////////////////////////////////////////////////////////////////////////////
#endif
