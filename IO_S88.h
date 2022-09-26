/*
    © 2022, Marcel Maage. All rights reserved.

    This file is part of DCC++EX API

    This is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    It is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CommandStation.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef IO_S88_H
#define IO_S88_H

#include "IODevice.h"
#include "DIAG.h"  // for DIAG calls

class S88: public IODevice {
  public:
    // Constructor
    S88(VPIN firstVpinAddress, int nPins, int dataPin, int clkPin, int psPin, int resetPin) {
      _firstVpin = firstVpinAddress;
      _nPins = nPins;

      _resetPin = resetPin;    //Reset
      _psPin = psPin;      //PS/LOAD
      _clkPin = clkPin;      //Clock
      _dataPin = dataPin;    //Data input
      _numberOfModuls = (_nPins / 8) + (_nPins % 8) ?  1 : 0;
      _moduls = new uint8_t[_numberOfModuls];
      // initalize pins
      pinMode(_resetPin, OUTPUT);    //Reset
      pinMode(_psPin, OUTPUT);      //PS/LOAD
      pinMode(_clkPin, OUTPUT);      //Clock
      pinMode(_dataPin, INPUT_PULLUP);    //Data input
      digitalWrite(_resetPin, LOW);
      digitalWrite(_psPin, LOW);      //init
      digitalWrite(_clkPin, LOW);

      addDevice(this);
    }
    static void create(VPIN firstVpin, int nPins, int dataPin, int clkPin, int psPin, int resetPin) {
      new S88(firstVpin, nPins, dataPin, clkPin, psPin, resetPin);
    }
  private:
    void _begin() override {
      // Initialise device
      // ...
    }
    void _loop(unsigned long currentMicros) override {
      _getInputData();
      delayUntil(currentMicros + intervalINus);  // 1000ms till next entry
    }

    int _read(VPIN vpin) override {
      // Return acquired data value, e.g.
      int pin = vpin - _firstVpin;
	  if(pin >= _nPins) return 0;
      return bitRead(_moduls[pin / 8], pin % 8);
    }

    void _display() override {
      DIAG(F("S88 Configured on Vpins:%d-%d %S"), _firstVpin, _firstVpin + _nPins - 1,
           _deviceState == DEVSTATE_FAILED ? F("OFFLINE") : F(""));
    }

    void _getInputData(void)
    {

      if (_rCount == 3)    //Load/PS Leitung auf 1, darauf folgt ein Schiebetakt nach 10 ticks!
        digitalWrite(_psPin, HIGH);
      else if (_rCount == 4)   //Schiebetakt nach 5 ticks und S88Module > 0
        digitalWrite(_clkPin, HIGH);       //1. Impuls
      else if (_rCount == 5)   //Read Data IN 1. Bit und S88Module > 0
        _readData();//LOW-Flanke während Load/PS Schiebetakt, dann liegen die Daten an
      else if (_rCount == 9)    //Reset-Plus, löscht die den Paralleleingängen vorgeschaltetetn Latches
        digitalWrite(_resetPin, HIGH);
      else if (_rCount == 10)    //Ende Resetimpuls
        digitalWrite(_resetPin, LOW);
      else if (_rCount == 11)    //Ende PS Phase
        digitalWrite(_psPin, LOW);
      else if (_rCount >= 12)
      { //Auslesen mit weiteren Schiebetakt der Latches links
        if (_rCount % 2 == 0)      //wechselnder Taktimpuls/Schiebetakt
          digitalWrite(_clkPin, HIGH);
        else _readData();    //Read Data IN 2. bis (Module*8) Bit
      }
      _rCount++;      //Zähler für Durchläufe/Takt
      if (_mCount == _numberOfModuls)
      { //Alle Module ausgelesen?
        _rCount = 0;                    //setzte Zähler zurück
        _mCount = 0;                  //beginne beim ersten Modul
        _pCount = 0;                  //beginne beim ersten Port
        //init der Grundpegel
        digitalWrite(_psPin, LOW);
        digitalWrite(_clkPin, LOW);
        digitalWrite(_resetPin, LOW);
      }
    }

    inline _readData(void)
    {
      digitalWrite(_clkPin, LOW);  //LOW-Flanke, dann liegen die Daten an
      byte getData = digitalRead(_dataPin);  //Bit einlesen
      bitWrite(_moduls[_mCount], _pCount, getData);        //Bitzustand Speichern
      _pCount++;
      if (_pCount == 8) {
        _pCount = 0;
        _mCount++;
      }
    }

    int _resetPin;    //Reset
    int _psPin;      //PS/LOAD
    int _clkPin;      //Clock
    int _dataPin;    //Data input

    uint8_t _numberOfModuls;
    uint8_t *_moduls;

    const unsigned long intervalINus{400};

    unsigned int _rCount;    //Lesezähler 0-39 Zyklen (S88Module * 8 * 2 + 10)
    uint8_t _mCount;   //Lesezähler für S88 Module
    uint8_t _pCount;   //Lesezähler für S88 Pin am Modul
};
#endif // IO_S88_H
