// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"
#include "../HUD/BlasterHUD.h"
#include "../HUD/CharacterOverlay.h"
#include "Components/Progressbar.h"
#include "Components/TextBlock.h"
#include "../Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "../GameMode/BlasterGameMode.h"
#include "../HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "../Component/CombatComponent.h"
#include "../GameState/BlasterGameState.h"
#include "../PlayerState/BlasterPlayerState.h"
#include "Components/Image.h"
#include "../HUD/ReturnToMainMenu.h"
#include "../HUD/ChatOverlay.h"
#include "Components/EditableTextBox.h"
#include "../BlasterType/Announcement.h"

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = GetHUD<ABlasterHUD>();
	
	ServerCheckMatchState();
}

void ABlasterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent == nullptr)
		return;

	InputComponent->BindAction(TEXT("Quit"), IE_Pressed, this, &ABlasterPlayerController::ShowReturnToMainMenu);
	InputComponent->BindAction(TEXT("UIMode"), IE_Pressed, this, &ABlasterPlayerController::ChangeUIMode);
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, MatchState);
	DOREPLIFETIME(ABlasterPlayerController, bShowTeamScores);
}
void ABlasterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();
	CheckTimeSync(DeltaTime);
	PollInit();

	CheckPing(DeltaTime);
}

void ABlasterPlayerController::CheckPing(float DeltaTime)
{
	HighPingRunningTime += DeltaTime;

	if (HighPingRunningTime >= CheckPingFrequency)
	{
		PlayerState = PlayerState == nullptr ? GetPlayerState<APlayerState>() : PlayerState;

		if (PlayerState)
		{
			//UE_LOG(LogTemp, Warning, TEXT("PlayerState->GetCompressedPing() * 4 : %d"), PlayerState->GetCompressedPing() * 4);
			// �𸮾� ������ GetCompressedPing�Լ��� ���� ����ϰ� �� ���� 4�� ������ uint8�� �����Ѵ�.
			// �׷��� ������ ���� �������� 4�� ���ؾ��Ѵ�.
			// �� GetPingInMilliseconds�Լ��� �� ����ü ������ ����´�.
			// �Ӱ�ġ�� �κ��� ���ٸ� SSR�� ����Ұ��̰� �׺��� �۴ٸ� SSR�� ������� �ʴ´�.
			if (PlayerState->GetCompressedPing() * 4 > HightPingThreshold)
			{
				HighPingWarning();
				PingAnimationRunningTime = 0.f;
				ServerReportPingStatus(true);
			}
			else
			{
				ServerReportPingStatus(false);
			}
		}
		HighPingRunningTime = 0.f;
	}

	bool bIsHighPingAnimationPlaying = BlasterHUD && BlasterHUD->GetCharacterOverlay()
		&& BlasterHUD->GetCharacterOverlay()->HighPingAnimation
		&& BlasterHUD->GetCharacterOverlay()->IsAnimationPlaying(BlasterHUD->GetCharacterOverlay()->HighPingAnimation);

	if (bIsHighPingAnimationPlaying)
	{
		PingAnimationRunningTime += DeltaTime;

		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}

void ABlasterPlayerController::ShowReturnToMainMenu()
{
	if (ReturnToMainMenuWidget == nullptr)
		return;

	if (ReturnToMainMenu == nullptr)
	{
		ReturnToMainMenu = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuWidget);
	}
	if (ReturnToMainMenu)
	{
		bReturnToMainMenuOpen = !bReturnToMainMenuOpen;
		if (bReturnToMainMenuOpen)
		{
			ReturnToMainMenu->MenuSetup();
		}
		else
		{
			ReturnToMainMenu->MenuTearDown();
		}
	}
}

void ABlasterPlayerController::ChangeUIMode()
{
	bUIOnlyModeEnable = !bUIOnlyModeEnable;
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValide = BlasterHUD && BlasterHUD->GetChatOverlay() && 
		BlasterHUD->GetChatOverlay()->GetChatInputText();

	if (!bHUDValide)
		return;

	if (bUIOnlyModeEnable)
	{
		SetInputMode(FInputModeGameAndUI());
		SetShowMouseCursor(true);
		BlasterHUD->GetChatOverlay()->SetVisibility(ESlateVisibility::Visible);
		BlasterHUD->GetChatOverlay()->GetChatInputText()->SetFocus();
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		BlasterHUD->GetChatOverlay()->SetVisibility(ESlateVisibility::Hidden);
		FSlateApplication::Get().ClearAllUserFocus();
	}
}

