#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <NewPing.h>
#include <TimeLib.h>
#include <ESP32Servo.h>
#include <UniversalTelegramBot.h>

/************** CONFIGURACIÓN **************/

#define WIFI_AP_NAME "XXXXXXX"
#define WIFI_PASSWORD "XXXXX"
#define DIST_MAX_AGUA 31
#define DIST_MIN_AGUA 10
#define DIST_MAX_PIENSO 31
#define DIST_MIN_PIENSO 10
#define BOT_TOKEN "XXXXXXXXXX:XXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
#define CHAT_ID "941876134"
#define CHAT_ID_CLIENTE "XXXXXXXXXXX"
#define NIVEL_ALERTA_PIENSO 20
#define NIVEL_ALERTA_AGUA 20

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
#define HORA_CIERRE_JUNIO           21
#define MINUTO_CIERRE_JUNIO         30

#define HORA_APERTURA_JULIO         8
#define MINUTO_APERTURA_JULIO       0
#define HORA_CIERRE_JULIO           21
#define MINUTO_CIERRE_JULIO         30

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
#define PIN_AGUA 33
#define PIN_PIENSO 32
#define PIN_SERVO 14

// Lectura sensores
#define N_LEC 20 // Número de lecturas para hacer la media de los niveles

// Mensajes Telegram

#define MENSAJE_PIENSO "Queda poco pienso."
#define MENSAJE_AGUA "Queda poca agua."

//Estados de configuración
#define CONFIG_SET_UP 0
#define CONFIG_DOOR_DOWN 1
#define CONFIG_DOOR_UP 2
#define CONFIG_DOOR_TEST_DOWN 3
#define CONFIG_DOOR_TEST_UP 4
#define CONFIG_DOOR_OK 5

// Estados de funcionamiento
#define OPERATION_WIFI 6
#define OPERATION_HORA 7
#define OPERATION_TELEGRAM 8
#define OPERATION_PUERTA 10
#define OPERATION_SENSORES 11

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

// Variables sensores
int nivel_agua = 0;
int nivel_pienso = 0;

// Telegram
const unsigned long BOT_MTBS = 1000; // mean time between scan messages
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

unsigned long bot_lasttime; // last time messages' scan has been done

int state = 0;
int contador_pienso = 0;
int contador_agua = 0;
bool aux = 0;
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
unsigned long  t_servo_down = 0;
unsigned long  t_servo_up = 0;
unsigned long  t_servo_crono = 0;


NewPing agua(PIN_AGUA, PIN_AGUA, DIST_MAX_AGUA + 10);
NewPing pienso(PIN_PIENSO, PIN_PIENSO, DIST_MAX_PIENSO + 10);

//Servo
Servo puerta;

// Tiempo
static const char ntpServerName[] = "3.es.pool.ntp.org";
const int timeZone = 1;     

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

        if(abierto == true){
            bot.sendMessage(CHAT_ID, "La puerta ya está abierta.", "");
            bot.sendMessage(CHAT_ID_CLIENTE, "La puerta ya está abierta.", "");
        } 
        else{
            cerrar = false;
            abrir = true;
            Serial.println("Abro manualmente");
        }
    }
    else if (text = "Cerrar" || text == "cerrar") {

        if(cerrado == true){
           bot.sendMessage(CHAT_ID, "La puerta ya está cerrada.", ""); 
           bot.sendMessage(CHAT_ID_CLIENTE, "La puerta ya está cerrada.", "");
        } 
        else{
            cerrar = true;
            abrir = false;
            Serial.println("Cierro manualmente");
        }
    }

    if(abrir || cerrar) puerta_telegram = true;
  }
}


