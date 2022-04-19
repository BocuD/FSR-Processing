#include <Joystick.h>
#include <EEPROM.h>

Joystick_ joystick;

int threshHold[] = {550, 400, 300, 450};
int mapping[] = {0, 1, 2, 3};
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

  joystick.begin(false);

  readOffsets();
  readMapping();
}

void loop() {
  readCommands();

  switch (currentMode) {
    case mode_idle:
      sendStatusPacketUSB();
      delayMicroseconds(200);
      break;

    case mode_active:
      sendStatusPacket();
      delayMicroseconds(50);
      break;

    case mode_debug:
      Serial.print("d");
      for (int i = 0; i < 4; i++) {
        readPad(i);

        Serial.print(i);
        Serial.print(":");
        Serial.print(analogRead(mapping[i]));
        Serial.print("|");
      }
      Serial.println();
      delayMicroseconds(200);
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

int b0, b1, b2, b3;
void sendStatusPacket() {
  b0 = readPad(0);
  b1 = readPad(1);
  b2 = readPad(2);
  b3 = readPad(3);
  
  byte out = ((b3 << 3) | (b2 << 2) | (b1 << 1) | b0);
  
  Serial.write(out);
  Serial.write('\n');

  joystick.setButton(0, b0);
  joystick.setButton(1, b1);
  joystick.setButton(2, b2);
  joystick.setButton(3, b3);
  joystick.sendState();
}

void sendStatusPacketUSB() {
  b0 = readPad(0);
  b1 = readPad(1);
  b2 = readPad(2);
  b3 = readPad(3);

  joystick.setButton(3, b0);
  joystick.setButton(0, b1);
  joystick.setButton(2, b2);
  joystick.setButton(1, b3);
  joystick.sendState();
}

bool readPad(int port) {
  if (analogRead(mapping[port]) > threshHold[port]) {
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

          Serial.println("/current mappings: ");

          for (int i = 0; i < 4; i++) {
            Serial.print(mapping[i]);
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

      case 'm': {
          Serial.println("/adjust mapping: please specify index");

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

          Serial.print("/adjust mapping: selected index: ");
          Serial.print(index);
          Serial.print("; current port: ");
          Serial.println(mapping[index]);
          Serial.println("/adjust mapping: please specify new port");

          number = "";
          c = '0';

          while (c != '\n') {
            if (Serial.available() > 0) {
              c = Serial.read();
              number += c;
            }
          }

          mapping[index] = number.toInt();

          Serial.print("/adjust mapping: selected port: ");
          Serial.println(mapping[index]);
          writeMapping();
        }
        break;

      case 'w': {
          writeOffsets();
          writeMapping();
          Serial.println("/wrote offsets");
        }
        break;

      case 'r': {
          resetOffsets();
          resetMapping();
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

void readMapping() {
  for (int i = 4; i < 8; i++) {
    mapping[i-4] = EEPROM.read(i);
  }
}

void writeOffsets() {
  for (int i = 0; i < 4; i++) {
    EEPROM.write(i, threshHold[i] / 4);
  }
}

void writeMapping() {
  for (int i = 4; i < 8; i++) {
    EEPROM.write(i, mapping[i-4]);
  }
}

void resetOffsets() {
  for (int i = 0; i < 4; i++) {
    EEPROM.write(i, 300 / 4);
  }

  readOffsets();
}

void resetMapping() {
  for (int i = 4; i < 8; i++) {
    EEPROM.write(i, i-4);
  }

  readMapping();
}
