/**
 *Supporting routines for WifiManualControl to handle locomotive data.
 */

#include "app_cfg.h"
#include "eep_cfg.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <Loclib.h>
#include <string.h>

/***********************************************************************************************************************
 */
LocLib::LocLib()
{
    m_AcOption          = 0;
    m_NumberOfLocs      = 1;
    m_ActualSelectedLoc = 0;
    memset(&m_LocLibData, 0, sizeof(LocLibData));
}

/***********************************************************************************************************************
 */
void LocLib::Init(LocStorage Storage)
{
    m_LocStorage = Storage;

    if (m_LocStorage.VersionCheck() == false)
    {
        InitialLocStore();
        m_LocStorage.NumberOfLocsSet(m_NumberOfLocs);
        StoreLoc(m_LocLibData.Addres, m_LocLibData.FunctionAssignment, NULL, storeAdd);

        m_LocStorage.AcOptionSet(0);
        m_LocStorage.EmergencyOptionSet(0);
    }

    /* Check AC option.*/
    m_AcOption = m_LocStorage.AcOptionGet();

    /* Read data from EEPROM of first loc. */
    m_ActualSelectedLoc = m_LocStorage.SelectedLocIndexGet();
    if (m_ActualSelectedLoc > MaxNumberOfLocs)
    {
        m_ActualSelectedLoc = 0;
    }
    m_LocStorage.LocDataGet(&m_LocLibData, m_ActualSelectedLoc);
    m_NumberOfLocs = m_LocStorage.NumberOfLocsGet();

    /* Get number of locs and check it... */
    if (m_NumberOfLocs > MaxNumberOfLocs)
    {
        m_LocStorage.NumberOfLocsSet(1);
        m_NumberOfLocs = 1;
    }
}

/***********************************************************************************************************************
 */
LocLibData* LocLib::DataGet(void) { return (&m_LocLibData); }

/***********************************************************************************************************************
 */
void LocLib::UpdateLocData(uint16_t address)
{
    uint16_t Index = 0;
    bool Found     = false;
    LocLibData Data;

    while ((Index < m_NumberOfLocs) && (Found == false))
    {
        // Read data from EEPROM and check address.
        m_LocStorage.LocDataGet(&Data, Index);

        if (Data.Addres == address)
        {
            Found = true;
            memcpy(&m_LocLibData, &Data, sizeof(LocLibData));
        }
        else
        {
            Index++;
        }
    }
}

/***********************************************************************************************************************
 */
uint16_t LocLib::SpeedSet(int8_t Delta)
{
    uint16_t Speed = 0xFFFF;

    if (m_AcOption == false)
    {
        if (Delta == 0)
        {
            /* Stop loc or when already stop change direction. */
            Speed = SpeedStopOrChangeDirection();
        }
        else if (Delta > 0)
        {
            /* Handle direction change or increase / decrease speed depending on
             * direction. */
            if ((m_LocLibData.Speed == 0) && (m_LocLibData.Dir == directionBackWard))
            {
                m_LocLibData.Dir = directionForward;
                Speed            = m_LocLibData.Speed;
            }
            else
            {
                if (m_LocLibData.Dir == directionForward)
                {
                    /* Handle speed increase*/
                    Speed = SpeedIncrease();
                }
                else
                {
                    /* Handle speed decrease*/
                    Speed = SpeedDecrease();
                }
            }
        }
        else if (Delta < 0)
        {
            /* Handle direction change or increase / decrease speed depending on
             * direction. */
            if ((m_LocLibData.Speed == 0) && (m_LocLibData.Dir == directionForward))
            {
                m_LocLibData.Dir = directionBackWard;
                Speed            = m_LocLibData.Speed;
            }
            else
            {
                if (m_LocLibData.Dir == directionForward)
                {
                    /* Handle speed decrease*/
                    Speed = SpeedDecrease();
                }
                else
                {

                    /* Handle speed increase*/
                    Speed = SpeedIncrease();
                }
            }
        }
    }
    else
    {
        /* AC option active, direction change only with push button or when loc is already stopped. */
        if (Delta > 0)
        {
            /* Handle speed increase*/
            Speed = SpeedIncrease();
        }
        else if (Delta < 0)
        {
            /* Handle speed decrease*/
            Speed = SpeedDecrease();
        }
        else
        {
            /* Stop loc or when already stop change direction. */
            SpeedStopOrChangeDirection();
        }
    }

    /* Limit speed based on decoder type. */
    if (Speed != 0xFFFF)
    {
        switch (m_LocLibData.Steps)
        {
        case decoderStep14:
            if (Speed > 14)
            {
                Speed = 14;
            }
            break;
        case decoderStep28:
            if (Speed > 28)
            {
                Speed = 28;
            }
            break;
        case decoderStep128:
            if (Speed > 127)
            {
                Speed = 127;
            }
            break;
        }
    }

    return (Speed);
}

