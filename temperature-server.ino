// ESP+AT
// https://docs.espressif.com/projects/esp-at/en/latest/esp32/index.html
// library:
// DHTlib	0.1.36
// inspired:
// https://www.circuitbasics.com/how-to-set-up-a-web-server-using-arduino-and-esp8266-01/
// https://www.instructables.com/Using-ESP-01-and-Arduino-UNO/
// https://www.instructables.com/Getting-Started-With-the-ESP8266-ESP-01/

#include <dht.h>
#include <SoftwareSerial.h>
// #include <math.h>
// #include <string.h>

const unsigned short rate = 2000;
const unsigned short dht_data_pin = 13;
const unsigned short dht_energy_pin = 12;
const unsigned short time_to_update = 3600000; //3600000
const unsigned short data_length = 10;
const char null_text[] = "sem dados";

String wifi_serial_cmd = "";
unsigned long start_time = 0;

dht DHT;
SoftwareSerial wifi_serial(7, 6); // RX, TX

float temperature = 0;
float humidity = 0;
float temperatures[data_length] = {};
float humidities[data_length] = {};
unsigned short registered = 0;
unsigned short index = 0;
float temperature_mean = 0;
float humidity_mean = 0;
float temperature_deviation = 0;
float humidity_deviation = 0;

void setup() {
	pinMode(dht_energy_pin, OUTPUT);
	digitalWrite(dht_energy_pin, HIGH);

	Serial.begin(9600);
	wifi_serial.begin(115200);

	delay(rate);
	wifi_serial.println("AT");
	delay(rate);
	wifi_serial.println("AT+CWMODE=1");
	delay(rate);
	wifi_serial.println("AT+CIPMUX=1");
	delay(rate);
	wifi_serial.println("AT+CIFSR");
	delay(rate);
	wifi_serial.println("AT+CIPSERVER=1,80");
	delay(rate);
}

void loop() {
	if (millis() > start_time + time_to_update) {
		// update_data();
		start_time = millis();
	}

	if (wifi_serial.available()){ 
		wifi_serial_cmd = wifi_serial.readString(); 
		Serial.println(wifi_serial_cmd);
	}

	if (wifi_serial_cmd.indexOf('\n') > -1 && wifi_serial_cmd.indexOf("+IPD") != -1) {
		String r = construct_html();
		// Serial.println(r);

		wifi_serial.println("AT+CIPSEND=0," + String(r.length()));

		delay(100);
		wifi_serial.println(r);
		
		delay(100);
		wifi_serial.println("AT+CIPCLOSE=0");
		wifi_serial_cmd = "";
	}
}

void update_data() {
	int dht_read = DHT.read11(dht_data_pin);

	temperature = DHT.temperature;
	humidity = DHT.humidity;
	temperatures[index] = temperature;
	humidities[index] = humidity;
	index++;

	if (index >= data_length) { index = 0; }
	if (registered < index) { registered = index; }

	if (registered != 0) {
		unsigned short temperature_summation = 0;
		unsigned short humidity_summation = 0;

		for (short i = 0; i < registered; i++) {
			temperature_summation += temperatures[i];
			humidity_summation += humidities[i];
		}

		temperature_mean = ((float)temperature_summation/registered);
		humidity_mean = ((float)humidity_summation/registered);

		for (short i = 0; i < registered; i++) {
			temperature_deviation += pow(temperatures[i] - temperature_mean, 2);
			humidity_deviation += pow(humidities[i] - humidity_mean, 2);
		}

		temperature_deviation = sqrt(temperature_deviation/registered);
		humidity_deviation = sqrt(humidity_deviation/registered);
	}
}

String construct_html(){
	String r = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
	r += "<html lang=\"pt\" charset=\"utf-8\">";

	r += 
		"<head>"
			"<style>"
				"body {"
					"width: 80%;"
					"font-family: sans-serif;"
					"margin: 40px auto;"
				"}\n"
				"h1, h2, h3 {"
					"font-weight: 500;"
				"}\n"
				"h1 {"
					"margin-bottom: 0px;"
				"}\n"
				"h2 {"
					"margin-top: 40px;"
					"margin-bottom: 20px;"
				"}\n"
			"</style>"
		"</head>";

	r += "<body>";

	r += "<h1>Metricas atuais</h1>";
	r += "<h3>";
	r += "Temperatura: ";
	if (registered != 0) { r += temperature; r += "C";	} else { r += null_text; }
	r += ", Umidade: ";
	if (registered != 0) { r += humidity; r += "%";	} else { r += null_text; }
	r += ".";
	r += "</h3>";

	r += "<h2>Media</h2>";
	r += "<h3>";
	r += "Temperatura: ";
	if (registered != 0) { r += temperature_mean; r += "C";	} else { r += null_text; }
	r += ", Umidade: ";
	if (registered != 0) { r += humidity_mean; r += "%";	} else { r += null_text; }
	r += ".";
	r += "</h3>";

	r += "<h2>Desvio padrao</h2>";
	r += "<h3>";
	r += "Temperatura: ";
	if (registered != 0) { r += temperature_deviation;	} else { r += null_text; }
	r += ", Umidade: ";
	if (registered != 0) { r += humidity_deviation;	} else { r += null_text; }
	r += ".";
	r += "</h3>";

	r += "<h2>Temperatura registradas</h2>";
	r += "<h3>";

	if (registered != 0) {
		for (unsigned short i; i < registered; i++) {
			r+= temperatures[i];
			r += "C ";
		}
	} else {
		r += null_text;
	}

	r += "</h3>";

	r += "<h2>Umidade registradas</h2>";
	r += "<h3>";

	if (registered != 0) {
		for (unsigned short i; i < registered; i++) {
			r+= humidities[i];
			r += "% ";
		}
	} else {
		r += null_text;
	}

	r += "</h3>";

	r += "</body>";

	r += "</html>";

	return r;
}
