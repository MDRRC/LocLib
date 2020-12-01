/***********************************************************************************************************************
   @file   wc_cv.cpp
   @brief  Main application of WifiManualControl (WMC).
 **********************************************************************************************************************/

/***********************************************************************************************************************
   I N C L U D E S
 **********************************************************************************************************************/
#include "LocStorage.h"
#include "app_cfg.h"
#include "eep_cfg.h"
#include <Arduino.h>

#if APP_CFG_UC == APP_CFG_UC_ESP8266
#include <EEPROM.h>
#include <spi_flash.h>
#else
#include <Wire.h>
#endif

/***********************************************************************************************************************
   D E F I N E S
 **********************************************************************************************************************/

/***********************************************************************************************************************
   F O R W A R D  D E C L A R A T I O N S
 **********************************************************************************************************************/

/***********************************************************************************************************************
   D A T A   D E C L A R A T I O N S (exported, local)
 **********************************************************************************************************************/

/***********************************************************************************************************************
  F U N C T I O N S
 **********************************************************************************************************************/

/***********************************************************************************************************************
 */
#if APP_CFG_UC == APP_CFG_UC_STM32
void i2c_eeprom_write_byte(int deviceaddress, unsigned int eeaddress, byte data)
{
    int rdata = data;
    Wire.beginTransmission(deviceaddress);
    Wire.write((int)(eeaddress >> 8));   // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.write(rdata);
    Wire.endTransmission();
    delay(100);
}

/***********************************************************************************************************************
 */
void i2c_eeprom_write_page(int deviceaddress, unsigned int eeaddresspage, byte* data, byte length)
{
    Wire.beginTransmission(deviceaddress);
    Wire.write((int)(eeaddresspage >> 8));   // MSB
    Wire.write((int)(eeaddresspage & 0xFF)); // LSB
    byte c;
    for (c = 0; c < length; c++)
        Wire.write(data[c]);
    Wire.endTransmission();
    delay(100);
}

/***********************************************************************************************************************
 */
byte i2c_eeprom_read_byte(int deviceaddress, unsigned int eeaddress)
{
    byte rdata = 0xFF;
    Wire.beginTransmission(deviceaddress);
    Wire.write((int)(eeaddress >> 8));   // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.endTransmission();
    Wire.requestFrom(deviceaddress, 1);
    if (Wire.available()) rdata = Wire.read();
    return rdata;
}
#endif

/***********************************************************************************************************************
 */
void LocStorage::Init()
{
#if APP_CFG_UC == APP_CFG_UC_ESP8266
    EEPROM.begin(SPI_FLASH_SEC_SIZE * 2);
#else
    I2CAddressAT24C256 = 0x50;
    Wire.begin();
#endif
}

/***********************************************************************************************************************
 */
bool LocStorage::VersionCheck()
{
    uint8_t Version = 255;
    bool Result     = true;

#if APP_CFG_UC == APP_CFG_UC_STM32
    Version = (uint8_t)(i2c_eeprom_read_byte(I2CAddressAT24C256, EepCfg::EepromVersionAddress));
#elif APP_CFG_UC == APP_CFG_UC_ESP8266
    Version = EEPROM.read(EepCfg::EepromVersionAddress);
#endif

    if (Version != EepCfg::EepromVersion)
    {
        EraseEeprom();
#if APP_CFG_UC == APP_CFG_UC_STM32
        i2c_eeprom_write_byte(I2CAddressAT24C256, EepCfg::EepromVersionAddress, (byte)(EepCfg::EepromVersion));
#elif APP_CFG_UC == APP_CFG_UC_ESP8266
        EEPROM.write(EepCfg::EepromVersionAddress, EepCfg::EepromVersion);
        EEPROM.commit();
#endif
        Result = false;
    }

    return (Result);
}

#if APP_CFG_UC == APP_CFG_UC_STM32
/***********************************************************************************************************************
 */
uint8_t LocStorage::XpNetAddressGet(void)
{
    uint8_t XpNetAddress;

    XpNetAddress = (uint8_t)(i2c_eeprom_read_byte(I2CAddressAT24C256, EepCfg::XpNetAddress));

    return (XpNetAddress);
}

/***********************************************************************************************************************
 */
void LocStorage::XpNetAddressSet(uint8_t XpNetAddress)
{
    i2c_eeprom_write_byte(I2CAddressAT24C256, EepCfg::XpNetAddress, (byte)(XpNetAddress));
}
#endif

