// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "../BlasterType/Team.h"
#include "TeamPlayerStart.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ATeamPlayerStart : public APlayerStart
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere)
	ETeam Team;

public:
	FORCEINLINE ETeam GetTeam() const { return Team; }
};
