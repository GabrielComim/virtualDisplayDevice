# Virtual Display - ESP32 Example Firmware
This project is an example firmware for ESP32-S3 using ESP-IDF, designed to demonstrate how to integrate IoT devices with the Virtual Display application.

## 🎯 Purpose
The goal of this firmware is to show how an ESP32 device can:

Connect to an MQTT broker
Publish sensor data in JSON format. \
Communicate with the Virtual Display app in real time

## ⚙️ Hardware
ESP32-S3

Wich item showed in the app can be 3 types:
>Numerics; \
>Boolean; \
>String; \
example: Humidity(numerics), temperature(numerics), motors(numerics), led(bool), message(String), ... )

## 📡 Communication Protocol
All communication is done using MQTT with JSON. \
For configuration, it's where you send all items you want to see in the app. \
Basicly you need to send 2 type messages, the configuration and payload to update the values for wich item. \
About received, there are messages of the confirmation of operations and returns about some button if you configurate it.

## 📤 MQTT Topics
So, you should subscribe to the following topics as needed:

This topic receives a request from the app indicating that it does not yet have a valid configuration. It will not display anything on the main screen.
>**virtualDisplay/request_config** 

App returns that it received a valid configuration 
>**virtualDisplay/config_ack** 

If you are configuring button widgets that will be triggered by the app, you need to subscribe to the topic specified by the app—which will be distinguished by the title provided in this item, for example. 
>**virtualDisplay/button/lamp1** 

And the topics to submitting informations are:

For send to configuration: 
>**virtualDisplay/response_config** 

For send to payload: 
>**virtualDisplay/data** 

Explain about the configuration message: \ 
**device:**
  Name of your device;

**id:** 
  You should utilyze some id there are like the following list: \
    - speed; \
    - temperature; \
    - humidity; \
    - voltage; \
    - current; \
    - pressure; \
    - magnetic; \
    - level; \
    - detector; \
    - weight; \
    - other; \
    - gps; \
    - led; \
    - buzzer; \
    - alarm; \
    - message;

**type:**
Indicates if the data value is: number / bool / string. \
  If you using id: \
    - speed; \
    - temperature; \
    - humidity; \
    - voltage; \
    - current; \
    - pressure; \
    - magnetic; \
    - level; \
    - detector; \
    - weight; \
    - other; \
 Needs to use number. \
 If you use: \
    - gps; \
    - led; \
    - buzzer; \
 Needs to use bool. \
 And if you use message needs to use string. 

**title:**
  Name you desire appear in the display.

**decimal:** \
  Indicates how many decimal places should be considered.

**unit:** \
  The desired unit should appear after the number.

**min:** \
  Minimum value to considered.

**max:** \
  Maximum value to considered.

**history:** \
  It should create a graphics or not.

**value:** 
  Value for the item being configured.

## ▶️ Examples messages of the configuration and payload

```
Example message for configuration:
{
  {
  "device":"ESP32",
  "widgets": [
    {
      "id": "temperature",
      "type": "number",
      "title": "Sensor temp.",
      "decimal": "2",
      "unit": "celsius",
      "min": "-55.0",
      "max": "200",
      "history": "true",
      "value":"10"
    },
    {
      "id": "speed",
      "type": "number",
      "title": "Veloc.",
      "decimal": "1",
      "unit": "RPM",
      "min": "0",
      "max": "200",
      "history": "true",
      "value":"95"
    },
    {
      "id": "led",
      "type": "bool",
      "title": "Lamp. 1",
      "value":"true"
    },
    {
      "id": "gps",
      "type": "bool",
      "title": "Lamp. 2",
      "value":"false"
    },
    {
      "id": "message",
      "type": "string",
      "title": "Alertas",
      "value":"Erro na comunicação"
    }
  ]
}
}
```

Example message for payload:
```
{
  "values": {
    "Sensor temp.": "60.45",
    "Lamp.1": true,
    "Veloc.": "91.0",
    "Alertas": "Erro de comunicação"
  }
}
```

## Tip
If you want create another type differente of the items in the list types above I suggest to use type = other \
For each type in the configuration, the app personalize different icons to create a more intuitive and beautiful interface.

## 🧠 Integration with Virtual Display
This firmware is designed to work directly with the Virtual Display app, which:

Visualizes incoming MQTT data in real time \
Creates dashboards dynamically based on JSON structure \
Allows sending commands back to the device

## 🚀 Goal
Provide a reference firmware for developers who want to build IoT systems using:

ESP32 \
MQTT \
Virtual Display dashboard \

## 📌 License
Free to use for learning and development.
