#include <WiFi.h>
#include <PubSubClient.h>
#define SOFT 400
#define MDEIUM 500
#define HARD 900
#define NUM_SENSOR 6
#define MQTT_SERVER "127.0.0.1"
#define MQTT_CLIENTID "yourCLIENTID"
 
const char* ssid = "AP NAME/SSID"; // GPIO 21 SDA
const char* password = "yourPASSWORD";   // GPIO 22 SCL
const char* mqttServer = "mqtt.thingspeak.com";  // MQTT server
const char* mqttUserName = "hi";
const char* mqttPwd = "12345";
const char* clientID = "any";
char* topic = "Patting_Data";

// define LED channels (total 16 channels)
#define LEDC_CHANNEL_0_R 0
#define LEDC_CHANNEL_0_G 1
#define LEDC_CHANNEL_0_B 2

#define fsrPin0 36 // GPIO 36
#define fsrPin1 39 // GPIO 39
#define fsrPin2 34
#define fsrPin3 35
#define fsrPin4 32
#define fsrPin5 33

#define LEDRpin 18 // GPIO 18: PWM
#define LEDGpin 19
#define LEDBpin 23
#define limit 300

int fsrData[NUM_SENSOR];
int force_data[NUM_SENSOR][100] = {0};
char json[25];
int cnt = 0, i = 0;
int patNo;
bool trigger;
String msgStr = "";
unsigned long prevMillis = 0;  // 暫存經過時間（毫秒）
 
WiFiClient espClient;
PubSubClient client(espClient);
void callback(char* topic, byte* payload, unsigned int length);

void callback(char* topic, byte* payload, unsigned int length){
    byte* p = (byte*)malloc(length);
    memcpy(p, payload, length);
    if(client.publish(topic, p, length))
        Serial.print("\nSend Success!");
    else Serial.print("\nSend Fail!");
    free(p);
    p = NULL;
}

void setup_led(){
    delay(10);
    ledcSetup(LEDC_CHANNEL_0_R, 5000, 8);
    ledcAttachPin(LEDRpin, LEDC_CHANNEL_0_R);
    ledcSetup(LEDC_CHANNEL_0_G, 5000, 8);
    ledcAttachPin(LEDGpin, LEDC_CHANNEL_0_G);
    ledcSetup(LEDC_CHANNEL_0_B, 5000, 8);
    ledcAttachPin(LEDBpin, LEDC_CHANNEL_0_B);
}

void setup_wifi() {
    delay(10);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
}

void reconnect() {
    while (!client.connected()) {
        if (client.connect(clientID, mqttUserName, mqttPwd)) {
            Serial.println("MQTT connected");
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);  // 等5秒之後再重試
        }
    }
}
 
void setup() {
    analogReadResolution(10); // resolution: 10-bit 0~1023
    Serial.begin(115200);
    setup_led();
    setup_wifi();
    client.setServer(mqttServer, 1883);

    for(int a=0; a<NUM_SENSOR; a++)
        for(int b=0; b<100; b++)
            force_data[a][b] = 0;
}
 
void loop() {
    if (!client.connected()) {
      reconnect();
    }

    initialize(patNo);
    readata();
    trigger = false;
    /* pick patting position */
    for(i=0; i<NUM_SENSOR; i++){
        if(force_data[i][0] > limit){
            patNo = i;
            trigger = true;
            showInfo();
            break;
        }
    }
    int num_input_data = 0;
    for(; trigger = true && force_data[patNo][num_input_data] > limit; ){
        Serial.print(force_data[patNo][num_input_data]);
        Serial.print(" ");
        num_input_data++;
        if(num_input_data >= 6) break;

        switch(patNo){
            case 0:
                force_data[patNo][num_input_data] = analogRead(fsrPin0); break;
            case 1:
                force_data[patNo][num_input_data] = analogRead(fsrPin1); break;
            case 2:
                force_data[patNo][num_input_data] = analogRead(fsrPin2); break;
            case 3:
                force_data[patNo][num_input_data] = analogRead(fsrPin3); break;
            case 4:
                force_data[patNo][num_input_data] = analogRead(fsrPin4); break;
            case 5:
                force_data[patNo][num_input_data] = analogRead(fsrPin5); break;
            default:
                break;
        }
        if(force_data[patNo][num_input_data] > HARD){ // too hard
            set_LED_color(255,0,0); // RED
        }
        else if(force_data[patNo][num_input_data] > MDEIUM){ // standard
            set_LED_color(0,255,0); // GREEN
        }
        else{ // too soft
            set_LED_color(0,0,255); // BLUE
        }
        delay(100);
    }
    client.loop();
    if(num_input_data){
        // Create MQTT message (JSON format)
        msgStr = msgStr + "{\"PatNo\":" + patNo + ",\"force\":";
        for(int j=0; j < num_input_data; j++){
            msgStr = msgStr + force_data[patNo][j] + " ";
        }
        msgStr = msgStr + "}";
        msgStr.toCharArray(json, 25);
        unsigned int len = strlen(json);
        callback(topic, (byte*)json, len);
        client.publish(topic, json);
        msgStr = ""; // clear message
    }
}

void initialize(int patNo){
    for(int b=0; b<100; b++)
        force_data[patNo][b] = 0;
}

void showInfo(){
    Serial.print("\n[No.");
    Serial.print(patNo);
    Serial.print("] ");
    Serial.print(++cnt);
    Serial.print(" th Patting: ");
}

void readata(){
    force_data[0][0] = analogRead(fsrPin0); // 0~1023
    force_data[1][0] = analogRead(fsrPin1);
    force_data[2][0] = analogRead(fsrPin2); 
    force_data[3][0] = analogRead(fsrPin3); 
    force_data[4][0] = analogRead(fsrPin4); 
    force_data[5][0] = analogRead(fsrPin5);
}

void set_LED_color(int R, int G, int B){
    ledcWrite(LEDRpin, R); // arg2 range is 0~255
    ledcWrite(LEDGpin, G);
    ledcWrite(LEDBpin, B);
}