/***********************************************************************************************************************
 */
bool LocStorage::AcOptionGet()
{
    bool Result = false;
    uint8_t AcOptionEep;

#if APP_CFG_UC == APP_CFG_UC_STM32
    AcOptionEep = (uint8_t)(i2c_eeprom_read_byte(I2CAddressAT24C256, EepCfg::AcTypeControlAddress));
#elif APP_CFG_UC == APP_CFG_UC_ESP8266
    AcOptionEep = EEPROM.read(EepCfg::AcTypeControlAddress);
#endif

    /* Check AC option.*/
    switch (AcOptionEep)
    {
    case 0: Result = false; break;
    case 1: Result = true; break;
    default: Result = false; break;
    }
    return (Result);
}

/***********************************************************************************************************************
 */
void LocStorage::AcOptionSet(uint8_t acOption)
{
#if APP_CFG_UC == APP_CFG_UC_STM32
    i2c_eeprom_write_byte(I2CAddressAT24C256, EepCfg::AcTypeControlAddress, (byte)(acOption));
#elif APP_CFG_UC == APP_CFG_UC_ESP8266
    EEPROM.write(EepCfg::AcTypeControlAddress, acOption);
    EEPROM.commit();
#endif
}

/***********************************************************************************************************************
 */
void LocStorage::EmergencyOptionSet(uint8_t emergency)
{
#if APP_CFG_UC == APP_CFG_UC_STM32
    i2c_eeprom_write_byte(I2CAddressAT24C256, EepCfg::EmergencyStopEnabledAddress, (byte)(emergency));
#elif APP_CFG_UC == APP_CFG_UC_ESP8266
    EEPROM.write(EepCfg::EmergencyStopEnabledAddress, emergency);
    EEPROM.commit();
#endif
}

/***********************************************************************************************************************
 */
bool LocStorage::EmergencyOptionGet(void)
{
    bool Result = false;
    uint8_t emergencyActive;

#if APP_CFG_UC == APP_CFG_UC_STM32
    emergencyActive = (uint8_t)(i2c_eeprom_read_byte(I2CAddressAT24C256, EepCfg::EmergencyStopEnabledAddress));
#elif APP_CFG_UC == APP_CFG_UC_ESP8266
    emergencyActive = EEPROM.read(EepCfg::EmergencyStopEnabledAddress);
#endif

    /* Check AC option.*/
    switch (emergencyActive)
    {
    case 0: Result = false; break;
    case 1: Result = true; break;
    default: Result = false; break;
    }
    return (Result);
}

/***********************************************************************************************************************
 */
void LocStorage::PulseSwitchInvertSet(uint8_t Invert)
{
#if APP_CFG_UC == APP_CFG_UC_STM32
    i2c_eeprom_write_byte(I2CAddressAT24C256, EepCfg::PulseSwitchInvertAddress, (byte)(Invert));
#elif APP_CFG_UC == APP_CFG_UC_ESP8266
    EEPROM.write(EepCfg::PulseSwitchInvertAddress, Invert);
    EEPROM.commit();
#endif
}

/***********************************************************************************************************************
 */
bool LocStorage::PulseSwitchInvertGet()
{
    bool Result = false;
    uint8_t Invert;

#if APP_CFG_UC == APP_CFG_UC_STM32
    Invert = (uint8_t)(i2c_eeprom_read_byte(I2CAddressAT24C256, EepCfg::PulseSwitchInvertAddress));
#elif APP_CFG_UC == APP_CFG_UC_ESP8266
    Invert    = EEPROM.read(EepCfg::PulseSwitchInvertAddress);
#endif

    /* Check AC option.*/
    switch (Invert)
    {
    case 0: Result = false; break;
    case 1: Result = true; break;
    default: Result = false; break;
    }
    return (Result);
}

/***********************************************************************************************************************
 */
uint8_t LocStorage::NumberOfLocsGet()
{
    uint8_t NumOfLocs;

#if APP_CFG_UC == APP_CFG_UC_STM32
    NumOfLocs = (uint8_t)(i2c_eeprom_read_byte(I2CAddressAT24C256, EepCfg::locLibEepromAddressNumOfLocs));
#elif APP_CFG_UC == APP_CFG_UC_ESP8266
    NumOfLocs = EEPROM.read(EepCfg::locLibEepromAddressNumOfLocs);
#endif
    return (NumOfLocs);
}

