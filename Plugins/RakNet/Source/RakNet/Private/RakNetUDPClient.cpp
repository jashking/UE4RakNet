// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "RakNetPrivatePCH.h"
#include "RakNetUDPClient.h"
#include "IRakNet.h"
#include "RakNet/RakPeerInterface.h"
#include "RakNet/BitStream.h"
#include "RakNet/MessageIdentifiers.h"
#include "RakNet/PacketLogger.h"

enum class CustomMessageIDTypes
{
	StartOffset = ID_USER_PACKET_ENUM,
	IDRakNetCustomData,
};

ARakNetUDPClient::ARakNetUDPClient()
{
	UDPConnection = RakNet::RakPeerInterface::GetInstance();
	
	NetThreadPriority = 0;
	ShutdownDelayMS = 500;

	DefaultPriority = EPacketPriority::ImmediatePriority;
	DefaultReliablitity = EPacketReliability::ReliableOrdered;

	TimeoutMS = 10000;

	PrimaryActorTick.bCanEverTick = true;

	CurrentReconnectTryCount = ReconnectTryCount = 3;
	ReconnectInterval = 2.f;

	IsClosedByUser = false;
}

EConnectionAttemptResult ARakNetUDPClient::Connect(const FString& InHost, int32 InPort)
{
	Host = InHost;
	Port = InPort;

	CurrentReconnectTryCount = ReconnectTryCount;

	return InternalConnect(InHost, InPort);
}

int32 ARakNetUDPClient::SendWithOption(const TArray<uint8>& Message, int32 Channel, EPacketPriority Priority, EPacketReliability Reliability)
{
	return SendRawDataWithOption((const char*)Message.GetData(), Message.Num(), Channel, Priority, Reliability);
}

int32 ARakNetUDPClient::Send(const TArray<uint8>& Message, int32 Channel)
{
	return SendWithOption(Message, Channel, DefaultPriority, DefaultReliablitity);
}

int32 ARakNetUDPClient::SendRawDataWithOption(const char* Message, int32 Size, int32 Channel, EPacketPriority Priority, EPacketReliability Reliability)
{
	return InternalSend((RakNet::MessageID)CustomMessageIDTypes::IDRakNetCustomData, Message, Size, false, Channel, Priority, Reliability);
}

int32 ARakNetUDPClient::SendRawData(const char* Message, int32 Size, int32 Channel)
{
	return SendRawDataWithOption(Message, Size, Channel, DefaultPriority, DefaultReliablitity);
}

bool ARakNetUDPClient::IsActive() const
{
	return UDPConnection ? UDPConnection->IsActive() : false;
}

void ARakNetUDPClient::Disconnect()
{
	using namespace RakNet;

	if (ReconnectTimer.IsValid())
	{
		GetWorldTimerManager().ClearTimer(ReconnectTimer);
	}

	IsClosedByUser = true;

	if (UDPConnection)
	{
		DataStructures::List<SystemAddress> SystemAddresses;
		DataStructures::List<RakNetGUID> GUIDs;

		UDPConnection->GetSystemList(SystemAddresses, GUIDs);

		for (uint32 i = 0; i < SystemAddresses.Size(); ++i)
		{
			const FString RemoteHost = ANSI_TO_TCHAR(SystemAddresses[i].ToString(false));
			const int32 RemotePort = SystemAddresses[i].GetPort();
			const uint32 Guid = RakNet::RakNetGUID::ToUint32(GUIDs[i]);

			UE_LOG(LogRakNet, Display, TEXT("RakNet connection closed by user! Host[%s], Port[%d]."), *RemoteHost, RemotePort);

			RaiseConnectionClosed(RemoteHost, RemotePort, Guid, EConnectionLostReason::ClosedByUser);

			UDPConnection->CloseConnection(GUIDs[i], true);
		}
	}
}

