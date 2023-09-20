// Fill out your copyright notice in the Description page of Project Settings.


#include "CaptureTheFlagGameMode.h"
#include "../Weapon/Flag.h"
#include "../CaptureTheFlag/FlagZone.h"
#include "../GameState/BlasterGameState.h"

void ACaptureTheFlagGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController,
	ABlasterPlayerController* AttackerController)
{
	ABlasterGameMode::PlayerEliminated(ElimmedCharacter, VictimController, AttackerController);
}

void ACaptureTheFlagGameMode::FlagCaptured(AFlag* Flag, AFlagZone* Zone)
{
	bool bValidCapture = Flag->GetTeam() != Zone->GetTeam();
	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(GameState);
	if (BlasterGameState)
	{
		if (Zone->GetTeam() == ETeam::ET_BlueTeam)
		{
			BlasterGameState->AddBlueTeamScores(10.f);
		}
		if (Zone->GetTeam() == ETeam::ET_RedTeam)
		{
			BlasterGameState->AddRedTeamScores(10.f);
		}
	}
}
