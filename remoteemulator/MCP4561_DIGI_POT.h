// Steve Quinn 11/09/18
//
// Copyright 2018 Steve Quinn
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//
//

#ifndef MCP4561_Digital_Pot_h
#define MCP4561_Digital_Pot_h

#include <inttypes.h>
#include <Wire.h>


// Source of MCP4561 data sheet from Microchip
// http://ww1.microchip.com/downloads/en/DeviceDoc/22107B.pdf
// See page pp75 for links to further details/app notes.

// To use the library you must first make a call to Wire.begin().
//
// ie.
// MCP4561 digitalPot;
//
// void setup() {
// 	 Wire.begin();
// }
//
// void loop() {
//   uint16_t u16VolLvl = 128;
//	 uint16_t u16RetVal = digitalPot.writeVal(MCP4561_VOL_WIPER_0,u16VolLvl);
// }
//

// This library does not implement EEPROM Write Protect or WiperLock.
// Given it was written for the MCP4561 the maximum number of
// potentiometer steps are #257. See table on page pp2.

// From page pp37
// Address	Function				Memory Type
// 00h 		Volatile Wiper 0 		RAM
// 01h 		Volatile Wiper 1 		RAM
// 02h 		Nonvolatile Wiper 0		EEPROM
// 03h 		Nonvolatile Wiper 1		EEPROM
// 04h 		Volatile TCON Register 	RAM
// 05h 		Status Register 		RAM
// 06h 		Data EEPROM 			EEPROM
// 07h 		Data EEPROM 			EEPROM
// 08h 		Data EEPROM 			EEPROM
// 09h 		Data EEPROM 			EEPROM
// 0Ah 		Data EEPROM 			EEPROM
// 0Bh 		Data EEPROM 			EEPROM
// 0Ch 		Data EEPROM 			EEPROM
// 0Dh 		Data EEPROM 			EEPROM
// 0Eh 		Data EEPROM 			EEPROM
// 0Fh 		Data EEPROM 			EEPROM

// TCON Reg bit constants
#define MCP4561_TERM_R0B        ((uint16_t)0x0001)
#define MCP4561_TERM_R0W        ((uint16_t)0x0002)
#define MCP4561_TERM_R0A        ((uint16_t)0x0004)
#define MCP4561_TERM_R0HW       ((uint16_t)0x0008)
#define MCP4561_TERM_R1B        ((uint16_t)0x0010) // Not applicable for the MCP4561
#define MCP4561_TERM_R1W        ((uint16_t)0x0020) // Not applicable for the MCP4561
#define MCP4561_TERM_R1A        ((uint16_t)0x0040) // Not applicable for the MCP4561
#define MCP4561_TERM_R1HW       ((uint16_t)0x0080) // Not applicable for the MCP4561
//#define MCP4561_TCON_GCEN_BIT     ((uint16_t)0x0100)

#define MCP4561_VOL_WIPER_0     ((uint8_t)0x00)
#define MCP4561_VOL_WIPER_1     ((uint8_t)0x01) // Not applicable for the MCP4561
#define MCP4561_NON_VOL_WIPER_0 ((uint8_t)0x02)
#define MCP4561_NON_VOL_WIPER_1 ((uint8_t)0x03) // Not applicable for the MCP4561
#define MCP4561_TCON_REG        ((uint8_t)0x04)
#define MCP4561_STATUS_REG      ((uint8_t)0x05)

// The MCP4561 has 10 General Purpose 9 bit wide 0x000 (0) - 0x1FF (511) EEPROM Non-Volatile registers These registers come factory set to 0x0FF
#define MCP4561_GP_EEPROM_0     ((uint8_t)0x06)
#define MCP4561_GP_EEPROM_1     ((uint8_t)0x07)
#define MCP4561_GP_EEPROM_2     ((uint8_t)0x08)
#define MCP4561_GP_EEPROM_3     ((uint8_t)0x09)
#define MCP4561_GP_EEPROM_4     ((uint8_t)0x0A)
#define MCP4561_GP_EEPROM_5     ((uint8_t)0x0B)
#define MCP4561_GP_EEPROM_6     ((uint8_t)0x0C)
#define MCP4561_GP_EEPROM_7     ((uint8_t)0x0D)
#define MCP4561_GP_EEPROM_8     ((uint8_t)0x0E)
#define MCP4561_GP_EEPROM_9     ((uint8_t)0x0F)


