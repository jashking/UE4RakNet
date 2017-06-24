// Fill out your copyright notice in the Description page of Project Settings.

#include "UE4RakNet.h"
#include "MyRakNetUDPClient.h"




AMyRakNetUDPClient::AMyRakNetUDPClient()
{

}

void AMyRakNetUDPClient::OnConnectionOpened(const FString& InHost, int32 InPort, uint32 Guid)
{
	Super::OnConnectionOpened(InHost, InPort, Guid);
}

void AMyRakNetUDPClient::OnReconnectStared(const FString& InHost, int32 InPort)
{
	Super::OnReconnectStared(InHost, InPort);
}

void AMyRakNetUDPClient::OnConnectionClosed(const FString& InHost, int32 InPort, uint32 Guid, EConnectionLostReason Reason)
{
	Super::OnConnectionClosed(InHost, InPort, Guid, Reason);
}

void AMyRakNetUDPClient::OnConnectionAttemptFailed(const FString& InHost, int32 InPort, uint32 Guid, EConnectionAttemptFailedReason Reason)
{
	Super::OnConnectionAttemptFailed(InHost, InPort, Guid, Reason);
}

void AMyRakNetUDPClient::OnReceived(const uint8* Data, uint32 Size)
{
	Super::OnReceived(Data, Size);
}
