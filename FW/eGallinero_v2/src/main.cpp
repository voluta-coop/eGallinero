#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <NewPing.h>
#include <TimeLib.h>
#include <ESP32Servo.h>
#include <UniversalTelegramBot.h>


/************** CONFIGURACIÓN **************/

#define WIFI_AP_NAME "NOMBRE_RED_WIFI"
#define WIFI_PASSWORD "CONTRASEÑA_RED_WIFI"
#define DIST_MAX_AGUA 50
#define DIST_MIN_AGUA 6
#define DIST_MAX_PIENSO 50
#define DIST_MIN_PIENSO 6
#define BOT_TOKEN "1734935708:AAE8JzUMjqdGA6_K_cLmFAsl9Mfd5kmDR3I"
#define CHAT_ID "XXXXXXXXX"
#define NIVEL_ALERTA_PIENSO 10
#define NIVEL_ALERTA_AGUA 10

/*·············· HORARIOS ···················*/                   
#define HORA_APERTURA_ENERO         8
#define MINUTO_APERTURA_ENERO       30
#define HORA_CIERRE_ENERO           19
#define MINUTO_CIERRE_ENERO         00

#define HORA_APERTURA_FEBRERO       8
#define MINUTO_APERTURA_FEBRERO     30
#define HORA_CIERRE_FEBRERO         19
#define MINUTO_CIERRE_FEBRERO       30

#define HORA_APERTURA_MARZO         8
#define MINUTO_APERTURA_MARZO       30
#define HORA_CIERRE_MARZO           20
#define MINUTO_CIERRE_MARZO         00

#define HORA_APERTURA_ABRIL         8
#define MINUTO_APERTURA_ABRIL       30
#define HORA_CIERRE_ABRIL           21
#define MINUTO_CIERRE_ABRIL         30

#define HORA_APERTURA_MAYO          8
#define MINUTO_APERTURA_MAYO        30
#define HORA_CIERRE_MAYO            22
#define MINUTO_CIERRE_MAYO          00

#define HORA_APERTURA_JUNIO         8
#define MINUTO_APERTURA_JUNIO       30
#define HORA_CIERRE_JUNIO           22
#define MINUTO_CIERRE_JUNIO         45

#define HORA_APERTURA_JULIO         8
#define MINUTO_APERTURA_JULIO       30
#define HORA_CIERRE_JULIO           22
#define MINUTO_CIERRE_JULIO         15

#define HORA_APERTURA_AGOSTO        8
#define MINUTO_APERTURA_AGOSTO      30
#define HORA_CIERRE_AGOSTO          22
#define MINUTO_CIERRE_AGOSTO        00

#define HORA_APERTURA_SEPTIEMPBRE   8
#define MINUTO_APERTURA_SEPTIEMPBRE 30
#define HORA_CIERRE_SEPTIEMPBRE     21
#define MINUTO_CIERRE_SEPTIEMPBRE   45

#define HORA_APERTURA_OCTUBRE       8
#define MINUTO_APERTURA_OCTUBRE     30
#define HORA_CIERRE_OCTUBRE         20
#define MINUTO_CIERRE_OCTUBRE       30

#define HORA_APERTURA_NOVIEMBRE     8
#define MINUTO_APERTURA_NOVIEMBRE   30
#define HORA_CIERRE_NOVIEMBRE       18
#define MINUTO_CIERRE_NOVIEMBRE     40

#define HORA_APERTURA_DICIEMBRE     8
#define MINUTO_APERTURA_DICIEMBRE   30
#define HORA_CIERRE_DICIEMBRE       18
#define MINUTO_CIERRE_DICIEMBRE     30

/****************** FIN DE LA CONFIGURACIÓN ******************/


// Pines
#define PIN_LED_1 2
#define PIN_LED_2 15
#define PIN_PULSADOR_1 12
#define PIN_PULSADOR_2 13
#define PIN_AGUA 32
#define PIN_PIENSO 33
#define PIN_SERVO 14

// Lectura sensores
#define N_LEC 20 // Número de lecturas para hacer la media de los niveles

// Mensajes Telegram
#define MENSAJE_DIARIO ""
#define MENSAJE_PIENSO "Queda poco pienso."
#define MENSAJE_AGUA "Queda poca agua."