ERakNetConnectionState ARakNetUDPClient::GetState()
{
	using namespace RakNet;

	if (UDPConnection)
	{
		DataStructures::List<SystemAddress> SystemAddresses;
		DataStructures::List<RakNetGUID> GUIDs;

		UDPConnection->GetSystemList(SystemAddresses, GUIDs);

		if (SystemAddresses.Size() > 0)
		{
			return static_cast<ERakNetConnectionState>(UDPConnection->GetConnectionState(GUIDs[0]));
		}
	}

	return ERakNetConnectionState::NotConnected;
}

int32 ARakNetUDPClient::GetAveragePing()
{
	using namespace RakNet;

	if (UDPConnection)
	{
		DataStructures::List<SystemAddress> SystemAddresses;
		DataStructures::List<RakNetGUID> GUIDs;

		UDPConnection->GetSystemList(SystemAddresses, GUIDs);

		if (SystemAddresses.Size() > 0)
		{
			return UDPConnection->GetAveragePing(GUIDs[0]);
		}
	}

	return -1;
}

int32 ARakNetUDPClient::GetLastPing() const
{
	using namespace RakNet;

	if (UDPConnection)
	{
		DataStructures::List<SystemAddress> SystemAddresses;
		DataStructures::List<RakNetGUID> GUIDs;

		UDPConnection->GetSystemList(SystemAddresses, GUIDs);

		if (SystemAddresses.Size() > 0)
		{
			return UDPConnection->GetLastPing(GUIDs[0]);
		}
	}

	return -1;
}

int32 ARakNetUDPClient::GetLowestPing() const
{
	using namespace RakNet;

	if (UDPConnection)
	{
		DataStructures::List<SystemAddress> SystemAddresses;
		DataStructures::List<RakNetGUID> GUIDs;

		UDPConnection->GetSystemList(SystemAddresses, GUIDs);

		if (SystemAddresses.Size() > 0)
		{
			return UDPConnection->GetLowestPing(GUIDs[0]);
		}
	}

	return -1;
}

void ARakNetUDPClient::SetTimeoutTime(int32 InTimeoutMS)
{
	if (UDPConnection)
	{
		UDPConnection->SetTimeoutTime(InTimeoutMS, RakNet::UNASSIGNED_SYSTEM_ADDRESS);
	}
}

