// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include <string>

#include "UObject/NoExportTypes.h"
#include "RakNetUDPClient.generated.h"

namespace RakNet
{

struct Packet;
class RakPeerInterface;

} // namespace Raknet

namespace ZCompress
{
class Decoder;
}

UENUM(BlueprintType)
enum class EConnectionAttemptResult : uint8
{
	None,
	ConnectionAttemptStarted,
	InvalidParameter,
	CannotResolveDomainName,
	AlreadyConnectedToEndpoint,
	ConnectionAttemptAlreadyInProgress,
	SecurityInitializationFailed,
	ConnectionStartupFailed,
	InvalidInterfaceInstance,
	UnknownError,
};

UENUM(BlueprintType)
enum class EPacketPriority : uint8
{
	/// The highest possible priority. These message trigger sends immediately, and are generally not buffered or aggregated into a single datagram.
	ImmediatePriority,

	/// For every 2 IMMEDIATE_PRIORITY messages, 1 HIGH_PRIORITY will be sent.
	/// Messages at this priority and lower are buffered to be sent in groups at 10 millisecond intervals to reduce UDP overhead and better measure congestion control. 
	HighPriority,

	/// For every 2 HIGH_PRIORITY messages, 1 MEDIUM_PRIORITY will be sent.
	/// Messages at this priority and lower are buffered to be sent in groups at 10 millisecond intervals to reduce UDP overhead and better measure congestion control. 
	MediumPriority,

	/// For every 2 MEDIUM_PRIORITY messages, 1 LOW_PRIORITY will be sent.
	/// Messages at this priority and lower are buffered to be sent in groups at 10 millisecond intervals to reduce UDP overhead and better measure congestion control. 
	LowPriority,
};

UENUM(BlueprintType)
enum class EPacketReliability : uint8
{
	/// Same as regular UDP, except that it will also discard duplicate datagrams.  RakNet adds (6 to 17) + 21 bits of overhead, 16 of which is used to detect duplicate packets and 6 to 17 of which is used for message length.
	Unreliable,

	/// Regular UDP with a sequence counter.  Out of order messages will be discarded.
	/// Sequenced and ordered messages sent on the same channel will arrive in the order sent.
	UnreliableSequenced,

	/// The message is sent reliably, but not necessarily in any order.  Same overhead as UNRELIABLE.
	Reliable,

	/// This message is reliable and will arrive in the order you sent it.  Messages will be delayed while waiting for out of order messages.  Same overhead as UNRELIABLE_SEQUENCED.
	/// Sequenced and ordered messages sent on the same channel will arrive in the order sent.
	ReliableOrdered,

	/// This message is reliable and will arrive in the sequence you sent it.  Out or order messages will be dropped.  Same overhead as UNRELIABLE_SEQUENCED.
	/// Sequenced and ordered messages sent on the same channel will arrive in the order sent.
	ReliableSequenced,
};

UENUM(BlueprintType)
enum class EConnectionLostReason : uint8
{
	/// Called RakPeer::CloseConnection()
	ClosedByUser,

	/// Got ID_DISCONNECTION_NOTIFICATION
	ClosedByRemote,

	/// GOT ID_CONNECTION_LOST
	ConnectionLost,
};

UENUM(BlueprintType)
enum class EConnectionAttemptFailedReason : uint8
{
	ConnectionAttemptFailed,
	AlreadyConnected,
	NoFreeIncomingConnections,
	SecurityPublicKeyMismatch,
	ConnectionBanned,
	InvalidPassword,
	IncompatibleProtocol,
	IpRecentlyConnected,
	RemoteSystemRequiresPublicKey,
	OurSystemRequiresSecurity,
	PublicKeyMismatch,
};

UENUM(BlueprintType)
enum class ERakNetConnectionState : uint8
{
	/// Connect() was called, but the process hasn't started yet
	PendingStart,
	/// Processing the connection attempt
	Connecting,
	/// Is connected and able to communicate
	Connected,
	/// Was connected, but will disconnect as soon as the remaining messages are delivered
	Disconnecting,
	/// A connection attempt failed and will be aborted
	SilentlyDisconnecting,
	/// No longer connected
	Disconnected,
	/// Was never connected, or else was disconnected long enough ago that the entry has been discarded
	NotConnected,
};

