import controlP5.*; //<>//

import java.util.*;
import java.awt.AWTException;
import java.awt.Robot;
import java.awt.event.KeyEvent;
import processing.serial.*;

Robot robot;
ControlP5 cp5;
ScrollableList list;

Serial arduino;
boolean buttonState[] = new boolean[4];
int buttonDebug[] = new int[4];
int buttonOffset[] = new int[4];
int buttonMapping[] = new int[4];

int auto_offset = 150;

int keys[] = { KeyEvent.VK_RIGHT, KeyEvent.VK_LEFT, KeyEvent.VK_UP, KeyEvent.VK_DOWN };

String keyNames[] = { "right", "left", "up", "down" };

boolean config = false;

int autoCount = 0;
boolean init = true;

int remapStep = 0;
int lastPort = -1;

String[] serialList;

void setup() {
  size(320, 300);
  surface.setTitle("FSR Pad Controller");
  surface.setAlwaysOnTop(true);

  cp5 = new ControlP5(this);

  printArray(Serial.list());

  serialList = Serial.list();

  List l = Arrays.asList(serialList);

  list = cp5.addScrollableList("serialList")
    .setCaptionLabel("Select serial port")
    .setPosition(0, 20)
    .setSize(100, 100)
    .setBarHeight(20)
    .setItemHeight(20)
    .setItems(l);

  try {
    robot = new Robot();
  }
  catch (AWTException e) {
    e.printStackTrace();
    exit();
  }

  buttonState[0] = false;
  buttonState[1] = false;
  buttonState[2] = false;
  buttonState[3] = false;

  cp5.addButton("Remap")
    .setPosition(250, 220)
    .setSize(50, 19);
  cp5.addButton("Config")
    .setPosition(250, 240)
    .setSize(50, 19);
  cp5.addButton("Auto")
    .setPosition(250, 260)
    .setSize(50, 19);
  cp5.addButton("Reset")
    .setPosition(250, 280)
    .setSize(50, 19);
    
  cp5.addButton("Refresh")
    .setPosition(0, 0)
    .setSize(100, 19);
}

void serialList(int n) {
  if (arduino != null) {
    arduino.write("q\n");
    arduino.stop();
  }
  try {
    arduino = new Serial(this, serialList[n], 115200);
    init = true;
  }
  catch(Exception e) {
  }
}

int lastMillis = 0;
int count = 0;

int rate = 0;

void draw() {
  background(0);

  if (arduino == null) return;

  if (init) {
    arduino.write("s\n");
  }

  if (millis()-lastMillis > 1000) {
    rate = count;
    count = 0;
    lastMillis = millis();
  }

  text("Polling rate: " + rate + "hz", 10, 50);

  if (remap) {
    textAlign(CENTER);
    text("Please step on the panel for " + keyNames[remapStep], 150, 150);
    textAlign(LEFT);
    return;
  }

  if (config) {
    for (int i = 0; i < 4; i++) {
      boolean state = (buttonDebug[i] > buttonOffset[i]);

      if (state) {
        if (!buttonState[i]) {
          buttonState[i] = true;
          robot.keyPress(keys[i]);
        }
      } else {
        if (buttonState[i]) {
          buttonState[i] = false;
          robot.keyRelease(keys[i]);
        }
      }
    }

    text(buttonDebug[0], 300, 110);
    text(buttonDebug[1], 100, 110);
    text(buttonDebug[2], 200, 10);
    text(buttonDebug[3], 200, 210);

    if (buttonState[0]) fill(0, 255, 0);
    else fill(255);

    rect(300, 200, 20, -buttonDebug[0]/10);

    if (buttonState[1]) fill(0, 255, 0);
    else fill(255);

    rect(100, 200, 20, -buttonDebug[1]/10);

    if (buttonState[2]) fill(0, 255, 0);
    else fill(255);

    rect(200, 100, 20, -buttonDebug[2]/10);

    if (buttonState[3]) fill(0, 255, 0);
    else fill(255);

    rect(200, 300, 20, -buttonDebug[3]/10);

    fill(255);

    stroke(255, 0, 0);
    line(300, 200 - buttonOffset[0]/10, 320, 200 - buttonOffset[0]/10);
    line(100, 200 - buttonOffset[1]/10, 120, 200 - buttonOffset[1]/10);
    line(200, 100 - buttonOffset[2]/10, 220, 100 - buttonOffset[2]/10);
    line(200, 300 - buttonOffset[3]/10, 220, 300 - buttonOffset[3]/10);
    stroke(255);
  }

  //right
  if (buttonState[0]) {
    square(200, 100, 100);
  }
  //left
  if (buttonState[1]) {
    square(0, 100, 100);
  }
  //up
  if (buttonState[2]) {
    square(100, 0, 100);
  }
  //down
  if (buttonState[3]) {
    square(100, 200, 100);
  }
}

