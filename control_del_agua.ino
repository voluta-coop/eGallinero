#define HumedadPin 35  // used for ESP32
#define BoyaPin 26
bool boya = 1;
bool lastBoya = 0;
int lastValue = 0;
unsigned long  lastTime=0; 
unsigned long tiempoinicio;
unsigned long tiempofin;
unsigned long haTardado; 
  //tiempo desde que se inició el programa
void setup() { 
  pinMode(HumedadPin, INPUT);
  pinMode(BoyaPin, INPUT); 
  Serial.begin(9600);
  tiempoinicio = millis();
}

void loop() {
  boya = digitalRead(BoyaPin);
  int humidity = analogRead(HumedadPin);
  int fromLow = 0;
  int fromHigh = 2000;
  int toLow = 0;
  int toHigh = 100;
 
  int value = map(humidity, fromLow, fromHigh, toLow, toHigh);
  if(value>100){
    value = 100;
  }
  
  if(abs(lastValue - value) >=20){
    lastValue = value;
    Serial.print("La humedad ha cambiado al ");
    Serial.print(value);
    Serial.print("%\n");

}
  if(boya!=lastBoya){
    lastBoya = boya;
    if(boya == 1){
      Serial.printf("El depósito nº1 está lleno\n");
    }
    if(boya == 0){
      unsigned long haTardado; 
     
      tiempofin=millis();
      haTardado = (tiempofin-tiempoinicio)/1000;
      Serial.print("El depósito nº1 está vacío, tiempo lleno: ");
      Serial.print(haTardado);
      Serial.print(" segundos\n");
      tiempoinicio=millis();
    }

  }


  delay(1000);
  

} 
