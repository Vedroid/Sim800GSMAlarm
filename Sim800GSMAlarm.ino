#include <Arduino.h>
#include <SoftwareSerial.h>
#define SIM800_RX 2
#define SIM800_TX 3
#define SIM800_RST 4

#define loadledpin 13
#define ledpin 8
#define ledpin_GND 6
#define button 12
#define button_GND 10
#define voltmeter_pin A0
#define batteryQuantity 1

String whiteListPhones = "+380674016509, +380675056266";   // Белый список телефонов
String phoneNumber = "+380674016509";

float voltage = 0.00;
boolean ledStatus = false;
long lastcmd = millis();
long lastVoltageMeasurement = millis();
String _response = "";
long lastUpdate = millis();
long updatePeriod   = 60000;

SoftwareSerial SIM800(SIM800_TX, SIM800_RX); // RX Arduino (TX SIM800L), TX Arduino (RX SIM800L)

byte voltage_measure() {
  int volts = voltage / batteryQuantity * 100;
  if (volts > 387)
    return map(volts, 420, 387, 100, 77);
  else if ((volts <= 387) && (volts > 375) )
    return map(volts, 387, 375, 77, 54);
  else if ((volts <= 375) && (volts > 368) )
    return map(volts, 375, 368, 54, 31);
  else if ((volts <= 368) && (volts > 340) )
    return map(volts, 368, 340, 31, 8);
  else if ((volts <= 340) && (volts > 255))
    return map(volts, 340, 255, 8, 0);
  else return 0;
}

String waitResponse() {                         // Функция ожидания ответа и возврата полученного результата
  String _resp = "";                            // Переменная для хранения результата
  long _timeout = millis() + 10000;             // Переменная для отслеживания таймаута (10 секунд)
  while (!SIM800.available() && millis() < _timeout)  {}; // Ждем ответа 10 секунд, если пришел ответ или наступил таймаут, то...
  if (SIM800.available()) {                     // Если есть, что считывать...
    _resp = SIM800.readString();                // ... считываем и запоминаем
  }
  else {                                        // Если пришел таймаут, то...
    Serial.println("Timeout...");               // ... оповещаем об этом и...
  }
  _resp.trim();
  return _resp;                                 // ... возвращаем результат. Пусто, если проблема
}


String sendATCommand(String cmd, bool waiting) {
  String _resp = "";                            // Переменная для хранения результата
  Serial.println(">> " + cmd);                          // Дублируем команду в монитор порта
  SIM800.println(cmd);                          // Отправляем команду модулю
  if (waiting) {                                // Если необходимо дождаться ответа...
    _resp = waitResponse();                     // ... ждем, когда будет передан ответ
    // Если Echo Mode выключен (ATE0), то эти 3 строки можно закомментировать
    if (_resp.startsWith(cmd)) {  // Убираем из ответа дублирующуюся команду
      _resp = _resp.substring(_resp.indexOf("\r", cmd.length()) + 2);
    }
    Serial.println("<< " + _resp);                      // Дублируем ответ в монитор порта
  }
  return _resp;                                 // Возвращаем результат. Пусто, если проблема
}

String uptime() {
  String uptime = "";
  unsigned long time = millis() / 1000;
  if (time / 60 / 60 < 10) {
    uptime += "0";
  }
  uptime += (time / 60 / 60);
  uptime += ":";
  if (time / 60 % 60 < 10) {
    uptime += "0";
  }
  uptime += ((time / 60) % 60);
  uptime += ":";
  if (time % 60 < 10) {
    uptime += "0";
  }
  uptime += (time % 60);
  return uptime;
}

String rssi() {
  // read the RSSI  AT+CSQ
  uint8_t n = sendATCommand("AT+CSQ", true).substring(6,8).toInt();
  int8_t r;

  Serial.print(n); Serial.print(": ");
  if (n == 0) r = -115;
  if (n == 1) r = -111;
  if (n == 31) r = -52;
  if ((n >= 2) && (n <= 30)) {
    r = map(n, 2, 30, -110, -54);
  }
  Serial.print(r); Serial.println(F(" dBm"));
  return (String)n + ": " + r + " dBm";
}

void call() {
  sendATCommand("ATD" + phoneNumber + ";", true);
}

