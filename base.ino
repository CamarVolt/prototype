#define PINO_SENSOR 34  // A MESMA DO FIO VERDE

void setup() {
  Serial.begin(115200);
}

void loop() {
  int leitura = analogRead(PINO_SENSOR);
  float tensao = leitura * (3.3 / 4095.0);

  Serial.print("LEITURA BRUTA: ");
  Serial.print(leitura);
  Serial.print(" | TENSAO FILTRADA: ");
  Serial.println(tensao, 3);

  delay(300);
}
