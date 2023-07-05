#include <IRremote.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
#define IR_RECEIVE_PIN 10
void(* resetFunc) (void) = 0;
int countDigits(int number) {
  int count = 0;
  while (number != 0) {
    number = number / 10;
    count++;
  }
  return count;
}
int getCharFromNumber(int number) {
  switch (number) {
    case 12:
      return 1;
    case 24:
      return 2;
    case 94:
      return 3;
    case 8:
      return 4;
    case 28:
      return 5;
    case 90:
      return 6;
    case 66:
      return 7;
    case 82:
      return 8;
    case 74:
      return 9;
    default:
      return 0;
  }
}
char dataTerima[20];
const int trigPin = 9;
const int echoPin = 8;
int menuIndex = 0;
const int MENU_COUNT = 7;
const char* menuItems[MENU_COUNT] = {"Ganti Password", "WIFI Scan", "Sensor Jarak", "Relay", "RFID", "EvilTwin Mode", "Exit"};
String str = "";
String strn = "";
long newPassword = strn.toInt();
long password = 1234;
int passwordDigits = countDigits(password);
bool enteringPassword = true;
long inputPassword = str.toInt();
int passwordIndex;
int receivedNumber = IrReceiver.decodedIRData.command;
const int MAX_NETWORKS = 10;
char networks[MAX_NETWORKS][32];
int networkCount = 0;
char receivedChar = getCharFromNumber(receivedNumber);
int wifiIndex = 0;
String pass = "";
int relay_1 = 2;
int relay_2 = 3;
int relay_3 = 4;
int relayIndex = 0;
const int RELAY_COUNT = 3;
const char* relayItems[RELAY_COUNT] = {"Relay 1", "Relay 2", "Relay 3"};
bool menu = false;
bool newPwd= false;
bool vNewPwd= false;
bool wifi = false;
bool wifiScn = false;
bool evillTwin = false;
bool relayMenu = false;
bool r1Mode = false;
bool r2Mode = false;
bool r3Mode = false;
bool passwordWifi = false;

void setup() {
  Serial.begin(9600);
  IrReceiver.begin(IR_RECEIVE_PIN);
  lcd.begin(16, 2);
  lcd.init();
  lcd.noBacklight();
  lcd.clear();
  pinMode(relay_1, OUTPUT);
  pinMode(relay_2, OUTPUT);
  pinMode(relay_3, OUTPUT);
  digitalWrite(relay_1, HIGH);
  digitalWrite(relay_2, HIGH);
  digitalWrite(relay_3, HIGH);
}

void loop() {
  if (enteringPassword) {
    handlePasswordEntry();
  } else if (menu){
    handleMenuSelection();
  } else if (newPwd) {
    inputNewPwd();
  } else if (vNewPwd) {
    verifikasiNewPwd();
  }else if (evillTwin) {
    evillMode();
  }else if (relayMenu) {
    Relay();
  }
  delay(100);
}

void handlePasswordEntry() {
  if (IrReceiver.decode()) {
    IrReceiver.resume();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enter Password:");
    int receivedNumber = IrReceiver.decodedIRData.command;
    char receivedChar = getCharFromNumber(receivedNumber);
    if (IrReceiver.decodedIRData.command == 70) {
      passwordIndex = 0;
      inputPassword = 0 ;
      exit();
    }else if (IrReceiver.decodedIRData.command == 64) {
      inputPassword = inputPassword / 10;
      passwordIndex = (passwordIndex - 1);
    }else if (receivedChar != '\0') {
      inputPassword = inputPassword * 10 + receivedChar;
      passwordIndex = (passwordIndex + 1);
    }
    lcd.setCursor(0, 1);
    lcd.print(inputPassword);
    if (IrReceiver.decodedIRData.command == 21) {
      if (inputPassword == password) {
        enteringPassword = false;
        lcd.clear();
        lcd.print("Password OK");
        delay(2000);
        lcd.clear();
        lcd.print("Select Menu:");
        lcd.setCursor(0, 0);
        lcd.print("Select Menu: ");
        lcd.setCursor(0, 1);
        lcd.print(menuIndex + 1);
        lcd.setCursor(1, 1);
        lcd.print(".");
        lcd.setCursor(2, 1);
        lcd.print(menuItems[menuIndex]);
        passwordIndex = 0;
        inputPassword = 0;
        menu = true;
      } else {
        lcd.clear();
        lcd.print("Invalid Password");
        delay(2000);
        lcd.clear();
        lcd.print("Enter Password:");
        passwordIndex = 0;
        inputPassword = 0 ;
      }
    }
  }
}