/***********************************************************************************************************************
 */
uint8_t LocLib::SpeedGet(void) { return (m_LocLibData.Speed); }

/***********************************************************************************************************************
 */
void LocLib::SpeedUpdate(uint8_t Speed) { m_LocLibData.Speed = Speed; }

/***********************************************************************************************************************
 */
void LocLib::DecoderStepsUpdate(decoderSteps Steps) { m_LocLibData.Steps = Steps; }

/***********************************************************************************************************************
 */
decoderSteps LocLib::DecoderStepsGet(void) { return (m_LocLibData.Steps); }

/***********************************************************************************************************************
 */
void LocLib::DirectionToggle(void)
{
    if (m_LocLibData.Dir == directionForward)
    {
        m_LocLibData.Dir = directionBackWard;
    }
    else
    {
        m_LocLibData.Dir = directionForward;
    }
}

/***********************************************************************************************************************
 */
direction LocLib::DirectionGet(void) { return (m_LocLibData.Dir); }

/***********************************************************************************************************************
 */
void LocLib::DirectionSet(direction dir) { m_LocLibData.Dir = dir; }

/***********************************************************************************************************************
 */
void LocLib::FunctionUpdate(uint32_t FunctionData) { m_LocLibData.Function = FunctionData; }

/***********************************************************************************************************************
 */
void LocLib::FunctionToggle(uint8_t number) { m_LocLibData.Function ^= (1 << number); }

/***********************************************************************************************************************
 */
uint8_t LocLib::FunctionAssignedGet(uint8_t number)
{
    uint8_t Index = 255;

    if (number < 5)
    {
        Index = m_LocLibData.FunctionAssignment[number];
    }

    return (Index);
}

/***********************************************************************************************************************
 */
bool LocLib::FunctionAssignedGetStored(uint16_t address, uint8_t* functions)
{
    bool Found    = false;
    uint8_t Index = 0;
    LocLibData Data;

    while ((Index < m_NumberOfLocs) && (Found == false))
    {
        // Read data from EEPROM and check address.
        m_LocStorage.LocDataGet(&Data, Index);

        if (Data.Addres == address)
        {
            Found = true;
            memcpy(functions, Data.FunctionAssignment, 5);
        }
        else
        {
            Index++;
        }
    }

    return (Found);
}

/***********************************************************************************************************************
 */
LocLib::function LocLib::FunctionStatusGet(uint32_t number)
{
    function Result = functionNone;

    if (number <= 28)
    {
        if (m_LocLibData.Function & (1 << number))
        {
            Result = functionOn;
        }
        else
        {
            Result = functionOff;
        }
    }

    return (Result);
}

/***********************************************************************************************************************
 */
uint16_t LocLib::GetNextLoc(int8_t Delta)
{
    if (Delta != 0)
    {
        /* Increase or decrease locindex, and if required roll over from begin to
         * end or end to begin. */
        if (Delta > 0)
        {
            m_ActualSelectedLoc++;

            if (m_ActualSelectedLoc >= m_NumberOfLocs)
            {
                m_ActualSelectedLoc = 0;
            }
        }
        else
        {
            if (m_ActualSelectedLoc == 0)
            {
                m_ActualSelectedLoc = (m_NumberOfLocs - 1);
            }
            else
            {
                m_ActualSelectedLoc--;
            }
        }

        m_LocStorage.LocDataGet(&m_LocLibData, m_ActualSelectedLoc);
        m_LocStorage.SelectedLocIndexStore(m_ActualSelectedLoc);
    }

    return (m_LocLibData.Addres);
}

/***********************************************************************************************************************
 */
uint16_t LocLib::GetActualLocAddress(void) { return (m_LocLibData.Addres); }

/***********************************************************************************************************************
 */
