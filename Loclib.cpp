/**
 *Supporting routines for WifiManualControl to handle locomotive data.
 */

#include <Arduino.h>
#include <EEPROM.h>
#include <Loclib.h>
#include <string.h>

/***********************************************************************************************************************
 */
LocLib::LocLib() {
  m_NumberOfLocs = 0;
  m_ActualSelectedLoc = 0;
  memset(&m_LocLibData, 0, sizeof(LocLibData));
}

/***********************************************************************************************************************
 */
void LocLib::Init() {
  uint8_t Version;

  EEPROM.begin(1024);

  Version = EEPROM.read(locLibEepromAddressVersion);

  /* If new EEPROM version or initial empty EEPROM create one loc and store loc
   * in EEPROM.*/
  if (Version != locLibEepromVersion) {
    m_NumberOfLocs = 1;
    m_LocLibData.Addres = 3;
    m_LocLibData.Steps = decoderStep28;
    m_LocLibData.Dir = directionForward;
    m_LocLibData.Speed = 0;
    m_ActualSelectedLoc = 0;
    m_LocLibData.FunctionAssignment[0] = 0;
    m_LocLibData.FunctionAssignment[1] = 1;
    m_LocLibData.FunctionAssignment[2] = 2;
    m_LocLibData.FunctionAssignment[3] = 3;
    m_LocLibData.FunctionAssignment[4] = 4;

    EEPROM.write(locLibEepromAddressVersion, locLibEepromVersion);
    EEPROM.write(locLibEepromAddressNumOfLocs, m_NumberOfLocs);
    EEPROM.put(locLibEepromAddressData, m_LocLibData);
    EEPROM.commit();
  } else {
    /* Read data from EEPROM of first loc. */
    m_NumberOfLocs = EEPROM.read(locLibEepromAddressNumOfLocs);
    EEPROM.get(locLibEepromAddressData, m_LocLibData);
  }
}

/***********************************************************************************************************************
 */
LocLib::LocLibData* LocLib::DataGet(void) { return (&m_LocLibData); }

bool LocLib::SpeedSet(int8_t Delta) {
  bool Result = true;

  if (Delta == 0) {
    /* Stop loc or when already stop change direction. */
    if (m_LocLibData.Speed != 0) {
      m_LocLibData.Speed = 0;
    } else {
      DirectionToggle();
    }
  } else if (Delta > 0) {
    /* Handle direction change or increase / decrease speed depending on
     * direction. */
    if ((m_LocLibData.Speed == 0) && (m_LocLibData.Dir == directionBackWard)) {
      m_LocLibData.Dir = directionForward;
    } else {
      if (m_LocLibData.Dir == directionForward) {
        m_LocLibData.Speed++;
      } else {
        if (m_LocLibData.Speed > 0) {
          m_LocLibData.Speed--;
        }
      }
    }
  } else if (Delta < 0) {
    /* Handle direction change or increase / decrease speed depending on
     * direction. */
    if ((m_LocLibData.Speed == 0) && (m_LocLibData.Dir == directionForward)) {
      m_LocLibData.Dir = directionBackWard;
    } else {
      if (m_LocLibData.Dir == directionForward) {
        if (m_LocLibData.Speed > 0) {
          m_LocLibData.Speed--;
        }
      } else {
        m_LocLibData.Speed++;
      }
    }
  } else {
    Result = false;
  }

  /* Limit speed based on decoder type. */
  switch (m_LocLibData.Steps) {
    case decoderStep14:
      if (m_LocLibData.Speed > 14) {
        m_LocLibData.Speed = 14;
        Result = false;
      }
      break;
    case decoderStep28:
      if (m_LocLibData.Speed > 28) {
        m_LocLibData.Speed = 28;
        Result = false;
      }
      break;
    case decoderStep128:
      if (m_LocLibData.Speed > 127) {
        m_LocLibData.Speed = 127;
        Result = false;
      }
      break;
  }
  return (Result);
}

/***********************************************************************************************************************
 */
uint8_t LocLib::SpeedGet(void) { return (m_LocLibData.Speed); }

/***********************************************************************************************************************
 */
void LocLib::SpeedUpdate(uint8_t Speed) { m_LocLibData.Speed = Speed; }