/***********************************************************************************************************************
 */
void LocStorage::NumberOfLocsSet(uint8_t numberOfLocs)
{
#if APP_CFG_UC == APP_CFG_UC_STM32
    i2c_eeprom_write_byte(I2CAddressAT24C256, EepCfg::locLibEepromAddressNumOfLocs, (byte)(numberOfLocs));
#elif APP_CFG_UC == APP_CFG_UC_ESP8266
    EEPROM.write(EepCfg::locLibEepromAddressNumOfLocs, numberOfLocs);
    EEPROM.commit();
#endif
}

/***********************************************************************************************************************
 */
bool LocStorage::LocDataGet(LocLibData* DataPtr, uint8_t Index)
{
    int Address = 0;
    bool Result = true;
#if APP_CFG_UC == APP_CFG_UC_ESP8266
    LocLibData Data;
#else
    uint8_t IndexRead    = 0;
    uint8_t* DataReadPtr = (uint8_t*)(DataPtr);
#endif

#if APP_CFG_UC == APP_CFG_UC_ESP8266
    Address = EepCfg::locLibEepromAddressData + ((sizeof(LocLibData) * Index));
    EEPROM.get(Address, Data);
    memcpy(DataPtr, &Data, sizeof(LocLibData));
#else
    /* Get address and read data. */
    Address = EepCfg::locLibEepromAddressLocData + (EepCfg::EepromPageSize * Index);
    for (IndexRead = 0; IndexRead < sizeof(LocLibData); IndexRead++)
    {
        DataReadPtr[IndexRead] = (uint8_t)(i2c_eeprom_read_byte(I2CAddressAT24C256, Address + IndexRead));
    }
#endif

    return (Result);
}

/***********************************************************************************************************************
 */
bool LocStorage::LocDataSet(LocLibData* DataPtr, uint8_t Index)
{
    bool Result = true;
    int Address = 0;
#if APP_CFG_UC == APP_CFG_UC_STM32
    uint8_t* DataWritePtr = (uint8_t*)(DataPtr);
#endif

#if APP_CFG_UC == APP_CFG_UC_ESP8266
    LocLibData Data;

    Address = EepCfg::locLibEepromAddressData + ((sizeof(LocLibData) * Index));
    memcpy(&Data, DataPtr, sizeof(LocLibData));
    EEPROM.put(Address, Data);
    EEPROM.commit();
#else
    /* Put data of a loc on a single page in the AT24C256. */
    Address = EepCfg::locLibEepromAddressLocData + (EepCfg::EepromPageSize * Index);
    i2c_eeprom_write_page(I2CAddressAT24C256, Address, (byte*)(DataWritePtr), sizeof(LocLibData));
#endif

    return (Result);
}

/***********************************************************************************************************************
 */
void LocStorage::SelectedLocIndexStore(uint8_t Index)
{
#if APP_CFG_UC == APP_CFG_UC_APP_CFG_UC_ESP8266
    EEPROM.put(EepCfg::SelectedLocAddress, Index);
    EEPROM.commit();
#else
    i2c_eeprom_write_byte(I2CAddressAT24C256, EepCfg::SelectedLocAddress, Index);
#endif
}

/***********************************************************************************************************************
 */
uint8_t LocStorage::SelectedLocIndexGet()
{
    uint8_t Index = 255;

#if APP_CFG_UC == APP_CFG_UC_APP_CFG_UC_ESP8266
    EEPROM.get(EepCfg::SelectedLocAddress, Index);
#else
    Index = (uint8_t)(i2c_eeprom_read_byte(I2CAddressAT24C256, EepCfg::SelectedLocAddress));
#endif

    return (Index);
}

/***********************************************************************************************************************
 */
void LocStorage::EraseEeprom(void)
{
#if APP_CFG_UC == APP_CFG_UC_ESP8266
    uint16_t Index = 0;

    for (Index = 0; Index < SPI_FLASH_SEC_SIZE; Index++)
    {
        EEPROM.write(Index, 0xFF);
    }
    EEPROM.commit();
#else

#endif
}

#if APP_CFG_UC == APP_CFG_UC_ESP8266
void LocStorage::InvalidateAdc(void)
{
    uint8_t buttonAdcValid = 0;
    EEPROM.write(EepCfg::ButtonAdcValuesAddressValid, buttonAdcValid);
    EEPROM.commit();
}
#endif