char* LocLib::GetLocName(void) { return (m_LocLibData.Name); }

/***********************************************************************************************************************
 */
uint8_t LocLib::CheckLoc(uint16_t address)
{
    bool Found    = false;
    uint8_t Index = 0;
    LocLibData Data;

    while ((Index < m_NumberOfLocs) && (Found == false))
    {
        // Read data from EEPROM and check address.
        m_LocStorage.LocDataGet(&Data, Index);

        if (Data.Addres == address)
        {
            Found = true;
        }
        else
        {
            Index++;
        }
    }

    if (Index >= m_NumberOfLocs)
    {
        Index = 255;
    }

    return (Index);
}

/***********************************************************************************************************************
 */
bool LocLib::StoreLoc(uint16_t address, uint8_t* FunctionAssignment, char* Name, store storeAction)
{
    LocLibData Data;
    uint8_t LocIndex;
    bool Result = false;

    LocIndex = CheckLoc(address);

    /* Check if loc is already present in eeprom. */
    if (LocIndex != 255)
    {
        if (storeAction == storeChange)
        {
            /* Read data, update function data and store. */
            m_LocStorage.LocDataGet(&Data, LocIndex);

            if (Name != NULL)
            {
                memset(Data.Name, '\0', sizeof(Data.Name));
                memcpy(Data.Name, Name, sizeof(Data.Name) - 1);
            }
            if (FunctionAssignment != NULL)
            {
                memcpy(Data.FunctionAssignment, FunctionAssignment, sizeof(Data.FunctionAssignment));
            }

            m_LocStorage.LocDataSet(&Data, LocIndex);

            Result = true;
        }
    }
    else
    {
        switch (storeAction)
        {
        case storeAdd:
        case storeAddNoAutoSelect:
            /* Not present, add data. */
            if (m_NumberOfLocs < MaxNumberOfLocs)
            {
                /* Max number of locs not exceeded. */
                Data.Addres   = address;
                Data.Steps    = decoderStep28;
                Data.Dir      = directionForward;
                Data.Speed    = 0;
                Data.Function = 0;

                memset(Data.Name, '\0', sizeof(Data.Name));
                if (Name != NULL)
                {
                    memcpy(Data.Name, Name, sizeof(Data.Name) - 1);
                }

                memcpy(Data.FunctionAssignment, FunctionAssignment, sizeof(Data.FunctionAssignment));
                m_NumberOfLocs++;

                m_LocStorage.NumberOfLocsSet(m_NumberOfLocs);
                m_LocStorage.LocDataSet(&Data, m_NumberOfLocs - 1);

                /* Get newly added loc data. */
                if (storeAction == storeAdd)
                {
                    m_ActualSelectedLoc = m_NumberOfLocs - 1;
                    m_LocStorage.LocDataGet(&m_LocLibData, m_ActualSelectedLoc);
                }
                Result = true;
            }
            break;
        case storeChange: break;
        }
    }

    return (Result);
}

/***********************************************************************************************************************
 */
bool LocLib::RemoveLoc(uint16_t address)
{
    bool Result = false;
    LocLibData Data;
    uint8_t LocIndex;
    uint8_t Index;

    /* If at least two locs are present delete loc. */
    if (m_NumberOfLocs > 1)
    {
        LocIndex = CheckLoc(address);

        /* If loc is present delete it. */
        if (LocIndex != 255)
        {
            Index = LocIndex;
            /* Copy data next loc to this location so loc is removed. */
            while ((Index + 1) < m_NumberOfLocs)
            {
                m_LocStorage.LocDataGet(&Data, Index + 1);
                m_LocStorage.LocDataSet(&Data, Index);
                Index++;
            }

            m_NumberOfLocs--;
            m_LocStorage.NumberOfLocsSet(m_NumberOfLocs);

            Result = true;

            /* Load data for "next" loc... */
            if (LocIndex < m_NumberOfLocs)
            {
                m_LocStorage.LocDataGet(&m_LocLibData, LocIndex);
                m_ActualSelectedLoc = LocIndex;
            }
            else
            {
                /* Last item in list was deleted. */
                m_LocStorage.LocDataGet(&m_LocLibData, m_NumberOfLocs - 1);
                m_ActualSelectedLoc = m_NumberOfLocs - 1;
            }
        }
    }

    return (Result);
}

