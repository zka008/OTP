#include <Wire.h>
#include <ThreeWire.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <RtcDS1302.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

//WIFI
const char* ssid = "ssid";
const char* password = "";
WiFiUDP UDP;
NTPClient timeClient(UDP, "kr.pool.ntp.org", 32400, 3600000);
//최소 1시간을 간격으로 업데이트 시도, 1시간 이전에는 update()를 호출해도 실행하지 않음

//RTC
const int RC_RST = 4;                      //주황 D4
const int RC_DAT = 14;                     //빨강 D5
const int RC_CLK = 12;                     //갈색 D6
ThreeWire myWire(RC_DAT, RC_CLK, RC_RST);  // IO, SCLK, CE
RtcDS1302<ThreeWire> RTC(myWire);

//BT
const int BL_RX = 13;             // 초록 D7
const int BL_TX = 0;              // 파랑 D8
SoftwareSerial BT(BL_TX, BL_RX);  //보드 RX와 BT모듈 TX, 보드 TX와 BT모듈 RX 연결

//LCD
const int LCD_SDA = 4;               //파랑 D14
const int LCD_SCL = 5;               //초록 D15
LiquidCrystal_I2C LCD(0x27, 16, 2);  // I2C 어댑터 모듈 주소와 LCD 모듈 행과 열 수를 지정

//지문인식

//버튼

//부저
bool SOUND = false;

//OTP
const int OTP_SERIAL = 1004;
int YEAR, MONTH, DATE, HOUR, MINUTE, SECOND = 0;

void setup() {
  Serial.begin(9600);
  Serial.setDebugOutput(true);
  BT_init();
  RTC_init();
  LCD_init();
  WIFI_NTP_init();
  // pinMode(15, PULL);
}

void loop() {
  delay(1000);
  BT_TxRx();     //블루투스 송수신, 매 초마다 실행
  NTP_update();  //ntp 업데이트 및 시간 출력

  static unsigned long last_rtc_update = 0;
  unsigned long current_time = millis();
  if (current_time - last_rtc_update >= 5000) {
    last_rtc_update = current_time;
    RTC_refresh();
  }

  get_NTP();
  LCD_print(BTN_press());
}

//블루투스 초기화 및 송수신
void BT_init() {
  BT.begin(9600);
}
void BT_TxRx() {
  //시리얼 모니터에 수신 내역 출력
  while (BT.available()) {
    Serial.write(BT.read());
  }
  //시리얼 모니터에 입력한 문자열 BT 송신
  while (Serial.available()) {
    BT.write(Serial.read());
  }

  String message = BT.readString();
  Serial.write("\n");
  Serial.print(message);
  if (message.equals("getSerialNumgetSerialNum")) {
    BT.write(OTP_SERIAL);
    Serial.println("serial tx");
  }
}

//와이파이 및 NTP 초기화
void WIFI_NTP_init() {
  WiFi.disconnect();
  WiFi.setOutputPower(19.25);
  // WiFi.forceSleepWake();
  // WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // WiFi.setAutoConnect(true);//자동 연결 설정
  // WiFi.setAutoReconnect(true);//자동 재연결 설정
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WIFI CONNECTED");
  timeClient.begin();
  timeClient.update();
}

//NTP 업데이트 및 전역 변수(시간정보)에 전달
void NTP_update() {
  Serial.println(timeClient.getFormattedTime());
}
void get_NTP() {
  DATE = timeClient.getDay();
  HOUR = timeClient.getHours();
  MINUTE = timeClient.getMinutes();
  SECOND = timeClient.getSeconds();
  String s1 = " ";
  String s2 = ":";
  String date = MONTH + s1 + DATE + s1 + YEAR;
  String time = HOUR + s2 + MINUTE + s2 + SECOND;
}

//RTC 초기화 및 출력
void RTC_init() {
  RTC.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);  //컴파일한 시간으로 설정
  RTC.SetDateTime(compiled);
}
void RTC_print() {
  RtcDateTime now = RTC.GetDateTime();
  Serial.print(now.Year(), DEC);
  Serial.print('/');
  Serial.print(now.Month(), DEC);
  Serial.print('/');
  Serial.print(now.Day(), DEC);
  Serial.print(' ');
  Serial.print(now.Hour(), DEC);
  Serial.print(':');
  Serial.print(now.Minute(), DEC);
  Serial.print(':');
  Serial.print(now.Second(), DEC);
  Serial.println();
}
void RTC_refresh() {
  //RTC에 NTP로 받아온 시간을 넣어서 보정해야함
}

//LCD 초기화 및 버튼 입력시 otp 10초간 출력
void LCD_init() {
  LCD.init();                  // LCD 모듈 초기화
  LCD.backlight();             // 백라이트 켜기
  LCD.print("Hello, world!");  // 초기값
}
void LCD_print(bool show) {
  if (show == true) {
    LCD.init();
    LCD.print(get_otp());
    delay(10000);
  } else {
    LCD.init();
    LCD.setCursor(0, 0);
    LCD.print("   ");
    LCD.print(YEAR);
    LCD.print("/");
    LCD.print(MONTH);
    LCD.print("/");
    LCD.print(DATE);
    LCD.setCursor(0, 1);
    LCD.print("     ");
    LCD.print(HOUR);
    LCD.print(":");
    LCD.print(MINUTE);
  }
}

//버튼
bool BTN_press() {
  Serial.println(digitalRead(15));
  if (digitalRead(15) == 1) {
    return true;
  } else {
    return false;
  }
}

//OTP 생성
int get_otp() {
  long otp = OTP_SERIAL * YEAR * MONTH * DATE * HOUR * MINUTE;
  bool check = true;
  while (check) {
    if (otp > 1000000)
      otp = otp / 10;
    else if (otp < 99999)
      otp = otp * 10;
    else
      check = false;
  }
  return otp;
}