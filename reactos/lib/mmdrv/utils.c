/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/mmdrv/utils.c
 * PURPOSE:              Multimedia User Mode Driver (utility functions)
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Jan 30, 2004: Imported into ReactOS tree
 */

#include "mmdrv.h"


typedef struct _DEVICE_LIST
{
    struct _DEVICE_LIST *Next;
    DWORD   DeviceType;
    ULONG   CardIndex;
    PVOID   DeviceInstanceData;
    ULONG   DeviceInstanceDataSize;
    WCHAR   Name[1];
} DEVICE_LIST, *PDEVICE_LIST;

PDEVICE_LIST DeviceList;


DWORD TranslateStatus(void)
{
    switch(GetLastError())
    {
        case NO_ERROR :
        case ERROR_IO_PENDING :
            return MMSYSERR_NOERROR;
            
        case ERROR_BUSY :
            return MMSYSERR_ALLOCATED;
            
        case ERROR_NOT_SUPPORTED :
        case ERROR_INVALID_FUNCTION :
            return MMSYSERR_NOTSUPPORTED;
            
        case ERROR_NOT_ENOUGH_MEMORY :
            return MMSYSERR_NOMEM;
            
        case ERROR_ACCESS_DENIED :
            return MMSYSERR_BADDEVICEID;
            
        case ERROR_INSUFFICIENT_BUFFER :
            return MMSYSERR_INVALPARAM;
            
        default :
            return MMSYSERR_ERROR;
    };
}



MMRESULT OpenDevice(UINT DeviceType, DWORD ID, PHANDLE pDeviceHandle,
                    DWORD Access)
{
    printf("OpenDevice()\n");
    WCHAR Name[SOUND_MAX_DEVICE_NAME];
    *pDeviceHandle = INVALID_HANDLE_VALUE;

    if (ID > SOUND_MAX_DEVICES)
        return MMSYSERR_BADDEVICEID;

    wsprintf(Name, L"\\\\.%ls%d",
             (DeviceType == WaveOutDevice ? WAVE_OUT_DEVICE_NAME_U :
              DeviceType == WaveInDevice ? WAVE_IN_DEVICE_NAME_U :
              DeviceType == MidiOutDevice ? MIDI_OUT_DEVICE_NAME_U :
              DeviceType == MidiInDevice ? MIDI_IN_DEVICE_NAME_U :
              AUX_DEVICE_NAME_U) + strlen("\\Device"), ID);
              
    printf("Attempting to open %S\n", Name);

    *pDeviceHandle = CreateFile(Name, Access, FILE_SHARE_WRITE, NULL,
                                OPEN_EXISTING, Access != GENERIC_READ ? FILE_FLAG_OVERLAPPED : 0,
                                NULL);

    printf("DeviceHandle == 0x%x\n", (int)*pDeviceHandle);

    return *pDeviceHandle != INVALID_HANDLE_VALUE ? MMSYSERR_NOERROR : TranslateStatus();
}


// DEVICE LIST MANAGEMENT


BOOL AddDeviceToList(PDEVICE_LIST* pList, DWORD DeviceType, DWORD CardIndex,
                     LPWSTR Name)
{
    PDEVICE_LIST pNewDevice;

    printf("AddDeviceToList()\n");

    pNewDevice = (PDEVICE_LIST) HeapAlloc(Heap, 0,
        sizeof(DEVICE_LIST) + lstrlen(Name) * sizeof(WCHAR));

    if ( !pNewDevice )
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    
    pNewDevice->DeviceType = DeviceType;
    pNewDevice->CardIndex = CardIndex;
    lstrcpy(pNewDevice->Name, Name);
    pNewDevice->DeviceInstanceData = NULL;
    pNewDevice->Next = *pList;
    *pList = pNewDevice;
    
    printf("Success!\n");

    return TRUE;
}


VOID FreeDeviceList()
{
    PDEVICE_LIST pDevice;

    printf("FreeDeviceList()\n");
    
    while (DeviceList)
    {
        pDevice = DeviceList;
        DeviceList = pDevice->Next;

        if (pDevice->DeviceInstanceData)
            HeapFree(Heap, 0, (LPVOID)pDevice->DeviceInstanceData);

        HeapFree(Heap, 0, (LPSTR)pDevice);
    }
}


MMRESULT FindDevices()
{
//    DWORD Index;
//    HKEY DriverKey;

    printf("Finding devices\n");
    
//    DriverKey = OpenParametersKey();
//  see drvutil.c of MS DDK for how this SHOULD be done...

    // FIXME: This is hard-coded for now, to use the hardware emulated by Bochs...

    // MIDI Out 0: MPU-401 UART
    AddDeviceToList(&DeviceList, MidiOutDevice, 0, L"MidiOut0");
    // Wave Out 0: Sound Blaster 16 (ok?)
    AddDeviceToList(&DeviceList, WaveOutDevice, 0, L"WaveOut0");

    return MMSYSERR_NOERROR; // ok?
}



DWORD GetDeviceCount(UINT DeviceType)
{
    int i;
    PDEVICE_LIST List;

    for (List = DeviceList, i = 0; List != NULL; List = List->Next)
        if (List->DeviceType == DeviceType)
            i ++;

    return i;
}
