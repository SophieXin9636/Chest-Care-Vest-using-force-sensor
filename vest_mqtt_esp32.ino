#include <WiFi.h>
#include <PubSubClient.h>
#define SOFT 300
#define MDEIUM 500
#define HARD 1000
#define NUM_SENSOR 6
 
const char* ssid = ""; // GPIO 21 SDA
const char* password = "";   // GPIO 22 SCL
const char* mqttServer = ""; //"";
const char* mqttUserName = "";
const char* mqttPwd = "";
const char* clientID = "";
char* mqtt_topic = "";

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

#define LEDRpin 23 // GPIO 18: PWM
#define LEDGpin 22
#define LEDBpin 21
#define limit 200

int fsrData[NUM_SENSOR];
int force_data[NUM_SENSOR][100] = {0};
char json[50];
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
    if(client.publish(mqtt_topic, p, length))
        Serial.print("\nSend Success!");
    else Serial.print("\nSend Fail!");
    free(p);
    p = NULL;
}

void setup_led(){
    delay(10);
    pinMode(LEDRpin, OUTPUT);
    pinMode(LEDGpin, OUTPUT);
    pinMode(LEDBpin, OUTPUT);
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
            set_LED_color(HIGH,LOW,LOW); // RED
        }
        else if(force_data[patNo][num_input_data] > MDEIUM){ // standard
            set_LED_color(LOW,HIGH,LOW); // GREEN
        }
        else{ // too soft
            set_LED_color(LOW,LOW,HIGH); // BLUE
        }
        delay(100);
    }
    client.loop();
    if(num_input_data){
        // Create MQTT message (JSON format)
        msgStr = msgStr + "{\"No\":" + patNo + ", \"force\":";
        for(int j=0; j < num_input_data; j++){
            msgStr = msgStr + force_data[patNo][j] + " ";
        }
        msgStr = msgStr + "}";
        msgStr.toCharArray(json, 50);
        unsigned int len = strlen(json);
        callback(mqtt_topic, (byte*)json, len);
        client.publish(mqtt_topic, json);
        msgStr = ""; // clear message
    }
    delay(10);
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
    digitalWrite(LEDRpin, R);
    digitalWrite(LEDGpin, G);
    digitalWrite(LEDBpin, B);
}
