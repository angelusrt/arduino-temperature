#include <dht.h>
#include <SoftwareSerial.h>
#include <string.h>

#define rate 1000
#define rx_pin 7
#define tx_pin 6
#define dht_data_pin 13
#define dht_energy_pin 12
// #define time_to_update 30000 
#define time_to_update 3600000 
#define text_size 5
#define data_size 5
#define cat_size 21
#define int_base 10
#define cmd_size 128

dht DHT;
SoftwareSerial wifi_serial(rx_pin, tx_pin);
char wifi_serial_cmd[cmd_size] = "";
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
  delay(rate);
  wifi_serial.println("AT+CWMODE=1");
  delay(rate);
  wifi_serial.println("AT+CIPMUX=1");
  delay(rate);
  wifi_serial.println("AT+CIFSR");
  delay(rate);
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
    wifi_serial.readStringUntil('\n').toCharArray(wifi_serial_cmd, cmd_size);
    Serial.println(wifi_serial_cmd);
  }

  if (strstr(wifi_serial_cmd, "+IPD") != NULL) {
    char r[512] = "";
    construct_html(r);
    Serial.println(r);

    char cmd[32] = "";
    sprintf(cmd, "AT+CIPSEND=0,%d", strlen(r));

    wifi_serial.println(cmd);
    delay(100);
    wifi_serial.println(r);
    delay(100);
    wifi_serial.println("AT+CIPCLOSE=0");

    wifi_serial_cmd[0] = '\0';
  }
}

void construct_html(char* html) {
  html[0] = '\0';
  char temp[text_size] = "nada", 
       hum[text_size] = "nada", 
       temps[cat_size] = "nada", 
       temp_mean[text_size] = "nada", 
       hum_mean[text_size] = "nada",
       temp_range[text_size] = "nada", 
       hum_range[text_size] = "nada",
       buff[text_size] = "";

  if (registered != -1){
    itoa(temperatures[index], temp, int_base);
    strncat(temp, "C", text_size);
    itoa(humidities[index], hum, int_base);
    strncat(hum, "%", text_size);

    temps[0] = '\0'; 
    short temp_total = 0, 
          hum_total = 0,
          temp_min = temperatures[index], 
          hum_min = humidities[index], 
          temp_max = 0, 
          hum_max = 0;

    for (size_t i = 0; i < registered+1; i++) {
      itoa(temperatures[i], buff, int_base);
      strncat(buff, "C ", text_size);
      strncat(temps, buff, cat_size);

      temp_total += temperatures[i];
      hum_total += humidities[i];

      if (temperatures[i] < temp_min) { temp_min = temperatures[i]; }
      if (temperatures[i] > temp_max) { temp_max = temperatures[i]; }
      if (humidities[i] < hum_min) { hum_min = humidities[i]; }
      if (humidities[i] > hum_max) { hum_max = humidities[i]; }
    }
    
    itoa(temp_total/(registered+1), temp_mean, int_base);
    strncat(temp_mean, "C", text_size);
    itoa(hum_total/(registered+1), hum_mean, int_base);
    strncat(hum_mean, "%", text_size);

    itoa(temp_max - temp_min, temp_range, int_base);
    strncat(temp_range, "C", text_size);
    itoa(hum_max - hum_min, hum_range, int_base);
    strncat(hum_range, "%", text_size);
  } 

  sprintf(html,
    "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
    "<html lang=\"pt\" charset=\"utf-8\">"
    "<head>"
    "<style>"
    "body{width:80%%;font-family:sans-serif;margin:40px auto;}"
    "h1,h2,h3,h4{font-weight:500;}"
    "h1{margin-bottom:0px;}"
    "h2{margin-top:40px;margin-bottom:20px;}"
    "</style>"
    "</head>"
    "<body>"
    "<h4>Os dados (max: 5) sao capturados a cada 1h</h4>"
    "<h1>Metricas atuais</h1>"
    "<h3>%s, %s.</h3>"
    "<h2>Dados registrados</h2>"
    "<h3>%s</h3>"
    "<h2>Medias</h2>"
    "<h3>%s, %s.</h3>"
    "<h2>Intervalo</h2>"
    "<h3>%s, %s.</h3>"
    "</body>"
    "</html>",
    temp, hum, temps, temp_mean, hum_mean, temp_range, hum_range
  );
}