int32 ARakNetUDPClient::InternalSend(unsigned char ID, const char* Message, int32 Size, bool Compress,
	int32 Channel, EPacketPriority Priority, EPacketReliability Reliability)
{
	if (UDPConnection)
	{
		RakNet::BitStream Bits;
		Bits.Write((RakNet::MessageID)ID);
		Bits.Write((unsigned char)(Compress ? 1 : 0));
		Bits.Write(Message, (const unsigned int)Size);

		return UDPConnection->Send(&Bits, (PacketPriority)Priority, (PacketReliability)Reliability, Channel, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
	}

	return -1;
}

int32 ARakNetUDPClient::GetRakNetMessageDataOffset(int32 MessageID)
{
	int32 DataOffset = 0;

	switch (MessageID)
	{
	case ID_TIMESTAMP:
		DataOffset = sizeof(RakNet::MessageID) + sizeof(RakNet::Time);
		break;
	default:
		DataOffset = sizeof(RakNet::MessageID);
		break;
	}

	return DataOffset;
}

void ARakNetUDPClient::RaiseConnectionClosed(const FString& InHost, int32 InPort, uint32 Guid, EConnectionLostReason Reason)
{
	if (Reason == EConnectionLostReason::ClosedByUser || CurrentReconnectTryCount <= 0)
	{
		OnConnectionClosed(InHost, InPort, Guid, Reason);
	}
	else
	{
		UE_LOG(LogRakNet, Error,
			TEXT("RakNet connection lost! Reconnect after [%f]s, try count[%d]."), ReconnectInterval, CurrentReconnectTryCount);

		--CurrentReconnectTryCount;

		GetWorldTimerManager().SetTimer(ReconnectTimer, this, &ARakNetUDPClient::Reconnect, ReconnectInterval, false);
	}
}

void ARakNetUDPClient::RaiseConnectionAttemptFailed(const FString& InHost, int32 InPort, uint32 Guid, EConnectionAttemptFailedReason Reason)
{
	if (CurrentReconnectTryCount <= 0)
	{
		OnConnectionAttemptFailed(InHost, InPort, Guid, Reason);
	}
	else
	{
		UE_LOG(LogRakNet, Error,
			TEXT("RakNet connection attempt failed! Reconnect after [%f]s, try count[%d]."), ReconnectInterval, CurrentReconnectTryCount);

		--CurrentReconnectTryCount;

		GetWorldTimerManager().SetTimer(ReconnectTimer, this, &ARakNetUDPClient::Reconnect, ReconnectInterval, false);
	}
}

void ARakNetUDPClient::Reconnect()
{
	UE_LOG(LogRakNet, Display, TEXT("RakNet connection reconnect to Host[%s], Port[%d]."), *Host, Port);

	OnReconnectStared(Host, Port);

	InternalConnect(Host, Port);
}

void ARakNetUDPClient::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (UDPConnection)
	{
		for (RakNet::Packet* InPacket = UDPConnection->Receive(); InPacket; InPacket = UDPConnection->Receive())
		{
			if (!InPacket || InPacket->bitSize <= 0)
			{
				UE_LOG(LogRakNet, Error, TEXT("RakNet received an invalid message!"));
				continue;
			}

			const FString InHost = ANSI_TO_TCHAR(InPacket->systemAddress.ToString(false));
			const int32 InPort = InPacket->systemAddress.GetPort();
			const uint32 Guid = RakNet::RakNetGUID::ToUint32(InPacket->guid);
			const int32 MessageType = InPacket->data[0];
			const int32 DataOffset = GetRakNetMessageDataOffset(MessageType);

			switch (MessageType)
			{
			case ID_DISCONNECTION_NOTIFICATION:
				if (!IsClosedByUser)
				{
					UE_LOG(LogRakNet, Error, TEXT("RakNet connection closed by remote! Host[%s], Port[%d]."), *InHost, InPort);
					RaiseConnectionClosed(InHost, InPort, Guid, EConnectionLostReason::ClosedByRemote);
				}
				break;

			case ID_CONNECTION_LOST:
				if (!IsClosedByUser)
				{
					UE_LOG(LogRakNet, Error, TEXT("RakNet connection lost! Host[%s], Port[%d]."), *InHost, InPort);
					RaiseConnectionClosed(InHost, InPort, Guid, EConnectionLostReason::ConnectionLost);
				}
				break;

			case ID_CONNECTION_REQUEST_ACCEPTED:
				UE_LOG(LogRakNet, Display, TEXT("RakNet connection opened! Host[%s], Port[%d]."), *InHost, InPort);
				OnConnectionOpened(InHost, InPort, Guid);
				break;

			case ID_CONNECTION_ATTEMPT_FAILED:
				UE_LOG(LogRakNet, Error, TEXT("RakNet connection attempt failed! Host[%s], Port[%d]."), *InHost, InPort);
				RaiseConnectionAttemptFailed(InHost, InPort, Guid, EConnectionAttemptFailedReason::ConnectionAttemptFailed);
				break;

			case ID_REMOTE_SYSTEM_REQUIRES_PUBLIC_KEY:
				UE_LOG(LogRakNet, Error,
					TEXT("RakNet connect failed! Remote system requires public key! Host[%s], Port[%d]."), *InHost, InPort);
				RaiseConnectionAttemptFailed(InHost, InPort, Guid, EConnectionAttemptFailedReason::RemoteSystemRequiresPublicKey);
				break;

			case ID_OUR_SYSTEM_REQUIRES_SECURITY:
				UE_LOG(LogRakNet, Error,
					TEXT("RakNet connect failed! Our system requires security! Host[%s], Port[%d]."), *InHost, InPort);
				RaiseConnectionAttemptFailed(InHost, InPort, Guid, EConnectionAttemptFailedReason::OurSystemRequiresSecurity);
				break;

			case ID_PUBLIC_KEY_MISMATCH:
				UE_LOG(LogRakNet, Error,
					TEXT("RakNet connect failed! Public key mismatch! Host[%s], Port[%d]."), *InHost, InPort);
				RaiseConnectionAttemptFailed(InHost, InPort, Guid, EConnectionAttemptFailedReason::PublicKeyMismatch);
				break;

			case ID_ALREADY_CONNECTED:
				UE_LOG(LogRakNet, Warning, TEXT("RakNet connection already connected! Host[%s], Port[%d]."), *InHost, InPort);
				RaiseConnectionAttemptFailed(InHost, InPort, Guid, EConnectionAttemptFailedReason::AlreadyConnected);
				break;

			case ID_NO_FREE_INCOMING_CONNECTIONS:
				UE_LOG(LogRakNet, Error,
					TEXT("RakNet connect failed! No free incoming connections! Host[%s], Port[%d]."), *InHost, InPort);
				RaiseConnectionAttemptFailed(InHost, InPort, Guid, EConnectionAttemptFailedReason::NoFreeIncomingConnections);
				break;

			case ID_CONNECTION_BANNED:
				UE_LOG(LogRakNet, Error,
					TEXT("RakNet connect failed! Connection banned! Host[%s], Port[%d]."), *InHost, InPort);
				RaiseConnectionAttemptFailed(InHost, InPort, Guid, EConnectionAttemptFailedReason::ConnectionBanned);
				break;

			case ID_INVALID_PASSWORD:
				UE_LOG(LogRakNet, Error,
					TEXT("RakNet connect failed! Invalid password! Host[%s], Port[%d]."), *InHost, InPort);
				RaiseConnectionAttemptFailed(InHost, InPort, Guid, EConnectionAttemptFailedReason::InvalidPassword);
				break;

			case ID_INCOMPATIBLE_PROTOCOL_VERSION:
				UE_LOG(LogRakNet, Error,
					TEXT("RakNet connect failed! Incompatible protocol version! Host[%s], Port[%d]."), *InHost, InPort);
				RaiseConnectionAttemptFailed(InHost, InPort, Guid, EConnectionAttemptFailedReason::IncompatibleProtocol);
				break;

			case ID_IP_RECENTLY_CONNECTED:
				UE_LOG(LogRakNet, Error,
					TEXT("RakNet connect failed! IP recently connected! Host[%s], Port[%d]."), *InHost, InPort);
				RaiseConnectionAttemptFailed(InHost, InPort, Guid, EConnectionAttemptFailedReason::IpRecentlyConnected);
				break;

			case (int32)CustomMessageIDTypes::IDRakNetCustomData:
			{
				OnReceived(InPacket->data + DataOffset, InPacket->length - DataOffset);
				
				break;
			}

			default:
				UE_LOG(LogRakNet, Warning,
					TEXT("Unknown raknet message type! ID[%d][%s], Host[%s], Port[%d]."),
					MessageType, ANSI_TO_TCHAR(RakNet::PacketLogger::BaseIDTOString(MessageType)), *InHost, InPort);
				break;
			}

			UDPConnection->DeallocatePacket(InPacket);
		}
	}
}

