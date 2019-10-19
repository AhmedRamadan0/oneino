#include <Arduino.h>
#include <pinDef.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <stdint.h>
#include <template.h>
#include <memory>



//Global declarations start here>>
const unsigned int GPIOPIN[TOTAL_DEVICES] = {D1, D2, D5, D7};  //{D1, D2, D3, D4, D5, D6, D7, D8};
String device_Actstate[TOTAL_DEVICES] = {"On", "On", "On", "On"};
bool dev_active_state[TOTAL_DEVICES] = { false };
const uint8_t boot_mode_flag = 5;
bool dev_timer_off_status[TOTAL_DEVICES] = { false };
bool dev_timer_on_status[TOTAL_DEVICES] = { false };
unsigned long dev_timer_user_input[TOTAL_DEVICES] = {0, 0, 0, 0};
unsigned long device_timer_stop_timestamp[TOTAL_DEVICES] = {0, 0, 0, 0 };
bool device_rollover_flip_flag[TOTAL_DEVICES] = { false };
unsigned long currentTime = 0;



//Function declarations start here>>
void define_pins();
uint8_t check_boot_flag();
bool boot_manager();
void nought_init();
String ap_ssid();
void handleRoot();
void wifiScan();
void wifiManScan();
void apConfig();
void handleNotFound();
void handleConfig();
String deviceControl();
void dev_handler();
void updateGPIO(int );
void handleTimer(bool , int , String , String );
bool deviceTimerRolloverCheck(unsigned long , int );
void timerFunction();
void configPortal();
void handleReset();
//Function declarations end here<<

ESP8266WebServer server(80);

void setup() {
  define_pins();
  if(boot_manager() == true && MDNS.begin("oneino"))
  {
    Serial.println("MDNS started at oneino.local");
    server.on("/", dev_handler);
    configPortal();
  }
  else
  {
    nought_init();
    configPortal();
  }
}

void loop() {
  server.handleClient();
  delay(100);
  timerFunction();
}

//Define pins
void define_pins()
{
  for(int pin_no = 0 ; pin_no < 4 ; pin_no++)
  {
    pinMode(GPIOPIN[pin_no],OUTPUT);
    digitalWrite(GPIOPIN[pin_no],HIGH);   //Check initial state with your relay and switch accordingly
  }
  Serial.begin(57600);
  delay(500);
  Serial.println();
  Serial.println("Oneino initializing");
}

//initializing starts here>>
uint8_t check_boot_flag()
{
  EEPROM.begin(512);
  Serial.println("Checking boot mode");
  uint8_t boot_mode = EEPROM.read(boot_mode_flag);
  yield();
  if (boot_mode==AP_FLAG)
    {                   //boot_mode=2 : WiFiStation
      Serial.println("Starting as Access Point");
      return AP_FLAG;
    }
    if (boot_mode==STA_FLAG)
    {                   //boot_mode=3 : SoftAP
      Serial.println("Starting as WiFiStation");
      return STA_FLAG;
    }
    if (boot_mode==AP_STA_FLAG)
    {                   //boot_mode=4 : WiFiStation+SoftAP
      Serial.println("Starting as WiFiStation+SoftAP");
      return AP_STA_FLAG;
    }
    else
    {
      Serial.println("No Config Found. initializing....");
      delay(100);
      return 0;
    }
}

