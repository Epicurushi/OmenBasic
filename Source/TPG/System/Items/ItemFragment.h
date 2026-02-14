#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemFragment.generated.h"

class UItemDefinition;

/**
 * Item Fragment£ºÄÜÁ¦Ä£¿é
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class TPG_API UItemFragment : public UObject
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(const UItemDefinition* ItemDef) {}
};