void ARakNetUDPClient::BeginDestroy()
{
	UE_LOG(LogRakNet, Display, TEXT("RakNet destroy begin. Shutdown connection."));

	if (UDPConnection)
	{
		UDPConnection->Shutdown(ShutdownDelayMS);

		RakNet::RakPeerInterface::DestroyInstance(UDPConnection);
	}

	UDPConnection = nullptr;

	UE_LOG(LogRakNet, Display, TEXT("RakNet destroy finish."));

	Super::BeginDestroy();
}

void ARakNetUDPClient::OnConnectionOpened(const FString& InHost, int32 InPort, uint32 Guid)
{
	SetTimeoutTime(TimeoutMS);

	CurrentReconnectTryCount = ReconnectTryCount;
}

void ARakNetUDPClient::OnReconnectStared(const FString& InHost, int32 InPort)
{

}

void ARakNetUDPClient::OnConnectionClosed(const FString& InHost, int32 InPort, uint32 Guid, EConnectionLostReason Reason)
{
}

void ARakNetUDPClient::OnConnectionAttemptFailed(const FString& InHost, int32 InPort, uint32 Guid, EConnectionAttemptFailedReason Reason)
{

}

void ARakNetUDPClient::OnReceived(const uint8* Data, uint32 Size)
{

}