bool boot_manager()
{
  uint8_t boot_mode = check_boot_flag();
  if(boot_mode == AP_FLAG)
  {
    uint8_t connected_clients = WiFi.softAPgetStationNum();
    for(unsigned int count_try=0; connected_clients < 1; count_try++)
    {
      Serial.printf("Waiting for a client for %d sec\n", count_try);
      delay(2000);
      if (WiFi.softAPgetStationNum() > 0)
      {
        Serial.println("Client found at initialization");
        return true;
      }
      if (count_try > 300)   //300=10mins approx due to 2sec delay
      {
        Serial.println("Timed out waiting for a client. initializing in default configuration");
        nought_init();
        break;
      }
    }
  }
  if (boot_mode == STA_FLAG)
  {
    int retry_time = 0;
    Serial.println("Waiting for Connection to the Access Point");
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.printf("Waiting for %d sec", retry_time);
      delay(2000);
      retry_time++;
      if (retry_time > 60)
      {
        Serial.println("Timed out waiting for connection");
        nought_init();
        break;
      }
      if (WiFi.status() == WL_NO_SSID_AVAIL)
      {
        Serial.println("The configured SSID is unavailable");
        nought_init();
        break;
      }
      if (retry_time > 10 && WiFi.status() == WL_CONNECT_FAILED)
      {
        Serial.println("Error connecting to the host. Password Mismatch");
        nought_init();
        break;
      }
      if(WiFi.status() == WL_CONNECTED)
      return true;
    }
  }
  //Insert condition for AP_STA_FLAG
}

void nought_init()
{
  WiFi.disconnect();

  WiFi.mode(WIFI_AP_STA);
  yield();
  WiFi.softAP(ap_ssid().c_str(), ap_ssid().c_str());    //Modifying the Password field may be necessary
  Serial.println(WiFi.softAPIP());
  yield();
}

String ap_ssid()
{
  long int chip_id = ESP.getChipId();
  yield();
  unsigned int init_serial = 0;
  init_serial = chip_id % 10000000000;   //Last ten digits of the chip_id
  String ssid = "oneino";
  ssid += String(init_serial);
  yield();
  return ssid;
}
//initializing ends here<<

//WiFiManager starts here>>
void handleRoot()
{
  String html;
  html = FPSTR(HTTP_HEAD);
  html += FPSTR(HTTP_STYLE);
  html += FPSTR(HTTP_HEAD_END);
  html += FPSTR(HTTP_ROOT);

  server.send(200, "text/html", html);
}

void wifiScan()
{
  WiFi.scanNetworks();
  delay(5000);
  String html = FPSTR(HTTP_HEAD);
  html += FPSTR(HTTP_STYLE);
  html += FPSTR(HTTP_SCRIPT);
  html += FPSTR(HTTP_HEAD_END);
  html += FPSTR(WIFI_AUTO_SCAN);
  yield();
  html.replace("@SSID1@", WiFi.SSID(0));
  html.replace("@SIG1@", (String) WiFi.RSSI(0));
  html.replace("@SSID2@", WiFi.SSID(1));
  html.replace("@SIG2@", (String) WiFi.RSSI(1));
  html.replace("@SSID3@", WiFi.SSID(2));
  html.replace("@SIG3@", (String) WiFi.RSSI(2));
  html.replace("@SSID4@", WiFi.SSID(3));
  html.replace("@SIG4@", (String) WiFi.RSSI(3));
  yield();
  server.send(200, "text/html", html);
}

void wifiManScan()
{
  String html = FPSTR(HTTP_HEAD);
  html += FPSTR(HTTP_STYLE);
  html += FPSTR(HTTP_HEAD_END);
  html += FPSTR(WIFI_MAN_SCAN);
  server.send(200, "text/html", html);
}

void apConfig()
{
  String html = FPSTR(HTTP_HEAD);
  html += FPSTR(HTTP_STYLE);
  html += FPSTR(HTTP_HEAD_END);
  html += FPSTR(AP_CONFIG);
  server.send(200, "text/html", html);
}

void handleReset()
{
  EEPROM.begin(512);
  EEPROM.write(boot_mode_flag, 1);
  EEPROM.commit();
  ESP.reset();
}

void handleNotFound()
{
  server.send(400, "text/html", "<h1 style=\"margin-left:auto; margin-right:auto\">Please Configure the device first to access control.</h1>");
}

