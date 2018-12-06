/*
 * Copyright (c) 2018 Viktor Prutyanov
 *
 * This work is licensed under the terms of the GNU GPL, version 3.
 *
 * Based on source of UEFI:NTFS and efi-hello-world projects.
 *
 */

#include <efi.h>
#include <efilib.h>

//#define SEC_TO_USEC(value) ((value) * 1000 * 1000)

#define TargetPath L"\\EFI\\ubuntu\\grubx64.efi"

static void WaitAnyKey()
{
    EFI_INPUT_KEY Key;

    Print(L"Press any key...\n");

    WaitForSingleEvent(ST->ConIn->WaitForKey, 0);
    uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &Key);
}

static EFI_DEVICE_PATH* GetLastDevicePath(CONST EFI_DEVICE_PATH_PROTOCOL* dp)
{
    EFI_DEVICE_PATH *next, *p;

    if (IsDevicePathEnd(dp)) {
        return NULL;
    }

    for (p = (EFI_DEVICE_PATH *)dp, next = NextDevicePathNode(p);
            !IsDevicePathEnd(next);
            p = next, next = NextDevicePathNode(next));

    return p;
}

static EFI_DEVICE_PATH* GetParentDevice(CONST EFI_DEVICE_PATH* DevicePath)
{
    EFI_DEVICE_PATH *dp, *ldp;

    dp = DuplicateDevicePath((EFI_DEVICE_PATH *)DevicePath);
    if (dp == NULL) {
        return NULL;
    }

    ldp = GetLastDevicePath(dp);
    if (ldp == NULL) {
        return NULL;
    }

    ldp->Type = END_DEVICE_PATH_TYPE;
    ldp->SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;

    SetDevicePathNodeLength(ldp, sizeof(*ldp));

    return dp;
}

EFI_STATUS IdentifyBootDisk(EFI_HANDLE ImageHandle)
{
    EFI_STATUS Status;
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
    EFI_DEVICE_PATH *BootPartitionPath;
    EFI_DEVICE_PATH *BootDiskPath;
    EFI_DEVICE_PATH *TargetDevicePath;
    CHAR16 *DevicePathString;
    EFI_HANDLE TargetHandle;

    Status = uefi_call_wrapper(BS->OpenProtocol, 6, ImageHandle, &gEfiLoadedImageProtocolGuid,
            (VOID **)&LoadedImage, ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);

    if (EFI_ERROR(Status)) {
        Print(L"Unable to access boot image interface\n");

        return Status;
    }

    BootPartitionPath = DevicePathFromHandle(LoadedImage->DeviceHandle);
    BootDiskPath = GetParentDevice(BootPartitionPath);

    DevicePathString = DevicePathToStr(BootDiskPath);
    Print(L"Boot disk: %s\n", DevicePathString);

    FreePool(BootDiskPath);
    FreePool(DevicePathString);

    TargetDevicePath = FileDevicePath(LoadedImage->DeviceHandle, TargetPath);
    if (TargetDevicePath == NULL) {
        Status = EFI_DEVICE_ERROR;
        Print(L"Unable to set path for '%s'\n", TargetPath);

        return Status;
    }

    DevicePathString = DevicePathToStr(TargetDevicePath);
    Print(L"Target device path: %s\n", DevicePathString);
    FreePool(DevicePathString);

    Status = uefi_call_wrapper(BS->LoadImage, 6, FALSE, ImageHandle, TargetDevicePath,
            NULL, 0, &TargetHandle);
    FreePool(TargetDevicePath);
    if (EFI_ERROR(Status)) {
        Print(L"Unable to load '%s'\n", TargetPath);

        return Status;
    }

    Print(L"Target '%s' is ready to start!\n", TargetPath);
    WaitAnyKey();
    Status = uefi_call_wrapper(BS->StartImage, 3, TargetHandle, NULL, NULL);
    if (EFI_ERROR(Status)) {
        Print(L"Unable to start '%s'\n", TargetPath);

        return Status;
    }

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    InitializeLib(ImageHandle, SystemTable);

    uefi_call_wrapper(SystemTable->ConOut->ClearScreen, 1, SystemTable->ConOut);
    Print(L"Hello, world!\n");
    WaitAnyKey();

//    uefi_call_wrapper(BS->Stall, 1, SEC_TO_USEC(5));

    IdentifyBootDisk(ImageHandle);
    WaitAnyKey();

    return EFI_SUCCESS;
}
