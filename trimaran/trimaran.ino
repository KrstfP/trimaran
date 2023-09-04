#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>

#define PIN_DIRECTION 15 // Output pin used
#define PIN_MAIN_SAIL 22 // Output pin used
#define PIN_JIB_SAIL 26 // Output pin used

// Direction moves in range [-90:90] w=> servo [0:270]
#define DIRECTION_TO_SERVO(d) d + 90
#define DIRECTION_FROM_SERVO(d) d - 90
// Sails move in range [0:90] <=> servo [0:270]
#define SAIL_TO_SERVO(d) d * 3
#define SAIL_FROM_SERVO(d) d / 3


Servo servo_direction;
Servo servo_main_sail;
Servo servo_jib_sail;
bool sailsSync = false;

/* Put your SSID & Password */
const char* ssid = "TRIMARAN";  // Enter SSID here
const char* password = "Dinard35";  //Enter Password here

/* Put IP Address details */
IPAddress local_ip(192,168,1,35);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

DynamicJsonDocument json_buffer_in(256);
DynamicJsonDocument json_buffer_out(1024);
WebServer server(80);


void setup() {

  // Let's start with Servos
  servo_direction.setPeriodHertz(50); // Fréquence PWM pour le SG90
  servo_direction.attach(PIN_DIRECTION, 500, 2500); // Largeur minimale et maximale de l'impulsion (en µs) pour aller de 0° à 270°
  servo_main_sail.setPeriodHertz(50); // Fréquence PWM pour le SG90
  servo_main_sail.attach(PIN_MAIN_SAIL, 500, 2500); // Largeur minimale et maximale de l'impulsion (en µs) pour aller de 0° à 270°
  servo_jib_sail.setPeriodHertz(50); // Fréquence PWM pour le SG90
  servo_jib_sail.attach(PIN_JIB_SAIL, 500, 2500); // Largeur minimale et maximale de l'impulsion (en µs) pour aller de 0° à 270°
  
  // Re-init positions
  servo_direction.write(90);
  servo_main_sail.write(0);
  servo_jib_sail.write(0);


  // Then the web server 
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  
  server.on("/", handle_OnConnect);
  server.on("/state", handle_OnState);
  server.on("/move", HTTP_POST, handle_OnMove);
  server.on("/sync", HTTP_POST, handle_OnSync);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient(); 
}

String getState() {
  const int direction = servo_direction.read();
  const int main_sail = servo_main_sail.read();
  const int jib_sail = servo_jib_sail.read();
  json_buffer_out["direction"] = DIRECTION_FROM_SERVO(direction);
  json_buffer_out["mainsail"] = SAIL_FROM_SERVO(main_sail);
  json_buffer_out["jibsail"] = SAIL_FROM_SERVO(jib_sail);
  json_buffer_out["sync"] = sailsSync;
  String response;
  serializeJson(json_buffer_out, response);
  return response;
}

void handle_OnState() {
  server.send(200, "application/json", getState());
}

void handle_OnSync() {
  const String body = server.arg("plain");
    deserializeJson(json_buffer_in, body);
    const bool value = json_buffer_in["value"];
    // adjust jib to main sail
    if (value && sailsSync != value) {
      servo_jib_sail.write(servo_main_sail.read());
      delay(200);
    }    
    sailsSync = value;
    server.send(200, "application/json", getState());
}

void handle_OnMove() {
    const String body = server.arg("plain");
    deserializeJson(json_buffer_in, body);
    const int target = json_buffer_in["target"];
    const int position = json_buffer_in["value"];
    switch (target) {
      case 0 : {
        servo_direction.write(DIRECTION_TO_SERVO(position));
        break;
      }
      case 1 : {
        servo_main_sail.write(SAIL_TO_SERVO(position));
        if (sailsSync) servo_jib_sail.write(SAIL_TO_SERVO(position));
        break;
      }
      case 2 : {
        servo_jib_sail.write(SAIL_TO_SERVO(position));        
        break;
      }
    }
    delay(200);
    server.send(200, "application/json", getState());
}

void handle_OnConnect() {
  server.send(200, "text/html", SendHTML()); 
}