void handleMenuSelection() {
  if (IrReceiver.decode()) {
    IrReceiver.resume();
    if (IrReceiver.decodedIRData.command == 22) {
      menuIndex = (menuIndex + 1) % MENU_COUNT;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Select Menu: ");
      lcd.setCursor(0, 1);
      lcd.print(menuIndex + 1);
      lcd.setCursor(1, 1);
      lcd.print(".");
      lcd.setCursor(2, 1);
      lcd.print(menuItems[menuIndex]);
    } else if (IrReceiver.decodedIRData.command == 25) {
      menuIndex = (menuIndex - 1 + MENU_COUNT) % MENU_COUNT;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Select Menu: ");
      lcd.setCursor(0, 1);
      lcd.print(menuIndex + 1);
      lcd.setCursor(1, 1);
      lcd.print(".");
      lcd.setCursor(2, 1);
      lcd.print(menuItems[menuIndex]);
    } else if (IrReceiver.decodedIRData.command == 21) {
      if (menuItems[menuIndex] == "Exit") {
        exit();
        menu = false;
      }
      else if (menuItems[menuIndex] == "Ganti Password"){
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Password Baru :");
        newPwd= true;
        menu = false;
      }else if (menuItems[menuIndex] == "WIFI Scan") {
        lcd.clear();
        lcd.print("Scanning WiFi...");
        wifiScan();
        handleWifiSetting();
      }else if (menuItems[menuIndex] == "EvilTwin Mode") {
        lcd.clear();
        lcd.print("EvilTwin Mode");
        Serial.println("Eviltwin Mode");
        evillTwin = true;
        menu = false;
      }else if (menuItems[menuIndex] == "Relay") {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Relay Select :");
        lcd.setCursor(0, 1);
        lcd.print(relayIndex + 1);
        lcd.setCursor(1, 1);
        lcd.print(".");
        lcd.setCursor(2, 1);
        lcd.print(relayItems[relayIndex]);
        relayMenu = true;
        menu = false;
      }
      
    }
  }
}

