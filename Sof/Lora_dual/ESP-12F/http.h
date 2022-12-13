#ifndef _HTTP_H_
#define _HTTP_H_

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

ESP8266WebServer server(80);
//////////////////////////////////////////////////////////////////////////////////////
// Se ejecutara en la URL '/'
//////////////////////////////////////////////////////////////////////////////////////
void handleRoot()
{
    if (APmode)
    {
        int8_t numSsid;
        server.send(200, "text/html", "Servidor Http Lora E220 en modo punto de acceso<br>)");
        numSsid = WiFi.scanNetworks();
        if (numSsid <= 0)
        {
            server.send(200, "text/html", "NO SE ENCONTRARON PUNTOS DE ACCESOS)");
            return;
        }
        for (uint8_t i = 0; i < numSsid; i++)
        {
            server.send(200, "text/html", "AP: " + WiFi.SSID(i) + "RSSI: " + WiFi.RSSI(i) + "<br>");
        }
        return;
    }
    server.send(200, "text/html", "Servidor Http Lora E220 (<a href=\"http://" + WiFi.localIP().toString() + "/data/json\">Obtener datos Json</a>)");
}
//////////////////////////////////////////////////////////////////////////////////////
// Se ejecutara en la URL '/data/json'
//////////////////////////////////////////////////////////////////////////////////////
void handleJson ()
{
    if (APmode)
    {
        server.send(200, "text/html", "Acceso NO valido, en modo punto de acceso)");
        return;
    }
    String response = "{";
    response += "\"LoraE220\":[{";
    response += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
    response += ",\"gw\":\"" + WiFi.gatewayIP().toString() + "\"";
    response += ",\"mac\":\"" + WiFi.macAddress() + "\"";
    response += ",\"ssid\":\"" + String(WiFi.RSSI()) + "\"";
    response += ",\"ID\":\"" + clientId + "\"";
    response += ",\"conectado\":\"" + String(locate) + "\"";
    response += ",\"vpp\":\"" + String(ESP.getVcc() / 1023.0F) + "\"";
    if (statusBatt == "?")
        response += ",\"bateria_remota\":\"" + String(statusBatt) + "\"";
    else
        response += ",\"bateria_remota\":\"" + String(statusBatt == "O" ? "OK" : "baja") + "\"";
    response += ",\"puerta_remota\":\"" + statusDoor + "\"";
    response += ",\"RSSI-E220\":\"" + String(rssiRx) + "\"";
    if (rssiRx)
    {
        response += ",\"RSSI-E220_%\":\"" + String(map(rssiRx, 0, 255, 0, 100)) + "\"";
        response += ",\"RSSI-E220_dBm\":\"-" + String(256 - rssiRx) + "\"";
    }
    response += ",\"UpdateTime_E220\":\"" + String(updateE220) + "\"";
    response += ",\"UpdateTX_E220\":\"" + String(TxUpdate) + "\"";
    response += ",\"Error_E220\":\"" + String(errorE220) + "\"";
    response += "}]}";
    server.send(200, "application/json", response);

#if DebugSerial == 1
    debugPrintln(String(F("TX Json por servidor HTTP")));
#endif
}

// Se ejecutara si la URL es desconocida
void handleNotFound()
{
    server.send(404, "application/json", "{\"message\":\"NO encontrada\"}");
#if DebugSerial == 1
    debugPrintln(String(F("URL desconocida")));
#endif
}

void InitServer()
{
    // Ruteo para '/'
    server.on("/", handleRoot);
    // Ruteo para data/json
    server.on("/data/json", handleJson);
    // Ruteo para URL desconocida
    server.onNotFound(handleNotFound);
    // Iniciar servidor
    server.begin();
#if DebugSerial == 1
    debugPrintln(String(F("Servidor HTTP inicializado")));
#endif
}
#endif
