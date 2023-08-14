// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "../Character/BlasterCharacter.h"
#include "../PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "../PlayerState/BlasterPlayerState.h"

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, 
	ABlasterPlayerController* AttackerController)
{
	// ������ �÷��̾� ����
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	// ������ �÷��̾� ����
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;
	if (AttackerPlayerState == nullptr)  UE_LOG(LogTemp, Warning, TEXT("AttackerPlayerState ��ȿ���� ����"));
	if (AttackerPlayerState == VictimPlayerState)  UE_LOG(LogTemp, Warning, TEXT("AttackerPlayerState == VictimPlayerState"));

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.f);
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