void handleConfig()
{
  if ((!server.hasArg("stassid") || !server.hasArg("stapass") || server.arg("stassid")==NULL || server.arg("stapass")==NULL) && (!server.hasArg("passap1") || server.hasArg("passap2") || server.arg("passap1")==NULL || server.arg("passap2")==NULL))
  {
     server.send(400, "text/plain", "400: Invalid request");
  }
  if (server.hasArg("stassid") && server.hasArg("stapass"))
  {
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(server.arg("stassid").c_str(), server.arg("stapass").c_str());
    Serial.println("Starting in Station mode");
    Serial.println(WiFi.localIP());
    EEPROM.begin(512);
    EEPROM.write(boot_mode_flag, STA_FLAG);
    EEPROM.commit();
    ESP.reset();
  }
  if (server.hasArg("passap1") && server.hasArg("passap2"))
  {
    if (server.arg("passap1") != server.arg("passap2"))
    {
      server.send(400, "text/plain", "Passwords do not match. Retry.");
    }
    else
    {
      WiFi.disconnect();
      WiFi.mode(WIFI_AP);
      WiFi.softAP(ap_ssid().c_str(), server.arg("passap1").c_str());
      Serial.println("Starting in AP mode");
      Serial.println(WiFi.softAPIP());
      EEPROM.begin(512);
      EEPROM.write(boot_mode_flag, AP_FLAG);
      EEPROM.commit();
      ESP.reset();
    }
  }
}


//Device controller starts here>>

/*String deviceControl()
{
  String html = FPSTR(HTTP_HEAD);
  html += FPSTR(HTTP_STYLE);
  html += FPSTR(HTTP_HEAD_END);
  html += FPSTR(DEVICE_CONTROL);
  return html;
}
*/

String deviceControl()
{
  String html = FPSTR(HTTP_HEAD);
  html += FPSTR(HTTP_STYLE);
  html += FPSTR(HTTP_HEAD_END);
  html +="<body> <div style=\"text-align:left;display:inline-block;min-width:260px;\"> <h4 style=\"text-align: center;\"> Device 1</h4> <h5>Timer Set : ";
  html += dev_timer_user_input[0]/60000;
  html +="</h5> <div> <form action=\"/\" method=\"POST\"> <button name=\"D1\">";
  html += device_Actstate[0];
  html +="</button> </form> <div>Timer: <form action=\"/\" method=\"POST\"> <input type=\"number\" name=\"timerD1h\" min=\"1\" max=\"20\" placeholder=\"hour(s)\"> <input type=\"number\" name=\"timerD1m\" min=\"1\" max=\"60\" placeholder=\"min(s)\"> <input type=\"submit\" value=\"Set Timer\"></form> </div> </div> <h4 style=\"text-align: center;\"> Device 2</h4> <h5>Timer Set : ";
  html += dev_timer_user_input[1]/60000;
  html +="</h5> <div> <form action=\"/\" method=\"POST\"> <button name=\"D2\">";
  html += device_Actstate[1];
  html +="</button> </form> <div>Timer: <form action=\"/\" method=\"POST\"> <input type=\"number\" name=\"timerD2h\" min=\"1\" max=\"20\" placeholder=\"hour(s)\"> <input type=\"number\" name=\"timerD2m\" min=\"1\" max=\"60\" placeholder=\"min(s)\"> <input type=\"submit\" value=\"Set Timer\"></form> </div> </div> <h4 style=\"text-align: center;\"> Device 3</h4> <h5>Timer Set : ";
  html += dev_timer_user_input[2]/60000;
  html +="</h5> <div> <form action=\"/\" method=\"POST\"> <button name=\"D3\">";
  html += device_Actstate[2];
  html +="</button> </form> <div>Timer: <form action=\"/\" method=\"POST\"> <input type=\"number\" name=\"timerD3h\" min=\"1\" max=\"20\" placeholder=\"hour(s)\"> <input type=\"number\" name=\"timerD3m\" min=\"1\" max=\"60\" placeholder=\"min(s)\"> <input type=\"submit\" value=\"Set Timer\"></form> </div> </div> <h4 style=\"text-align: center;\"> Device 4</h4> <h5>Timer Set : ";
  html += dev_timer_user_input[3]/60000;
  html +="</h5> <div> <form action=\"/\" method=\"POST\"> <button name=\"D4\">";
  html += device_Actstate[3];
  html +="</button> </form> <div>Timer: <form action=\"/\" method=\"POST\"> <input type=\"number\" name=\"timerD4h\" min=\"1\" max=\"20\" placeholder=\"hour(s)\"> <input type=\"number\" name=\"timerD4m\" min=\"1\" max=\"60\" placeholder=\"min(s)\"> <input type=\"submit\" value=\"Set Timer\"></form> </div> </div> </div> </body> </html>";
  return html;
}

