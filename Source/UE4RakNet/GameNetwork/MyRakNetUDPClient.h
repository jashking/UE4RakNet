// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "RakNetUDPClient.h"
#include "MyRakNetUDPClient.generated.h"

/**
 * 
 */
UCLASS()
class UE4RAKNET_API AMyRakNetUDPClient : public ARakNetUDPClient
{
	GENERATED_BODY()
	
public:
	AMyRakNetUDPClient();
	
protected:
	virtual void OnConnectionOpened(const FString& InHost, int32 InPort, uint32 Guid);
	virtual void OnReconnectStared(const FString& InHost, int32 InPort);
	virtual void OnConnectionClosed(const FString& InHost, int32 InPort, uint32 Guid, EConnectionLostReason Reason);
	virtual void OnConnectionAttemptFailed(const FString& InHost, int32 InPort, uint32 Guid, EConnectionAttemptFailedReason Reason);
	virtual void OnReceived(const uint8* Data, uint32 Size);
};