/**
 * TODO:
 * 1. uncompress [DONE]
 * 2. reconnect [DONE]
 * 3. proxy
 */
UCLASS()
class RAKNET_API ARakNetUDPClient : public AActor
{
	GENERATED_BODY()
	
public:

	ARakNetUDPClient();

	int32 SendRawDataWithOption(const char* Message, int32 Size, int32 Channel, EPacketPriority Priority, EPacketReliability Reliability);

	int32 SendRawData(const char* Message, int32 Size, int32 Channel);

	UFUNCTION(BlueprintCallable, Category = UDPClient)
	virtual EConnectionAttemptResult Connect(const FString& InHost, int32 InPort);

	UFUNCTION(BlueprintCallable, Category = UDPClient)
	int32 SendWithOption(const TArray<uint8>& Message, int32 Channel, EPacketPriority Priority, EPacketReliability Reliability);

	UFUNCTION(BlueprintCallable, Category = UDPClient)
	int32 Send(const TArray<uint8>& Message, int32 Channel);

	UFUNCTION(BlueprintCallable, Category = UDPClient)
	bool IsActive() const;

	UFUNCTION(BlueprintCallable, Category = UDPClient)
	void Disconnect();

	UFUNCTION(BlueprintCallable, Category = UDPClient)
	ERakNetConnectionState GetState();

	UFUNCTION(BlueprintCallable, Category = UDPClient)
	int32 GetAveragePing();

	UFUNCTION(BlueprintCallable, Category = UDPClient)
	int32 GetLastPing() const;

	UFUNCTION(BlueprintCallable, Category = UDPClient)
	int32 GetLowestPing() const;

	UFUNCTION(BlueprintCallable, Category = UDPClient)
	void SetTimeoutTime(int32 InTimeoutMS);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UDPClient)
	int32 NetThreadPriority;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UDPClient)
	int32 ShutdownDelayMS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UDPClient)
	EPacketPriority DefaultPriority;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UDPClient)
	EPacketReliability DefaultReliablitity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UDPClient)
	int32 TimeoutMS;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UDPClient)
	int32 ReconnectTryCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UDPClient)
	float ReconnectInterval;

protected:

	virtual void Tick(float DeltaSeconds) override;

	virtual void BeginDestroy() override;

	virtual void OnConnectionOpened(const FString& InHost, int32 InPort, uint32 Guid);
	virtual void OnReconnectStared(const FString& InHost, int32 InPort);
	virtual void OnConnectionClosed(const FString& InHost, int32 InPort, uint32 Guid, EConnectionLostReason Reason);
	virtual void OnConnectionAttemptFailed(const FString& InHost, int32 InPort, uint32 Guid, EConnectionAttemptFailedReason Reason);
	virtual void OnReceived(const uint8* Data, uint32 Size);

	EConnectionAttemptResult InternalConnect(const FString& InHost, int32 InPort);
	int32 InternalSend(unsigned char ID, const char* Message, int32 Size, bool Compress,
		int32 Channel, EPacketPriority Priority, EPacketReliability Reliability);

	int32 GetRakNetMessageDataOffset(int32 MessageID);

	void RaiseConnectionClosed(const FString& InHost, int32 InPort, uint32 Guid, EConnectionLostReason Reason);
	void RaiseConnectionAttemptFailed(const FString& InHost, int32 InPort, uint32 Guid, EConnectionAttemptFailedReason Reason);

	void Reconnect();

private:
	RakNet::RakPeerInterface* UDPConnection{ nullptr };

	std::string UncompressBuffer;

	int32 CurrentReconnectTryCount;

	FTimerHandle ReconnectTimer;

	FString Host;
	int32 Port;

	bool IsClosedByUser;
};