void dev_handler()
{
  if(server.hasArg("D1"))
  updateGPIO(0);
  if(server.hasArg("D2"))
  updateGPIO(1);
  if(server.hasArg("D3"))
  updateGPIO(2);
  if(server.hasArg("D4"))
  updateGPIO(3);
  if (dev_active_state[0]==true && (server.hasArg("timerD1h") || server.hasArg("timerD1m")))
      handleTimer(1, 0, server.arg("timerD1h"), server.arg("timerD1m"));
  if (dev_active_state[1]==true && (server.hasArg("timerD2h") || server.hasArg("timerD2m")))
      handleTimer(1, 1, server.arg("timerD2h"), server.arg("timerD2m"));
  if (dev_active_state[2]==true && (server.hasArg("timerD3h") || server.hasArg("timerD3m")))
      handleTimer(1, 2, server.arg("timerD3h"), server.arg("timerD3m"));
  if (dev_active_state[3]==true && (server.hasArg("timerD4h") || server.hasArg("timerD4m")))
      handleTimer(1, 3, server.arg("timerD4h"), server.arg("timerD4m"));
  if (dev_active_state[0]==false && (server.hasArg("timerD1h") || server.hasArg("timerD1m")))
      handleTimer(0, 0, server.arg("timerD1h"), server.arg("timerD1m"));
  if (dev_active_state[1]==false && (server.hasArg("timerD2h") || server.hasArg("timerD2m")))
      handleTimer(0, 1, server.arg("timerD2h"), server.arg("timerD2m"));
  if (dev_active_state[2]==false && (server.hasArg("timerD3h") || server.hasArg("timerD3m")))
      handleTimer(0, 2, server.arg("timerD3h"), server.arg("timerD3m"));
  if (dev_active_state[3]==false && (server.hasArg("timerD4h") || server.hasArg("timerD4m")))
      handleTimer(0, 3, server.arg("timerD4h"), server.arg("timerD4m"));
  else
  server.send(200, "text/html", deviceControl());
}

void updateGPIO(int device_number)
{
  Serial.println("");
  Serial.println("Update GPIO ");
  Serial.print(GPIOPIN[device_number]);
  Serial.print(" -> ");
  if(dev_active_state[device_number] == false)
  {
    digitalWrite(GPIOPIN[device_number], LOW);
    device_Actstate[device_number] = "Off";
    Serial.print(!dev_active_state[device_number]);
    server.sendHeader("Location", String("/"), true);
    server.send(302, "text/html", deviceControl());
    dev_active_state[device_number] = true;
  }
  else if(dev_active_state[device_number] == true)
  {
    digitalWrite(GPIOPIN[device_number], HIGH);
    device_Actstate[device_number] = "On";
    Serial.print(!dev_active_state[device_number]);
    server.sendHeader("Location", String("/"), true);
    server.send(302, "text/html", deviceControl());
    dev_active_state[device_number] = false;
  }
}

//Device Controller ends here<<


