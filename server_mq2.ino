#define BLYNK_TEMPLATE_ID "TMPL6FpA9iFxV"
#define BLYNK_TEMPLATE_NAME "MQ2"
#define BLYNK_AUTH_TOKEN "4rz1cwia5PiocScBbGg0GhAT_nlWQjc3"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

char auth[] =  BLYNK_AUTH_TOKEN;
char ssid[] = "?";
char pass[] = "12345678";

String uartData = "";
int systemOn = 0;
int gasPPM = 0;
int alertLevel = 0;

void setup() {
  Serial.begin(115200);    
  Serial2.begin(115200, SERIAL_8N1, 16, 17);  // RX=16, TX=17

  Blynk.begin(auth, ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    Blynk.virtualWrite(V0, 0);  // trạng thái kết nối
  }

  Serial.println("Connected to WiFi");
  Blynk.virtualWrite(V0, 1);
}

void loop() {
  Blynk.run();

  while (Serial2.available()) {
    char c = Serial2.read();

    if (c == '\n') {
      Serial.print("Received: ");
      Serial.println(uartData);  // in chuỗi nhận được
      parseUART(uartData);
      uartData = "";
    } else {
      uartData += c;
    }
  }
}

void parseUART(String data) {
  // Giả sử chuỗi là dạng: <1,1234,2>
  int firstComma = data.indexOf(',');
  int secondComma = data.indexOf(',', firstComma + 1);

  if (data.startsWith("<") && data.endsWith(">") && firstComma > 0 && secondComma > firstComma) {
    systemOn  = data.substring(1, firstComma).toInt();
    gasPPM    = data.substring(firstComma + 1, secondComma).toInt();
    alertLevel = data.substring(secondComma + 1, data.length() - 1).toInt();

    Serial.println("Parsed:");
    Serial.print("System: "); Serial.println(systemOn);
    Serial.print("PPM: ");    Serial.println(gasPPM);
    Serial.print("Alert: ");  Serial.println(alertLevel);

    sendToBlynk();
  } else {
    Serial.println("Invalid data format");
  }
}

void sendToBlynk() {
  // Gửi trạng thái hệ thống
  Blynk.virtualWrite(V2, systemOn);    // V2: LED on/off
  Blynk.virtualWrite(V1, gasPPM);      // V1: Gauge hiển thị PPM

  String warning_state;
  switch(alertLevel) {
    case 0: warning_state = "Không có khí gas"; break;
    case 1: warning_state = "Nồng độ thấp"; break;
    case 2: warning_state = "Nồng độ cao"; break;
    case 3: warning_state = "Nguy hiểm!"; break;
    default: warning_state = "Lỗi dữ liệu"; break;
  }

  Blynk.virtualWrite(V3, warning_state); // V3: hiển thị trạng thái cảnh báo
}

BLYNK_WRITE(V10) {
  int value = param.asInt(); // 0 = OFF, 1 = ON

  Serial.print("Switch V10 changed: ");
  Serial.println(value);

  // Gửi thông điệp đến STM32 qua UART
  if (value == 1) {
    Serial2.println("SWITCH_ON\n");
  } else {
    Serial2.println("SWITCH_OFF\n");
  }
}

BLYNK_WRITE(V11) {
  int value = param.asInt();

  if (value == 1) {
    Serial2.println("RESET\n");

    // Tự động tắt switch sau 300ms
    delay(300);
    Blynk.virtualWrite(V11, 0);
  }
}

int pinV6 = 0;
int pinV7 = 0;
int pinV8 = 0;


BLYNK_WRITE(V6) {
  pinV6 = param.asInt();
  sendPinsToSTM32();
}

BLYNK_WRITE(V7) {
  pinV7 = param.asInt();
  sendPinsToSTM32();
}

BLYNK_WRITE(V8) {
  pinV8 = param.asInt();
  sendPinsToSTM32();
}


void sendPinsToSTM32() {
  // Gửi dạng: <v6_value,v7_value,v8_value>
  String message = "<" + String(pinV6) + "," + String(pinV7) + "," + String(pinV8) + ">";
  Serial2.println(message);  // Có \n ở cuối
  Serial.print("Sent to STM32: ");
  Serial.println(message);
}
