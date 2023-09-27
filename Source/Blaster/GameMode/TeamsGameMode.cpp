// Fill out your copyright notice in the Description page of Project Settings.


#include "TeamsGameMode.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "../PlayerState/BlasterPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "../PlayerController/BlasterPlayerController.h"

ATeamsGameMode::ATeamsGameMode()
{
	bTeamMatch = true;
}

void ATeamsGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));

	if (BlasterGameState)
	{
		ABlasterPlayerState* BlasterPlayerState = NewPlayer->GetPlayerState<ABlasterPlayerState>();

		if (BlasterPlayerState && BlasterPlayerState->GetTeam() == ETeam::ET_NoTeam)
		{
			if (BlasterGameState->BlueTeam.Num() >= BlasterGameState->RedTeam.Num())
			{
				BlasterGameState->RedTeam.AddUnique(BlasterPlayerState);
				BlasterPlayerState->SetTeam(ETeam::ET_RedTeam);
			}
			else
			{
				BlasterGameState->BlueTeam.AddUnique(BlasterPlayerState);
				BlasterPlayerState->SetTeam(ETeam::ET_BlueTeam);
			}
		}
	}
}

void ATeamsGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	ABlasterPlayerState* BlasterPlayerState = Exiting->GetPlayerState<ABlasterPlayerState>();

	if (BlasterGameState && BlasterPlayerState)
	{
		if (BlasterGameState->RedTeam.Contains(BlasterPlayerState))
		{
			BlasterGameState->RedTeam.Remove(BlasterPlayerState);
		}
		if (BlasterGameState->BlueTeam.Contains(BlasterPlayerState))
		{
			BlasterGameState->BlueTeam.Remove(BlasterPlayerState);
		}
	}
}

float ATeamsGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	ABlasterPlayerState* AttackerPlayerState = Attacker->GetPlayerState<ABlasterPlayerState>();
	ABlasterPlayerState* VictimPlayerState = Victim->GetPlayerState<ABlasterPlayerState>();

	if (AttackerPlayerState == nullptr || VictimPlayerState == nullptr)
		return BaseDamage;

	if (VictimPlayerState == AttackerPlayerState)
	{
		return BaseDamage;
	}

	// 공격자와 피해자가 같은 팀이라면 대미지가 들어가면 안된다 0리턴
	if (AttackerPlayerState->GetTeam() == VictimPlayerState->GetTeam())
	{
		return 0.f;
	}

	return BaseDamage;
}

void ATeamsGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, 
	ABlasterPlayerController* AttackerController)
{
	Super::PlayerEliminated(ElimmedCharacter, VictimController, AttackerController);

	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));

	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;

	if (BlasterGameState && AttackerPlayerState)
	{
		if (AttackerPlayerState->GetTeam() == ETeam::ET_BlueTeam)
		{
			BlasterGameState->AddBlueTeamScores();
		}
		if (AttackerPlayerState->GetTeam() == ETeam::ET_RedTeam)
		{
			BlasterGameState->AddRedTeamScores();
		}
	}
}

void ATeamsGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	Super::RequestRespawn(ElimmedCharacter, ElimmedController);

	HandleTeamMatchStarted(ElimmedController);
}

void ATeamsGameMode::HandleTeamMatchStarted(AController* ElimmedController)
{
	ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(ElimmedController);

	if (PlayerController)
	{
		PlayerController->MulticastWeaponSelectOverlay();
	}
}

void ATeamsGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));

	if (BlasterGameState)
	{
		for (auto PlayerState : BlasterGameState->PlayerArray)
		{
			ABlasterPlayerState* BlasterPlayerState = Cast<ABlasterPlayerState>(PlayerState.Get());

			if (BlasterPlayerState && BlasterPlayerState->GetTeam() == ETeam::ET_NoTeam)
			{
				if (BlasterGameState->BlueTeam.Num() >= BlasterGameState->RedTeam.Num())
				{
					BlasterGameState->RedTeam.AddUnique(BlasterPlayerState);
					BlasterPlayerState->SetTeam(ETeam::ET_RedTeam);
				}
				else
				{
					BlasterGameState->BlueTeam.AddUnique(BlasterPlayerState);
					BlasterPlayerState->SetTeam(ETeam::ET_BlueTeam);
				}
			}
		}
	}
}
