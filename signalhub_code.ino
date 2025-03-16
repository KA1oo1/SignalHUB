#include <RCSwitch.h>
#include <EEPROM.h>

RCSwitch mySwitch = RCSwitch();

struct Signal {
  unsigned long value;
  unsigned int bitlength;
  unsigned int delay;
  unsigned int protocol;
  char name[11];
};

Signal signals[12];
int signalCount = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("RF Receiver and Transmitter Setup");
  Serial.println("Commands:");
  Serial.println("  SAVE <name>: Save received signal with a name (max 10 chars).");
  Serial.println("  SEND <name>: Transmit signal by name.");
  Serial.println("  DELETE <name>: Delete a signal by name.");
  Serial.println("  LIST: List all saved signals.");
  Serial.println("  RESET: Clear all saved signals and reset EEPROM.");
  mySwitch.enableReceive(0);
  mySwitch.enableTransmit(10);
  mySwitch.setRepeatTransmit(15);
  loadSignalsFromEEPROM();
}

void loop() {
  if (mySwitch.available()) {
    if (signalCount < 12) {
      signals[signalCount].value = mySwitch.getReceivedValue();
      signals[signalCount].bitlength = mySwitch.getReceivedBitlength();
      signals[signalCount].delay = mySwitch.getReceivedDelay();
      signals[signalCount].protocol = mySwitch.getReceivedProtocol();
      Serial.println("Signal received! Use 'SAVE <name>' to save it.");
    } else {
      Serial.println("Storage full! No more signals can be saved.");
    }
    mySwitch.resetAvailable();
  }

  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.startsWith("SAVE ")) {
      if (signalCount < 12) {
        String name = command.substring(5);
        if (name.length() > 10) {
          Serial.println("Error: Name too long (max 10 characters).");
        } else {
          name.toCharArray(signals[signalCount].name, 11);
          saveSignalToEEPROM(signalCount);
          Serial.print("Signal saved with name: ");
          Serial.println(name);
          signalCount++;
        }
      } else {
        Serial.println("Storage full! No more signals can be saved.");
      }
    } else if (command.startsWith("SEND ")) {
      String name = command.substring(5);
      bool found = false;
      for (int i = 0; i < signalCount; i++) {
        if (name == signals[i].name) {
          found = true;
          Serial.print("Transmitting signal: ");
          Serial.println(name);
          mySwitch.setPulseLength(signals[i].delay);
          mySwitch.setProtocol(signals[i].protocol);
          mySwitch.send(signals[i].value, signals[i].bitlength);
          Serial.println("Transmission complete.");
          break;
        }
      }
      if (!found) {
        Serial.println("Error: Signal name not found.");
      }
    } else if (command.startsWith("DELETE ")) {
      String name = command.substring(7);
      bool found = false;
      for (int i = 0; i < signalCount; i++) {
        if (name == signals[i].name) {
          found = true;
          deleteSignal(i);
          Serial.print("Signal deleted: ");
          Serial.println(name);
          break;
        }
      }
      if (!found) {
        Serial.println("Error: Signal name not found.");
      }
    } else if (command == "LIST") {
      if (signalCount == 0) {
        Serial.println("No signals saved.");
      } else {
        Serial.println("Saved Signals:");
        for (int i = 0; i < signalCount; i++) {
          Serial.print("  ");
          Serial.println(signals[i].name);
        }
      }
    } else if (command == "RESET") {
      resetEEPROM();
      Serial.println("EEPROM reset complete. All saved signals cleared.");
    } else {
      Serial.println("Invalid command. Use SAVE, SEND, DELETE, LIST, or RESET.");
    }
  }
}

void saveSignalToEEPROM(int index) {
  int baseAddress = index * sizeof(Signal);
  EEPROM.put(baseAddress, signals[index]);
}

void loadSignalsFromEEPROM() {
  for (int i = 0; i < 12; i++) {
    int baseAddress = i * sizeof(Signal);
    Signal tempSignal;
    EEPROM.get(baseAddress, tempSignal);
    if (tempSignal.value != 0) {
      signals[i] = tempSignal;
      signalCount++;
    }
  }
}

void deleteSignal(int index) {
  for (int i = index; i < signalCount - 1; i++) {
    signals[i] = signals[i + 1];
    saveSignalToEEPROM(i);
  }
  int lastAddress = (signalCount - 1) * sizeof(Signal);
  Signal emptySignal = {0, 0, 0, 0, ""};
  EEPROM.put(lastAddress, emptySignal);
  signalCount--;
}

void resetEEPROM() {
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0);
  }
  signalCount = 0;
}