void ABlasterPlayerController::OnRep_ShowTeamScores()
{
	if (bShowTeamScores)
	{
		InitTeamScores();
		HideHUDScore();
	}
	else
	{
		HideTeamScores();
	}
}


// ���� ���ٸ�?
void ABlasterPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}

void ABlasterPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	// 5�ʸ��� ������ Ŭ�� ��Ÿ ����� �� �ֵ��� ����
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void ABlasterPlayerController::HighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD && BlasterHUD->GetCharacterOverlay()
		&& BlasterHUD->GetCharacterOverlay()->HighPingImage
		&& BlasterHUD->GetCharacterOverlay()->HighPingAnimation;
	if (bHUDValid)
	{
		BlasterHUD->GetCharacterOverlay()->HighPingImage->SetOpacity(1.f);
		BlasterHUD->GetCharacterOverlay()->PlayAnimation(BlasterHUD->GetCharacterOverlay()->HighPingAnimation, 0.f, 5);
	}
}

void ABlasterPlayerController::StopHighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD && BlasterHUD->GetCharacterOverlay()
		&& BlasterHUD->GetCharacterOverlay()->HighPingImage
		&& BlasterHUD->GetCharacterOverlay()->HighPingAnimation;
	if (bHUDValid)
	{
		BlasterHUD->GetCharacterOverlay()->HighPingImage->SetOpacity(0.f);
		if (BlasterHUD->GetCharacterOverlay()->IsAnimationPlaying(BlasterHUD->GetCharacterOverlay()->HighPingAnimation))
		{
			BlasterHUD->GetCharacterOverlay()->StopAnimation(BlasterHUD->GetCharacterOverlay()->HighPingAnimation);
		}
	}
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	// Ŭ���̾�Ʈ���� ������ ȣ���� �ϰ� �������� ����Ǵ°��̱� ������ ���Ӹ�带 ���� �� �ִ�.
	ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		// �÷��̾� ��Ʈ�ѷ����� ���Ӹ�忡�� ������ �� ��ġ ������ ���ӽð��� �˷��ִ°��̴�.
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();

		// �ش� �������� �ٽ� Ŭ���̾�Ʈ�� �����ؾߵȴ�.
		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);

		/*if (BlasterHUD && MatchState == MatchState::WaitingToStart)
		{
			BlasterHUD->AddAnnouncement();
		}*/
	}
}
void ABlasterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	// ������ �����ϰ� �����ؾ��ϱ� ������ ������ ������ ���°�
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	// ��ġ���� ����
	OnMatchStateSet(MatchState);

	if (BlasterHUD && MatchState == MatchState::WaitingToStart)
	{
		BlasterHUD->AddAnnouncement();
	}
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);

	if (BlasterCharacter)
	{
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
		SetHUDShield(BlasterCharacter->GetShield(), BlasterCharacter->GetMaxShield());
	}
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD && BlasterHUD->GetCharacterOverlay()
		&& BlasterHUD->GetCharacterOverlay()->HealthBar
		&& BlasterHUD->GetCharacterOverlay()->HealthText;
	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		BlasterHUD->GetCharacterOverlay()->HealthBar->SetPercent(HealthPercent);
		// ������ �ݿø��ؾ���
		FString HealthText = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		BlasterHUD->GetCharacterOverlay()->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ABlasterPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD && BlasterHUD->GetCharacterOverlay()
		&& BlasterHUD->GetCharacterOverlay()->ShieldBar
		&& BlasterHUD->GetCharacterOverlay()->ShieldText;
	if (bHUDValid)
	{
		const float ShieldPercent = Shield / MaxShield;
		BlasterHUD->GetCharacterOverlay()->ShieldBar->SetPercent(ShieldPercent);
		// ������ �ݿø��ؾ���
		FString ShieldText = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		BlasterHUD->GetCharacterOverlay()->ShieldText->SetText(FText::FromString(ShieldText));
	}
	else
	{
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
	}
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
	UE_LOG(LogTemp, Error, TEXT("SetHUDSCore Start"));
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() && 
		BlasterHUD->GetCharacterOverlay()->ScoreAmount;
	if (bHUDValid)
	{
		FString ScoreAmount = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		BlasterHUD->GetCharacterOverlay()->ScoreAmount->SetText(FText::FromString(ScoreAmount));
	}
	else
	{
		bInitializeScore = true;
		HUDScore = Score;
	}
}