/***********************************************************************************************************************
 */
void LocLib::DecoderStepsUpdate(decoderSteps Steps) {
  m_LocLibData.Steps = Steps;
}

/***********************************************************************************************************************
 */
LocLib::decoderSteps LocLib::DecoderStepsGet(void) {
  return (m_LocLibData.Steps);
}

/***********************************************************************************************************************
 */
void LocLib::DirectionToggle(void) {
  if (m_LocLibData.Dir == directionForward) {
    m_LocLibData.Dir = directionBackWard;
  } else {
    m_LocLibData.Dir = directionForward;
  }
}

/***********************************************************************************************************************
 */
LocLib::direction LocLib::DirectionGet(void) { return (m_LocLibData.Dir); }

/***********************************************************************************************************************
 */
void LocLib::DirectionSet(direction dir) { m_LocLibData.Dir = dir; }

/***********************************************************************************************************************
 */
void LocLib::FunctionUpdate(uint32_t FunctionData) {
  m_LocLibData.Function = FunctionData << 1;
}

/***********************************************************************************************************************
 */
void LocLib::FunctionToggle(uint8_t number) {
  m_LocLibData.Function ^= (1 << (number + 1));
}

/***********************************************************************************************************************
 */
uint8_t LocLib::FunctionAssignedGet(uint8_t number) {
  uint8_t Index = 255;

  if (number < 5) {
    Index = m_LocLibData.FunctionAssignment[number];
  }

  return (Index);
}

/***********************************************************************************************************************
 */
LocLib::function LocLib::FunctionStatusGet(uint32_t number) {
  function Result = functionNone;

  if (number <= 28) {
    if ((m_LocLibData.Function & (1 << (number + 1))) == (1 << (number + 1))) {
      Result = functionOn;
    } else {
      Result = functionOff;
    }
  }

  return (Result);
}

/***********************************************************************************************************************
 */
uint16_t LocLib::GetNextLoc(int8_t Delta) {
  if (Delta != 0) {
    /* Increase or decrease locindex, and if required roll over from begin to
     * end or end to begin. */
    if (Delta > 0) {
      m_ActualSelectedLoc++;

      if (m_ActualSelectedLoc >= m_NumberOfLocs) {
        m_ActualSelectedLoc = 0;
      }
    } else {
      if (m_ActualSelectedLoc == 0) {
        m_ActualSelectedLoc = (m_NumberOfLocs - 1);
      } else {
        m_ActualSelectedLoc--;
      }
    }

    EEPROM.get(
        locLibEepromAddressData + (m_ActualSelectedLoc * sizeof(LocLibData)),
        m_LocLibData);
  }

  return (m_LocLibData.Addres);
}

/***********************************************************************************************************************
 */
uint16_t LocLib::GetActualLocAddress(void) { return (m_LocLibData.Addres); }

/***********************************************************************************************************************
 */
uint8_t LocLib::CheckLoc(uint16_t address) {
  bool Found = false;
  uint8_t Index = 0;
  LocLibData Data;
  int EepromAddressData = 2;

  while ((Index < m_NumberOfLocs) && (Found == false)) {
    // Read data from EEPROM and check address.
    EEPROM.get(EepromAddressData, Data);

    if (Data.Addres == address) {
      Found = true;
    } else {
      Index++;
      EepromAddressData += sizeof(LocLibData);
    }
  }

  if (Index >= m_NumberOfLocs) {
    Index = 255;
  }

  return (Index);
}

/***********************************************************************************************************************
 */