//Timer controller starts here
void handleTimer(bool timer_mode, int device_number, String timeSetHour, String timeSetMin)
{
  if (timer_mode == 1)  //Stop after the set timer
  {
    if ((timeSetHour.toInt() > 0 || timeSetMin.toInt() > 0) && (timeSetHour != NULL || timeSetMin != NULL))
    {
      if(timeSetHour == NULL)
      {
        timeSetHour = "0";
      }
      if (timeSetMin == NULL)
      {
        timeSetMin = "0";
      }
      dev_timer_user_input[device_number] = ((timeSetHour.toInt()) * 60 * 60 * 1000) + (timeSetMin.toInt() * 60 * 1000);
      device_rollover_flip_flag[device_number] = deviceTimerRolloverCheck(dev_timer_user_input[device_number], device_number);
      if(device_rollover_flip_flag[device_number] == true)
      {
        Serial.println("Timer Rollover detected");
      }
      else
      {
        Serial.println("No Rollover detected");
      }
      Serial.printf("%d GPIO will stop in\n", GPIOPIN[device_number]);
      Serial.println(dev_timer_user_input[device_number]/60000);
      Serial.print("Min(s)");
      dev_timer_off_status[device_number] = true;
    }
  }
  if (timer_mode == 0)  // Start after the set timer
  {
    if ((timeSetHour.toInt() > 0 || timeSetMin.toInt() > 0) && (timeSetHour != NULL || timeSetMin != NULL))
    {
      if(timeSetHour == NULL)
      {
        timeSetHour = "0";
      }
      if (timeSetMin == NULL)
      {
        timeSetMin = "0";
      }
    }
    dev_timer_user_input[device_number] = ((timeSetHour.toInt()) * 60 * 60 * 1000) + (timeSetMin.toInt() * 60 * 1000);
    device_rollover_flip_flag[device_number] = deviceTimerRolloverCheck(dev_timer_user_input[device_number], device_number);
    if(device_rollover_flip_flag[device_number] == true)
    {
      Serial.println("Timer Rollover detected");
    }
    else
    {
      Serial.println("No Rollover detected");
    }
    Serial.printf(" %d GPIO will start in\n", GPIOPIN[device_number]);
    Serial.println(dev_timer_user_input[device_number]/60000);
    Serial.print("Min(s)");
    dev_timer_on_status[device_number] = true;
  }
}

bool deviceTimerRolloverCheck(unsigned long dev_timer, int device_number)
{
  unsigned long device_timer_start_timestamp = millis();
  if ((device_timer_start_timestamp + dev_timer) >= TIMER_ROLLOVER_FLAG)
  {
    device_timer_stop_timestamp[device_number] = TIMER_ROLLOVER_FLAG - (device_timer_start_timestamp + dev_timer);

    return true;
  }
  else
  {
    device_timer_stop_timestamp[device_number] = dev_timer + device_timer_start_timestamp;
    return false;
  }
}


void timerFunction()
{
  currentTime = millis();
  for (uint8_t i = 0; i < TOTAL_DEVICES; i++)
  {
    if (dev_timer_off_status[i]==true && device_timer_stop_timestamp[i] <= currentTime)
    {
      if(device_rollover_flip_flag[i] == true)
      {
        if (device_timer_stop_timestamp[i] >= currentTime)
        {
          device_rollover_flip_flag[i] = false;
        }
        else
        {
          continue;
        }
      }
      dev_active_state[i] = true;
      updateGPIO(i);
      //server.send(200, "text/html", deviceControl());
      dev_timer_off_status[i] = false;
      //dev_active_state[i] = false;
    }
    yield();
    if (dev_timer_on_status[i]==true && device_timer_stop_timestamp[i] <= currentTime)
    {
      if(device_rollover_flip_flag[i] == true)
      {
        if (device_timer_stop_timestamp[i] >= currentTime)
        {
          device_rollover_flip_flag[i] = false;
        }
        else
        {
          continue;
        }
      }
      dev_active_state[i] = false;
      updateGPIO(i);
      //server.send(200, "text/html", deviceControl());
      dev_timer_on_status[i] = false;
      //dev_active_state[i] = true;
    }
    yield();
  }
}

//Timer controller ends here

void configPortal()
{
  server.on("/root", HTTP_GET, handleRoot);
  server.on("/wifiscan", HTTP_GET, wifiScan);
  server.on("/ap_config", HTTP_GET, apConfig);
  server.on("/manscan", HTTP_GET, wifiManScan);
  server.on("/handleConfig", HTTP_POST, handleConfig);
  server.on("/reset", HTTP_POST, handleReset);
  server.onNotFound(handleNotFound);
  server.begin();
}