void ABlasterPlayerController::HideHUDScore()
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->ScoreAmount &&
		BlasterHUD->GetCharacterOverlay()->DefeatsAmount &&
		BlasterHUD->GetCharacterOverlay()->ScoreText &&
		BlasterHUD->GetCharacterOverlay()->DefeatsText;
	if (bHUDValid)
	{
		BlasterHUD->GetCharacterOverlay()->ScoreAmount->SetText(FText());
		BlasterHUD->GetCharacterOverlay()->DefeatsAmount->SetText(FText());
		BlasterHUD->GetCharacterOverlay()->ScoreText->SetText(FText());
		BlasterHUD->GetCharacterOverlay()->DefeatsText->SetText(FText());
	} 
	else
	{
		bHideHUDScore = true;
	}
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->DefeatsAmount;
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		BlasterHUD->GetCharacterOverlay()->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeDefeats = true;
		HUDDefeats = Defeats;
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->WeaponAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->GetCharacterOverlay()->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->CarriedAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->GetCharacterOverlay()->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->MatchCountdownText;
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			BlasterHUD->GetCharacterOverlay()->MatchCountdownText->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		FString CountdownText = FString::Printf(TEXT("%02d : %02d"), Minutes, Seconds);
		BlasterHUD->GetCharacterOverlay()->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetAnnouncement() &&
		BlasterHUD->GetAnnouncement()->WarmupTime;
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			BlasterHUD->GetAnnouncement()->WarmupTime->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		FString CountdownText = FString::Printf(TEXT("%02d : %02d"), Minutes, Seconds);
		BlasterHUD->GetAnnouncement()->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetHUDGrenades(int32 Grenades)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GrenadeText;
	if (bHUDValid)
	{
		FString GrenadeText = FString::Printf(TEXT("%d"), Grenades);
		BlasterHUD->GetCharacterOverlay()->GrenadeText->SetText(FText::FromString(GrenadeText));
	}
	else
	{
		bInitializeGrenades = true;
		HUDGrenades = Grenades;
	}
}

void ABlasterPlayerController::BroadcastElim(APlayerState* Attacker, APlayerState* Victim)
{
	//ClientElimAnnouncement(Attacker, Victim);
	if (Attacker && Victim)
	{
		BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

		if (BlasterHUD)
		{
			BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());
		}
	}
}

void ABlasterPlayerController::SendMessage(const FText& Message)
{
	// �޼��� �տ� �÷��̾� �̸��� �־���Ѵ�.

	PlayerState = PlayerState == nullptr ? GetPlayerState<APlayerState>() : PlayerState;

	if (PlayerState)
	{
		FString UserName = PlayerState->GetPlayerName();
		FString NewMessage = FString::Printf(TEXT("%s : %s"), *UserName, *Message.ToString());

		ServerSendChatMessage(NewMessage);
	}
}

void ABlasterPlayerController::HideTeamScores()
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;
	bool bHUDValid = BlasterHUD && BlasterHUD->GetCharacterOverlay() && 
		BlasterHUD->GetCharacterOverlay()->RedTeamScore &&
		BlasterHUD->GetCharacterOverlay()->BlueTeamScore &&
		BlasterHUD->GetCharacterOverlay()->ScoreSpacerText;

	if (bHUDValid)
	{
		BlasterHUD->GetCharacterOverlay()->RedTeamScore->SetText(FText());
		BlasterHUD->GetCharacterOverlay()->BlueTeamScore->SetText(FText());
		BlasterHUD->GetCharacterOverlay()->ScoreSpacerText->SetText(FText());
	}
}