void outputs(){

    digitalWrite(PIN_LED_1, state_led_1);
    digitalWrite(PIN_LED_2, state_led_2);
    if(state != OPERATION_HORA && state != OPERATION_WIFI) puerta.write(servo);

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

        delay(2500);
        for(int i = 0; i < N_LEC; i++){   
            lec_pienso += pienso.ping_cm();
            delay(80);
        }

        media_agua = lec_agua/N_LEC;
        media_pienso = lec_pienso/N_LEC;

        Serial.print("media agua: "); Serial.println(media_agua);
        Serial.print("media pienso: "); Serial.println(media_pienso);

        if(media_agua != 0 && media_pienso != 0){

            nivel_agua = map(media_agua,DIST_MIN_AGUA,DIST_MAX_AGUA,100,0);
            nivel_pienso = map(media_pienso,DIST_MIN_PIENSO,DIST_MAX_PIENSO,100,0);

            if(nivel_pienso <= 0) nivel_pienso = 0;
            if(nivel_agua <= 0) nivel_agua = 0;

            if(nivel_agua >= 100) nivel_agua = 100;
            if(nivel_pienso >= 100) nivel_pienso = 100;

            Serial.println("niveles");
            Serial.println(nivel_agua);
            Serial.println(nivel_pienso);

            niveles_ok = true;
        }

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

    previousMillis_2 = millis();

        while (millis() - previousMillis_2 < 2 * MIN_TO_MS){
            
            if (millis() - bot_lasttime > BOT_MTBS){
                int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
            
                while (numNewMessages){
                    Serial.println("got response");
                    handleNewMessages(numNewMessages);
                    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
                }

                bot_lasttime = millis();
            }

            if(puerta_telegram == true && (abrir == true || cerrar == true)){
            state = OPERATION_PUERTA;
            break;
            }
        }
    String niveles_pienso[] = {String(nivel_pienso)};


    if((abrir == true && puerta_telegram == false) || !aux) {
        aux = 1;
        bot.sendMessage(CHAT_ID, {"Buenos días Teresa, este es el estado de tu gallinero:\n - Agua: " + String(nivel_agua) + "%\n- Pienso: " + String(nivel_pienso) + "%"}, "");
        bot.sendMessage(CHAT_ID_CLIENTE,{"Buenos días Teresa, este es el estado de tu gallinero:\n - Agua: " + String(nivel_agua) + "%\n- Pienso: " + String(nivel_pienso) + "%"} , "");
    }
    else if(abrir == true && puerta_telegram == true) bot.sendMessage(CHAT_ID, "Abriendo...", "");
    else if(cerrar == true && puerta_telegram == true) bot.sendMessage(CHAT_ID, "Cerrando...", "");

    if(nivel_pienso <= NIVEL_ALERTA_PIENSO) contador_pienso++; 
    if(nivel_agua <= NIVEL_ALERTA_AGUA) contador_agua++;

    if((t_servo_up == 0 || t_servo_down == 0) && aviso_error_config_puerta == false) {
        aviso_error_config_puerta = true;
        bot.sendMessage(CHAT_ID, "La puerta no está configurada, consulta el manual de configuración: https://github.com/voluta-coop/eGallinero", "");
        bot.sendMessage(CHAT_ID_CLIENTE, "La puerta no está configurada, consulta el manual de configuración: https://github.com/voluta-coop/eGallinero", "");
    }
    
    if(contador_pienso >= 30) {
        bot.sendMessage(CHAT_ID, MENSAJE_PIENSO, "");
        bot.sendMessage(CHAT_ID_CLIENTE, MENSAJE_PIENSO, "");
    }
    
    if(contador_agua >= 30) {
        bot.sendMessage(CHAT_ID_CLIENTE, MENSAJE_AGUA, "");
        bot.sendMessage(CHAT_ID, MENSAJE_AGUA, "");
    }
    
    if(contador_agua >= 30) contador_agua = 0;
    if(contador_pienso >= 30) contador_pienso = 0;
}

void setup(){
    
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
    
	puerta.attach(PIN_SERVO, 500, 2400);

    outputs();
}