bool LocLib::StoreLoc(uint16_t address, uint8_t* FunctionAssigment) {
  bool Result = false;
  LocLibData Data;
  int EepromAddressData;
  uint8_t LocIndex;

  LocIndex = CheckLoc(address);

  /* Check if loc is already present in eeprom. */
  if (LocIndex != 255) {
    /* Read data, update function data and store. */
    EEPROM.get(locLibEepromAddressData + (LocIndex * sizeof(LocLibData)), Data);
    memcpy(Data.FunctionAssignment, FunctionAssigment, 5);
    EEPROM.put(locLibEepromAddressData + (LocIndex * sizeof(LocLibData)), Data);
    EEPROM.commit();

    /* Read data to update actual loc data. */
    EEPROM.get(locLibEepromAddressData + (LocIndex * sizeof(LocLibData)),
               m_LocLibData);

    Result = true;
  } else {
    /* Not present, add data. */
    if (m_NumberOfLocs < locLibMaxNumberOfLocs) {
      /* Max number of locs not exceeded. */
      Data.Addres = address;
      Data.Steps = decoderStep28;
      Data.Dir = directionForward;
      Data.Speed = 0;
      Data.Function = 0;
      memcpy(Data.FunctionAssignment, FunctionAssigment, 5);

      EepromAddressData =
          locLibEepromAddressData + (m_NumberOfLocs * sizeof(LocLibData));
      m_NumberOfLocs++;

      EEPROM.write(locLibEepromAddressNumOfLocs, m_NumberOfLocs);
      EEPROM.put(EepromAddressData, Data);
      EEPROM.commit();

      /* Get newly added locdata. */
      m_ActualSelectedLoc = m_NumberOfLocs - 1;
      EEPROM.get(
          locLibEepromAddressData + (m_ActualSelectedLoc * sizeof(LocLibData)),
          m_LocLibData);

      Result = true;
    }
  }

  return (Result);
}

/***********************************************************************************************************************
 */
bool LocLib::RemoveLoc(uint16_t address) {
  bool Result = false;
  LocLibData Data;
  uint8_t LocIndex;
  uint8_t Index;

  /* If at least two locs are presewnt delete loc. */
  if (m_NumberOfLocs > 1) {
    LocIndex = CheckLoc(address);

    /* If loc is present delete it. */

    if (LocIndex != 255) {
      Index = LocIndex;
      /* Copy data next loc to this location so loc is removed. */
      while ((Index + 1) < m_NumberOfLocs) {
        EEPROM.get(locLibEepromAddressData + ((Index + 1) * sizeof(LocLibData)),
                   Data);
        EEPROM.put(locLibEepromAddressData + (Index * sizeof(LocLibData)),
                   Data);
        EEPROM.commit();
        Index++;
      }
    }

    m_NumberOfLocs--;
    EEPROM.write(locLibEepromAddressNumOfLocs, m_NumberOfLocs);
    EEPROM.commit();

    Result = true;

    /* Load data for "next" loc... */
    if (LocIndex < m_NumberOfLocs) {
      EEPROM.get(locLibEepromAddressData + (LocIndex * sizeof(LocLibData)),
                 m_LocLibData);
    } else {
      /* Last item in list was deleted. */
      EEPROM.get(
          locLibEepromAddressData + ((m_NumberOfLocs - 1) * sizeof(LocLibData)),
          m_LocLibData);
      m_ActualSelectedLoc = m_NumberOfLocs - 1;
    }
  }

  return (Result);
}

/***********************************************************************************************************************
 */
uint16_t LocLib::GetNumberOfLocs(void) { return (m_NumberOfLocs); }

/***********************************************************************************************************************
 */
uint16_t LocLib::GetActualSelectedLocIndex(void) {
  return (m_ActualSelectedLoc + 1);
}

/***********************************************************************************************************************
 */
void LocLib::LocBubbleSort(void) {
  int i, j;
  LocLibData Data_1;
  LocLibData Data_2;
  LocLibData DataTemp;

  for (i = 0; i < (m_NumberOfLocs - 1); ++i) {
    for (j = 0; (j < m_NumberOfLocs - 1 - i); ++j) {
      EEPROM.get(locLibEepromAddressData + (j * sizeof(LocLibData)), Data_1);
      EEPROM.get(locLibEepromAddressData + ((j + 1) * sizeof(LocLibData)),
                 Data_2);

      if (Data_1.Addres > Data_2.Addres) {
        memcpy(&DataTemp, &Data_2, sizeof(LocLibData));
        memcpy(&Data_2, &Data_1, sizeof(LocLibData));
        memcpy(&Data_1, &DataTemp, sizeof(LocLibData));

        EEPROM.put(locLibEepromAddressData + (j * sizeof(LocLibData)), Data_1);
        EEPROM.put(locLibEepromAddressData + ((j + 1) * sizeof(LocLibData)),
                   Data_2);
        EEPROM.commit();
      }
    }
  }
}