#define MCP4561_WIPER_0         ((uint8_t)0x00)
#define MCP4561_WIPER_1         ((uint8_t)0x01) // Not applicable for the MCP4561

#define MCP4561_ERR  		    ((uint16_t)0x0FFF)
#define MCP4561_SUCCESS		    ((uint16_t)0x0000)

#define MCP4561_DEFAULT_ADDRESS ((uint8_t) 0x2E) // I2C Address default assumes Pin1 tied low to 0v. ie. A0 = 0.
												 // See Table 6-2 pp50 MCP4561 Microchip data sheet
												 // So A0 = 0 : B00101110, 46, 0x2E
												 //    A0 = 1 : B00101111, 47, 0x2F

//    Resistor model
//    ~~~~~~~~~~~~~~
//
//                 ---
//    Terminal A --o o---+
//    RxA                |
//                      | |   ---
//                      | |<--o o--- Wiper
//                      | |          RxW
//                 ---   |
//    Terminal B --o o---+
//    RxB
//
//    Where x : Is wiper 0 or 1. For the MCP4561 only 0 is relevant.
//          A : Is typically 'high side', say VCC
//          B : Is typically 'low side', say 0V


class MCP4561 {
public:
	// Constructor
	MCP4561(uint8_t u8address = MCP4561_DEFAULT_ADDRESS);

	// Destructor
	~MCP4561();

	// Description
	// -----------
	// This method is used to connect all Terminals A and B and the Wiper with one call.
	//
	// Parameters
	// ----------
	// u8WhichPot : This parameter is used to identify which potentiometer connect. For the MCP4561 only
	//              potentiometer 0 is relevant.
	//              MCP4561_WIPER_0, MCP4561_WIPER_1
	//
	// Return
	// ------
	// uint16_t : MCP4561_SUCCESS if successful, otherwise MCP4561_ERR
	//
	// Usage
	// -----
	// digitalPot.potConnectAll(MCP4561_WIPER_0);
	//
	uint16_t potConnectAll(uint8_t u8WhichPot);

	// Description
	// -----------
	// This method is used to disconnect all Terminals A and B and the Wiper with one call.
	//
	// Parameters
	// ----------
	// u8WhichPot : This parameter is used to identify which potentiometer disconnect. For the MCP4561 only
	//              potentiometer 0 is relevant.
	//              MCP4561_WIPER_0, MCP4561_WIPER_1
	//
	// Return
	// ------
	// uint16_t : MCP4561_SUCCESS if successful, otherwise MCP4561_ERR
	//
	// Usage
	// -----
	// digitalPot.potDisconnectAll(MCP4561_WIPER_0);
	//
	uint16_t potDisconnectAll(uint8_t u8WhichPot);

	// Description
	// -----------
	// This method is used to isolate either Terminal A or B or the Wiper, or any combination thereof.
	//
	// Parameters
	// ----------
	// u16WhichTerminals : This parameter take any of the following values OR'ed together. The method
	//                     only changes those items in the parameter list. For the MCP4561 only
	//                     potentiometer 0 is relevant.
	//                     MCP4561_TERM_R0B, MCP4561_TERM_R0W, MCP4561_TERM_R0A, MCP4561_TERM_R0HW
	//                     MCP4561_TERM_R1B, MCP4561_TERM_R1W, MCP4561_TERM_R1A, MCP4561_TERM_R1HW
    //
	// Return
	// ------
	// uint16_t : MCP4561_SUCCESS if successful, otherwise MCP4561_ERR
	//
	// Usage
	// -----
	// digitalPot.potConnectSelective(MCP4561_TERM_R0B | MCP4561_TERM_R0W | MCP4561_TERM_R0A);
	//
	uint16_t potConnectSelective(uint16_t u16WhichTerminals);