void evillMode () {
  if (Serial.available() > 0) {
    String receivedData = Serial.readStringUntil('\n');
    receivedData.trim();

    if (receivedData == "EvillTwinON") {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("EvilTwin Mode ON");
      delay(1000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Wifi: OPPO A12");
      lcd.setCursor(0, 1);
      lcd.print("PWD: nakano22");
      while (true) {
        if (Serial.available() > 0) {
          String receivedSSID = Serial.readStringUntil('\n');
          receivedSSID.trim();
          if (receivedSSID == "Good password was entered !"){
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Please Wait..");
            break;
          }
        }else if (IrReceiver.decode()) {
          IrReceiver.resume();
          if (IrReceiver.decodedIRData.command == 70) {
            Serial.println("EndMode");
            lcd.clear();
            lcd.setCursor(0,1);
            lcd.println("Exit");
            delay(1000);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Select Menu: ");
            lcd.setCursor(0, 1);
            lcd.print(menuIndex + 1);
            lcd.setCursor(1, 1);
            lcd.print(".");
            lcd.setCursor(2, 1);
            lcd.print(menuItems[menuIndex]);
            evillTwin = false;
            menu = true;
            break;
          }
        }
      }
      delay(750);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Password ");
      lcd.setCursor(0,1);
      lcd.print("<======|==<");
      while (true) {
        if (IrReceiver.decode()) {
          IrReceiver.resume();
          if (IrReceiver.decodedIRData.command == 70) {
            Serial.println("EndMode");
            lcd.clear();
            lcd.setCursor(0,1);
            lcd.println("Exit");
            delay(1000);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Select Menu: ");
            lcd.setCursor(0, 1);
            lcd.print(menuIndex + 1);
            lcd.setCursor(1, 1);
            lcd.print(".");
            lcd.setCursor(2, 1);
            lcd.print(menuItems[menuIndex]);
            evillTwin = false;
            menu = true;
            break;
          }
        }
      }
    }
  }
}

void wifiScan () {
  Serial.println("wifiScan");
  while (true) {
    if (Serial.available() > 0) {
      String receivedData = Serial.readStringUntil('\n');
      receivedData.trim();

      if (receivedData == "ScanningWiFi") {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Scanning...");
        networkCount = 0;
        while (true) {
          if (Serial.available() > 0) {
            String receivedSSID = Serial.readStringUntil('\n');
            receivedSSID.trim();
            if (receivedSSID == "END") {
              break;
            }
            receivedSSID.toCharArray(networks[networkCount], 32);
            networkCount++;
          }
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wi-Fi: ");
        lcd.setCursor(0, 1);
        lcd.print(wifiIndex + 1);
        lcd.setCursor(1, 1);
        lcd.print(".");
        lcd.setCursor(3, 1);
        lcd.print(networks[wifiIndex]);
        break;
      }
    }
  }
}

void handleWifiSetting() {
  while (true) {
    if (IrReceiver.decode()) {
      IrReceiver.resume();
      if (IrReceiver.decodedIRData.command == 22) {
        wifiIndex = (wifiIndex + 1) % networkCount;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wi-Fi: ");
        lcd.setCursor(0, 1);
        lcd.print(wifiIndex + 1);
        lcd.setCursor(1, 1);
        lcd.print(".");
        lcd.setCursor(3, 1);
        lcd.print(networks[wifiIndex]);
      } else if (IrReceiver.decodedIRData.command == 25) {
        wifiIndex = (wifiIndex - 1 + networkCount) % networkCount;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wi-Fi: ");
        lcd.setCursor(0, 1);
        lcd.print(wifiIndex + 1);
        lcd.setCursor(1, 1);
        lcd.print(".");
        lcd.setCursor(3, 1);
        lcd.print(networks[wifiIndex]);
      } else if (IrReceiver.decodedIRData.command == 70) {
        Serial.println(networks[wifiIndex]);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Select Menu: ");
        lcd.setCursor(0, 1);
        lcd.print(menuIndex + 1);
        lcd.setCursor(1, 1);
        lcd.print(".");
        lcd.setCursor(2, 1);
        lcd.print(menuItems[menuIndex]);
        break;
      }
    }
  }
}

void inputNewPwd() {
  if (IrReceiver.decode()) {
    IrReceiver.resume();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Password Baru :");
    int receivedNumber = IrReceiver.decodedIRData.command;
    char receivedChar = getCharFromNumber(receivedNumber);
    if (receivedChar != '\0') {
      inputPassword = inputPassword * 10 + receivedChar;
      passwordIndex = (passwordIndex + 1);
      Serial.print(inputPassword);
    } else if (IrReceiver.decodedIRData.command == 64) {
      inputPassword = inputPassword / 10;
      passwordIndex = (passwordIndex - 1);
    }
    lcd.setCursor(0, 1);
    lcd.print(inputPassword);
    if (IrReceiver.decodedIRData.command == 21) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Verifikasi");
      newPwd= false;
      menu = false;
      passwordIndex = 0;
      enteringPassword = false;
      vNewPwd= true;
    }
  }
}

void verifikasiNewPwd() {
  if (IrReceiver.decode()) {
    IrReceiver.resume();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Verifikasi");
    int receivedNumber = IrReceiver.decodedIRData.command;
    char receivedChar = getCharFromNumber(receivedNumber);
    if (receivedChar != '\0') {
      newPassword = newPassword * 10 + receivedChar;
      passwordIndex = (passwordIndex + 1);
    }
    else if (IrReceiver.decodedIRData.command == 64) {
      newPassword = newPassword / 10;
      passwordIndex = (passwordIndex - 1);
    }
    lcd.setCursor(0, 1);
    lcd.print(newPassword);
    if (IrReceiver.decodedIRData.command == 21) {
      if (inputPassword == newPassword){
        password = newPassword;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Succes");
        newPassword = 0;
        inputPassword = 0;
        passwordIndex = 0;
        delay(3000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enter Password:");
        exit();
      }else if (inputPassword != newPassword) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Invalid");
        newPassword = 0;
        inputPassword = 0;
        passwordIndex = 0;
        delay(3000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enter Password:");
        exit();
      }
    }
  }
}

void relayMode() {
  while (true) {
    if (IrReceiver.decode()) {
      IrReceiver.resume();
      if (IrReceiver.decodedIRData.command == 22) {
        switch (relayIndex) {
        case 0:
          r1Mode = true;
          break;
        case 1:
          r2Mode = true;
          break;
        case 2:
          r3Mode = true;
          break;
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ON");
        lcd.setCursor(0, 1);
        lcd.print("OFF <--");
      } else if (IrReceiver.decodedIRData.command == 25) {
        switch (relayIndex) {
        case 0:
          r1Mode = false;
          break;
        case 1:
          r2Mode = false;
          break;
        case 2:
          r3Mode = false;
          break;
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ON  <--");
        lcd.setCursor(0, 1);
        lcd.print("OFF");
      } else if (IrReceiver.decodedIRData.command == 21) {
        if (relayItems[relayIndex] == "Relay 1") {
          switch (r1Mode)
          {
          case true:
            digitalWrite(relay_1, HIGH);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(relayItems[relayIndex]);
            lcd.setCursor(0, 1);
            lcd.print("OFF");
            delay(2000);
            break;
          case false:
            digitalWrite(relay_1, LOW);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(relayItems[relayIndex]);
            lcd.setCursor(0, 1);
            lcd.print("ON");
            delay(2000);
            break;
          }
          break;
        } else if (relayItems[relayIndex] == "Relay 2") {
          switch (r2Mode)
          {
          case true:
            digitalWrite(relay_2, HIGH);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(relayItems[relayIndex]);
            lcd.setCursor(0, 1);
            lcd.print("OFF");
            delay(2000);
            break;
          case false:
            digitalWrite(relay_2, LOW);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(relayItems[relayIndex]);
            lcd.setCursor(0, 1);
            lcd.print("ON");
            delay(2000);
            break;
          }
          break;
        } else if (relayItems[relayIndex] == "Relay 3") {
          switch (r3Mode)
          {
          case true:
            digitalWrite(relay_3, HIGH);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(relayItems[relayIndex]);
            lcd.setCursor(0, 1);
            lcd.print("OFF");
            delay(2000);
            break;
          case false:
            digitalWrite(relay_3, LOW);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(relayItems[relayIndex]);
            lcd.setCursor(0, 1);
            lcd.print("ON");
            delay(2000);
            break;
          }
          break;
        }
      }
    }
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Relay Select :");
  lcd.setCursor(0, 1);
  lcd.print(relayIndex + 1);
  lcd.setCursor(1, 1);
  lcd.print(".");
  lcd.setCursor(2, 1);
  lcd.print(relayItems[relayIndex]);
}

void Relay() {
  if (IrReceiver.decode()) {
    IrReceiver.resume();
    if (IrReceiver.decodedIRData.command == 22) {
      relayIndex = (relayIndex + 1) % RELAY_COUNT;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Relay Select :");
      lcd.setCursor(0, 1);
      lcd.print(relayIndex + 1);
      lcd.setCursor(1, 1);
      lcd.print(".");
      lcd.setCursor(2, 1);
      lcd.print(relayItems[relayIndex]);
    } else if (IrReceiver.decodedIRData.command == 25) {
      relayIndex = (relayIndex - 1 + RELAY_COUNT) % RELAY_COUNT;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Relay Select :");
      lcd.setCursor(0, 1);
      lcd.print(relayIndex + 1);
      lcd.setCursor(1, 1);
      lcd.print(".");
      lcd.setCursor(2, 1);
      lcd.print(relayItems[relayIndex]);
    } else if (IrReceiver.decodedIRData.command == 21) {
      r1Mode = false;
      r2Mode = false;
      r3Mode = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ON  <--");
      lcd.setCursor(0, 1);
      lcd.print("OFF");
      relayMode ();
    }else if (IrReceiver.decodedIRData.command == 70) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Select Menu: ");
      lcd.setCursor(0, 1);
      lcd.print(menuIndex + 1);
      lcd.setCursor(1, 1);
      lcd.print(".");
      lcd.setCursor(2, 1);
      lcd.print(menuItems[menuIndex]);
      relayMenu = false;
      menu = true ;
    }
  }
}

void exit() {
  lcd.setCursor(0,0);
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.clear();
  lcd.noBacklight();
  enteringPassword = true;
  menu = false;
  newPwd= false;
  vNewPwd= false;
  wifi = false;
  wifiScn = false;
}