void ABlasterPlayerController::InitTeamScores()
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;
	bool bHUDValid = BlasterHUD && BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->RedTeamScore &&
		BlasterHUD->GetCharacterOverlay()->BlueTeamScore &&
		BlasterHUD->GetCharacterOverlay()->ScoreSpacerText;

	if (bHUDValid)
	{
		FString Zero("0");
		FString Spacer(":");
		BlasterHUD->GetCharacterOverlay()->RedTeamScore->SetText(FText::FromString(Zero));
		BlasterHUD->GetCharacterOverlay()->BlueTeamScore->SetText(FText::FromString(Zero));
		BlasterHUD->GetCharacterOverlay()->ScoreSpacerText->SetText(FText::FromString(Spacer));
	}
}

void ABlasterPlayerController::SetHUDRedTeamScore(int32 RedScore)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;
	bool bHUDValid = BlasterHUD && BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->RedTeamScore;

	if (bHUDValid)
	{
		FString Score = FString::Printf(TEXT("%d"), RedScore);
		BlasterHUD->GetCharacterOverlay()->RedTeamScore->SetText(FText::FromString(Score));
	}
}

void ABlasterPlayerController::SetHUDBlueTeamScore(int32 BlueScore)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;
	bool bHUDValid = BlasterHUD && BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->BlueTeamScore;

	if (bHUDValid)
	{
		FString Score = FString::Printf(TEXT("%d"), BlueScore);
		BlasterHUD->GetCharacterOverlay()->BlueTeamScore->SetText(FText::FromString(Score));
	}
}

void ABlasterPlayerController::SwitchViewToOtherPlayer()
{
	BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;

	if (BlasterGameMode)
	{
		//// �÷��̾� ��Ʈ�ѷ��� ������ �� Ÿ���� ������ �÷��̾� �ε����� ����ϴ�.
		//int32 NewPlayerIndex = BlasterGameMode->GetNextPlayerToView();

		//// �ش� �ε����� �ش��ϴ� �÷��̾� ��Ʈ�ѷ��� �����ɴϴ�.
		//ABlasterPlayerController* NewPlayerController = BlasterGameMode->GetPlayerControllerByIndex(NewPlayerIndex);

		//if (NewPlayerController)
		//{
		//	// ���� �÷��̾� ��Ʈ�ѷ��� ������ �� Ÿ���� �����մϴ�.
		//	SetViewTargetWithBlend(NewPlayerController, YourBlendTime, YourBlendFunc, YourBlendExp);
		//}
	}
}

void ABlasterPlayerController::ServerSendChatMessage_Implementation(const FString& Message)
{
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetPawn()->GetWorld(), APlayerController::StaticClass(), OutActors);

	// �������� ��� Ŭ���̾�Ʈ���� �����Ѵ�.
	for (auto Actor : OutActors)
	{
		ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(Actor);

		if (PlayerController)
		{
			PlayerController->ClientSendChatMessage(Message);
		}
	}
}

void ABlasterPlayerController::ClientSendChatMessage_Implementation(const FString& Message)
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

	if (BlasterHUD && BlasterHUD->GetChatOverlay())
	{
		BlasterHUD->GetChatOverlay()->AddChatText(Message);
	}
}

void ABlasterPlayerController::ClientElimAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim)
{
	if (Attacker && Victim)
	{
		BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;

		if (BlasterHUD)
		{
			BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());
		}
	}
}

void ABlasterPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart)
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;

	else if (MatchState == MatchState::InProgress)
		TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;

	else if (MatchState == MatchState::Cooldown)
		TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;

	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if (HasAuthority())
	{
		BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;

		if (BlasterGameMode)
		{
			LevelStartingTime = BlasterGameMode->LevelStartingTime;
			SecondsLeft = FMath::CeilToInt(BlasterGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}

	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}

	CountdownInt = SecondsLeft;
}

void ABlasterPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (BlasterHUD && BlasterHUD->GetCharacterOverlay())
		{
			CharacterOverlay = BlasterHUD->GetCharacterOverlay();

			if (CharacterOverlay)
			{
				if (bInitializeHealth)
					SetHUDHealth(HUDHealth, HUDMaxHealth);
				if (bInitializeShield)
					SetHUDShield(HUDShield, HUDMaxShield);
				if (bInitializeScore)
					SetHUDScore(HUDScore);
				if (bInitializeDefeats)
					SetHUDDefeats(HUDDefeats);
				if (bInitializeCarriedAmmo)
					SetHUDCarriedAmmo(HUDCarriedAmmo);
				if (bInitializeWeaponAmmo)
					SetHUDWeaponAmmo(HUDWeaponAmmo);
				/*if (bHideHUDScore)
					HideHUDScore();*/
				ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
				if (BlasterCharacter && BlasterCharacter->GetCombatComponent())
				{
					if (bInitializeGrenades)
						SetHUDGrenades(BlasterCharacter->GetCombatComponent()->GetGrenades());
				}
			}
		}
	}
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	// ������ �ܼ��� ��û�� ������ ���� Ŭ���̾�Ʈ RPC�� ȣ���Ѵ�
	// �̶� ��û �ð��� �Բ��� �ڽ��� �ð��� �ǵ��� ������.
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	// �պ��ð�
	// Ŭ���̾�Ʈ�� ������ RPC�� ���� �� �󸶳� ���� �ð��� �귶���� �� �� �ִ�.
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	// ���� �ð� �պ��� ����
	SingleTripTime = 0.5f * RoundTripTime;
	// Ŭ���̾�Ʈ���� ������ ���� �ð��� ���
	// ������ �ð��� �� RPC / 2��ŭ�� �ð��� ���� �ð��� �����ָ� �ȴ�.
	float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float ABlasterPlayerController::GetServerTime()
{
	//������� �׳� ���� �ð��� �����ϰ�
	if (HasAuthority())
		return GetWorld()->GetTimeSeconds();
	else
		// ������ �ƴ϶�� ���� �ð����� Ŭ�� �������̰��� ���Ѵ�.
		return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ABlasterPlayerController::OnMatchStateSet(FName State, bool bTeamMatch)
{
	MatchState = State;

	// ��ġ ���°� InProgress(�ΰ��� ����)�� �Ǿ��ٸ� �׶� �������̸� �����Ѵ�.
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted(bTeamMatch);
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::OnRep_MatchState()
{
	// ��ġ ���°� InProgress(�ΰ��� ����)�� �Ǿ��ٸ� �׶� �������̸� �����Ѵ�.
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}



void ABlasterPlayerController::HandleMatchHasStarted(bool bTeamMatch)
{
	// HandleMatchHasStarted�� ���������� ������ �ȴ� �׷���
	// bShowTeamScores��� ������ �����Ͽ� OnRep_ShowTeamScores �Լ����̿��Ͽ�
	// Ŭ�󿡼��� ���ھ� �ؽ�Ʈ�� ������ �Ǵ��� �� �� �ֵ��� ���⼭ �Ű������� ���� ���� �־��ִ°�
	if (HasAuthority())
		bShowTeamScores = bTeamMatch;

	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;
	if (BlasterHUD)
	{
		if (BlasterHUD->GetCharacterOverlay() == nullptr)
			BlasterHUD->AddCharacterOverlay();

		if (BlasterHUD->GetChatOverlay() == nullptr)
		{
			BlasterHUD->AddChatOverlay();
			BlasterHUD->GetChatOverlay()->SetVisibility(ESlateVisibility::Hidden);
		}

		SetInputMode(FInputModeGameOnly());

		// ��ġ�� ���۵Ǹ� ������ �ִ� Announcement Widget�� �Ⱥ��̰� ����
		if (BlasterHUD->GetAnnouncement())
		{
			BlasterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Hidden);
		}

		if (!HasAuthority())
			return;

		if (bTeamMatch)
		{
			InitTeamScores();
			//HideHUDScore();
		}
		else
		{
			HideTeamScores();
		}
	}
}

void ABlasterPlayerController::HandleCooldown()
{
	BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;
	if (BlasterHUD)
	{
		BlasterHUD->GetCharacterOverlay()->RemoveFromParent();
		bool bHUDValid = BlasterHUD->GetAnnouncement() &&
			BlasterHUD->GetAnnouncement()->AnnouncementText &&
			BlasterHUD->GetAnnouncement()->InfoText;
		// ��ġ�� ���۵Ǹ� ������ �ִ� Announcement Widget�� �Ⱥ��̰� ����
		if (bHUDValid)
		{
			BlasterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText = Announcement::NewMatchStartsIn;
			BlasterHUD->GetAnnouncement()->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
			if (BlasterGameState && BlasterPlayerState)
			{
				TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;
				
				//FString InfoTextString = GetInfoText(TopPlayers, BlasterPlayerState);
				FString InfoTextString = bShowTeamScores ? GetTeamsInfoText(BlasterGameState) : GetInfoText(TopPlayers, BlasterPlayerState);

				BlasterHUD->GetAnnouncement()->InfoText->SetText(FText::FromString(InfoTextString));
			}
		}
	}
	// ���⼭ bDisableGameplay�� �����ϴ� ������
	// �ù� ���Ͻó� �ŷ��� ���� ĳ���Ϳ� ���� �༮�� Blaster HUD�� ���̴�
	// �̷��� ĳ���͵��� HUD��Ҹ� �� ����Ʈ�� ȭ���� ���� ����
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());

	if (BlasterCharacter && BlasterCharacter->GetCombatComponent())
	{
		BlasterCharacter->SetDisableGameplay(true);
		// �÷��̾ ������ ������ �ִ� ��Ȳ�� �� ������ false�� ����
		BlasterCharacter->GetCombatComponent()->FireButtonPressed(false);
	}
}

