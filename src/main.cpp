// Подключаем библиотеки
#include "WiFi.h"
#include "AsyncUDP.h"
#include <GyverPortal.h>


// Определяем выводы
#define FLASH_PIN 4  // AIThinker esp32 CAM builtIn Led 33. Flash Led 4
#define RED_PIN 13  // AIThinker esp32 CAM builtIn Led. Flash Led 4
#define SENSOR 14 // подключение датчика
#define PAUSE 6200 // пауза между сразабываниями 6,2 секунд


// Определяем название и пароль точки доступа и  ее ip
const char* WIFIssid = "esp32";
const char* WIFIpassword = "1234567812345678";  // пароль от 1 до 8 2 раза
IPAddress wifiAPip(192, 168, 4, 1);

// Создаём объект UDP соединения
AsyncUDP udp;
const uint16_t port = 12345;  // Определяем порт
uint8_t number = 0;
uint32_t ms = 0, prevMs = 0, second = 0;


// Определяем callback функцию обработки udp пакета
void parseUdpPacket(AsyncUDPPacket packet) {
  // Выводим в последовательный порт все полученные данные
  // хороший сайт для работы с библиотекой AsyncUdp
  // https://community.appinventor.mit.edu/t/esp32-with-udp-send-receive-text-chat-mobile-mobile-udp-testing-extension-udp-by-ullis-ulrich-bien/72664/2

  Serial.write(packet.data(), packet.length());
  
  Serial.println();
}  // parseUdpPacket()


// подключение по udp
bool udpFine() {
  Serial.print("try udp");
  // Если удалось подключится по UDP
  for (int i = 0; i < 50; i++) {
    if (!udp.connect(wifiAPip, port)) {
      Serial.print(".");
      digitalWrite(RED_PIN, 1);
      delay(160);
      digitalWrite(RED_PIN, 0);
      delay(40);
      if (i == 50) return 0;  // вернем ошибку, если не смогли подключиться за 50 попыток
    } else break;             // выйдем из цикла, если подключились
  }                           // for
  Serial.println("\nUDP fine");
  udp.onPacket(parseUdpPacket);  // вызываем callback функцию при получении пакета
  return 1;
}  // udpFine()


// автоподключение к wifi , если ок, возвращает 1
bool wifiFine() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Got wifi");
    //Serial.printf("Got wifi %-32.32s\n `th %4d, Chan %2d, ip %s", WiFi.SSID().c_str(), WiFi.RSSI(), WiFi.channel(), WiFi.localIP().toString());
    return 1;
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFIssid, WIFIpassword);
    Serial.print("try SSID ");
    //Serial.print(WiFi.SSID());
    Serial.print(WIFIssid);
    int i = 0;
    while (WiFi.status() != WL_CONNECTED) {
      i++;
      Serial.print(".");
      //Serial.print(WiFi.status());
      digitalWrite(FLASH_PIN, 1);
      delay(50);
      digitalWrite(FLASH_PIN, 0);
      delay(450);
      if (i > 60) {
        Serial.println("Wifi unavlbl");
        return 0;
      }
    }//while
    Serial.print("\n\t Got wifi. IP ");
    Serial.println(WiFi.localIP());
    return 1;
  }  //.status()
}  // wifiFine()


// установка полного подключения по wifi и udp
void fullConnection() {
  // ждем подключения wifi
  for (int tries = 0; tries < 5; tries++) {
    if (wifiFine()) break;
    else {
      Serial.print("\n\n\t\t !!! can't connect WIFI !!!\n\t RESTART \n\n ");
      ESP.restart();
    }
  }  //for
     // ждем подключения udp
  for (int tries = 0; tries < 2; tries++) {
    if (udpFine()) break;
    else {
      Serial.print("\n\n\t\t !!! can't connect UDP !!!\n\t RESTART \n\n ");
      ESP.restart();
    }
  }  //for
}  // fullConnection()




void setup() {
  // Инициируем последовательный порт
  Serial.begin(115200);
  // Устанавливаем режим работы вывода светодиода
  pinMode(FLASH_PIN, OUTPUT);
  digitalWrite(FLASH_PIN, 0);
  pinMode(RED_PIN, OUTPUT);
  digitalWrite(RED_PIN, 0);
  pinMode(SENSOR, INPUT);
  // установка связи по wifi и udp
  fullConnection();
}  // setup()


// 5 раз отправляем по udp 
// если связь будет отваливаться, отключаться, теряться, и не появляться
//поработать над восстановлением функции , вызывающей коллбек обработчика udp пакетов 
void send_udp_message() {
  /*
    //альтернативный вариант отправки 1: отправляем udpMessage
    char udpMessage[20];
    sprintf(udpMessage, "%lu", second);
    Serial.println(udpMessage);
  */

  //альтернативный вариант отправки 2: отправляем udpData
  String udpData = String("1");
  //  number++;
  //  udpData += String((int)number);

    // за 500 мс отправляем 5 сообщений
  for (int i = 0; i < 5; i++) {
    //udp.broadcastTo(udpMessage, port); //альтернативный вариант отправки 1
   // udp.broadcastTo(udpData.c_str(), port); //альтерн вариант отправки 2
    // отправляем единичку
    udp.broadcastTo("1", port);
    digitalWrite(RED_PIN, 1);
    delay(50);
    digitalWrite(RED_PIN, 0);
    delay(50);
  }
}  //send_udp_message()

void loop() {
  ms = millis();
  //если связь налажена, работаем
  if ((WiFi.status() == WL_CONNECTED) && (udp.connect(wifiAPip, port))) {
    if (digitalRead(SENSOR)) {
      delay(20);
      if (digitalRead(SENSOR)) {
        send_udp_message(); // отправка 
        delay(PAUSE - 500ul); // задержка в PAUSE милисекунд 
      }
    }//digRead
  }    //if connected
  //потеряли сетевую  связь с ESP32 Server, восстанавливаем
  else
    fullConnection();
}  //loop()