EConnectionAttemptResult ARakNetUDPClient::InternalConnect(const FString& InHost, int32 InPort)
{
	using namespace RakNet;

	IsClosedByUser = false;

	if (UDPConnection)
	{
		if (!UDPConnection->IsActive())
		{
			SocketDescriptor SocketDesc;

			UE_LOG(LogRakNet, Display, TEXT("RakNet startup start."));

			const StartupResult Result = UDPConnection->Startup(1, &SocketDesc, 1, NetThreadPriority);
			if (Result != RAKNET_STARTED && Result != RAKNET_ALREADY_STARTED)
			{
				UE_LOG(LogRakNet, Error,
					TEXT("RakNet startup failed! Error[%d]! Host[%s], Port[%d]."),
					(int32)Result, *InHost, InPort);

				return EConnectionAttemptResult::ConnectionStartupFailed;
			}

			UDPConnection->SetOccasionalPing(true);

			UE_LOG(LogRakNet, Display, TEXT("RakNet startup finish."));
		}

		const ConnectionAttemptResult AttemptResult = UDPConnection->Connect(StringCast<ANSICHAR>(*InHost).Get(), InPort, NULL, 0);
		switch (AttemptResult)
		{
		case CONNECTION_ATTEMPT_STARTED:
			UE_LOG(LogRakNet, Display, TEXT("RakNet start connection to Host[%s], Port[%d]."), *InHost, InPort);
			return EConnectionAttemptResult::ConnectionAttemptStarted;

		case INVALID_PARAMETER:
			UE_LOG(LogRakNet, Error, TEXT("RakNet connect failed! Invalid Parameter! Host[%s], Port[%d]."), *InHost, InPort);
			return EConnectionAttemptResult::InvalidParameter;

		case CANNOT_RESOLVE_DOMAIN_NAME:
			UE_LOG(LogRakNet, Error, TEXT("RakNet connect failed! Cannot resolve domain name! Host[%s], Port[%d]."), *InHost, InPort);
			return EConnectionAttemptResult::CannotResolveDomainName;

		case ALREADY_CONNECTED_TO_ENDPOINT:
			UE_LOG(LogRakNet, Display, TEXT("RakNet already connected to Host[%s], Port[%d]."), *InHost, InPort);
			return EConnectionAttemptResult::AlreadyConnectedToEndpoint;

		case CONNECTION_ATTEMPT_ALREADY_IN_PROGRESS:
			UE_LOG(LogRakNet, Display, TEXT("RakNet connection attempt already in progress! Host[%s], Port[%d]."), *InHost, InPort);
			return EConnectionAttemptResult::ConnectionAttemptAlreadyInProgress;

		case SECURITY_INITIALIZATION_FAILED:
			UE_LOG(LogRakNet, Error, TEXT("RakNet connect failed! Security Initialization failed! Host[%s], Port[%d]."), *InHost, InPort);
			return EConnectionAttemptResult::SecurityInitializationFailed;

		default:
			UE_LOG(LogRakNet, Error, TEXT("RakNet connect failed! Unknown Error[%d]!"), (int32)AttemptResult);
			return EConnectionAttemptResult::UnknownError;
		}
	}
	else
	{
		UE_LOG(LogRakNet, Error,
			TEXT("RakNet connect failed! RakNet client initialize failed! Host[%s], Port[%d]."), *InHost, InPort);

		return EConnectionAttemptResult::InvalidInterfaceInstance;
	}
}

