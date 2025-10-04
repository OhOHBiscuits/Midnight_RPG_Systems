#include "Inventory/WeaponStatComputeLibrary.h"

static void ApplyOneMod(FRangedComputedStats& S, const FRangedWeaponStatMod& M)
{
	auto ApplyFloat = [&](float& Target)
	{
		switch (M.Op)
		{
			case EStatModOp::Set: Target = M.Value; break;
			case EStatModOp::Add: Target += M.Value; break;
			case EStatModOp::Mul: Target *= (M.Value); break;
		}
	};

	switch (M.Stat)
	{
		case ERangedWeaponStat::MagazineCapacity:
		{
			float Tmp = static_cast<float>(S.MagazineCapacity);
			if (M.Op == EStatModOp::Set)      Tmp = M.Value;
			else if (M.Op == EStatModOp::Add) Tmp += M.Value;
			else if (M.Op == EStatModOp::Mul) Tmp *= M.Value;
			S.MagazineCapacity = FMath::Max(0, FMath::RoundToInt(Tmp));
			break;
		}
		case ERangedWeaponStat::FireRateRPM:     ApplyFloat(S.FireRateRPM);     break;
		case ERangedWeaponStat::AccuracyDeg:     ApplyFloat(S.AccuracyDeg);     break;
		case ERangedWeaponStat::SpreadDeg:       ApplyFloat(S.SpreadDeg);       break;
		case ERangedWeaponStat::RecoilKick:      ApplyFloat(S.RecoilKick);      break;
		case ERangedWeaponStat::ReloadSeconds:   ApplyFloat(S.ReloadSeconds);   break;
		case ERangedWeaponStat::Handling:        ApplyFloat(S.Handling);        break;
		case ERangedWeaponStat::EffectiveRange:  ApplyFloat(S.EffectiveRange);  break;
		case ERangedWeaponStat::MuzzleVelocity:  ApplyFloat(S.MuzzleVelocity);  break;
		case ERangedWeaponStat::BulletDropScale: ApplyFloat(S.BulletDropScale); break;
		case ERangedWeaponStat::Loudness:        ApplyFloat(S.Loudness);        break;
	}
}

FRangedComputedStats UWeaponStatComputeLibrary::ComputeRangedStats(
	const URangedWeaponItemDataAsset* Base,
	const TArray<UWeaponModItemDataAsset*>& EquippedMods)
{
	FRangedComputedStats Out;

	if (Base)
	{
		Out.MagazineCapacity = FMath::Max(0, Base->MagazineSize);
		Out.FireRateRPM      = Base->FireRateRPM;
		Out.AccuracyDeg      = Base->AccuracyDeg;
		Out.SpreadDeg        = Base->SpreadDeg;
		Out.RecoilKick       = Base->RecoilKick;
		Out.ReloadSeconds    = Base->ReloadSeconds;
		Out.Handling         = Base->Handling;
		Out.EffectiveRange   = Base->EffectiveRange;
		Out.MuzzleVelocity   = Base->MuzzleVelocity;
		Out.BulletDropScale  = Base->BulletDropScale;
		Out.Loudness         = Base->Loudness;
	}

	// Apply mods in order; last Set wins naturally.
	for (const UWeaponModItemDataAsset* Mod : EquippedMods)
	{
		if (!Mod) continue;
		for (const FRangedWeaponStatMod& M : Mod->RangedStatMods)
		{
			ApplyOneMod(Out, M);
		}
	}

	return Out;
}

int32 UWeaponStatComputeLibrary::GetEffectiveMagazineCapacity(
	const URangedWeaponItemDataAsset* Base,
	const TArray<UWeaponModItemDataAsset*>& EquippedMods)
{
	return ComputeRangedStats(Base, EquippedMods).MagazineCapacity;
}