FString ABlasterPlayerController::GetInfoText(const TArray<class ABlasterPlayerState*>& TopScorePlayerStates, ABlasterPlayerState* BlasterPlayerState)
{
	FString InfoTextString;
	if (TopScorePlayerStates.Num() == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	// ����� ����� ���� �Ѹ��϶�
	else if (TopScorePlayerStates.Num() == 1 && TopScorePlayerStates[0] == BlasterPlayerState)
	{
		InfoTextString = Announcement::YouAreTheWinner;
	}
	// ����� ����� �Ѹ��ε� ������ �ƴҶ�?
	else if (TopScorePlayerStates.Num() == 1)
	{
		InfoTextString = Announcement::PlayersTiedForTheWin;
	}
	else if (TopScorePlayerStates.Num() > 1)
	{
		InfoTextString = Announcement::PlayersTiedForTheWin;
		InfoTextString.Append(FString("\n"));
		for (auto TiedPlayer : TopScorePlayerStates)
		{
			InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
		}
	}

	return InfoTextString;
}

FString ABlasterPlayerController::GetTeamsInfoText(ABlasterGameState* BlasterGameState)
{
	if (BlasterGameState == nullptr)
		return FString();

	FString InfoTextString;

	const int32 RedTeamScore = BlasterGameState->RedTeamScore;
	const int32 BlueTeamScore = BlasterGameState->BlueTeamScore;

	if (RedTeamScore == 0 && BlueTeamScore == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (RedTeamScore == BlueTeamScore)
	{
		InfoTextString = FString::Printf(TEXT("%s\n"), *Announcement::TeamsTiedForTheWind);
		InfoTextString.Append(Announcement::RedTeam);
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(Announcement::BlueTeam);
		InfoTextString.Append(TEXT("\n"));
	}
	else if (RedTeamScore > BlueTeamScore)
	{
		InfoTextString = Announcement::RedTeamWins;
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(FString::Printf(TEXT("%s : %d"), *Announcement::RedTeam, RedTeamScore));
		InfoTextString.Append(FString::Printf(TEXT("%s : %d"), *Announcement::BlueTeam, BlueTeamScore));
	}
	else if(BlueTeamScore > RedTeamScore)
	{
		InfoTextString = Announcement::BlueTeamWins;
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(FString::Printf(TEXT("%s : %d"), *Announcement::BlueTeam, BlueTeamScore));
		InfoTextString.Append(FString::Printf(TEXT("%s : %d"), *Announcement::RedTeam, RedTeamScore));
	}

	return InfoTextString;
}