void sendSMS(String message) {
  sendATCommand("AT+CMGS=\"" + phoneNumber + "\"", true);       // Переходим в режим ввода текстового сообщения
  sendATCommand(message + "\r\n" + (String)((char)26), true);   // После текста отправляем перенос строки и Ctrl+Z
}

void parseSMS(String msg) {                                   // Парсим SMS
  String msgheader  = "";
  String msgbody    = "";
  String msgphone   = "";

  msg = msg.substring(msg.indexOf("+CMGR: "));
  msgheader = msg.substring(0, msg.indexOf("\r"));            // Выдергиваем телефон

  msgbody = msg.substring(msgheader.length() + 2);
  msgbody = msgbody.substring(0, msgbody.lastIndexOf("OK"));  // Выдергиваем текст SMS
  msgbody.trim();

  int firstIndex = msgheader.indexOf("\",\"") + 3;
  int secondIndex = msgheader.indexOf("\",\"", firstIndex);
  msgphone = msgheader.substring(firstIndex, secondIndex);

  Serial.println("Phone: " + msgphone);                       // Выводим номер телефона
  Serial.println("Message: " + msgbody);                      // Выводим текст SMS

  if (msgphone.length() > 6 && whiteListPhones.indexOf(msgphone) > -1) { // Если телефон в белом списке, то...
    //setLedState(msgbody, msgphone);                           // ...выполняем команду
  }
  else {
    Serial.println("Unknown phonenumber");
    }
}

