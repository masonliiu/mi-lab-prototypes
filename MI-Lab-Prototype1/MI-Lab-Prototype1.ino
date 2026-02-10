#define TRIG_PIN 2
#define ECHO_PIN 3

bool running = true;

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

}

void loop() {

  if (Serial.available() > 0) {
    char cmd = Serial.read();

    if (cmd == 'p' || cmd == 'P') {
      running = false;
      Serial.println("Paused");
    }

    if (cmd == 'r' || cmd == 'R') {
      running = true;
    }
  }
  if(!running) {
    delay(1500);
    return;
  }

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);


  //convert to cm
  float distance = duration * 0.034 / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  delay(1500);

}
