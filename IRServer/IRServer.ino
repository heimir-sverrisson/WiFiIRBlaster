/*
 * IRremoteESP8266: IRServer - demonstrates sending IR codes controlled from a webserver
 * Version 0.3 May, 2019
 * Version 0.2 June, 2017
 * Copyright 2015 Mark Szabo
 * Copyright 2019 David Conran
 *
 * An IR LED circuit *MUST* be connected to the ESP on a pin
 * as specified by kIrLed below.
 *
 * TL;DR: The IR LED needs to be driven by a transistor for a good result.
 *
 * Suggested circuit:
 *     https://github.com/crankyoldgit/IRremoteESP8266/wiki#ir-sending
 *
 * Common mistakes & tips:
 *   * Don't just connect the IR LED directly to the pin, it won't
 *     have enough current to drive the IR LED effectively.
 *   * Make sure you have the IR LED polarity correct.
 *     See: https://learn.sparkfun.com/tutorials/polarity/diode-and-led-polarity
 *   * Typical digital camera/phones can be used to see if the IR LED is flashed.
 *     Replace the IR LED with a normal LED if you don't have a digital camera
 *     when debugging.
 *   * Avoid using the following pins unless you really know what you are doing:
 *     * Pin 0/D3: Can interfere with the boot/program mode & support circuits.
 *     * Pin 1/TX/TXD0: Any serial transmissions from the ESP8266 will interfere.
 *     * Pin 3/RX/RXD0: Any serial transmissions to the ESP8266 will interfere.
 *   * ESP-01 modules are tricky. We suggest you use a module with more GPIOs
 *     for your first time. e.g. ESP-12 etc.
 */
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <WiFiClient.h>

const char* kSsid = "YourActualSID";
const char* kPassword = "YourActualPassword";
MDNSResponder mdns;

ESP8266WebServer server(80);
#undef HOSTNAME
#define HOSTNAME "ir-blaster"

#undef STANDALONE
// #define STANDALONE

const uint16_t kIrLed = 4;  // ESP GPIO pin to use. Recommended: 4 (D2).

IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

#ifdef STANDALONE
void handleRoot() {
  server.send(200, "text/html",
              "<html>" \
                "<head><title>" HOSTNAME " Demo </title>" \
                "<meta http-equiv=\"Content-Type\" " \
                    "content=\"text/html;charset=utf-8\">" \
                "<meta name=\"viewport\" content=\"width=device-width," \
                    "initial-scale=1.0,minimum-scale=1.0," \
                    "maximum-scale=5.0\">" \
                "</head>" \
                "<body>" \
                  "<h1>Codes for hifi</h1>" \
                  "<p><a href=\"ir?code=3778941932\">NAD Off</a></p>" \
                  "<p><a href=\"ir?code=3778978907\">NAD On</a></p>" \
                  "<p><a href=\"ir?code=3778941422\">NAD Volume up</a></p>" \
                  "<p><a href=\"ir?code=3778949582\">NAD Volume down</a></p>" \
                  "<p><a href=\"ir?code=3778984007\">NAD Source up</a></p>" \
                  "<p><a href=\"ir?code=3778959527\">NAD Source down</a></p>" \
                  "</p>" \
                  "<p><a href=\"ir?code=46505977807053\">Denon Previous track</a></p>" \
                  "<p><a href=\"ir?code=46505977364614\">Denon Play/Pause</a></p>" \
                  "<p><a href=\"ir?code=46505977282757\">Denon Next track</a></p>" \
                  "<p><a href=\"ir?code=46505978069193\">Denon Backward</a></p>" \
                  "<p><a href=\"ir?code=46505977675971\">Denon Stop</a></p>" \
                  "<p><a href=\"ir?code=46505977544897\">Denon Fast forward</a></p>" \                
                 "</body>" \
              "</html>");
}
#else
void handleRoot() {
  server.send(200, "text/html","OK");
}
#endif

void handleIr() {
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "code") {
      uint64_t code = strtoull(server.arg(i).c_str(), NULL, 10);
      if(code > 4294967295){
        irsend.sendNEC(code, 48);
      } else {
        irsend.sendNEC(code, 32);
      }
      char msg[30];
      sprintf(msg, "Sent: %llu", code);
      Serial.println(msg);
    }
  }
  handleRoot();
}

void handleNotFound() {
  String message = "Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  server.send(404, "text/plain", message);
}

void setup(void) {
  irsend.begin();

  Serial.begin(115200);
  WiFi.begin(kSsid, kPassword);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(kSsid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP().toString());

  if (mdns.begin(HOSTNAME, WiFi.localIP())) {
    Serial.println("MDNS responder started");
    mdns.addService("http", "tcp", 80);
  }

  server.on("/", handleRoot);
  server.on("/ir", handleIr);

  server.on("/inline", [](){
    server.send(200, "text/plain", "Server is up and running");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  mdns.update();
  server.handleClient();
}