/***********************************************************************************************************************
 */
void LocLib::RemoveAllLocs(void) { m_NumberOfLocs = 1; }

/***********************************************************************************************************************
 */
uint16_t LocLib::GetNumberOfLocs(void) { return (m_NumberOfLocs); }

/***********************************************************************************************************************
 */
uint8_t LocLib::GetActualSelectedLocIndex(void) { return (m_ActualSelectedLoc + 1); }

/***********************************************************************************************************************
 */
void LocLib::LocBubbleSort(void)
{
    int i, j;
    LocLibData Data_1;
    LocLibData Data_2;
    LocLibData DataTemp;

    for (i = 0; i < (m_NumberOfLocs - 1); ++i)
    {
        for (j = 0; (j < m_NumberOfLocs - 1 - i); ++j)
        {
            m_LocStorage.LocDataGet(&Data_1, j);
            m_LocStorage.LocDataGet(&Data_2, (j + 1));

            if (Data_1.Addres > Data_2.Addres)
            {
                memcpy(&DataTemp, &Data_2, sizeof(LocLibData));
                memcpy(&Data_2, &Data_1, sizeof(LocLibData));
                memcpy(&Data_1, &DataTemp, sizeof(LocLibData));
                m_LocStorage.LocDataSet(&Data_1, j);
                m_LocStorage.LocDataSet(&Data_2, (j + 1));
            }
        }
    }
}

/***********************************************************************************************************************
 */
LocLibData* LocLib::LocGetAllDataByIndex(uint8_t Index)
{
    m_LocStorage.LocDataGet(&m_LocLibData, Index);
    return (&m_LocLibData);
}

/***********************************************************************************************************************
 */
uint16_t LocLib::SpeedIncrease(void)
{
    uint16_t Speed = m_LocLibData.Speed;

    if ((m_LocLibData.Speed >= 20) && (m_LocLibData.Steps == decoderStep128))
    {
        Speed += 2;
    }
    else
    {
        Speed++;
    }

    return (Speed);
}

/***********************************************************************************************************************
 */
uint16_t LocLib::SpeedDecrease(void)
{
    uint16_t Speed = m_LocLibData.Speed;

    if (Speed > 0)
    {
        if ((Speed > 20) && (m_LocLibData.Steps == decoderStep128))
        {
            Speed -= 2;
        }
        else if (m_LocLibData.Speed > 0)
        {
            Speed--;
        }
    }

    return (Speed);
}

/***********************************************************************************************************************
 */
uint16_t LocLib::SpeedStopOrChangeDirection(void)
{
    uint16_t Speed;
    if (m_LocLibData.Speed != 0)
    {
        Speed              = 0;
        m_LocLibData.Speed = 0;
    }
    else
    {
        DirectionToggle();
        Speed = m_LocLibData.Speed;
    }

    return (Speed);
}

/***********************************************************************************************************************
 * limit maximum loc addres.
 */
uint16_t LocLib::limitLocAddress(uint16_t locAddress)
{
    uint16_t locAdrresReturn = locAddress;
    if (locAdrresReturn > ADDRESS_LOC_MAX)
    {
        locAdrresReturn = ADDRESS_LOC_MIN;
    }
    else if (locAdrresReturn == 0)
    {
        locAdrresReturn = ADDRESS_LOC_MAX;
    }

    return (locAdrresReturn);
}

/***********************************************************************************************************************
 */
void LocLib::InitialLocStore(void)
{
    m_NumberOfLocs                     = 1;
    m_LocLibData.Addres                = 3;
    m_LocLibData.Steps                 = decoderStep28;
    m_LocLibData.Dir                   = directionForward;
    m_LocLibData.Speed                 = 0;
    m_ActualSelectedLoc                = 0;
    m_LocLibData.FunctionAssignment[0] = 0;
    m_LocLibData.FunctionAssignment[1] = 1;
    m_LocLibData.FunctionAssignment[2] = 2;
    m_LocLibData.FunctionAssignment[3] = 3;
    m_LocLibData.FunctionAssignment[4] = 4;
    memset(m_LocLibData.Name, '\0', sizeof(m_LocLibData.Name));

    m_LocStorage.LocDataSet(&m_LocLibData, 0);
    m_LocStorage.SelectedLocIndexStore(0);
    m_LocStorage.NumberOfLocsSet(1);
}
