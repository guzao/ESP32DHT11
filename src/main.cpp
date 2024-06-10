
#include <DHT.h>
#include <SPI.h>
#include <WiFi.h>
#include <cJSON.h>
#include <DHT_U.h>
#include <Arduino.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>

/** 温湿度采集引脚 */
#define DHTPIN 13

/** 温湿度采集器型号 */
#define DHTTYPE DHT11

// MQTT Broker 服务端连接
const int mqtt_port = 1883;
const char *mqtt_topic = "attributes";
const char *mqtt_password = "xaOLytUEQI";
const char *mqtt_username = "5mk5srrjyi245nf4";
const char *mqtt_broker = "sh-3-mqtt.iot-api.com";

/**
 * WIFI 连接
*/
WiFiClient espClient;

/**
 * 客户端事件
*/
sensors_event_t event;


/**
 * 客户端事件
*/
PubSubClient client(espClient);


/**
 * 老的 温度
 */
float oldTemperature;

/**
 * 老的 湿度
 */
float oldHumidity;

/**
 * wifi 连接
 */
void connectingWifi();

/**
 * mqtt 连接
 */
void connectingMqtt();

/**
 * mqtt 连接
 */
void mqttCallback(char *topic, byte *payload, unsigned int length);

/**
 * mqtt 数据上送
 */
void publishMqttMesage();

/**
 *  数据采集
 */
void dataCollection();

/**
 * 初始化温度采集器
*/
DHT_Unified dht(DHTPIN, DHTTYPE);

void setup()
{
  Serial.begin(9600);
  dht.begin();
  connectingWifi();
  connectingMqtt();
  client.setCallback(mqttCallback);
}

void loop()
{
  dataCollection();
  client.loop();
  delay(1000 * 5);
}

void connectingWifi()
{
  const char *wifiName = "Redmi";
  const char *wifiPassword = "12345678";
  WiFi.disconnect(true);
  WiFi.begin(wifiName, wifiPassword);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("wifi 连接成功");
}

void connectingMqtt()
{
  client.setServer(mqtt_broker, mqtt_port);
  while (!client.connected())
  {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password))
    {
      Serial.println("连接成功");
      client.subscribe(mqtt_topic);
    }
    else
    {
      Serial.print("连接失败");
      Serial.print(client.state()); // 返回连接状态
      delay(2000);
    }
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message received on topic: ");
  Serial.println(topic);
  Serial.println("-----------------------");
}

void dataCollection()
{
  dht.temperature().getEvent(&event);
  float temperature = event.temperature;
  dht.humidity().getEvent(&event);
  float humidity = event.relative_humidity;
  bool dataHasChange = temperature != oldTemperature || humidity != oldHumidity;
  if (dataHasChange)
  {
    Serial.println("数据变化了");
    oldHumidity = humidity;
    oldTemperature = temperature;
    publishMqttMesage();
  }
}

void publishMqttMesage()
{
  if (!client.connected())
  {
    Serial.println("数据重连");
    connectingMqtt();
  }
  Serial.println("publish");
  char msg[200];
  snprintf(
      msg, sizeof(msg),
      "{ \"temperature\":%.1f, \"humidity\":%.1f}",
      oldTemperature,
      oldHumidity);
  client.publish(mqtt_topic, msg);
}