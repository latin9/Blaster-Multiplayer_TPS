// Fill out your copyright notice in the Description page of Project Settings.


#include "CapturePointGameMode.h"
#include "../GameState/BlasterGameState.h"
#include "../PlayerController/BlasterPlayerController.h"

void ACapturePointGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	BlasterGameState = BlasterGameState == nullptr ? GetGameState<ABlasterGameState>() : BlasterGameState;

	if (BlasterGameState)
	{
		if (static_cast<int>(BlasterGameState->RedTeamScore) == 100 ||
			static_cast<int>(BlasterGameState->BlueTeamScore) == 100)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
}

void ACapturePointGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
}

void ACapturePointGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController,
	ABlasterPlayerController* AttackerController)
{
	ABlasterGameMode::PlayerEliminated(ElimmedCharacter, VictimController, AttackerController);
}

void ACapturePointGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	Super::RequestRespawn(ElimmedCharacter, ElimmedController);
}

void ACapturePointGameMode::PointCaptured(float BlueTeamCount, float RedTeamCount)
{
	BlasterGameState = BlasterGameState == nullptr ? GetGameState<ABlasterGameState>() : BlasterGameState;
	if (BlasterGameState)
	{
		float BlueTeamAmmount = BlueTeamCount - RedTeamCount;
		float RedTeamAmmount = RedTeamCount - BlueTeamCount;

		// 블루팀이 점령중인 사람이 더 많다는 의미
		if (BlueTeamAmmount > 0.f)
		{
			float AmmountToBlueScore = BlueTeamAmmount;

			if (BlasterGameState->RedTeamScore > 0.f)
			{
				if (BlasterGameState->RedTeamScore >= BlueTeamAmmount)
				{
					BlasterGameState->AddRedTeamScores(-BlueTeamAmmount);
					AmmountToBlueScore = 0.f;
				}
				else
				{
					AmmountToBlueScore = FMath::Clamp(AmmountToBlueScore - BlasterGameState->RedTeamScore, 0.f, BlueTeamAmmount);
					BlasterGameState->AddRedTeamScores(-BlasterGameState->RedTeamScore);
				}
			}
			BlasterGameState->AddBlueTeamScores(AmmountToBlueScore);
		}
		if(RedTeamAmmount > 0.f)
		{
			float AmmountToRedScore = RedTeamAmmount;

			if (BlasterGameState->BlueTeamScore > 0.f)
			{
				if (BlasterGameState->BlueTeamScore >= RedTeamAmmount)
				{
					BlasterGameState->AddBlueTeamScores(-RedTeamAmmount);
					AmmountToRedScore = 0.f;
				}
				else
				{
					AmmountToRedScore = FMath::Clamp(AmmountToRedScore - BlasterGameState->BlueTeamScore, 0.f, RedTeamAmmount);
					BlasterGameState->AddBlueTeamScores(-BlasterGameState->BlueTeamScore);
				}
			}
			BlasterGameState->AddRedTeamScores(AmmountToRedScore);
		}
	}
}