//Estados de configuración
#define CONFIG_STBY 0
#define CONFIG_DOOR_DOWN 1
#define CONFIG_DOOR_UP 2
#define CONFIG_DOOR_TEST_DOWN 3
#define CONFIG_DOOR_TEST_UP 4
#define CONFIG_DOOR_OK 5

// Estados de funcionamiento
#define OPERATION_WIFI 6
#define OPERATION_HORA 7
#define OPERATION_TELEGRAM 8
#define OPERATION_PUERTA 9
#define OPERATION_SENSORES 10

//Constantes Servo
#define PARO 90
#define SUBIR 0
#define BAJAR 180

// Constantes LEDs
#define OFF 1000
#define ON 100
#define F_1HZ 1
#define F_2HZ 2
#define F_5HZ 5
#define F_10HZ 10

// Conversiones
#define MIN_TO_MS 60000

// Telegram
const unsigned long BOT_MTBS = 1000; // mean time between scan messages

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

unsigned long bot_lasttime; // last time messages' scan has been done

int state = CONFIG_STBY;
int contador_pienso = 0;
int contador_agua = 0;

bool puerta_telegram = false;
bool aviso_error_config_puerta = false;


// Variables puerta
int servo = PARO;
bool abrir = false;
bool cerrar = false;
bool abierto = false;
bool cerrado = false;

// Variables de los LEDs
bool state_led_1 = LOW;
bool state_led_2 = LOW;
bool state_led_1_last = LOW;
bool state_led_2_last = LOW;

//Variables pulsadores
bool state_p_1 = false;
bool state_p_2 = false;
bool state_p_1_last = false;
bool state_p_2_last = false;

// Variables de tiempo generales
unsigned long  previousMillis_1 = 0;
unsigned long  previousMillis_2 = 0;
unsigned long  t_p_1 = 0;
unsigned long  t_p_2 = 0;
unsigned long  t_servo = 0;
unsigned long  t_servo_crono = 0;

// Variables sensores
int nivel_agua = 0;
int nivel_pienso = 0;


NewPing agua(PIN_AGUA, PIN_AGUA, DIST_MAX_AGUA);
NewPing pienso(PIN_PIENSO, PIN_PIENSO, DIST_MAX_PIENSO);

//Servo
Servo puerta;

// Tiempo
static const char ntpServerName[] = "1.es.pool.ntp.org";
const int timeZone = 1;     // Central European Time

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

void handleNewMessages(int numNewMessages)
{
  for (int i = 0; i < numNewMessages; i++)
  {
    String text = bot.messages[i].text;
    
    if(text == "Abrir" || text == "abrir") {

        if(abierto == true) bot.sendMessage(CHAT_ID, "La puerta ya está abierta.", "");
        else{
            cerrar = false;
            abrir = true;
        }
    }
    else if (text = "Cerrar" || text == "cerrar") {

        if(cerrado == true) bot.sendMessage(CHAT_ID, "La puerta ya está cerrada.", "");
        else{
            cerrar = true;
            abrir = false;
        }
    }

    if(abrir || cerrar) puerta_telegram = true;
  }
}


void outputs(){
    digitalWrite(PIN_LED_1, state_led_1);
    digitalWrite(PIN_LED_2, state_led_2);
    puerta.write(servo);
}

void leds(int freq_1, int freq_2){ // Esta función enciende el led que se indica y a la frecuencia que se le indica en Hz
    
    int per_1 = 500/freq_1;
    int per_2 = 500/freq_2;

    
    if((millis() - previousMillis_1) >= per_1 && freq_1 != OFF && freq_1 != ON){
        state_led_1 = !state_led_1;
        previousMillis_1 = millis();
    }
    else if(freq_1 == OFF) state_led_1 = LOW;
    else if(freq_1 == ON) state_led_1 = HIGH;

    if((millis() - previousMillis_2) >= per_2 && freq_2 != OFF && freq_2 != ON){
        state_led_2 = !state_led_2;
        previousMillis_2 = millis();
    }
    else if(freq_2 == OFF) state_led_2 = LOW;
    else if(freq_2 == ON) state_led_2 = HIGH;

    outputs();
}

