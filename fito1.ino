
#define BLYNK_PRINT Serial
#define NASOS 0
#define SVET 2
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <SimpleTimer.h>
#include <WiFiUdp.h>                                                    // библиотека для получения времени из сети
unsigned int localPort = 2390; 
// local port to listen for UDP packets

/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;


char chas, minut, secund;



// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "c22ca6192f2a4026923b7806adec95c2";

//const char* ssid = "Wi-Fi_Chek";
//const char* password = "01020304bb";
const char ssid[] = "Wi-Fi_Chek";
const char pass[] = "01020304bb";

SimpleTimer com_timer;
SimpleTimer timer_oprosa;
SimpleTimer timer;


void setup()
{
  
  pinMode(NASOS, OUTPUT); //gpio0                          
  pinMode(SVET, OUTPUT); //gpio2                          
                                              
  digitalWrite(NASOS , HIGH);
  digitalWrite(SVET , HIGH);

com_timer.setInterval(10000L, send_com_port);
timer.setInterval(10000L, sendUptime);
timer_oprosa.setInterval(10000L, opros);


   // Debug console
  Serial.begin(9600);

  Blynk.begin(auth, ssid, pass);

  ArduinoOTA.setHostname("Fito-001");
  Serial.println("Booting");
 
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
    }
  
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


    Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  
}



void Time_zapros ()
{

  
//  Serial.print();
   //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP); 

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println(" ... нет ");
    //Time_zapros ();
  }
  else {
//    Serial.print("packet received, length=");
//    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
//    Serial.print("Seconds since Jan 1 1900 = " );
//    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
//    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
//    Serial.println(epoch);


    // print the hour, minute and second:
    //Serial.print("Time ");       // UTC is the time at Greenwich Meridian (GMT)
    
    chas = ((epoch  % 86400L) / 3600) +3;
    minut = (epoch  % 3600) / 60;
    secund = epoch % 60; 
    Serial.print(((epoch  % 86400L) / 3600) +3); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print(epoch % 60); // print the second
    Serial.println(" ");
  }
  // wait ten seconds before asking for the time again
  //delay(10000);
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
//  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
  //Serial.println();
  
  
  
  };



void opros()
{
yield();
  
  if (  chas >= 8 && chas <= 20 )
  {
     //digitalWrite(NASOS , LOW);
     digitalWrite(SVET , LOW);
     }
  else
  {
    digitalWrite(SVET , HIGH);
    
    };

  if (  chas >= 8 && chas <= 20 &&minut >= 10 && minut <= 15)
    {
      digitalWrite(NASOS , LOW);
    
    } 
    else
    {
      digitalWrite(NASOS , HIGH);
            
      };



    

   
};


void send_com_port()

{
 yield();                                                                                       // вывод в порт


  Time_zapros();

    
};

void poliv ()
{



}
  


  
void sendUptime()                                                    // опрос по тамеру и принятие решений на счет влажности почвы
{

  yield();

  poliv();                                                          // запуск и остановка полива
   
};
  



void loop()
{
  ArduinoOTA.handle();
  Blynk.run();
  com_timer.run();                                                       // запуск таймера для отправки данных в КОМ порт
  timer.run();                                                           // запуск таймера
  timer_oprosa.run();
}