void loop() {
    switch(state){
        
        case CONFIG_SET_UP:

            Serial.println("SET UP");

            while(state == CONFIG_SET_UP){

                inputs();

                if((state_p_2 && state_p_2_last)){
                    leds(F_5HZ, OFF);
                     servo = SUBIR;
                }

                else{
                    leds(ON, OFF);
                     servo = PARO;
                }

                if(state_p_1 && !state_p_1_last) state = CONFIG_DOOR_DOWN;

                outputs();
            }
        break;

        case CONFIG_DOOR_DOWN:

            while(state == CONFIG_DOOR_DOWN){
                inputs();
                Serial.println("DOWN");

                if((state_p_2 && state_p_2_last)){
                    leds(ON,F_5HZ);
                     servo = BAJAR;
                }
                else{
                    leds(OFF, F_5HZ);
                     servo = PARO;
                }

                if(state_p_2 && !state_p_2_last) t_p_2 = millis();
                if((!state_p_2 && state_p_2_last)) t_servo_down += millis() - t_p_2;

                if(state_p_1 && !state_p_1_last) state = CONFIG_DOOR_UP;
                
                outputs();
            }
        break;

        case CONFIG_DOOR_UP:
            while(state == CONFIG_DOOR_UP){
                inputs();
                Serial.println("UP");

                if((state_p_2 && state_p_2_last)){
                    leds(F_5HZ,ON);
                     servo = SUBIR;
                }
                else{
                    leds(F_5HZ, OFF);
                     servo = PARO;
                }

                if(state_p_2 && !state_p_2_last) t_p_2 = millis();
                if((!state_p_2 && state_p_2_last)) t_servo_up += millis() - t_p_2;

                if(state_p_1 && !state_p_1_last) state = CONFIG_DOOR_TEST_DOWN;
                
                outputs();
            }
        break;

        case CONFIG_DOOR_TEST_DOWN:

            Serial.println("Test Down");


            t_servo_crono = millis();

            while(state == CONFIG_DOOR_TEST_DOWN){
                
                inputs();

                if(millis() - t_servo_crono < t_servo_down){
                    leds(OFF, F_2HZ);
                    servo = BAJAR;
                    }
                else {
                    servo = PARO;
                    leds(OFF, ON);
                    if(state_p_1 && !state_p_1_last) state = CONFIG_DOOR_TEST_UP;
                    if(state_p_2 && !state_p_2_last) state = CONFIG_SET_UP;  
                }

                outputs();
            }
        break;

        case CONFIG_DOOR_TEST_UP:

            Serial.println("Test Up");

            t_servo_crono = millis();

            while(state == CONFIG_DOOR_TEST_UP){

                inputs();

                if(millis() - t_servo_crono < t_servo_up){
                    leds(F_2HZ, OFF);
                    servo = SUBIR;
                    }
                else {
                    servo = PARO;
                    leds(ON, OFF);
                    abierto = true;
                    if(state_p_1 && !state_p_1_last) state = CONFIG_DOOR_OK;
                }

                outputs();
            }
        break;

        case CONFIG_DOOR_OK:

            Serial.println("Ok?");
            delay(500);

            while(state == CONFIG_DOOR_OK){

                inputs();
                
                leds(F_1HZ, F_1HZ);         

                if(state_p_1 && !state_p_1_last) state = OPERATION_WIFI;
                else if(state_p_2 && !state_p_2_last) state = CONFIG_DOOR_TEST_DOWN;

                outputs();
            }
        break;

        case OPERATION_WIFI:

            Serial.println("WiFi");

            leds(ON, OFF);
            outputs();

            WiFi.begin(WIFI_AP_NAME, WIFI_PASSWORD);

            secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
            
            while (WiFi.status() != WL_CONNECTED){
                delay(100);
            }

            if(WiFi.status() == WL_CONNECTED) state = OPERATION_HORA;
            
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

                int hora = hour() + 1;
                if (hora >= 24 || day() == 0) hora = 0;

                int minuto = minute();
                int dia = day();
                int mes = month();
                int minuto_diario = hora * 60 + minuto;
                int minuto_apertura = horario_apertura[mes-1][0] * 60 + horario_apertura[mes-1][1];
                int minuto_cierre = horario_cierre[mes-1][0] * 60 + horario_cierre[mes-1][1];
                inputs();
                
                Serial.println(dia);
                Serial.println(mes);
                Serial.println(minuto_diario);
                Serial.println(minuto_apertura);
                Serial.println(minuto_cierre);

                if(cerrado == true && ( minuto_diario >= minuto_apertura && minuto_diario < minuto_cierre)) {
                    abrir   = true;
                    cerrar  = false;
                    Serial.println("Es hora de abrir.");
                }

                else if(abierto == true && ( minuto_diario < minuto_apertura || minuto_diario >= minuto_cierre)){
                     cerrar = true;
                     abrir  = false;
                    Serial.println("Es hora de cerrar.");
                }
                else{
                    abrir   = false;
                    cerrar  = false;
                    if(abierto == true) Serial.println("Ya está abierto");
                    else if(cerrado == true) Serial.println("Ya está cerrado");
                }
                
                outputs();

                if(WiFi.status() != WL_CONNECTED) state = OPERATION_WIFI;

                else if((hour() != 0 && dia != 0 && mes != 0) && (abrir == true || cerrar == true) ) {
                    state = OPERATION_PUERTA;
                    Serial.println("Hora: ");
                    Serial.print(hora);Serial.print(":");Serial.println(minuto);
                }

                else {
                    Serial.println("Hora: ");
                    Serial.print(hora);Serial.print(":");Serial.println(minuto);
                    state = OPERATION_SENSORES;
                }

                if(hora * 60 + minuto <= 30) puerta_telegram = false;
            }



        if(abierto == true) abrir = false;
        if(cerrado == true) cerrar = false;

        break;

        case OPERATION_SENSORES:

        Serial.println("Sensores");

        sensores();

        state = OPERATION_TELEGRAM;

        break;

        case OPERATION_TELEGRAM:

        Serial.println("Telegram");

            telegram();

        if(state != OPERATION_PUERTA) state = OPERATION_HORA;

        break;

        case OPERATION_PUERTA:

            Serial.println("Puerta");
            Serial.println();
            Serial.print("abrir ");Serial.println(abrir);
            Serial.print("cerrar: ");Serial.println(cerrar);
            Serial.print("abierto: ");Serial.println(abierto);
            Serial.print("cerrado: ");Serial.println(cerrado);
            Serial.print("puerta_telegram: ");Serial.println(puerta_telegram);

            bool var_aux_abrir = false;
            bool var_aux_cerrar = false; 

            while(state == OPERATION_PUERTA){

                if(abrir == true && cerrado == true){

                    if(var_aux_abrir == false) {
                        t_servo_crono = millis();
                        var_aux_abrir = true;
                        Serial.println("T_servo_crono");
                    }


                    if(millis() - t_servo_crono < t_servo_up && var_aux_abrir == true){
                         servo = SUBIR;
                        Serial.println("Empiezo a abrir manualmente");
                    }
                    else if(millis() - t_servo_crono >= t_servo_up)  {
                        servo = PARO;
                        abrir = false;
                        cerrar = false;
                        abierto = true;
                        cerrado = false;
                        Serial.println("He acabado de abrir");
                    }
                }

                else if(cerrar == true && abierto == true){
                    
                    Serial.println("Está abierta y hay que cerrar");
                    if(var_aux_cerrar == false) {
                        t_servo_crono = millis();
                        var_aux_cerrar = true;
                        Serial.println(t_servo_crono);
                    }

                    if(millis() - t_servo_crono < t_servo_down && var_aux_cerrar == true) {
                        servo = BAJAR;
                        Serial.println("Empieza a bajar");
                    }
                    else if(millis() - t_servo_crono >= t_servo_down)  {
                        servo = PARO;
                        abrir = false;
                        cerrar = false;
                        abierto = false;
                        cerrado = true;
                        Serial.println("El servo para, está cerrada");
                    }
                }

                if(abrir == false && cerrar == false) state = OPERATION_SENSORES;
                
                outputs();
            }

            if(abierto == true && cerrar == true) abierto = false;
            if(cerrado == true && abrir == true) cerrado = false;

            servo = PARO;

            outputs();

        break;
    }
}