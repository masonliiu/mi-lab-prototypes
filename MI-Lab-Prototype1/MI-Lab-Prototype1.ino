#define TRIG_PIN 2
#define ECHO_PIN 3
#define MOTOR_PIN 5

bool running = true;

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(MOTOR_PIN, OUTPUT);

}

void loop() {

  if (Serial.available() > 0) {
    char cmd = Serial.read();

    if (cmd == 'p' || cmd == 'P') {
      running = false;
      Serial.println("Paused");
      analogWrite(MOTOR_PIN, 0);
    }

    if (cmd == 'r' || cmd == 'R') {
      running = true;
    }
  }
  if(!running) {
    digitalWrite(MOTOR_PIN, LOW);
    delay(200);
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

  if (distance >= 1 && distance <= 55) {
    int power = map(distance, 1, 55, 255, 0);
    power = constrain(power, 0, 255);
    analogWrite(MOTOR_PIN, power);

  } else analogWrite(MOTOR_PIN, 0);

  delay(100);

}