void niveles(){
    
    bool niveles_ok = false;

    while(niveles_ok == false){

        int lec_agua = 0;
        int lec_pienso = 0;
        int media_agua = 0;
        int media_pienso = 0;
        

        for(int i = 0; i < N_LEC; i++){
            lec_agua += agua.ping_cm();
            delay(80);
        }

        for(int i = 0; i < N_LEC; i++){   
            lec_pienso += pienso.ping_cm();
            delay(80);
        }

        media_agua = lec_agua/N_LEC;
        media_pienso = lec_pienso/N_LEC;

        nivel_agua = map(media_agua,DIST_MIN_AGUA,DIST_MAX_AGUA,100,0);
        nivel_pienso = map(media_pienso,DIST_MIN_PIENSO,DIST_MAX_PIENSO,100,0);

        if((media_pienso =! 0) && (media_agua != 0)) niveles_ok = true;
        else niveles_ok = false;
    }
}

void inputs(){
    
    state_p_1_last = state_p_1;
    state_p_2_last = state_p_2;
    state_p_1 = digitalRead(PIN_PULSADOR_1);
    state_p_2 = digitalRead(PIN_PULSADOR_2);
}

void sensores(){
    niveles();
}

void telegram(){
    
    if (millis() - bot_lasttime > BOT_MTBS){
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages){
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    bot_lasttime = millis();
  }

    if(abrir == true && puerta_telegram == false) bot.sendMessage(CHAT_ID, MENSAJE_DIARIO, "");
    else if(abrir == true && puerta_telegram == true) bot.sendMessage(CHAT_ID, "Abriendo...", "");
    else if(cerrar == true && puerta_telegram == true) bot.sendMessage(CHAT_ID, "Cerrando...", "");

    if(nivel_pienso <= NIVEL_ALERTA_PIENSO) contador_pienso++;
    if(nivel_agua <= NIVEL_ALERTA_AGUA) contador_agua++;

    if(t_servo == 0 && aviso_error_config_puerta == false) {
        aviso_error_config_puerta = true;
        bot.sendMessage(CHAT_ID, "La puerta no está configurada, consulta el manual de configuración: https://github.com/voluta-coop/eGallinero", "");
    }
    if(contador_pienso >= 4) bot.sendMessage(CHAT_ID, MENSAJE_PIENSO, "");
    if(contador_agua >= 4) bot.sendMessage(CHAT_ID, MENSAJE_AGUA, "");

    if(contador_agua >= 4) contador_agua = 0;
    if(contador_pienso >= 4) contador_pienso = 0;
}

void setup() {
    
    Serial.begin(9600);

    pinMode(PIN_LED_1, OUTPUT);
    pinMode(PIN_LED_2, OUTPUT);
    pinMode(PIN_PULSADOR_1, INPUT);
    pinMode(PIN_PULSADOR_2, INPUT);

    //Config Servo

    // Allow allocation of all timers
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	puerta.setPeriodHertz(50);    // standard 50 hz servo
	puerta.attach(PIN_SERVO, 1000, 2000);
}

