#define SSID        "SSID"
#define PASS        "password"
#define DEST_HOST   "8.8.8.8"
#define TIMEOUT     10000 // mS
#define CONTINUE    false
#define HALT        true

#include <RH_NRF24.h>
#include <SPI.h>
#include <SoftwareSerial.h>

SoftwareSerial Serial1(2, 3); // RX, TX

RH_NRF24 nrf24;

char reqMsg[10];

int i = 0;
String data;

void setup()
{
  
  //Start general setup
  delay(200);
  Serial.begin(9600);             // Debugging only
  Serial1.begin(9600);            // Communication with ESP8266
  Serial1.setTimeout(TIMEOUT);    // Set timeout
  Serial.println("setup");

  // Radio setup
    if (!nrf24.init()){
      Serial.println("radio init failed");
    } else{
      Serial.println("radio init success");
    }
    
    // Set to 250kbps for increased range
    if (!nrf24.setChannel(2)){
      Serial.println("setChannel failed");
    }    
    if (!nrf24.setRF(RH_NRF24::DataRate250kbps, RH_NRF24::TransmitPower0dBm)){
      Serial.println("setRF failed");
    }
  
  //Wifi setup
    echoCmd("AT+RST", "ready", HALT);    // Reset & test if the module is ready  
    Serial.println("Module is ready.");
    echoCmd("AT+GMR", "OK", CONTINUE);   // Retrieves the firmware ID (version number) of the module. 
    echoCmd("AT+CWMODE?","OK", CONTINUE);// Get module access mode. 
    echoCmd("AT+CWMODE=1", "", HALT);    // Station mode

  //connect to the wifi
    boolean connection_established = false;
    for(int i=0;i<5;i++)
    {
      if(connectWiFi())
      {
        connection_established = true;
        break;
      }
    }
    if (!connection_established) errorHalt("Connection failed");
       
}

// DATA FUNCTIONS START HERE  ---------------------------------------------------------------------------------------------------------------------

// Function to trim
String trim(char* source)
{
  char* i = source;
  char* j = source;
  while(*j != 0)
  {
    *i = *j++;
    if(*i != ' ')
      i++;
  }
  *i = 0;
  return source;
}

// WIFI FUNCTIONS START HERE  ---------------------------------------------------------------------------------------------------------------------

boolean sendToServer(String data){
  
  // Establish TCP connection
  String cmd = "AT+CIPSTART=\"TCP\",\""; cmd += DEST_HOST; cmd += "\",80";
  if (!echoCmd(cmd, "OK", CONTINUE)) return false;
  delay(200);
    
  // Set CIPMODE=1
  String cip = "AT+CIPMODE=1";
  if (!echoCmd(cip, "OK", CONTINUE)) return false;
  delay(200);
  
  // Get connection status
  if (!echoCmd("AT+CIPSTATUS", "OK", CONTINUE)) return false;
  // Build HTTP request.
  cmd = "POST /add.php HTTP/1.0\r\nHost: 8.8.8.8:80\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: ";
  cmd += data.length();
  cmd += "\r\n";
  cmd += "Connection: Close";
  cmd += "\r\n\r\n";
  cmd += data;
  Serial.println(cmd);
  
  String cipstart = "AT+CIPSEND";
  delay(5000);

  if (!echoCmd(cipstart, ">", CONTINUE))
  {
    echoCmd("AT+CIPCLOSE", "", CONTINUE);
    Serial.println("Connection timeout.");
    return false;
  }
  delay(500);
  
  // Send the raw HTTP request
  echoCmd(cmd, "200", CONTINUE);  // GET
  delay(500);
  // Send a STOP command to ensure terminal does not hang
  Serial1.print("+++");
  delay(200);
  echoCmd("AT+CIPCLOSE", "OK", CONTINUE);
  delay(200);
}

// Connect to the specified wireless network.
boolean connectWiFi()
{
  String cmd = "AT+CWJAP=\""; cmd += SSID; cmd += "\",\""; cmd += PASS; cmd += "\"";
  if (echoCmd(cmd, "OK", CONTINUE)) // Join Access Point
  {
    Serial.println("Connected to WiFi.");
    return true;
  }
  else
  {
    Serial.println("Connection to WiFi failed.");
    return false;
  }
}

boolean echoCmd(String cmd, String ack, boolean halt_on_fail)
{
  Serial1.println(cmd);
  #ifdef ECHO_COMMANDS
    Serial.print("--"); Serial.println(cmd);
  #endif
  
  // If no ack response specified, skip all available module output.
  if (ack == "")
    echoSkip();
  else
    // Otherwise wait for ack.
    if (!echoFind(ack))          // timed out waiting for ack string 
      if (halt_on_fail)
        errorHalt(cmd+" failed");// Critical failure halt.
      else
        return false;            // Let the caller handle it.
  return true;                   // ack blank or ack found
}

void echoSkip()
{
  echoFind("\n");        // Search for nl at end of command echo
  echoFind("\n");        // Search for 2nd nl at end of response.
  echoFind("\n");        // Search for 3rd nl at end of blank line.
}
  
boolean echoFind(String keyword)
{
  byte current_char   = 0;
  byte keyword_length = keyword.length();
  
  // Fail if the target string has not been sent by deadline.
  long deadline = millis() + TIMEOUT;
  while(millis() < deadline)
  {
    if (Serial1.available())
    {
      char ch = Serial1.read();
      Serial.write(ch);
      if (ch == keyword[current_char])
        if (++current_char == keyword_length)
        {
          Serial.println();
          return true;
        }
    }
  }
  return false;  // Timed out
}

void errorHalt(String msg)
{
  Serial.println(msg);
  Serial.println("HALT");
  while(true){};
}

// LOOP STARTS HERE

void loop()
{
  
  // Radio Reception  
  uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  
  if (nrf24.available())
    {
      if (nrf24.available())
      {
        // Should be a message for us now
        uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
        if (nrf24.recv(buf, &len))
        {
          // NRF24::printBuffer("request: ", buf, len);
          Serial.print("Sensor responded: ");
          
          for (i = 0; i < len; i++)
          {
            char buffer =+ buf[i];
            String dataStr = String(buffer);
            data += dataStr;
          }
          
          Serial.println(data);
          
          // Copy String to char* for whitespace removal          
          char *cstr = new char[data.length() + 1];
          strcpy(cstr, data.c_str());
          
          String trimmedData = trim(cstr);
          Serial.print("Trimmed: ");
          Serial.println(trimmedData);
          
          digitalWrite(ledReceive, LOW);

          sendToServer(trimmedData);
        
        } else
        {
          Serial.println("recv failed");
        }
      }
      
      data = "";
    }
}
