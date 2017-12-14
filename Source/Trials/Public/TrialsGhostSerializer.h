#pragma once

#include "Trials.h"

#include "UTGhostData.h"
#include "UTGhostEvent.h"

const int32 GhostFileTypeTag = 0x67686f66; // "ghof"
const uint32 GhostFileVersion = 1;

class TRIALS_API GhostDataSerializer
{
public:
    static TArray<uint8> Serialize(UUTGhostData* Data)
    {
        TArray<uint8> ObjectBytes;
        FMemoryWriter MemoryWriter(ObjectBytes, true);

        int32 FileTypeTag = GhostFileTypeTag;
        MemoryWriter << FileTypeTag;

        uint32 FileVersion = GhostFileVersion;
        MemoryWriter << FileVersion;

        int32 PackageFileUE4Version = GPackageFileUE4Version;
        MemoryWriter << PackageFileUE4Version;

        FEngineVersion SavedEngineVersion = FEngineVersion::Current();
        MemoryWriter << SavedEngineVersion;

        FTransform StartTransform = Data->StartTransform;
        MemoryWriter << StartTransform;

        int32 Count = Data->Events.Num();
        MemoryWriter << Count;

        for (auto& Event : Data->Events)
        {
            FString EventClass = Event->GetClass()->GetName();
            MemoryWriter << EventClass;

            if (Event->IsA<UUTGhostEvent_Weapon>())
            {
                //TArray<uint8> EventProperties;
                //FObjectWriter Ar(Event, EventProperties, true, true, true, PPF_Duplicate | PPF_UseDeprecatedProperties);

                //MemoryWriter << EventProperties;

                MemoryWriter << Event->Time;

                FString WeaponClassName = Cast<UUTGhostEvent_Weapon>(Event)->WeaponClass->GetName();
                MemoryWriter << WeaponClassName;
            }
            else if (Event->IsA<UUTGhostEvent_JumpBoots>())
            {
                MemoryWriter << Event->Time;
            }
            else
            {
                Event->Serialize(MemoryWriter);
            }
        }

        return ObjectBytes;
    }

    static UUTGhostData* Serialize(TArray<uint8> ObjectBytes)
    {
        FMemoryReader MemoryReader(ObjectBytes, true);

        int32 FileTypeTag;
        MemoryReader << FileTypeTag;

        if (FileTypeTag != GhostFileTypeTag)
        {
            return nullptr; // error?
        }

        uint32 FileVersion;
        MemoryReader << FileVersion;

        int32 PackageFileUE4Version;
        MemoryReader << PackageFileUE4Version;

        FEngineVersion SavedEngineVersion;
        MemoryReader << SavedEngineVersion;

        MemoryReader.SetUE4Ver(PackageFileUE4Version);
        MemoryReader.SetEngineVer(SavedEngineVersion);

        UUTGhostData* OutData = NewObject<UUTGhostData>(GetTransientPackage(), UUTGhostData::StaticClass());

        FTransform StartTransform;
        MemoryReader << StartTransform;
        OutData->StartTransform = StartTransform;

        int32 Count;
        MemoryReader << Count;

        TArray<class UUTGhostEvent*> Events;
        for (int32 i = 0; i < Count; ++i)
        {
            FString EventClassName;
            MemoryReader << EventClassName;

            UClass* EventClass = FindObject<UClass>(ANY_PACKAGE, *EventClassName);

            UUTGhostEvent* Event = NewObject<UUTGhostEvent>(OutData, EventClass);
            if (Event->IsA<UUTGhostEvent_Weapon>())
            {
                //TArray<uint8> EventProperties;
                //MemoryReader Ar(Event, EventProperties, true, true);

                //MemoryReader << EventProperties;

                MemoryReader << Event->Time;

                FString WeaponClassName;
                MemoryReader << WeaponClassName;

                UClass* WeaponClass = FindObject<UClass>(ANY_PACKAGE, *WeaponClassName);
                Cast<UUTGhostEvent_Weapon>(Event)->WeaponClass = WeaponClass;

            }
            else if (Event->IsA<UUTGhostEvent_JumpBoots>())
            {
                MemoryReader << Event->Time;
            }
            else
            {
                Event->Serialize(MemoryReader);
            }
            Events.Add(Event);
        }

        OutData->Events = Events;
        return OutData;
    }
};
