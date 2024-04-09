#include <dht.h>
#include <SoftwareSerial.h>
#include <string.h>

#define rate 1000
#define rx_pin 7
#define tx_pin 6
#define dht_data_pin 13
#define dht_energy_pin 12
#define time_to_update 3600000 
#define precision 0 
#define num_size 2
#define text_size 5
#define data_size 10
#define cat_size 31

dht DHT;
SoftwareSerial wifi_serial(rx_pin, tx_pin);
String wifi_serial_cmd = "";
unsigned long start_time = 0;
unsigned short temperatures[data_size] = {0};
unsigned short humidities[data_size] = {0};
short registered = -1;
short index = -1;

void setup() {
  pinMode(dht_energy_pin, OUTPUT);
  digitalWrite(dht_energy_pin, HIGH);

  Serial.begin(9600);
  wifi_serial.begin(115200);

  wifi_serial.println("AT");
  wifi_serial.println("AT+CWMODE=1");
  wifi_serial.println("AT+CIPMUX=1");
  wifi_serial.println("AT+CIFSR");
  wifi_serial.println("AT+CIPSERVER=1,80");
  Serial.println(wifi_serial.readString());
}

void loop() {
  if (millis() > start_time + time_to_update) {
    int dht_read = DHT.read11(dht_data_pin);

    index++;
    if (index >= data_size) { index = 0; }
    if (registered < index) { registered = index; }

    temperatures[index] = DHT.temperature;
    humidities[index] = DHT.humidity;
    start_time = millis();
  }

  if (wifi_serial.available()) {
    wifi_serial_cmd = wifi_serial.readString();
    Serial.println(wifi_serial_cmd);
  }

  if (wifi_serial_cmd.indexOf('\n') > -1 && wifi_serial_cmd.indexOf("+IPD") != -1) {
    char r[512] = "";
    construct_html(r);

    char cmd[32] = "";
    sprintf(cmd, "AT+CIPSEND=0,%d", strlen(r));

    wifi_serial.println(cmd);
    delay(100);
    wifi_serial.println(r);
    delay(100);
    wifi_serial.println("AT+CIPCLOSE=0");

    wifi_serial_cmd = "";
  }
}

void construct_html(char* html) {
  html[0] = '\0';
  char temp[text_size] = "nada", hum[text_size] = "nada", temps[cat_size] = "nada", hums[cat_size] = "nada";

  if (registered != -1){
    dtostrf(temperatures[index], num_size, precision, temp);
    strncat(temp, "C ", text_size);
    dtostrf(humidities[index], num_size, precision, hum);
    strncat(hum, "% ", text_size);

    temps[0] = '\0'; hums[0] = '\0';

    for (int i = 0; i < registered+1; i++) {
      char temp[text_size];
      dtostrf(temperatures[index], num_size, precision, temp);
      strncat(temp, "C ", text_size);
      strncat(temps, temp, cat_size);

      char hum[text_size];
      dtostrf(humidities[index], num_size, precision, hum);
      strncat(hum, "% ", text_size);
      strncat(hums, hum, cat_size);
    }
  } 

  sprintf(html,
    "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
    "<html lang=\"pt\" charset=\"utf-8\">"
    "<head>"
    "<style>"
    "body { width: 80%%; font-family: sans-serif; margin: 40px auto;}"
    "h1, h2, h3, h4 { font-weight: 500; }"
    "h1 { margin-bottom: 0px; }"
    "h2 { margin-top: 40px; margin-bottom: 20px; }"
    "</style>"
    "</head>"
    "<body>"
    "<h4>Os dados (maximo: 10) sao capturados a cada 1h</h4>"
    "<h1>Metricas atuais</h1>"
    "<h3>"
    "Temperatura: %s, Umidade: %s."
    "</h3>"
    "<h2>Temperatura registradas</h2>"
    "<h3>%s</h3>"
    "<h2>Umidade registradas</h2>"
    "<h3>%s</h3>"
    "</body>"
    "</html>",
    temp, hum, temps, hums
  );
}
