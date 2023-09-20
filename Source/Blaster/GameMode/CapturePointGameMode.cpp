// Fill out your copyright notice in the Description page of Project Settings.


#include "CapturePointGameMode.h"

void ACapturePointGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, 
	ABlasterPlayerController* AttackerController)
{
	ABlasterGameMode::PlayerEliminated(ElimmedCharacter, VictimController, AttackerController);
}
