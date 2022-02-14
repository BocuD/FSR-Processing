#include <EEPROM.h>

int threshHold[] = {550, 400, 300, 450};
bool debug = false;

enum mode {
  mode_idle,
  mode_active,
  mode_debug
};

mode currentMode = mode_idle;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  readOffsets();
}

void loop() {
  readCommands();
  delayMicroseconds(500);

  switch (currentMode) {
    case mode_idle:
      animation();
      break;

    case mode_active:
      sendStatusPacket();
      break;

    case mode_debug:
      Serial.print("d");
      for (int i = 0; i < 4; i++) {
        readPad(i);

        Serial.print(i);
        Serial.print(":");
        Serial.print(analogRead(i));
        Serial.print("|");
      }
      Serial.println();
      break;
  }
}

void animation() {
  digitalWrite(2, LOW);
  digitalWrite(3, LOW);
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);

  digitalWrite(2, HIGH);
  delay(100);
  digitalWrite(2, LOW);
  
  digitalWrite(4, HIGH);
  delay(100);
  digitalWrite(4, LOW);
  
  digitalWrite(5, HIGH);
  delay(100);
  digitalWrite(5, LOW);
  
  digitalWrite(3, HIGH);
  delay(100);
  digitalWrite(3, LOW);
}

void sendStatusPacket() {
  byte out = (readPad(3) << 3) | (readPad(2) << 2) | (readPad(1) << 1) | (readPad(0));
  Serial.write(out);
  Serial.write('\n');
}

bool readPad(int port) {
  if (analogRead(port) > threshHold[port]) {
    digitalWrite(port + 2, HIGH);
    return true;
  }

  digitalWrite(port + 2, LOW);
  return false;
}

void readCommands() {
  while (Serial.available() > 0) {
    // read the incoming byte:
    char in = Serial.read();

    switch (in) {
      case 's': {
          Serial.print("s");

          for (int i = 0; i < 4; i++) {
            Serial.print(threshHold[i]);
            Serial.print("|");
          }

          Serial.println();

          digitalWrite(2, HIGH);
          digitalWrite(3, HIGH);
          digitalWrite(4, HIGH);
          digitalWrite(5, HIGH);

          delay(50);

          digitalWrite(2, LOW);
          digitalWrite(3, LOW);
          digitalWrite(4, LOW);
          digitalWrite(5, LOW);

          currentMode = mode_active;
          Serial.println("/init");
        }
        break;

      case 'd': {
          Serial.println(debug ? "/disabled debug mode" : "/enabled debug mode");
          debug = !debug;
          currentMode = debug ? mode_debug : mode_active;
        }
        break;

      case 'p': {
          Serial.println("/current offsets: ");

          for (int i = 0; i < 4; i++) {
            Serial.print(threshHold[i]);
            Serial.print("|");
          }

          Serial.println();
        }
        break;

      case 'o': {
          Serial.println("/adjust offset: please specify index");

          Serial.read();

          char c = '0';
          String number = "";

          while (c != '\n') {
            if (Serial.available() > 0) {
              c = Serial.read();
              number += c;
            }
          }

          int index = number.toInt();

          Serial.print("/adjust offset: selected index: ");
          Serial.print(index);
          Serial.print("; current offset: ");
          Serial.println(threshHold[index]);
          Serial.println("/adjust offset: please specify new offset");

          number = "";
          c = '0';

          while (c != '\n') {
            if (Serial.available() > 0) {
              c = Serial.read();
              number += c;
            }
          }

          threshHold[index] = number.toInt();

          Serial.print("/adjust offset: selected offset: ");
          Serial.println(threshHold[index]);
          writeOffsets();
        }
        break;

      case 'w': {
          writeOffsets();
          Serial.println("/wrote offsets");
        }
        break;

      case 'r': {
          resetOffsets();
          Serial.println("/reset offsets");
        }
        break;

      case 'q': {
          Serial.println("/returning to idle mode");
          digitalWrite(2, HIGH);
          digitalWrite(3, HIGH);
          digitalWrite(4, HIGH);
          digitalWrite(5, HIGH);

          currentMode = mode_idle;

          delay(500);
        }
        break;
    }
  }
}

void readOffsets() {
  for (int i = 0; i < 4; i++) {
    threshHold[i] = EEPROM.read(i) * 4;
  }
}

void writeOffsets() {
  for (int i = 0; i < 4; i++) {
    EEPROM.write(i, threshHold[i] / 4);
  }
}

void resetOffsets() {
  for (int i = 0; i < 4; i++) {
    EEPROM.write(i, 300 / 4);
  }

  readOffsets();
}