String getAGPS() {
  String aGPS = "";
sendATCommand("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"",true);
sendATCommand("AT+SAPBR=3,1,\"APN\",\"www.kyivstar.net\"",true);
Serial.println("getAGPS::"+sendATCommand("AT+SAPBR=1,1",true));
Serial.println("getAGPS::"+sendATCommand("AT+SAPBR=2,1",true));
Serial.println(aGPS = sendATCommand("AT+CIPGSMLOC=1,1",true));
Serial.println("getAGPS::"+sendATCommand("AT+SAPBR=0,1",true)); 
return aGPS;
}

void modemSetup() {
  Serial.begin(9600);                       // Скорость обмена данными с компьютером
  SIM800.begin(9600);                       // Скорость обмена данными с модемом
  Serial.println("Start!");

  sendATCommand("AT", true);                // Автонастройка скорости

  // Команды настройки модема при каждом запуске
  _response = sendATCommand("AT+CLIP=1", true);  // Включаем АОН

}

void pinSetup() {
  pinMode(ledpin, OUTPUT);
  pinMode(ledpin_GND, OUTPUT);
  digitalWrite(ledpin_GND, LOW);
  pinMode(button_GND, OUTPUT);
  digitalWrite(button_GND, LOW);
  pinMode(button, INPUT_PULLUP);
  delay(10);
}

void setup() {
  digitalWrite(loadledpin, 1);
  modemSetup();
  pinSetup();
  digitalWrite(loadledpin, 0);
}

boolean hasmsg = false;                                              // Флаг наличия сообщений к удалению
void loop() {
  boolean ring = false;
  voltage = map(analogRead(voltmeter_pin), 0, 1023, 0, 500) / 100.00;
  if (voltage < 3.20 && millis() - lastcmd > (long)5 * 60 * 1000) {
    Serial.println("Low voltage!");
    digitalWrite(loadledpin, 1);
    sendSMS((String)"Attention! Low voltage: " + voltage_measure() + "%(" + voltage + "v)");
    lastcmd = millis();
    digitalWrite(loadledpin, 0);
  }

  if (lastUpdate + updatePeriod < millis() ) {                    // Пора проверить наличие новых сообщений
    digitalWrite(loadledpin, 1);
    do {
      _response = sendATCommand("AT+CMGL=\"REC UNREAD\",1", true);// Отправляем запрос чтения непрочитанных сообщений
      if (_response.indexOf("+CMGL: ") > -1) {                    // Если есть хоть одно, получаем его индекс
        int msgIndex = _response.substring(_response.indexOf("+CMGL: ") + 7, _response.indexOf("\"REC UNREAD\"", _response.indexOf("+CMGL: ")) - 1).toInt();
        char i = 0;                                               // Объявляем счетчик попыток
        do {
          i++;                                                    // Увеличиваем счетчик
          _response = sendATCommand("AT+CMGR=" + (String)msgIndex + ",1", true);  // Пробуем получить текст SMS по индексу
          _response.trim();                                       // Убираем пробелы в начале/конце
          if (_response.endsWith("OK")) {                         // Если ответ заканчивается на "ОК"
            if (!hasmsg) hasmsg = true;                           // Ставим флаг наличия сообщений для удаления
            sendATCommand("AT+CMGR=" + (String)msgIndex, false);   // Делаем сообщение прочитанным
            sendATCommand("\n", false);                            // Перестраховка - вывод новой строки
            parseSMS(_response);                                  // Отправляем текст сообщения на обработку
            break;                                                // Выход из do{}
          }
          else {                                                  // Если сообщение не заканчивается на OK
            Serial.println ("Error answer");                      // Какая-то ошибка
            sendATCommand("\n", true);                            // Отправляем новую строку и повторяем попытку
          }
        } while (i < 10);
        break;
      }
      else {
        lastUpdate = millis();                                    // Обнуляем таймер
        if (hasmsg) {
          sendATCommand("AT+CMGDA=\"DEL READ\"", true);           // Удаляем все прочитанные сообщения
          hasmsg = false;
        }
        break;
      }
    } while (1);
    digitalWrite(loadledpin, 0);
  }


  if (SIM800.available())   {                   // Если модем, что-то отправил...
    digitalWrite(loadledpin, 1);
    _response = waitResponse();                 // Получаем ответ от модема для анализа
    Serial.println("<< " + _response);                  // Если нужно выводим в монитор порта
    // ... здесь можно анализировать данные полученные от GSM-модуля


    if (_response.indexOf("+CMTI:")>-1) {             // Пришло сообщение об отправке SMS
      lastUpdate = millis() -  updatePeriod;          // Теперь нет необходимости обрабатываеть SMS здесь, достаточно просто
                                                      // сбросить счетчик автопроверки и в следующем цикле все будет обработано
    }


    if (_response.startsWith("RING")) {         // Есть входящий вызов
      sendATCommand("ATH", true);               // Отклоняем вызов
      int phoneindex = _response.indexOf("+CLIP: \"");// Есть ли информация об определении номера, если да, то phoneindex>-1
      String innerPhone = "";                   // Переменная для хранения определенного номера
      if (phoneindex >= 0) {                    // Если информация была найдена
        phoneindex += 8;                        // Парсим строку и ...
        innerPhone = _response.substring(phoneindex, _response.indexOf("\"", phoneindex)); // ...получаем номер
        Serial.println("Number: " + innerPhone); // Выводим номер в монитор порта
      }
      // Проверяем, чтобы длина номера была больше 6 цифр, и номер должен быть в списке
      if (innerPhone.length() >= 7 && whiteListPhones.indexOf(innerPhone) >= 0) {
        ring = true;
            sendSMS((String)"Voltage: " + voltage_measure() + "%(" + voltage + "v),"
                 + " RSSI: " + rssi()
                 + ", Up time: " + uptime()
                 + ", GPS: " + getAGPS());
      }
      
    }


    if (_response.startsWith("+CMGS:")) {       // Пришло сообщение об отправке SMS
      int index = _response.lastIndexOf("\r\n");// Находим последний перенос строки, перед статусом
      String result = _response.substring(index + 2, _response.length()); // Получаем статус
      result.trim();                            // Убираем пробельные символы в начале/конце

      if (result == "OK") {                     // Если результат ОК - все нормально
        Serial.println ("Message was sent. OK");
      }
      else {                                    // Если нет, нужно повторить отправку
        Serial.println ("Message was not sent. Error");
      }
    }


    digitalWrite(loadledpin, 0);
  }
  if (Serial.available())  {                    // Ожидаем команды по Serial...
    SIM800.write(Serial.read());                // ...и отправляем полученную команду модему
  };



  if (!digitalRead(button)) {
    Serial.println("Butt1 pressed");
    digitalWrite(loadledpin, 1);
    ledStatus = !ledStatus;
    digitalWrite(ledpin, ledStatus);

    String msg = (String) "Voltage: " + voltage_measure() + "%(" + voltage + "v),"
                 + " RSSI: " + rssi()
                 + ", Up time: " + uptime();

    if (ledStatus) sendSMS("LedOn. " + msg + ".");
    else sendSMS("LedOff. " + msg + ".");

    digitalWrite(loadledpin, 0);
  }
}