void loop() {
    switch(state){
        case CONFIG_STBY:
            while(state == CONFIG_STBY){
                Serial.println("STBY");
                inputs();

                leds(F_1HZ, OFF);

                t_servo = 0;
                t_servo_crono = 0;

                if (state_p_1 && !state_p_1_last){
                    state = CONFIG_DOOR_DOWN;
                    t_p_2 = millis();
                }
            }

        break;

        case CONFIG_DOOR_DOWN:

            while(state == CONFIG_DOOR_DOWN){
                inputs();
                Serial.println("DOWN");

                if((state_p_2 && state_p_2_last)){
                    leds(ON, F_2HZ);
                     servo = BAJAR;
                }
                else{
                    leds(OFF, F_2HZ);
                     servo = PARO;
                }

                if(state_p_1 && !state_p_1_last) state = CONFIG_DOOR_UP;

                outputs();
            }
        break;

        case CONFIG_DOOR_UP:
            while(state == CONFIG_DOOR_UP){
                inputs();
                Serial.println("UP");

                if((state_p_2 && state_p_2_last)){
                    leds(F_2HZ,ON);
                     servo = SUBIR;
                }
                else{
                    leds(F_2HZ, OFF);
                     servo = PARO;
                }

                if(state_p_2 && !state_p_2_last) t_p_2 = millis();
                if((!state_p_2 && state_p_2_last)) t_servo += millis() - t_p_2;

                if(state_p_1 && !state_p_1_last) state = CONFIG_DOOR_TEST_DOWN;
                
                outputs();
            }
        break;

        case CONFIG_DOOR_TEST_DOWN:
            while(state == CONFIG_DOOR_TEST_DOWN){
                inputs();

                leds(OFF, F_1HZ);

                if(t_servo_crono == 0) t_servo_crono = millis();

                if(millis() - t_servo_crono < t_servo){
                    leds(OFF, F_1HZ);
                    servo = BAJAR;
                    }
                else {
                    servo = PARO;
                    leds(OFF, OFF);
                }

                if(state_p_1 && !state_p_1_last) state = CONFIG_DOOR_TEST_UP;
                if(state_p_2 && !state_p_2_last) state = CONFIG_STBY;
                outputs();
            }
        break;

        case CONFIG_DOOR_TEST_UP:

            t_servo_crono = 0;

            while(state == CONFIG_DOOR_TEST_UP){
                inputs();
                
                if(t_servo_crono == 0) t_servo_crono = millis();

                if(millis() - t_servo_crono < t_servo){
                    leds(F_1HZ, OFF);
                    servo = SUBIR;
                    }
                else {
                    servo = PARO;
                    leds(OFF, OFF);
                    abierto = true;
                }

                if(state_p_1 && !state_p_1_last) state = CONFIG_DOOR_OK;

                outputs();
            }
        break;

        case CONFIG_DOOR_OK:

            t_servo_crono = 0;

            while(state == CONFIG_DOOR_OK){
                inputs();
                
                leds(F_1HZ, F_1HZ);

                if(t_servo_crono == 0) t_servo_crono = millis();

                if(millis() - t_servo_crono < t_servo) servo = SUBIR;
                else servo = PARO;

                if(state_p_1 && !state_p_1_last) state = OPERATION_WIFI;

                outputs();
            }
        break;

        case OPERATION_WIFI:

            Serial.println("WiFi");
            inputs();
            WiFi.begin(WIFI_AP_NAME, WIFI_PASSWORD);

            secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
            
            while (WiFi.status() != WL_CONNECTED){
                delay(100);
                leds(F_2HZ, OFF);
            }

            leds(ON, OFF);

            if(WiFi.status() == WL_CONNECTED) state = OPERATION_HORA;
            outputs();
        break;

        case OPERATION_HORA:

            Serial.println("Hora");

            Udp.begin(localPort);
            setSyncProvider(getNtpTime);
            setSyncInterval(300);

            while(state == OPERATION_HORA && WiFi.status() == WL_CONNECTED){

                int horario_apertura[12][2] = 
                {{HORA_APERTURA_ENERO, MINUTO_APERTURA_ENERO},
                {HORA_APERTURA_FEBRERO, MINUTO_APERTURA_FEBRERO},
                {HORA_APERTURA_MARZO, MINUTO_APERTURA_MARZO},
                {HORA_APERTURA_ABRIL, MINUTO_APERTURA_ABRIL},
                {HORA_APERTURA_MAYO, MINUTO_APERTURA_MAYO},
                {HORA_APERTURA_JUNIO, MINUTO_APERTURA_JUNIO},
                {HORA_APERTURA_JULIO, MINUTO_APERTURA_JULIO},
                {HORA_APERTURA_AGOSTO, MINUTO_APERTURA_AGOSTO},
                {HORA_APERTURA_SEPTIEMPBRE, MINUTO_APERTURA_SEPTIEMPBRE},
                {HORA_APERTURA_OCTUBRE, MINUTO_APERTURA_OCTUBRE},
                {HORA_APERTURA_NOVIEMBRE, MINUTO_APERTURA_NOVIEMBRE},
                {HORA_APERTURA_DICIEMBRE, MINUTO_APERTURA_DICIEMBRE}};

                int horario_cierre[12][2] = {{HORA_CIERRE_ENERO, MINUTO_CIERRE_ENERO},
                {HORA_CIERRE_FEBRERO, MINUTO_CIERRE_FEBRERO},
                {HORA_CIERRE_MARZO, MINUTO_CIERRE_MARZO},
                {HORA_CIERRE_ABRIL, MINUTO_CIERRE_ABRIL},
                {HORA_CIERRE_MAYO, MINUTO_CIERRE_MAYO},
                {HORA_CIERRE_JUNIO, MINUTO_CIERRE_JUNIO},
                {HORA_CIERRE_JULIO, MINUTO_CIERRE_JULIO},
                {HORA_CIERRE_AGOSTO, MINUTO_CIERRE_AGOSTO},
                {HORA_CIERRE_SEPTIEMPBRE, MINUTO_CIERRE_SEPTIEMPBRE},
                {HORA_CIERRE_OCTUBRE, MINUTO_CIERRE_OCTUBRE},
                {HORA_CIERRE_NOVIEMBRE, MINUTO_CIERRE_NOVIEMBRE},
                {HORA_CIERRE_DICIEMBRE, MINUTO_CIERRE_DICIEMBRE}};

                

                int hora = hour();
                int minuto = minute();
                int dia = day();
                int mes = month();

                inputs();
                
                leds(OFF, OFF);

                if(hora * 60 + minuto >= horario_apertura[mes][0] * 60 + horario_apertura[mes][1] && hora < horario_cierre[mes][0]) {
                    abrir   = true;
                    cerrar  = false;
                }
                else if(hora * 60 + minuto >= horario_cierre[mes][0] * 60 + horario_cierre[mes][1] && hora * 60 + minuto <  24 * 60 - 1){
                     cerrar = true;
                     abrir  = false;
                }
                else{
                    abrir   = false;
                    cerrar  = false;
                }
               
                outputs();

                if(WiFi.status() != WL_CONNECTED) state = OPERATION_WIFI;
                else if((hora != 0 && minuto != 0 && dia != 0 && mes != 0) && (abrir == true || cerrar == true) ) state = OPERATION_PUERTA;
                else state = OPERATION_SENSORES;
            }
        break;

        case OPERATION_SENSORES:

        Serial.println("Sensores");

        sensores();

        state = OPERATION_TELEGRAM;

        break;

        case OPERATION_TELEGRAM:

        Serial.println("Telegram");
        previousMillis_2 = millis();

        while(millis() < previousMillis_2 + 5 * MIN_TO_MS){

            telegram();

            if(puerta_telegram == true){
                state = OPERATION_PUERTA;
                break;
            }
        }

        if(puerta_telegram == false) state = OPERATION_WIFI;

        break;

        case OPERATION_PUERTA:

            Serial.println("Puerta");
            
            bool var_aux_abrir = false;
            bool var_aux_cerrar = false;

            if (puerta_telegram == true) puerta_telegram = false;

            if(abierto == true && cerrar == true) abierto = false;
            if(cerrado == true && abrir == true) cerrado = false;

            while(state == OPERATION_PUERTA){
                
                if(abrir == true){
                    
                    if(var_aux_abrir == false) {
                        t_servo_crono = millis();
                        var_aux_abrir = true;
                    }


                    if(millis() - t_servo_crono < t_servo){
                        servo = SUBIR;
                    }
                    else if(millis() - t_servo_crono >= t_servo)  {
                        servo = PARO;
                        abrir = false;
                        cerrar = false;
                        abierto = true;
                        cerrado = false;
                    }
                }

                else if(cerrar == true){

                    if(var_aux_cerrar == false) {
                        t_servo_crono = millis();
                        var_aux_cerrar = true;
                    }

                    if(millis() - t_servo_crono < t_servo) servo = BAJAR;

                    else if(millis() - t_servo_crono >= t_servo)  {
                        servo = PARO;
                        abrir = false;
                        cerrar = false;
                        abierto = false;
                        cerrado = true;
                    }
                }


                if (abierto == true || cerrado == true) state = OPERATION_SENSORES;

                outputs();
            }
        break;
    }
    
}