String currentString;

void serialEvent(Serial arduino) {
  while (arduino.available() > 0) {
    char c = arduino.readChar();
    currentString += c;
    if (c == '\n') {
      processString(currentString);
      currentString = "";
    }
  }
}

void processString(String input) {
  if (input == null) return;

  //get initial button offsets
  if (init) {
    if (input.contains("|")) {
      print(input);

      //parse button offsets
      String states[] = split(input.substring(1), "|");

      for (int i = 0; i < 4; i++) {
        buttonOffset[i] = int(states[i]);
      }

      init = false;
    }
    return;
  }

  //when in config mode we use a different protocol
  if (config) {
    if (input.length() > 2) {
      String lines[] = split(input.substring(1), "|");

      if (lines.length < 4) return;

      count++;

      for (int i = 0; i < 4; i++) {
        buttonDebug[i] = int(lines[i].substring(2));
      }

      if (auto) {
        if (autoCount < 100) {
          autoCount++;
        } else {
          for (int i = 0; i < 4; i++) {
            arduino.write("o\n");
            arduino.write(Integer.toString(i));
            arduino.write("\n");
            buttonOffset[i] = buttonDebug[i] + auto_offset;
            arduino.write(Integer.toString(buttonOffset[i]));
            arduino.write("\n");
          }

          auto = false;
          autoCount = 0;

          if (!wasConfig)
            Config();
        }
      } 
      else if (remap) 
      {
        for (int i = 0; i < 4; i++) {
          if (buttonDebug[i] > buttonOffset[i] && i != lastPort) {
            buttonMapping[remapStep] = i;
            lastPort = i;
            remapStep++;
            println(remapStep);
            if (remapStep > 3)
            {
              remap = false;

              for (int j = 0; j < 4; j++) {
                arduino.write("m\n");
                arduino.write(Integer.toString(j));
                arduino.write("\n");
                arduino.write(Integer.toString(buttonMapping[j]));
                arduino.write("\n");
              }

              if (!wasConfig) 
                Config();
            }
            break;
          }
        }
      }
    }
    return;
  }

  //in normal mode the protocol is as efficient as possible for higher polling rates
  byte packed = (byte)int(input.charAt(0));

  for (int i = 0; i < 4; i++) {
    boolean state = checkBit(packed, i);

    if (state) {
      if (!buttonState[i]) {
        buttonState[i] = true;
        robot.keyPress(keys[i]);
      }
    } else {
      if (buttonState[i]) {
        buttonState[i] = false;
        robot.keyRelease(keys[i]);
      }
    }
  }

  count++;
}

boolean checkBit(byte value, int x) {
  if (value << ~x < 0) {
    return true;
  } else {
    return false;
  }
}

void exit() {
  for (int i = 0; i < 4; i++) {
    robot.keyRelease(keys[i]);
  }

  arduino.write("q\n");
  arduino.stop();
}

void Config() {
  if(arduino == null) return;

  config = !config;

  //exiting config
  if (!config) {
    arduino.write("w\n");

    //entering config
  } else {
    for (int i = 0; i < 4; i++) {
      robot.keyRelease(keys[i]);
    }
  }

  //toggle debug
  arduino.write("d\n");
}

boolean wasConfig = false;
boolean auto = false;
void Auto() {
  if(arduino == null) return;
  
  wasConfig = config;

  if (!config)
    Config();

  auto = true;
}

void Refresh() {
  serialList = Serial.list();

  List l = Arrays.asList(serialList);

  list.setItems(l);
}

boolean remap = false;
void Remap() {
  if(arduino == null) return;

  if (!remap) {
    wasConfig = config;

    if (!config)
      Config();

    remap = true;
    remapStep = 0;
    lastPort = -1;
  }
}

void Reset() {
  if(arduino == null) return;
    
  arduino.write("r\n");
}