	// Description
	// -----------
	// This method is used to connect either Terminal A or B or the Wiper, or any combination thereof.
	//
	// Parameters
	// ----------
	// u16WhichTerminals : This parameter take any of the following values OR'ed together. The method
	//                     only changes those items in the parameter list. For the MCP4561 only
	//                     potentiometer 0 is relevant.
	//                     MCP4561_TERM_R0B, MCP4561_TERM_R0W, MCP4561_TERM_R0A, MCP4561_TERM_R0HW
	//                     MCP4561_TERM_R1B, MCP4561_TERM_R1W, MCP4561_TERM_R1A, MCP4561_TERM_R1HW
	//
	// Return
	// ------
	// uint16_t : MCP4561_SUCCESS if successful, otherwise MCP4561_ERR
	//
	// Usage
	// -----
	// digitalPot.potDisconnectSelective(MCP4561_TERM_R0B | MCP4561_TERM_R0W | MCP4561_TERM_R0A);
	//
	uint16_t potDisconnectSelective(uint16_t u16WhichTerminals);

	// Description
	// -----------
	// This method is used read the 9 bit value held in the register of choice.
	//
	// Parameters
	// ----------
	// u8WhichRegister : This parameter can take any of the following values;
    //                   MCP4561_VOL_WIPER_0, MCP4561_VOL_WIPER_1, MCP4561_NON_VOL_WIPER_0, MCP4561_NON_VOL_WIPER_1, MCP4561_TCON_REG, MCP4561_STATUS_REG
    //                   MCP4561_GP_EEPROM_0, MCP4561_GP_EEPROM_1, MCP4561_GP_EEPROM_2, MCP4561_GP_EEPROM_3, MCP4561_GP_EEPROM_4,
    //                   MCP4561_GP_EEPROM_5, MCP4561_GP_EEPROM_6, MCP4561_GP_EEPROM_7, MCP4561_GP_EEPROM_8, MCP4561_GP_EEPROM_9
    //
	//                   Note, again for potentiometer specific registers for the MCP4561 only potentiometer 0 is relevant.
	//
	// Return
	// ------
	// uint16_t : 9 bit value from the register of interest if successful, otherwise MCP4561_ERR
	//
	// Usage
	// -----
	// uint16_t retVal = digitalPot.readVal(MCP4561_VOL_WIPER_0);
	//
	uint16_t readVal(uint8_t u8WhichRegister);

	// Description
	// -----------
	// This method is used write a 9 bit value into the register of choice.
	//
	// Parameters
	// ----------
	// u8WhichRegister : This parameter can take any of the following values;
    //                   MCP4561_VOL_WIPER_0, MCP4561_VOL_WIPER_1, MCP4561_NON_VOL_WIPER_0, MCP4561_NON_VOL_WIPER_1, MCP4561_TCON_REG, MCP4561_STATUS_REG
    //                   MCP4561_GP_EEPROM_0, MCP4561_GP_EEPROM_1, MCP4561_GP_EEPROM_2, MCP4561_GP_EEPROM_3, MCP4561_GP_EEPROM_4,
    //                   MCP4561_GP_EEPROM_5, MCP4561_GP_EEPROM_6, MCP4561_GP_EEPROM_7, MCP4561_GP_EEPROM_8, MCP4561_GP_EEPROM_9
    //
	//                   Note 1 : Again for potentiometer specific registers for the MCP4561 only potentiometer 0 is relevant.
	//                   Note 2 : When using this method it should be pointed out, when writing to EEPROM it can take up to 10ms to complete the operation.
	//                            See page pp12. This delay must be handeled at the appliocation level to prevent this methond from being blocking.
	//
	// u16Value        : Value to write to register of choice.
	//
	// Return
	// ------
	// uint16_t : MCP4561_SUCCESS if successful, otherwise MCP4561_ERR
	//
	// Usage
	// -----
	// uint16_t retVal = digitalPot.writeVal(MCP4561_VOL_WIPER_0, (uint16_t)0x00FF);
	//
	uint16_t writeVal(uint8_t u8WhichRegister, uint16_t u16Value);

private:
	uint8_t _u8address;
};


#endif // MCP4561_Digital_Pot_h