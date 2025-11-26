// ====== Configuração básica ======
#define PINO_SENSOR 34      // Mesma do fio verde no ESP32

// ADC do ESP32 é 12 bits: 0..4095
const float VREF        = 3.3;        // Tensão de referência do ADC (ver sua placa!)
const float ADC_MAX     = 4095.0;
const float ADC_TO_V    = VREF / ADC_MAX;

// Janela de cálculo de RMS (em ms)
const unsigned long JANELA_MS = 1000; // 1 segundo

// Filtro exponencial (0.0 a 1.0) -> quanto maior, mais rápido responde
const float EMA_ALPHA = 0.1f;

// ====== Parâmetros elétricos (AJUSTE AQUI) ======
// Tensão da rede (use 127.0 ou 220.0 conforme seu caso)
const float TENS_REDE = 127.0;

// Fator de conversão do seu circuito: quantos ampères por VOLT RMS no sensor
// Isso depende do SCT-013 + resistor de carga + divisores, etc.
// Comece com um chute (ex: 30 A/V) e depois ajuste medindo com alicate amperímetro.
float FATOR_A_POR_V = 30.0;  // AJUSTE NA CALIBRAÇÃO

// ====== Variáveis internas ======
float offsetAdc = 0.0f;   // nível DC (meio da onda)
float emaAdc    = 0.0f;   // filtro exponencial da leitura bruta

// Acumuladores para RMS
double somaQuadrados = 0.0;
unsigned long contAmostras = 0;
unsigned long inicioJanela = 0;

// ------------------------------------------------------
// Faz várias leituras para descobrir o offset (nível DC)
// idealmente: fazer isso com pouquíssima carga, ou nenhuma
// ------------------------------------------------------
float medirOffsetAdc(int pino, int nLeituras = 2000) {
  long soma = 0;
  for (int i = 0; i < nLeituras; i++) {
    soma += analogRead(pino);
    delay(1);  // pequeno delay só pra acalmar
  }
  return (float)soma / (float)nLeituras;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // Calibra o offset no início
  Serial.println("Calibrando offset do sensor...");
  offsetAdc = medirOffsetAdc(PINO_SENSOR);
  emaAdc    = offsetAdc;
  inicioJanela = millis();

  Serial.print("Offset ADC encontrado: ");
  Serial.println(offsetAdc, 2);
  Serial.println("Iniciando leituras...");
}

void loop() {
  // 1) Leitura bruta do ADC
  int leitura = analogRead(PINO_SENSOR);

  // 2) Filtro exponencial (suavização)
  emaAdc = EMA_ALPHA * leitura + (1.0f - EMA_ALPHA) * emaAdc;

  // 3) Conversão pra tensão (instantânea e filtrada)
  float tensaoInstant = leitura * ADC_TO_V;
  float tensaoFiltrada = emaAdc * ADC_TO_V;

  // 4) Remove offset (centra a onda em torno de zero)
  float leituraCentralizada = leitura - offsetAdc;
  float vSensor = leituraCentralizada * ADC_TO_V; // tensão AC relativa ao zero virtual

  // 5) Acumula para cálculo de RMS
  somaQuadrados  += (double)vSensor * (double)vSensor;
  contAmostras++;

  unsigned long agora = millis();

  // Quando fecha a janela de 1 segundo, calcula RMS e imprime
  if (agora - inicioJanela >= JANELA_MS) {
    float vRms = 0.0f;
    float iRms = 0.0f;
    float pAparente = 0.0f;

    if (contAmostras > 0) {
      vRms = sqrt(somaQuadrados / (double)contAmostras); // Vrms no secundário
      iRms = vRms * FATOR_A_POR_V;                        // Irms estimada
      pAparente = iRms * TENS_REDE;                       // VA (sem considerar fator de potência)
    }

    // ==== PRINT BONITÃO ====
    Serial.println("======================================");
    Serial.print("ADC BRUTO.......: ");
    Serial.println(leitura);

    Serial.print("V_inst (bruta)..: ");
    Serial.print(tensaoInstant, 3);
    Serial.println(" V");

    Serial.print("V_filtrada (EMA): ");
    Serial.print(tensaoFiltrada, 3);
    Serial.println(" V");

    Serial.print("V_RMS sensor....: ");
    Serial.print(vRms, 3);
    Serial.println(" V");

    Serial.print("I_RMS estimada..: ");
    Serial.print(iRms, 3);
    Serial.println(" A");

    Serial.print("Pot. aparente...: ");
    Serial.print(pAparente, 1);
    Serial.println(" VA");
    Serial.println("======================================\n");

    // Reseta janela
    somaQuadrados = 0.0;
    contAmostras = 0;
    inicioJanela = agora;
  }

  // Pequeno delay pra não saturar CPU/serial
  delayMicroseconds(500);  // ~2 kHz de amostragem
}
