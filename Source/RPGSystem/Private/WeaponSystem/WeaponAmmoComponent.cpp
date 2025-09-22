// WeaponAmmoComponent.cpp

#include "WeaponSystem/WeaponAmmoComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

UWeaponAmmoComponent::UWeaponAmmoComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UWeaponAmmoComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWeaponAmmoComponent, CurrentMagAmmo);
	DOREPLIFETIME(UWeaponAmmoComponent, bChamberLoaded);
	DOREPLIFETIME(UWeaponAmmoComponent, ChambersLoaded);
}

void UWeaponAmmoComponent::InitializeFromItemData(URangedWeaponItemDataAsset* InBase)
{
	// This should be called on the server when the weapon is equipped/spawned.
	Base = InBase;
	CurrentMagAmmo = 0;
	bChamberLoaded = false;
	ChambersLoaded = 0;

	if (!Base) return;

	if (IsMagazineFed())
	{
		CurrentMagAmmo = FMath::Max(0, Base->MagazineSize);
	}

	if (UsesChambers() && Base->bAutoChamberOnReload)
	{
		if (Base->FeedSystem == EWeaponFeedSystem::SingleChamber)
		{
			if (CurrentMagAmmo > 0)
			{
				bChamberLoaded = true;
				--CurrentMagAmmo;
			}
		}
		else // Multi-chamber (with or without magazine)
		{
			while (ChambersLoaded < Base->NumChambers && CurrentMagAmmo > 0)
			{
				++ChambersLoaded;
				--CurrentMagAmmo;
			}
		}
	}
}

// ---------------- Fire ----------------
bool UWeaponAmmoComponent::ServerFireRound_Validate() { return true; }
void UWeaponAmmoComponent::ServerFireRound_Implementation()
{
	if (!Base) return;

	// CHAMBERED PATHS FIRST
	if (UsesChambers())
	{
		if (Base->FeedSystem == EWeaponFeedSystem::SingleChamber)
		{
			if (!bChamberLoaded) return;

			// Consume the chambered round
			bChamberLoaded = false;

			// Auto-cycle from mag if available
			if (IsMagazineFed() && Base->bAutoCycleOnFire && CurrentMagAmmo > 0)
			{
				bChamberLoaded = true;
				--CurrentMagAmmo;
			}
			return;
		}
		else // Multi-chamber (with or w/o magazine)
		{
			if (ChambersLoaded <= 0) return;

			// Consume one chamber
			--ChambersLoaded;

			// Auto-cycle from mag if available
			if (IsMagazineFed() && Base->bAutoCycleOnFire && CurrentMagAmmo > 0)
			{
				++ChambersLoaded;
				--CurrentMagAmmo;
				ChambersLoaded = FMath::Clamp(ChambersLoaded, 0, Base->NumChambers);
			}
			return;
		}
	}

	// PURE MAGAZINE PATH (no chambers)
	if (CurrentMagAmmo > 0)
	{
		--CurrentMagAmmo;
	}
}

// ---------------- Reload ----------------
bool UWeaponAmmoComponent::ServerReload_Validate(bool /*bTactical*/) { return true; }
void UWeaponAmmoComponent::ServerReload_Implementation(bool bTactical)
{
	if (!Base) return;

	// MAG-FED (includes MultiChamberMagazine)
	if (IsMagazineFed())
	{
		// Top off mag (wire to inventory later; here we assume infinite supply for simplicity)
		const int32 Space = FMath::Max(0, Base->MagazineSize - CurrentMagAmmo);
		CurrentMagAmmo += Space;

		const bool bKeepChamber = bTactical && Base->bSupportsTacticalReload;

		if (UsesChambers() && Base->bAutoChamberOnReload && !bKeepChamber)
		{
			if (Base->FeedSystem == EWeaponFeedSystem::SingleChamber)
			{
				if (!bChamberLoaded && CurrentMagAmmo > 0)
				{
					bChamberLoaded = true;
					--CurrentMagAmmo;
				}
			}
			else // Multi-chamber + mag
			{
				while (ChambersLoaded < Base->NumChambers && CurrentMagAmmo > 0)
				{
					++ChambersLoaded;
					--CurrentMagAmmo;
				}
			}
		}
		return;
	}

	// NON-MAG CHAMBERED (break-action, double barrel without mag)
	if (Base->FeedSystem == EWeaponFeedSystem::SingleChamber)
	{
		// Fill single chamber
		if (!bChamberLoaded)
		{
			bChamberLoaded = true;
		}
	}
	else if (Base->FeedSystem == EWeaponFeedSystem::MultiChamber)
	{
		// Fill all chambers
		ChambersLoaded = Base->NumChambers;
	}
}
