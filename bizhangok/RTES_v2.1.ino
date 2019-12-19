// include libraries
//#include "SSD1306.h"
#include <Servo.h>
#include <ESP8266WiFi.h>
 
// setup servo
#define SERVORIGHT   45
#define SERVOCENTRE  90
#define SERVOLEFT    150
#define SERVOPIN     16
#define TRIGPIN      13    // pin no. 13  is D7 ESP8266
#define ECHOPIN      15    // pin no. 15  is D8 ESP8266
#define SAFEDIS      30

// setup WIFI
#define WIFINAME "testv"
#define WIFIPW   "123456789"

int Speed_F = 1000;  // max 1024
int Speed_S = 1000;
int TSpeed_F = 1000;  //Turning Speed
int TSpeed_S = 0;

// mode; 1 for general mode; 2 for obstacle avoidance mode.
int mod = 1; 

// WIFI part
int wifi_sig[5] = {-100, -100, -100, -100, -100};
bool flag = 0;

Servo servo;

void stoped()
{
    analogWrite(5, 1);
    analogWrite(4, 1);
}
 
void forward()
{
    analogWrite(5, Speed_S);
    analogWrite(4, Speed_F);
    digitalWrite(0, 1);
    digitalWrite(2, 1);
}
 
void back()
{
    analogWrite(5, Speed_S);
    analogWrite(4, Speed_F);
    digitalWrite(0, 0);
    digitalWrite(2, 0);
}
 
void left()
{
    analogWrite(5, TSpeed_F);
    analogWrite(4, TSpeed_S);
    digitalWrite(0, 1);
    digitalWrite(2, 1);
}
 
void right()
{
    analogWrite(5, TSpeed_S);
    analogWrite(4, TSpeed_F);
    digitalWrite(0, 1);
    digitalWrite(2, 1);
}

int ping()
{
    // pause for 50ms between scans
    delay(50);
 
    // send ping
    digitalWrite(TRIGPIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIGPIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGPIN, LOW);
 
    // read echo
    long duration = pulseIn(ECHOPIN, HIGH, 2900);
    
    // convert distance to cm
    unsigned int centimetres = int(duration / 2 / 29.1);
    if(centimetres==0) centimetres=60;
    
    return centimetres;
}

int getdis() 
{
    unsigned int sum_scan, min_scan, max_scan, temp_scan;
    min_scan = 100;
    max_scan = 0;
    sum_scan = 0;

    // scan for 5 times
    for (int i = 0; i < 5; i++) {
        temp_scan = ping();
        sum_scan += temp_scan;
        if (temp_scan < min_scan) min_scan = temp_scan;
        if (temp_scan > max_scan) max_scan = temp_scan;
    }
    sum_scan -= min_scan;
    sum_scan -= max_scan;
    
    return sum_scan / 3;
}
 
char scan()
{
    char choice;
    unsigned int left_scan, right_scan, centre_scan;

    // scan left
    servo.write(SERVOLEFT);
    delay(500);
    left_scan = getdis();
     
    // scan right
    servo.write(SERVORIGHT);
    delay(800);
    right_scan = getdis();
     
    // scan straight ahead
    servo.write(SERVOCENTRE);
    delay(500);
    centre_scan = getdis();    
    
    Serial.println("left: "+String(left_scan));
    Serial.println("center: " + String(centre_scan));
    Serial.println("right: "+String(right_scan));

    // direction
    if(right_scan < SAFEDIS || left_scan < SAFEDIS || centre_scan < SAFEDIS) choice = 'b';
    else if (left_scan>right_scan && left_scan>centre_scan) choice = 'l';
    else if (right_scan>left_scan && right_scan>centre_scan) choice = 'r';
    else choice = 'c';
    
    Serial.println("choice is "+ String(choice));
    return choice;
}

int getsignal() 
{   
    int temp, sum, max_s, min_s;
    sum = 0;
    max_s = -100;
    min_s = 0; 
    for (int i = 0; i < 10; i++) {
        int temp = WiFi.RSSI();
        sum += temp;
        if (temp > max_s) max_s = temp;
        if (temp < min_s) min_s = temp;
    }
    sum -= max_s;
    sum -= min_s;
    
    return sum / 8;
    
}

char wifimove() 
{   
    char action;
    if (wifi_sig[4] > -50) {
        action = 's';
    }
    else if (wifi_sig[4] >= wifi_sig[3]) {
        action = 'f';
    }
    else if (flag && wifi_sig[4] < wifi_sig[3]) {
        action = 'l';
    }
    else {
        action = 'r';
        flag = 1;
    }

    return action;
}
 
void setup()
{
    Serial.begin(115200);

    WiFi.begin(WIFINAME,WIFIPW);
    Serial.print("Connecting...");
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");    
    }
    
    // set the servo data pin
    servo.attach(SERVOPIN);
    pinMode(5, OUTPUT);
    pinMode(4, OUTPUT);
    pinMode(0, OUTPUT);
    pinMode(2, OUTPUT);
    digitalWrite(5, 0);
    digitalWrite(4, 0);
    digitalWrite(0, 1);
    digitalWrite(2, 1);
    // set the trig pin to output (send sound waves)
    pinMode(TRIGPIN, OUTPUT);
    // set the echo pin to input (receive sound waves)
    pinMode(ECHOPIN, INPUT);
}

void loop()
{
    stoped();
    delay(500);
    char dire = scan();
    
    int wifi = getsignal();
    for (int i = 0; i < 4; i++) {
        wifi_sig[i] = wifi_sig[i+1];  
        Serial.println("signal: "+String(wifi_sig[i]));
    }
    wifi_sig[4] = wifi;
    Serial.println("signal: "+String(wifi));
    
    switch (dire) {
        case 'l':
            Serial.println("turn left to avoid");
            left();
            delay(1000);
            break;
        case 'r':
            Serial.println("turn right to avoid");
            right();
            delay(1000);
            break;
        case 'b':
            back();
            delay(1000);
            right();
            delay(700);
            break;
        default:
            if (mod == 2) {
                Serial.println("forward to avoid");
                forward();
                delay(700);
                break;
            }
            char mov = wifimove();
            if (mov == 's') {
                stoped();
                delay(20000);
                while (1) {;}
            }
            else if (mov == 'f') {
                Serial.println("forward to wifi");
                forward();
                delay(700);
            }
            else if (mov == 'l') {
                Serial.println("turn left to wifi");
                left();
                delay(1000);
            }
            else {
                Serial.println("turn right to wifi");
                right();
                delay(1000);
            }
            break;  
            
    }

    Serial.println("\n");
}