String SendHTML(){
  String ptr = "<!DOCTYPE html><html lang=\"en\"><meta \ncharset=\"UTF-8\"><meta name=\"viewport\" \ncontent=\"width=device-width,initial-scale=1\"><meta\n http-equiv=\"X-UA-Compatible\" content=\"ie=edge\">\n<title>Trimaran remote control</title><style>\n</style><style>\n*{box-sizing:border-box;margin:0;padding:0}body{background-color:#fafafa;background-color:#2c3531;display:flex;flex-direction:column;align-items:center;color:#d9b08c}body,html{margin:0;height:100%;width:100%;overflow:hidden}main{display:flex;flex-direction:column;align-items:center;min-width:100%;min-height:100%}.params{padding-top:20px;display:flex;flex-direction:column;align-items:center;width:100%}.sliders{display:flex;flex-direction:row;align-items:center;width:100%;height:-moz-available;height:-webkit-fill-available}.block{display:block;flex-direction:column;align-items:center;width:100%}.block p{padding-top:5px;text-align:center;justify-content:center;color:#2c3531}.block p.label{font-weight:bolder;font-size:x-large;color:#116466;color:#d9b08c}.block p.label::after{content:'°'}.block .control-label{color:#d9b08c}.wrapper{position:relative;height:20rem;width:100%}.wrapper::after,.wrapper::before{display:block;position:absolute;z-index:99;color:#ffcb9a;width:100%;text-align:center;font-size:1.5rem;line-height:1;padding:.75rem 0;pointer-events:none}.wrapper::before{content:\"+\"}.wrapper::after{content:\"−\";bottom:0}input[type=range]{-webkit-appearance:none;background-color:#116466;position:absolute;top:50%;left:50%;margin:0;padding:0;width:20rem;height:3.5rem;transform:translate(-50%,-50%) rotate(-90deg);border-radius:1rem;overflow:hidden;cursor:row-resize}input[type=range][step]{background-color:transparent;background-image:repeating-linear-gradient(to right,rgba(255,203,154,.2),rgba(255,203,154,.2) calc(12.5% - 1px),#05051a 12.5%)}input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:0;box-shadow:-20rem 0 0 20rem #ffcb9a}input[type=range]::-moz-range-thumb{border:none;width:0;box-shadow:-20rem 0 0 20rem #ffcb9a}input[type=range]:disabled{background-color:rgba(17,100,102,.2)}input[type=range]:disabled::-webkit-slider-thumb{box-shadow:-20rem 0 0 20rem rgba(255,203,154,.2)}input[type=range]:disabled::-moz-range-thumb{box-shadow:-20rem 0 0 20rem rgba(255,203,154,.2)}@media (max-height:550px){.params,.title{display:none}}\n</style><style>\n.switch{position:relative;display:inline-block;width:60px;height:34px}.switch input{opacity:0;width:0;height:0}.slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background-color:#116466;-webkit-transition:.4s;transition:.4s}.slider:before{position:absolute;content:\"\";height:26px;width:26px;left:4px;bottom:4px;background-color:#d1e8e2;-webkit-transition:.4s;transition:.4s}input:checked+.slider{background-color:#ffcb9a}input:focus+.slider{box-shadow:0 0 1px #ffcb9a}input:checked+.slider:before{-webkit-transform:translateX(26px);-ms-transform:translateX(26px);transform:translateX(26px)}.slider.round{border-radius:34px}.slider.round:before{border-radius:50%}\n</style><h1 class=\"title\">Trimaran remote</h1>\n<main><div class=\"params\"><span class=\"label\">\nSynchroniser les voiles</span> <label \nclass=\"switch\"><input id=\"sailssync-switch\" \ntype=\"checkbox\" \nonchange=\"requestSailsSync(checked)\"> <span \nclass=\"slider round\"></span></label></div><div \nclass=\"sliders\"><div class=\"block\"><p \nclass=\"label\" id=\"direction-label\">-<div \nclass=\"wrapper\"><input id=\"direction\" \ntype=\"range\" min=\"-90\" max=\"90\" value=\"0\" \nonchange=\"requestMoveServo(id,value)\"></div><p \nclass=\"control-label\">direction</div><div \nclass=\"block\"><p class=\"label\" \nid=\"mainsail-label\">-<div class=\"wrapper\"><input \nid=\"mainsail\" type=\"range\" min=\"0\" max=\"90\" \nvalue=\"0\" onchange=\"requestMoveServo(id,value)\">\n</div><p class=\"control-label\">grand voile</div>\n<div class=\"block\"><p class=\"label\" \nid=\"jibsail-label\">-<div class=\"wrapper\"><input \nid=\"jibsail\" type=\"range\" min=\"0\" max=\"90\" \nvalue=\"0\" onchange=\"requestMoveServo(id,value)\">\n</div><p class=\"control-label\">foc</div></div>\n</main><script lang=\"javascript\">\nconst updateSailsSynchronzation=e=>{document.getElementById(\"jibsail\").disabled=e,document.getElementById(\"sailssync-switch\").checked=e},updateRangeValue=(e,t)=>{document.getElementById(e).value=parseInt(t),document.getElementById(e).dispatchEvent(new Event(\"input\"))},refreshControls=e=>{updateRangeValue(\"direction\",e.direction),updateRangeValue(\"mainsail\",e.mainsail),updateRangeValue(\"jibsail\",e.jibsail),updateSailsSynchronzation(e.sync)},updateState=()=>{fetch(\"/state\").then(e=>{e.json().then(e=>refreshControls(e))}).catch(e=>console.error(\"Failed to update state\",e))},requestMoveServo=(e,t)=>{fetch(\"/move\",{method:\"POST\",body:JSON.stringify({target:\"direction\"===e?0:\"mainsail\"===e?1:2,value:parseInt(t)})}).then(e=>{e.json().then(e=>refreshControls(e))}).catch(e=>console.error(\"Failed to move servo\",e))},requestSailsSync=e=>{fetch(\"/sync\",{method:\"POST\",body:JSON.stringify({value:e?1:0})}).then(e=>{e.json().then(e=>refreshControls(e))}).catch(e=>console.error(\"Failed to update sails synchronization\",e))};function init(){document.getElementById(\"direction\").addEventListener(\"input\",e=>document.getElementById(\"direction-label\").textContent=e.target.value),document.getElementById(\"mainsail\").addEventListener(\"input\",e=>document.getElementById(\"mainsail-label\").textContent=e.target.value),document.getElementById(\"jibsail\").addEventListener(\"input\",e=>document.getElementById(\"jibsail-label\").textContent=e.target.value),updateState(),setInterval(updateState,3e3)}init()\n</script>";
  return ptr;
}