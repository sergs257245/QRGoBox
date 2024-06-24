int vibrationSignal = 6;
int buzzer = 8;
int sensorState = 1;

void setup() {
  pinMode(vibrationSignal, INPUT);
  pinMode(buzzer, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  Serial.print("Vibration status: ");
  sensorState = digitalRead(vibrationSignal);

  if(sensorState == 1){
    Serial.println("HAKKKKKDOOOOOG");
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(buzzer, LOW);
    delay(500);
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(buzzer, LOW);
    delay(500);
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(buzzer, LOW);
    delay(500);
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(buzzer, LOW);
    delay(500);
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(buzzer, LOW);
    delay(500);
  }
  else{
    Serial.println("francis");
    digitalWrite(buzzer, LOW);
  }
  delay(750);
}
