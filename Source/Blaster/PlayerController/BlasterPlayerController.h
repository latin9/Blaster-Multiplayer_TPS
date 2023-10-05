// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;


public:
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDScore(float Score);
	void HideHUDScore();
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(float CountdownTime);
	void SetHUDAnnouncementCountdown(float CountdownTime);
	void SetHUDGrenades(int32 Grenades);

	void OnMatchStateSet(FName State, bool bTeamMatch = false);
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;
	// 서버 월드 시간과 동기화
	virtual float GetServerTime();
	// 가능한 빨리 서버 월드 시간과 동기화
	virtual void ReceivedPlayer() override;
	void HandleMatchHasStarted(bool bTeamMatch = false);
	void HandleCooldown();
	
	FHighPingDelegate HighPingDelegate;

	void BroadcastElim(APlayerState* Attacker, APlayerState* Victim);

	void SendMessage(const FText& Message);

	// 팀 관련
	void HideTeamScores();
	void InitTeamScores();
	void SetHUDRedTeamScore(int32 RedScore);
	void SetHUDBlueTeamScore(int32 BlueScore);

	void LocalWeaponSelectOverlay();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastWeaponSelectOverlay();
	
	UFUNCTION(Client, Reliable)
	void ClientWeaponSelectOverlay();
public:
	void SwitchViewToOtherPlayer();

protected:
	void SetHUDTime();

	void PollInit();

	// 클라이언트와 서버의 시간 동기화
	// 현재 서버 시간을 요청
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	// ServerRequestServerTime에 대한 응답으로 현재 서버 시간을 클라이언트에 보고
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	// 서버의 현재 시간과 왕복 시간의 절반을 뺀 값 사이의 델타
	float ClientServerDelta = 0.f; 

	// 5초마다 서버 타임과 동기화
	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;

	void CheckTimeSync(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientJoinMidgame(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime);

	void HighPingWarning();
	void StopHighPingWarning();
	void CheckPing(float DeltaTime);

	void ShowReturnToMainMenu();
	void ChangeUIMode();
	void FocusToChatWidgetAndChatting();

	UFUNCTION(Client, Reliable)
	void ClientElimAnnouncement(APlayerState* Attacker, APlayerState* Victim);

	UFUNCTION(Server, Reliable)
	void ServerSendChatMessage(const FString& Message);

	UFUNCTION(Client, Reliable)
	void ClientSendChatMessage(const FString& Message);

	UPROPERTY(ReplicatedUsing = OnRep_ShowTeamScores)
	bool bShowTeamScores = false;

	UFUNCTION()
	void OnRep_ShowTeamScores();


private:
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

	UPROPERTY()
	class ABlasterGameMode* BlasterGameMode;

	UPROPERTY(EditAnywhere, Category = HUD)
	TSubclassOf<class UUserWidget> ReturnToMainMenuWidget;

	UPROPERTY()
	class UReturnToMainMenu* ReturnToMainMenu;

	bool bReturnToMainMenuOpen = false;

	float LevelStartingTime = 0.f;
	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float CooldownTime = 0.f;
	uint32 CountdownInt = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	float HUDHealth;
	bool bInitializeHealth = false;
	float HUDMaxHealth;
	float HUDShield;
	bool bInitializeShield = false;
	float HUDMaxShield;
	float HUDScore;
	bool bInitializeScore = false;
	int32 HUDDefeats;
	bool bInitializeDefeats = false;
	int32 HUDGrenades;
	bool bInitializeGrenades = false;
	int32 HUDCarriedAmmo;
	bool bInitializeCarriedAmmo = false;
	int32 HUDWeaponAmmo;
	bool bInitializeWeaponAmmo = false;

	bool bHideHUDScore = false;

	bool bBeginValid = false;

	// 핑
	float HighPingRunningTime = 0.f;

	// HighPingAnimation을 몇초동안 실행(지속)할건지
	UPROPERTY(EditAnywhere)
	float HighPingDuration = 5.f;

	float PingAnimationRunningTime = 0.f;

	// 핑을 몇초마다 체크할건지?
	UPROPERTY(EditAnywhere)
	float CheckPingFrequency = 20.f;

	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);

	// 하이핑 임계값
	UPROPERTY(EditAnywhere)
	float HightPingThreshold = 50.f;

	float SingleTripTime = 0.f;

	bool bUIOnlyModeEnable = false;

	FString GetInfoText(const TArray<class ABlasterPlayerState*>& TopScorePlayerStates, class ABlasterPlayerState* BlasterPlayerState);

	FString GetTeamsInfoText(class ABlasterGameState* BlasterGameState);

public:
	FORCEINLINE float GetSingleTripTime() const { return SingleTripTime; }
	FORCEINLINE class ABlasterHUD* GetBlastertHUD() const { return BlasterHUD; }
	FORCEINLINE bool GetUIOnlyModeEnable() const { return bUIOnlyModeEnable; }
	FORCEINLINE void SetUIOnlyModeEnable(bool _Enable) { bUIOnlyModeEnable = _Enable; }
};
