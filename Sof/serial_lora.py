'''
En apps.yaml
serial_lora:
    module: serial_lora
    class: SerialApp
    port: /dev/ttyUSB0
    baudrate: 9600
'''
import appdaemon.plugins.hass.hassapi as hass
import serial
import json
from serial.tools import list_ports
import time

class SerialApp(hass.Hass):

    def initialize(self):
        self.ser = None
        self.status = True
        # Verificación previa del puerto serie especificado
        ports = serial.tools.list_ports.comports()
        ports_list = [port.device for port in ports]
        if '/dev/ttyUSB0' not in ports_list:
            self.status = False
            self.log("El puerto serie no esta disponible")
        else:
            self.log("El puerto serie OK")

        # Abrir conexión serie en el puerto especificado en apps.yaml
        self.ser = serial.Serial(port=self.args["port"], baudrate=self.args["baudrate"])
        # Escucha el cambio de estado del sensor a on se llama a self.send_serial
        self.listen_state(self.send_serial, "switch.update_lora", new = "on")
        # Escucha el cambio de estado del sensor y se ejecuta el metodo cuando ocurre el cambio
        self.listen_state(self.handle_state, "switch.rele_lora1")
        #self.listen_state(self.handle_state, "switch.rele_lora1", value = 1)
        # Escucha el cambio de estado del sensor y se ejecuta el metodo cuando ocurre el cambio
        self.listen_state(self.handle_state, "switch.rele_lora2")
        #self.listen_state(self.handle_state, "switch.rele_lora2", value = 2)
        # Escucha el cambio de estado del sensor y se ejecuta el metodo cuando ocurre el cambio
        self.listen_state(self.handle_state, "switch.rele_lora3")
        #self.listen_state(self.handle_state, "switch.rele_lora3", value = 3)
        # Escucha el cambio de estado del sensor y se ejecuta el metodo cuando ocurre el cambio
        self.listen_state(self.handle_state, "switch.rele_lora4")
        #self.listen_state(self.handle_state, "switch.rele_lora4", value = 4)
        #Iniciar el servicio RX
        self.run_every(self.read_serial, self.datetime(), 5)
        self.log("---****INICIALIZADO LORA-SERIAL****---")

    def try_connect(self):
        if not self.status:
            while True:
                try:
                    self.ser = serial.Serial(port=self.args["port"], baudrate=self.args["baudrate"])
                    self.log("Dispositivo re-conectado")
                    self.status = True
                    break
                except serial.serialutil.SerialException:
                    self.log("Dispositivo desconectado, -STOP-")
                    time.sleep(5)

    def send_serial(self, entity, attribute, old, new, kwargs):
        if self.status:
            self.ser.write(b"Update:")
            self.log("El valor 'Update' ha sido enviado a traves del puerto serie")
            time.sleep(3)
            self.call_service("switch/turn_off", entity_id = "switch.update_lora")

    # RX ->{"E220":[{"In1":"off","In2":"on","In3":"off","In4":"on","
    #       Out1":"on","Out2":"off","Out3":"on","Out4":"off","
    #       ADC1":"123","ADC2":"654","ADC3":"413","ADC4":"831","Power":"8.7"}]}
    def read_serial(self, kwargs):
        if self.ser.in_waiting > 100:
            # Leer los datos disponibles
            json_data = self.ser.read(self.ser.in_waiting-1).decode()
            rssi = self.ser.read()
            int_rssi = int.from_bytes(rssi, byteorder='big')
            string_rssi = str(int_rssi)
            # Mostrar los datos recibidos
            self.log("Datos recibidos: {}".format(json_data))
            self.log("RSSI: " + string_rssi)
            data = json.loads(json_data)
            if "E220" == list(data.keys())[0]:
                self.log("Trama RX correcta") 
                #entradas -binary_sensor-
                self.set_state("binary_sensor.entrada1_lora", state = data["E220"][0]["In1"], attributes = {"icon": "mdi:import"})
                self.set_state("binary_sensor.entrada2_lora", state = data["E220"][0]["In2"], attributes = {"icon": "mdi:import"})
                self.set_state("binary_sensor.entrada3_lora", state = data["E220"][0]["In3"], attributes = {"icon": "mdi:import"})
                self.set_state("binary_sensor.entrada4_lora", state = data["E220"][0]["In4"], attributes = {"icon": "mdi:import"})
                self.log("Entrada 1 a: " + data["E220"][0]["In1"])
                self.log("Entrada 2 a: " + data["E220"][0]["In2"])
                self.log("Entrada 3 a: " + data["E220"][0]["In3"])
                self.log("Entrada 4 a: " + data["E220"][0]["In4"])
                # salidas -switch-
                self.set_state("switch.rele_lora1", state = data["E220"][0]["Out1"], attributes = {"icon": "mdi:electric-switch"})
                self.set_state("switch.rele_lora2", state = data["E220"][0]["Out2"], attributes = {"icon": "mdi:electric-switch"})
                self.set_state("switch.rele_lora3", state = data["E220"][0]["Out3"], attributes = {"icon": "mdi:electric-switch"})
                self.set_state("switch.rele_lora4", state = data["E220"][0]["Out4"], attributes = {"icon": "mdi:electric-switch"})
                self.log("Salida Rele 1: " + data["E220"][0]["Out1"])
                self.log("Salida Rele 2: " + data["E220"][0]["Out2"])
                self.log("Salida Rele 3: " + data["E220"][0]["Out3"])
                self.log("Salida Rele 4: " + data["E220"][0]["Out4"])
                # Analogicas -sensor-
                self.set_state("sensor.sensor_adc_lora1", state = data["E220"][0]["ADC1"], attributes = {"icon": "mdi:poll"})
                self.set_state("sensor.sensor_adc_lora2", state = data["E220"][0]["ADC2"], attributes = {"icon": "mdi:poll"})
                self.set_state("sensor.sensor_adc_lora3", state = data["E220"][0]["ADC3"], attributes = {"icon": "mdi:poll"})
                self.set_state("sensor.sensor_adc_lora4", state = data["E220"][0]["ADC4"], attributes = {"icon": "mdi:poll"})
                self.set_state("sensor.sensor_power_lora", state = data["E220"][0]["Power"], attributes = {"icon":"mdi:meter-electric-outline"})
                self.set_state("sensor.sensor_rssi_lora", state = string_rssi, attributes = {"icon": "mdi:signal-variant"})
                self.log("Entrada ADC 1: " + data["E220"][0]["ADC1"])
                self.log("Entrada ADC 2: " + data["E220"][0]["ADC2"])
                self.log("Entrada ADC 3: " + data["E220"][0]["ADC3"])
                self.log("Entrada ADC 4: " + data["E220"][0]["ADC4"])
                self.log("Entrada alimentacion: " + data["E220"][0]["Power"])
                self.log("Nivel RF: " + string_rssi)
        else:
            if self.ser.in_waiting > 0:
                # Leer los datos disponibles
                clear = self.ser.read(self.ser.in_waiting)  
                self.log("Error RX: " + str (clear))
                #ultimo rssi incluido por el modulo
    
    # Out1:1/0,Out2:1/0,Out3:1/0,Out4:1/0
    def handle_state(self, attribute, entity, new_state, old_state, pin_app):
        relay1 = self.get_state("switch.rele_lora1")
        relay2 = self.get_state("switch.rele_lora2")
        relay3 = self.get_state("switch.rele_lora3")
        relay4 = self.get_state("switch.rele_lora4")

        if relay1 == "on":
            relay1 = "1"
            self.log("rele 1 a ON")
            self.call_service("switch/turn_on", entity_id = "switch.rele_lora1")
        elif relay1 == "off":
            relay1 = "0"
            self.log("rele 1 a OFF")
            self.call_service("switch/turn_off", entity_id = "switch.rele_lora1")
        
        if relay2 == "on":
            relay2 = "1"
            self.log("rele 2 a ON")
            self.call_service("switch/turn_on", entity_id = "switch.rele_lora2")
        elif relay2 == "off":
            relay2 = "0"
            self.log("rele 2 a OFF")
            self.call_service("switch/turn_off", entity_id = "switch.rele_lora2")

        
        if relay3 == "on":
            relay3 = "1"
            self.log("rele 3 a ON")
            self.call_service("switch/turn_on", entity_id = "switch.rele_lora3")
        elif relay3 == "off":
            relay3 = "0"
            self.log("rele 3 a OFF")
            self.call_service("switch/turn_off", entity_id = "switch.rele_lora3")
        
        if relay4 == "on":
            relay4 = "1"
            self.log("rele 4 a ON")
            self.call_service("switch/turn_on", entity_id = "switch.rele_lora4")
        elif relay4 == "off":
            relay4 = "0"
            self.log("rele 4 a OFF")
            self.call_service("switch/turn_off", entity_id = "switch.rele_lora4")

        tx_relay = "Out1:" + relay1 + ",Out2:" + relay2 + ",Out3:" + relay3 + ",Out4:" + relay4
        # Envia el string 'tx_relay' codificado a bytes
        self.ser.write(tx_relay.encode())
        self.log(tx_relay)
