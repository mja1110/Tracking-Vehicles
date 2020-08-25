#include <SoftwareSerial.h>
#include <Wire.h> //Thư viện giao tiếp I2C
SoftwareSerial sim808(11, 10); //Arduino(RX), Arduino(TX)

const int MPU_addr = 0x68; //Địa chỉ I2C của MPU6050
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ; //Các biến lấy giá trị
int state;
int accident;
#define DEBUG true
unsigned long time1 = 0;

String  latitude, longitude;

void getGPSLocation()
{
  sendData("AT+CGNSPWR=1", 1000, DEBUG); //Turn on GPS(GNSS - Global Navigation Satellite System)
  delay(50);
  sendData("AT+CGNSSEQ=RMC", 1000, DEBUG);

  delay(150);

  //--------------------- send SMS containing google map location---------
  sendTabData("AT+CGNSINF", 1000, DEBUG); //Get GPS info(location)
  Serial.println("GPS Initializing…");
}

void sendTabData(String command , const int timeout , boolean debug) {

  sim808.println(command);
  long int time = millis();
  int i = 0;

  while ((time + timeout) > millis()) {
    while (sim808.available()) {
      String c = sim808.readString();
      Serial.print(c);
      //      int x = c.indexOf(',');
      //      Serial.print(x);
      String x = c.substring(44, 68);
      Serial.println(x);
      String a = x.substring(x.indexOf(',') + 1, x.lastIndexOf(','));
      Serial.println(a);
      latitude = a.substring(0, a.indexOf(','));
      Serial.println("latitude: " + latitude);
      longitude = a.substring(a.indexOf(',') + 1, a.length());
      Serial.println("longitude: " + longitude);

      //      latitude = la.toFloat();
      //      longitude = lo.toFloat();

    }
  }
}

String sendData (String command , const int timeout , boolean debug) {
  String response = "";
  sim808.println(command);
  long int time = millis();
  int i = 0;

  while ( (time + timeout ) > millis()) {
    while (sim808.available()) {
      char c = sim808.read();
      response += c;
    }
  }
  if (debug) {
    Serial.print(response);
  }
  return response;
}


void ShowSerialData()
{
  while (sim808.available() != 0)
    Serial.write(sim808.read());
  delay(5000);

}

void sendThingspeak() {
  sim808.println("AT");
  delay(1000);

  sim808.println("AT+CPIN?");
  delay(1000);

  sim808.println("AT+CREG?");
  delay(1000);

  sim808.println("AT+CGATT?");
  delay(1000);

  sim808.println("AT+CIPSHUT");
  delay(1000);

  sim808.println("AT+CIPSTATUS");
  delay(2000);

  sim808.println("AT+CIPMUX=0");
  delay(2000);

  ShowSerialData();

  sim808.println("AT+CSTT=\"airtelgprs.com\"");//start task and setting the APN,
  delay(1000);

  ShowSerialData();

  sim808.println("AT+CIICR");//bring up wireless connection
  delay(3000);

  ShowSerialData();

  sim808.println("AT+CIFSR");//get local IP adress
  delay(2000);

  ShowSerialData();

  sim808.println("AT+CIPSPRT=0");
  delay(3000);

  ShowSerialData();

  sim808.println("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",\"80\"");//start up the connection
  delay(6000);

  ShowSerialData();

  sim808.println("AT+CIPSEND");//begin send data to remote server
  delay(4000);
  ShowSerialData();

  String str = "GET https://api.thingspeak.com/update?api_key=N48O1H3YEMZU1NTU&field1=" + latitude + "&field2=" + longitude;
  Serial.println(str);
  sim808.println(str);//begin send data to remote server

  delay(4000);
  ShowSerialData();

  sim808.println((char)26);//sending
  delay(5000);//waitting for reply, important! the time is base on the condition of internet
  sim808.println();

  ShowSerialData();

  sim808.println("AT+CIPSHUT");//close the connection
  delay(100);
  ShowSerialData();
}


void setup() {
  pinMode(5, INPUT_PULLUP);
  pinMode(7, OUTPUT);
  pinMode(6, OUTPUT);
  Wire.begin();
  Wire.beginTransmission(MPU_addr); //Gửi tín hiệu đến địa chỉ MPU
  Wire.write(0x6B);
  Wire.write(0);     //Đưa về 0 để bật MPU
  Wire.endTransmission(true);
  sim808.begin(9600);
  Serial.begin(9600);
  Serial.println("Arduino MQTT Tutorial, Valetron Systems @www.raviyp.com ");
  delay(3000);
}

void readMPU() {
  Wire.beginTransmission(MPU_addr); //Gửi tín hiệu đến địa chỉ MPU
  Wire.write(0x3B);                 //Gửi giá trị lên MPU
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 14, true); //Đề nghị thanh ghi của MPU

  //Đọc dữ liệu
  AcX = Wire.read() << 8 | Wire.read(); //Gia tốc trục x
  AcY = Wire.read() << 8 | Wire.read(); //Gia tốc trục y
  AcZ = Wire.read() << 8 | Wire.read(); //Gia tốc trục z
  Tmp = Wire.read() << 8 | Wire.read(); //Nhiệt độ
  GyX = Wire.read() << 8 | Wire.read(); //Góc nghiên trục x
  GyY = Wire.read() << 8 | Wire.read(); //Góc nghiên trục y
  GyZ = Wire.read() << 8 | Wire.read(); //Góc nghiên trục z

  //Xuất ra Serial
  Serial.print("AcX = "); Serial.print(AcX);
  Serial.print(" | AcY = "); Serial.print(AcY);
  Serial.print(" | AcZ = "); Serial.println(AcZ);

}


void loop() {
  //  unsigned long k= millis();
  readMPU();
  if (AcY > 15000 || AcY < -15000) {
    accident = 1;
    digitalWrite(7, 1);
    digitalWrite(6, 1);
    getGPSLocation();
    delay(5000);
    sendThingspeak();
  }
  else {
    accident = 0;
    digitalWrite(7, 0);
    digitalWrite(6, 0);
  }
  state = digitalRead(5);
  Serial.println("Switch: " + (String)state);

  while (state) {
    getGPSLocation();
    delay(5000);
    float last_latitude = latitude.toFloat();
    float last_longitude = longitude.toFloat();
    float x1 = latitude.toFloat() - last_latitude;
    float x2 = longitude.toFloat() - last_longitude;
    Serial.print("x1 = ");
    Serial.println(x1, 7);
    Serial.print("x2 = ");
    Serial.println(x2, 7);
    float r = sqrt((x1 * x1) + (x2 * x2));
    Serial.print("r = ");
    Serial.println(r, 7);
    if (r > 0.03 ) {
      sendThingspeak();
    }
    break;
  }

  if (millis - time1 > 300000)
  {
    getGPSLocation();
    delay(5000);
    sendThingspeak();
    //k=millis()-k;
    //Serial.println(k);
    time1 = millis();
  }
}
