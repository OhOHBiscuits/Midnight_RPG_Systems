// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/RangedWeaponItemDataAsset.h"
#include "Sound/SoundBase.h"

USoundBase* URangedWeaponItemDataAsset::GetFireSoundSync() const
{
	return FireSound.IsNull() ? nullptr : FireSound.LoadSynchronous();
}