// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "../Character/BlasterCharacter.h"
#include "../PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "../PlayerState/BlasterPlayerState.h"
#include "../GameState/BlasterGameState.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

ABlasterGameMode::ABlasterGameMode()
{
	// �Ʒ��� ������ true�� �����ϸ�
	// ���Ӹ���� ���´� ��ٸ��� ���·� ���� ������ ����Ʈ ���� ��� �÷��̾�� �����Ѵ�.
	// �׷��� �̰� �̿��� ������ ���ƴٴ� �� ����
	bDelayedStart = true;
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	// BlasterGameMode�� ���� �޴��� �ִ� ���� ���� ���� �ƴ� BlasterMap������ ���Ǳ� ������
	// ������ ������ ������ BlasterMap�� ������ ������� �󸶳� ���� �ð��� �ɷȴ��� �� �� �ִ�.
	// �� �ʿ� ������� �ɸ� �ð�
	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator iter = GetWorld()->GetPlayerControllerIterator(); iter; ++iter)
	{
		ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*iter);
		if (BlasterPlayer)
		{
			BlasterPlayer->OnMatchStateSet(MatchState);
		}
	}
}

void ABlasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController,
	ABlasterPlayerController* AttackerController)
{
	// ������ �÷��̾� ����
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	// ������ �÷��̾� ����
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;
	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();

	if (AttackerPlayerState == nullptr)  UE_LOG(LogTemp, Warning, TEXT("AttackerPlayerState ��ȿ���� ����"));
	if (AttackerPlayerState == VictimPlayerState)  UE_LOG(LogTemp, Warning, TEXT("AttackerPlayerState == VictimPlayerState"));

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && BlasterGameState)
	{
		AttackerPlayerState->AddToScore(1.f);
		BlasterGameState->UpdateTopScore(AttackerPlayerState);
	}

	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		// ������ ���� �ϰ� �����ؾߵȴ�.
		// ���� ���¸� �ʱ� ���·� �缳���ϱ� ���� ȣ��Ǵ°�
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
		//Cast<ABlasterCharacter>(ElimmedCharacter)->ServerElimDestroyed();
	}

	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		// �÷��̾� �����
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}
