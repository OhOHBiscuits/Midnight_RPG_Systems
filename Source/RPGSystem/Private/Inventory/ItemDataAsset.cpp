#include "Inventory/ItemDataAsset.h"

static const FItemAction GNullItemAction; // fallback

const FItemAction* UItemDataAsset::FindAction(const FGameplayTag& ActionTag) const
{
	if (!ActionTag.IsValid()) return nullptr;

	for (const FItemAction& A : ActionDefinitions)
	{
		if (A.ActionTag == ActionTag)
		{
			return &A;
		}
	}
	return nullptr;
}

bool UItemDataAsset::HasAction(const FGameplayTag& ActionTag) const
{
	if (FindAction(ActionTag)) return true;

	// Legacy compatibility: AllowedActions still counts as “has action”
	for (const FGameplayTag& T : AllowedActions)
	{
		if (T == ActionTag) return true;
	}
	return false;
}

const FItemAction& UItemDataAsset::GetActionOrDefault(const FGameplayTag& ActionTag, bool& bFound) const
{
	if (const FItemAction* A = FindAction(ActionTag))
	{
		bFound = true;
		return *A;
	}
	bFound = false;
	return GNullItemAction;
}

void UItemDataAsset::GetActionTags(TArray<FGameplayTag>& OutTags) const
{
	OutTags.Reset();

	// Prefer rich definitions if present
	if (ActionDefinitions.Num() > 0)
	{
		for (const FItemAction& A : ActionDefinitions)
		{
			if (A.ActionTag.IsValid())
			{
				OutTags.Add(A.ActionTag);
			}
		}
	}
	else
	{
		// Legacy path
		for (const FGameplayTag& T : AllowedActions)
		{
			if (T.IsValid())
			{
				OutTags.Add(T);
			}
		}
	}
}

