#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "secrets.h"

// OLED SCREEN
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// minimal Website in one string
const char* html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <style>
    * { box-sizing: border-box; }
    body { 
      font-family: -apple-system, sans-serif; 
      background: #121212; color: white; 
      margin: 0; display: flex; flex-direction: column; height: 100vh; 
    }
    #header { background: #1f1f1f; padding: 15px; text-align: center; border-bottom: 1px solid #333; font-weight: bold; }
    #chat { 
      flex: 1; overflow-y: auto; padding: 15px; 
      display: flex; flex-direction: column; gap: 8px; 
    }
    .msg-box { 
      background: #2c2c2c; padding: 8px 12px; border-radius: 12px; 
      max-width: 85%; align-self: flex-start; word-wrap: break-word;
    }
    .footer { 
      background: #1f1f1f; padding: 10px; display: flex; gap: 10px; 
      border-top: 1px solid #333; padding-bottom: calc(10px + env(safe-area-inset-bottom)); 
    }
    input { 
      flex: 1; background: #333; border: none; color: white; 
      padding: 12px; border-radius: 20px; outline: none; font-size: 16px; 
    }
    button { 
      background: #007bff; border: none; color: white; 
      padding: 0 20px; border-radius: 20px; font-weight: bold; cursor: pointer;
    }
  </style>
</head>
<body>
  <div id="header">Campfire</div>
  <div id="chat"></div>
  <div class="footer">
    <input id="msg" type="text" placeholder="Nachricht..." autocomplete="off">
    <button onclick="send()">Send</button>
  </div>

  <script>
    var user = prompt("Your Name:", "User_" + Math.floor(Math.random()*100));
    if(!user) user = "Guest";

    var ws = new WebSocket('ws://' + location.hostname + ':81/');
    var chat = document.getElementById('chat');
    var input = document.getElementById('msg');

    ws.onmessage = e => { 
      chat.innerHTML += `<div class="msg-box">${e.data}</div>`; 
      chat.scrollTop = chat.scrollHeight; 
    };

    function send() { 
      if(input.value.trim() != "") {
        ws.send(user + ": " + input.value); 
        input.value = ''; 
      }
    }

    input.addEventListener("keypress", e => { if(e.key === "Enter") send(); });
  </script>
</body>
</html>
)rawliteral";

void updateOLED(String msg) {
  static int lineCount = 0;
  
  // If display is full (ca. 5-6 lines with Text size 1), delete
  if(lineCount >= 6) {
    display.clearDisplay();
    display.setCursor(0, 0);
    lineCount = 0;
  }
  
  display.println(msg);
  display.display();
  lineCount++;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if(type == WStype_TEXT) {
    String message = String((char*)payload);
    
    // 1. Send to all browsers
    webSocket.broadcastTXT(message);
    
    // 2. Show on OLED screen
    updateOLED(message);
    
    Serial.println("OLED: " + message);
  }
}

void setup() {
  Serial.begin(115200);

  // OLED init
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println("OLED not found");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Chat ready...");
    display.display();
  }

  delay(500);
  Serial.println("\nESP32 Chat-Server starting...");

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("IP-Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", []() { 
    server.send(200, "text/html", html); 
  });
  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();
  server.handleClient();
}