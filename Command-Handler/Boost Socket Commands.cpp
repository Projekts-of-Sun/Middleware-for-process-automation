#include <afx.h>
#include <mmsystem.h>

#include "Boost Socket Commands.h"
#include "Command List.h"
#include "Method Command.h"
#include "Version Command.h"
#include "Resource Command.h"
#include "KeepAlive Command.h"
#include "Abort Command.h"

#include "..\Utf8\Utf8.h"

#include <boost/interprocess/sync/scoped_lock.hpp>


#ifndef NDEBUG
	#define new DEBUG_NEW
#endif

namespace MassHunter
{
	using namespace boost;
	using namespace boost::asio::ip;
	using namespace CommandHandler;

CMassHunterPeerConnection::CMassHunterPeerConnection(std::shared_ptr<MassHunter::CMassHunterServer> MassHunter, CString IP, unsigned short Port, CString ClientName)
	: CBasePeerConnection(MassHunter->IoService(), ClientName)
	, m_MassHunter(MassHunter)
	, m_Socket(new asio::ip::tcp::socket(MassHunter->IoService()))
	, m_ReadState(0)
	, m_PacketLength(0)
	, m_BytesReceived(0)
	, m_Peer(address::from_string(CStringToUtf8(IP)), Port)
	, m_IsConnecting(true)
	, m_IsConnected(false)
	, m_PeerTimeoutHandler(NULL)
	, m_PeerErrorHandler(NULL)
	, m_PeerActivateHandler(NULL)
	, m_LastReceiveTime(posix_time::microsec_clock::local_time())
	, m_LastKeepAliveTime(posix_time::microsec_clock::local_time())
	, m_LastResourceRequest(posix_time::microsec_clock::local_time())
	, m_PeerVersion(L"")
	, m_StartupComplete(false)
	, m_PeerProjectListHandler(NULL)
	, m_PeerMethodListHandler(NULL)
	, m_PeerMasterBatchListHandler(NULL)
	, m_PeerStatusListHandler(NULL)
	, m_PeerRunMethodHandler(NULL)
	, m_PeerAbortHandler(NULL)
	, m_ReceiveBuffer(new char[ReceiveBufferSize])
{
	TRACE(L"CMassHunterPeerConnection::CMassHunterPeerConnection\n");
	CString T;
	T.Format(L"MassHunter.%s.%s_%u.log.txt", (LPCTSTR)m_ClientName, (LPCTSTR)IP, Port);
	m_Logger.NewLogFileName(T);
	m_Logger.LogText(L"Outgoing Peer connection object created");
}

CMassHunterPeerConnection::CMassHunterPeerConnection(std::shared_ptr<asio::io_service> IoService, CString IP, unsigned short Port, CString ClientName, bool SilasLog)
	: CBasePeerConnection(*IoService, ClientName, SilasLog)
	, m_MassHunter(std::shared_ptr<CMassHunterServer>())
	, m_Socket(new asio::ip::tcp::socket(*IoService))
	, m_ReadState(0)
	, m_PacketLength(0)
	, m_BytesReceived(0)
	, m_Peer(address::from_string(CStringToUtf8(IP)), Port)
	, m_IsConnecting(true)
	, m_IsConnected(false)
	, m_PeerTimeoutHandler(NULL)
	, m_PeerErrorHandler(NULL)
	, m_PeerActivateHandler(NULL)
	, m_LastReceiveTime(posix_time::microsec_clock::local_time())
	, m_LastKeepAliveTime(posix_time::microsec_clock::local_time())
	, m_LastResourceRequest(posix_time::microsec_clock::local_time())
	, m_PeerVersion(L"")
	, m_StartupComplete(false)
	, m_PeerProjectListHandler(NULL)
	, m_PeerMethodListHandler(NULL)
	, m_PeerMasterBatchListHandler(NULL)
	, m_PeerStatusListHandler(NULL)
	, m_PeerRunMethodHandler(NULL)
	, m_PeerAbortHandler(NULL)
	, m_ReceiveBuffer(new char[ReceiveBufferSize])
{
	TRACE(L"CMassHunterPeerConnection::CMassHunterPeerConnection\n");
	CString T;
	T.Format(L"MassHunter.%s.%s_%u.log.txt", (LPCTSTR)m_ClientName, (LPCTSTR)IP, Port);
	m_Logger.NewLogFileName(T);
	m_Logger.LogText(L"Outgoing Peer connection object created");
}

CMassHunterPeerConnection::CMassHunterPeerConnection(std::shared_ptr<MassHunter::CMassHunterServer> MassHunter, CString ClientName)
	: CBasePeerConnection(MassHunter->IoService(), ClientName)
	, m_MassHunter(MassHunter)
	, m_Socket(new asio::ip::tcp::socket(MassHunter->IoService()))
	, m_ReadState(0)
	, m_PacketLength(0)
	, m_BytesReceived(0)
	, m_IsConnecting(true)
	, m_IsConnected(false)
	, m_PeerTimeoutHandler(NULL)
	, m_PeerErrorHandler(NULL)
	, m_PeerActivateHandler(NULL)
	, m_LastReceiveTime(posix_time::microsec_clock::local_time())
	, m_LastKeepAliveTime(posix_time::microsec_clock::local_time())
	, m_PeerVersion(L"")
	, m_StartupComplete(false)
	, m_PeerProjectListHandler(NULL)
	, m_PeerMethodListHandler(NULL)
	, m_PeerMasterBatchListHandler(NULL)
	, m_PeerStatusListHandler(NULL)
	, m_PeerRunMethodHandler(NULL)
	, m_PeerAbortHandler(NULL)
	, m_ReceiveBuffer(new char[ReceiveBufferSize])
{
	TRACE(L"CMassHunterPeerConnection::CMassHunterPeerConnection\n");
	m_Logger.NewLogFileName(L"MassHunter." + m_ClientName + L".log.txt");
	m_Logger.LogText(L"Incoming Peer connection object created");
}

CMassHunterPeerConnection::~CMassHunterPeerConnection()
{
	TRACE(L"CMassHunterPeerConnection::~CMassHunterPeerConnection\n");
	if (m_ReceiveBuffer) delete m_ReceiveBuffer;
	m_Logger.LogText(L"Command interpreter object destructed");
}

void CMassHunterPeerConnection::Connect()
{
	TRACE(L"CMassHunterPeerConnection::Connect\n");
	if (m_Logger.IsEnabled())
	{	std::string LT("Connecting to " + m_Peer.address().to_string());
		m_Logger.LogText(LT);
	}
	m_Socket->async_connect(m_Peer, bind(&CMassHunterPeerConnection::HandleConnect, shared_from_this(),
								asio::placeholders::error));
}

void CMassHunterPeerConnection::ReConnect()
{
	TRACE(L"CMassHunterPeerConnection::ReConnect\n");
	if (m_Socket->is_open())
	{	m_Socket->shutdown(asio::ip::tcp::socket::shutdown_both);
		m_Socket->close();
	}
	m_Socket.reset(new asio::ip::tcp::socket(m_Socket->get_executor()));
	if (m_Logger.IsEnabled())
	{	std::string LT("ReConnecting to " + m_Peer.address().to_string());
		m_Logger.LogText(LT);
	}
	m_Socket->async_connect(m_Peer, bind(&CMassHunterPeerConnection::HandleConnect, shared_from_this(),
								asio::placeholders::error));
}

void CMassHunterPeerConnection::Start(bool /*SelfTiming*/)
{
	TRACE(L"CMassHunterPeerConnection::Start\n");
	CString LN;
	LN.Format(L"MassHunter.%s.%s_%u.log.txt", (LPCTSTR)m_ClientName,
										(LPCTSTR)Utf8ToCString(m_Socket->remote_endpoint().address().to_string()),
										m_Socket->remote_endpoint().port());
	m_Logger.NewLogFileName(LN);
	m_ReadState = ReadLength;
	m_BytesReceived = 0;
	asio::async_read(*m_Socket, asio::buffer(m_ReceiveBuffer, ReceiveBufferSize), asio::transfer_at_least(4),
						bind(&CMassHunterPeerConnection::HandleRead, shared_from_this(),
						asio::placeholders::error,
						asio::placeholders::bytes_transferred));

	Activate(posix_time::seconds(5));
}

void CMassHunterPeerConnection::HandleConnect(const system::error_code& Error)
{
	TRACE(L"CMassHunterPeerConnection::HandleConnect = %d\n", Error.value());
	m_IsConnecting = false;
	if (Error)
	{	m_ErrorText = Error.message().c_str();
		DispatchError(Error);
		m_IsConnected = false;
		if (m_MassHunter) m_MassHunter->RemovePeer(Error, this);
		else if (m_PeerErrorHandler) m_PeerErrorHandler(Error, this);
		m_Socket->close();
		return;
	}
	m_IsConnected = true;
	Start(m_MassHunter==NULL);
}

void CMassHunterPeerConnection::HandleRead(const system::error_code& Error, size_t BytesTransferred)
{
//	TRACE(L"CMassHunterPeerConnection::HandleRead = %u (%d)\n", BytesTransferred, Error.value());
	if (Error)
	{	m_ErrorText = Error.message().c_str();
		DispatchError(Error);
		m_IsConnected = false;
		if (m_MassHunter) m_MassHunter->RemovePeer(Error, this);
		else if (m_PeerErrorHandler) m_PeerErrorHandler(Error, this);
		return;
	}
	m_LastReceiveTime = posix_time::microsec_clock::local_time();
	m_TotalBytesReceived+=BytesTransferred;
	bool MoreDataToProcess;
	do
	{	m_BytesReceived+=BytesTransferred;
		MoreDataToProcess = false;
		if (m_ReadState==ReadLength)
		{	if (m_BytesReceived<4) return;
			m_PacketLength = ntohl(*(unsigned int*)m_ReceiveBuffer);
			if (m_BytesReceived<ReceiveBufferSize)
			{	m_ReadState = ReadData;
			}
			else
			{	// do not accept any data larger than the receive buffer
				m_ReadState = ReadLength;
				m_BytesReceived = 0;
			}
		}
		if (m_ReadState==ReadData)
		{	if ((m_PacketLength + 4)<=m_BytesReceived)
			{
				if (m_Logger.IsEnabled())
				{	std::string X(m_ReceiveBuffer + 4, m_ReceiveBuffer + m_PacketLength + 4);
					X = "R: " + X;
					m_Logger.LogText(X);
				}
				_ASSERTE(ReceiveBufferSize>(m_PacketLength + 4));
				m_ReceiveBuffer[m_PacketLength + 4] = 0;
				if (m_MassHunter) m_MassHunter->DataFeedback(m_ReceiveBuffer + 4, m_PacketLength, shared_from_this());
				DataInterpreter(m_ReceiveBuffer + 4, m_PacketLength);
				BytesTransferred = m_BytesReceived - (m_PacketLength + 4);
				m_ReadState = ReadLength;
				m_BytesReceived = 0;
				// more data?
				if (BytesTransferred)
				{	MoreDataToProcess = true;
					memcpy(m_ReceiveBuffer, m_ReceiveBuffer + m_PacketLength + 4, BytesTransferred);
				}
			}
		}
	} while (MoreDataToProcess);
	_ASSERTE(ReceiveBufferSize>m_BytesReceived);
	asio::async_read(*m_Socket,
			asio::buffer(m_ReceiveBuffer + m_BytesReceived, ReceiveBufferSize - m_BytesReceived),
			asio::transfer_at_least(1),
			bind(&CMassHunterPeerConnection::HandleRead, shared_from_this(),
				asio::placeholders::error,
				asio::placeholders::bytes_transferred));
}


void CMassHunterPeerConnection::DoWrite(std::string* Message)
{
    Message->insert(0, 4, ' ');
		*(unsigned int*)Message->c_str() = htonl(unsigned long(Message->length() - 4));
		asio::async_write(*m_Socket,
        asio::buffer(Message->c_str(), Message->length()),
        bind(&CMassHunterPeerConnection::HandleWrite, shared_from_this(),
          asio::placeholders::error,
					asio::placeholders::bytes_transferred, Message));
}

void CMassHunterPeerConnection::HandleWrite(const system::error_code& Error, size_t BytesTransferred,
											std::string* Message)
{
//	TRACE(L"CMassHunterPeerConnection::HandleWrite = %d\n", Error.value());
	if (Message)
	{	delete Message;
	}
  if (Error)
  {	m_ErrorText = Error.message().c_str();
		DispatchError(Error);
		m_IsConnected = false;
		if (m_MassHunter) m_MassHunter->RemovePeer(Error, this);
		else if (m_PeerErrorHandler) m_PeerErrorHandler(Error, this);
		return;
	}
	m_TotalBytesSent+=BytesTransferred;
}


bool CMassHunterPeerConnection::DataInterpreter(char* Data, size_t /*DatenLength*/)
{
//	TRACE(L"\tCMassHunterPeerConnection::DataInterpreter[0x%p] D=0x%p\n", this, Data);
	if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) return false;
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	std::list<std::shared_ptr<CBaseCommand>> CL(CCommandList::ObjectsFromXmlString(Data));
	if (!CL.empty())
	{	if (m_Logger.IsEnabled()) m_Logger.LogText(L"R: " + CL.front()->Dump());
		if (CL.front()->Client.IsEmpty())
		{	return false;
		}
		for (std::list<std::shared_ptr<CBaseCommand>>::iterator i(CL.begin());i!=CL.end();++i)
		{
			// TODO: make this work for a list
			if ((*i)->IsResponse())
			{	std::map<unsigned int, std::shared_ptr<CCommandOnTheFly>>::iterator X(m_CurrentCommands.find((*i)->MessageID));
				if (X==m_CurrentCommands.end()) continue;
				// if ProcessResponse() returns true then the whole command (set) is complete
				if (X->second->ProcessResponse(CL))
				{	if (X->second->CommandCompleteHandler)
					{	X->second->CommandCompleteHandler(system::error_code(), X->second.get());
					}
					m_CurrentCommands.erase(X);
				}
			}
			else
			{	// TODO: REMOVE
				// Code for Testing only
				// ack everything
				//TRACE(L"\tReply to " + CL.front()->MessageType + L"\n");
#ifdef MASSHUNTER_MANAGER_COMMUNICATION_TEST
				SendCommandAck(CL.front());
#else
				m_ReceivedCommands.insert(std::make_pair((*i)->MessageID, std::shared_ptr<CReceivedCommandOnTheFly>(new CReceivedCommandOnTheFly(*i, (*i)->Client))));
				m_IoService.post(bind(&CMassHunterPeerConnection::DoReceivedCommands, shared_from_this()));
#endif
			}
		}
	}
	else
	{	// stream name mismatch?
		m_Socket->shutdown(asio::ip::tcp::socket::shutdown_both);
		m_Socket->close();
	}
	CoUninitialize();
	return false;
}


bool CMassHunterPeerConnection::Status(const CString& ClientName, posix_time::time_duration Timeout)
{
	return CBasePeerConnection::Status(ClientName, Timeout);
}

bool CMassHunterPeerConnection::StatusIsIdle(const CString& ClientName, posix_time::time_duration Timeout)
{
	CBaseResourceStatusInfo* RSI = dynamic_cast<CBaseResourceStatusInfo*>(m_PeerStatus.EntryWithName(ClientName));
	if (RSI)
	{	// TODO: use Status codes
		//       Upper/lower case
		CString T(RSI->StatusAsText);
		T.MakeLower();
		if (T!=L"idle") return false;
	}
	else
	{	TRACE(L"CMassHunterPeerConnection::StatusIsIdle: No resource information for '%s'\n", ClientName);
	}
	return CBasePeerConnection::StatusIsIdle(ClientName, Timeout);
}

bool CMassHunterPeerConnection::StatusIsIdle(posix_time::time_duration Timeout)
{
	CBaseResourceStatusInfo* RSI = dynamic_cast<CBaseResourceStatusInfo*>(m_PeerStatus.EntryWithName(PeerName()));
	if (RSI)
	{	// TODO: use Status codes
		//       Upper/lower case
		CString T(RSI->StatusAsText);
		T.MakeLower();
		if (T!=L"idle") return false;
	}
	else
	{	TRACE(L"CMassHunterPeerConnection::StatusIsIdle: No resource information for '%s'\n", PeerName());
	}
	return CBasePeerConnection::StatusIsIdle(PeerName(), Timeout);
}

CString CMassHunterPeerConnection::StatusAsText(const CString& ClientName)
{
	CBaseResourceStatusInfo* RSI = dynamic_cast<CBaseResourceStatusInfo*>(m_PeerStatus.EntryWithName(ClientName));
	if (RSI)
	{	return RSI->StatusAsText;
	}
	return CBasePeerConnection::StatusAsText(ClientName);
}

CString CMassHunterPeerConnection::StatusAsText()
{
	CBaseResourceStatusInfo* RSI = dynamic_cast<CBaseResourceStatusInfo*>(m_PeerStatus.EntryWithName(PeerName()));
	if (RSI)
	{	return RSI->StatusAsText;
	}
	return CBasePeerConnection::StatusAsText(PeerName());
}

bool CMassHunterPeerConnection::Activate(posix_time::time_duration Timeout)
{
//	TRACE(L"CMassHunterPeerConnection::Activate\n");
	std::shared_ptr<CBaseCommand> VC(new CVersionCommand());
	return SendCommand(VC, Timeout,
						bind(&CMassHunterPeerConnection::VersionCompleteHandler, shared_from_this(),
							 placeholders::_1, placeholders::_2));
}

void CMassHunterPeerConnection::VersionCompleteHandler(system::error_code Error, CCommandOnTheFly* Command)
{
	TRACE(L"CMassHunterPeerConnection::VersionCompleteHandler: %u\n", Error.value());
	if (Error)
	{	m_ErrorText = Utf8ToCString(Error.message());
		// recursion!! DispatchError(Error);
		return;
	}
	_ASSERTE(Command);
	_ASSERTE(!Command->Responses.empty());
	CVersionCommand* VC = dynamic_cast<CVersionCommand*>(Command->Responses.back().get());
	// do some Version depending stuff
	std::shared_ptr<CBaseCommand> RSC(new CResourceStatusCommand());
	CResourceStatusCommand* X = dynamic_cast<CResourceStatusCommand*>(RSC.get());
	X->List.push_back(std::shared_ptr<CBaseResourceStatusInfo>(new CBaseResourceStatusInfo()));
	SendCommand(RSC, posix_time::seconds(5),
				bind(&CMassHunterPeerConnection::ResourcesCompleteHandler, shared_from_this(),
					 placeholders::_1, placeholders::_2));
}

void CMassHunterPeerConnection::ResourcesCompleteHandler(system::error_code Error, CCommandOnTheFly* Command)
{
	//TRACE(L"CMassHunterPeerConnection::ResourcesCompleteHandler: %u\n", Error.value());
	if (Error)
	{	m_ErrorText = Utf8ToCString(Error.message());
		// recursion!! DispatchError(Error);
		return;
	}
	_ASSERTE(Command);
	_ASSERTE(!Command->Responses.empty());
	CResourceStatusCommand* RC = dynamic_cast<CResourceStatusCommand*>(Command->Responses.back().get());
	if (RC)
	{	if (m_PeerStatus.List.size() && m_PeerStatus.List.size()!=RC->List.size())
		{	// TODO: list of available stati changed
			// even if the size is the same - the names can be different
			TRACE(L"CMassHunterPeerConnection::ResourcesCompleteHandler: status list size changed\n");
		}
		m_PeerStatus = *RC;
	}
	// for now ..
	if (!m_StartupComplete) ActivateCompleteHandler(Error, Command);
}

void CMassHunterPeerConnection::ActivateCompleteHandler(system::error_code Error, CCommandOnTheFly* /*Command*/)
{
	TRACE(L"CMassHunterPeerConnection::ActivateCompleteHandler: %u\n", Error.value());
	if (Error)
	{	m_ErrorText = Utf8ToCString(Error.message());
		// recursion!! DispatchError(Error);
		if (m_PeerActivateHandler) m_PeerActivateHandler(Error, this);
		return;
	}
	if (m_PeerActivateHandler) m_PeerActivateHandler(Error, this);
	m_StartupComplete = true;
}


bool CMassHunterPeerConnection::Deactivate(posix_time::time_duration Timeout)
{
	TRACE(L"CMassHunterPeerConnection::Deactivate\n");
	m_StartupComplete = false;
	return StatusIsIdle(L"", Timeout);
}

bool CMassHunterPeerConnection::Abort()
{
	CBasePeerConnection::Abort();
	if (m_Socket->is_open())
	{	m_Socket->shutdown(asio::ip::tcp::socket::shutdown_both);
		m_Socket->close();
	}
	return true;
}

void CMassHunterPeerConnection::SecondTick(posix_time::ptime& Now, posix_time::time_duration& /*Delta*/, bool SelfTiming)
{
	if ((Now - m_LastResourceRequest)>posix_time::seconds(5))
	{
		std::shared_ptr<CBaseCommand> RSC(new CResourceStatusCommand());
		CResourceStatusCommand* X = dynamic_cast<CResourceStatusCommand*>(RSC.get());
		X->List.push_back(std::shared_ptr<CBaseResourceStatusInfo>(new CBaseResourceStatusInfo()));
		SendCommand(RSC, posix_time::seconds(5),
					bind(&CMassHunterPeerConnection::ResourcesCompleteHandler, shared_from_this(),
						 placeholders::_1, placeholders::_2));
		m_LastResourceRequest = Now;
	}
	CBasePeerConnection::SecondTick(system::error_code(), SelfTiming);
}

bool CMassHunterPeerConnection::SendCommand(std::shared_ptr<CBaseCommand>& Command, posix_time::time_duration Timeout,
												 CommandResponseHandler CommandCompleteHandler)
{
//	TRACE(L"\tCMassHunterPeerConnection::SendCommand\n");
	Command->MessageID = m_NextMessageID++;
	std::list<std::shared_ptr<CBaseCommand>> CL;
	Command->Client = m_ClientName;
	CL.push_back(Command);
	return SendCommandList(CL, Timeout, CommandCompleteHandler);
}

bool CMassHunterPeerConnection::SendCommandList(std::list<std::shared_ptr<CBaseCommand>>& Commands, posix_time::time_duration Timeout,
													 CommandResponseHandler CommandCompleteHandler)
{
//	TRACE(L"\tCMassHunterPeerConnection::SendCommandList\n");
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_ErrorText.Empty();
	if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) return false;
	std::pair<std::map<unsigned int, std::shared_ptr<CCommandOnTheFly>>::iterator, bool>
		X(m_CurrentCommands.insert(std::make_pair(Commands.front()->MessageID, std::shared_ptr<CCommandOnTheFly>(new CCommandOnTheFly(Commands, Timeout,
																												CommandCompleteHandler)))));
	X.first->second->ClientName = m_PeerVersion.Client;
	++Commands.front()->NumberOfSends;

	if (m_IoService.stopped())
	{	if (CommandCompleteHandler)
		{	boost::system::error_code Error(system::errc::connection_aborted, system::generic_category());
			CommandCompleteHandler(Error, X.first->second.get());
		}
		m_CurrentCommands.erase(X.first);
		return false;
	}

	m_IoService.post(bind(&CMassHunterPeerConnection::DoWrite,
							shared_from_this(),
							new std::string(Commands.front()->ToXmlString(Commands.front()->MessageID))));
	CoUninitialize();
	if (m_Logger.IsEnabled()) m_Logger.LogText(L"S: " + Commands.front()->Dump());
	return true;
}

bool CMassHunterPeerConnection::SendCommandReply(std::shared_ptr<CBaseCommand>& Command, bool AckReply, CommandResponseHandler Handler)
{
//	TRACE(L"\tCMassHunterPeerConnection::SendCommandReply\n");
	if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) return false;
	m_ErrorText.Empty();
	if (AckReply) Command->MessageType = CBaseCommand::MessageTypeACKReply;
	else Command->MessageType = CBaseCommand::MessageTypeReply;
	Command->Client = m_ClientName;
	++Command->NumberOfSends;

	std::list<std::shared_ptr<CBaseCommand>> CL;
	CL.push_back(Command);
	m_IoService.post(std::bind(&CMassHunterPeerConnection::DoWrite,
										shared_from_this(),
										new std::string(CL.front()->AsReplyToXmlString(Command->MessageID))));
	CoUninitialize();
	if (m_Logger.IsEnabled()) m_Logger.LogText(L"SR: " + CL.front()->Dump());
	return true;
}

bool CMassHunterPeerConnection::SendCommandAck(std::shared_ptr<CBaseCommand>& Command, bool Ack, bool ReplyWillFollow, CommandResponseHandler Handler)
{
//	TRACE(L"\tCMassHunterPeerConnection::SendCommandAck\n");
	if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) return false;
	m_ErrorText.Empty();
	_ASSERTE(Command->MessageType!=CBaseCommand::MessageTypeACK);
	_ASSERTE(Command->MessageType!=CBaseCommand::MessageTypeACKReply);
	_ASSERTE(Command->MessageType!=CBaseCommand::MessageTypeNACK);
	if (Ack)
	{	Command->MessageType = ReplyWillFollow?CBaseCommand::MessageTypeACK:CBaseCommand::MessageTypeACKReply;
	}
	else
	{	Command->MessageType = CBaseCommand::MessageTypeNACK;
	}
	Command->Client = m_ClientName;
	++Command->NumberOfSends;
	std::list<std::shared_ptr<CBaseCommand>> CL;
	CL.push_back(Command);
	m_IoService.post(bind(&CMassHunterPeerConnection::DoWrite,
										shared_from_this(),
										new std::string(CL.front()->AsReplyToXmlString(Command->MessageID))));
	CoUninitialize();
	if (m_Logger.IsEnabled()) m_Logger.LogText(L"SA: " + CL.front()->Dump());
	return true;
}

void CMassHunterPeerConnection::DoReceivedCommands()
{
//	TRACE(L"CMassHunterPeerConnection::DoReceivedCommands\n");
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	int NextProcessingInMS = 20;	// TODO: dynamic timing
	for (std::map<unsigned int, std::shared_ptr<CReceivedCommandOnTheFly>>::iterator i(m_ReceivedCommands.begin());i!=m_ReceivedCommands.end();)
	{	
#ifdef MassHunter_COMMUNICATION_TEST
		if (i->second->State==CReceivedCommandOnTheFly::CommandReceived)
		{	SendCommandAck(i->second->Command, true, true);
			i->second->State = CReceivedCommandOnTheFly::CommandAckSent;
		}
		else if (i->second->State==CReceivedCommandOnTheFly::CommandAckSent)
		{	SendCommandReply(i->second->Command);
			i->second->State = CReceivedCommandOnTheFly::CommandReplySent;
			i = m_ReceivedCommands.erase(i);
			continue;
		}
#else
		if (CVersionCommand* VC = dynamic_cast<CVersionCommand*>(i->second->Command.get()))
		{	m_PeerVersion = *VC;
			SendCommandAck(i->second->Command, true);
			i->second->State = CReceivedCommandOnTheFly::CommandAckReplySent;
			// we sent a Version already after connect
			/*shared_ptr<CBaseCommand> X(new CVersionCommand());
			dynamic_cast<CVersionCommand*>(X.get())->SetThisProgram();
			SendCommand(X, posix_time::seconds(5));*/
			i = m_ReceivedCommands.erase(i);
			continue;
		}
		else if (CKeepAliveCommand* KALC = dynamic_cast<CKeepAliveCommand*>(i->second->Command.get()))
		{	if (m_PeerVersion.Client.IsEmpty())
			{	m_PeerVersion.Client = KALC->Client;
			}
			SendCommandAck(i->second->Command, true);
			i->second->State = CReceivedCommandOnTheFly::CommandAckReplySent;
			i = m_ReceivedCommands.erase(i);
			continue;
		}
		else if (CAbortCommand* AC = dynamic_cast<CAbortCommand*>(i->second->Command.get()))
		{	if (m_PeerAbortHandler)
			{	m_PeerAbortHandler();
			}
			SendCommandAck(i->second->Command, true);
			i->second->State = CReceivedCommandOnTheFly::CommandAckReplySent;
			i = m_ReceivedCommands.erase(i);
			continue;
		}
		else if (CMethodCommand* MC = dynamic_cast<CMethodCommand*>(i->second->Command.get()))
		{	
			if (!m_PeerRunMethodHandler)
			{	std::shared_ptr<CBaseCommand> X(new CBaseError(*i->second->Command, L"Impossible! No run method handler.", 999));
				SendCommandAck(X, false);
				i->second->State = CReceivedCommandOnTheFly::CommandAckReplySent;
				i = m_ReceivedCommands.erase(i);
				continue;
			}
			// we run a method, check in which state we are
			switch (MC->RunState)
			{	
				case	CMethodCommand::RunState_CommandReceived	:
							{	SendCommandAck(i->second->Command, true, true);
								MC->ErrorText = MC->ProjectPath;
								// ErrorText will contain the error text on return
								MC->Success = m_PeerRunMethodHandler(MC->ErrorText, MC->MasterBatch, MC->MethodName,
																MC->DataPath, MC->LabwareList, boost::posix_time::seconds(MC->EstimateTime));
								if (MC->Success)
								{	MC->RunState = CMethodCommand::RunState_MethodStarted;
									MC->ErrorCode = 0;
									MC->ErrorText.Empty();
									i->second->Timeout = boost::posix_time::seconds(3600);	// TODO: dynamic timeout
								}
								else
								{	MC->ErrorCode = 1;
									MC->RunState = CMethodCommand::RunState_MethodError;
								}
							}
							break;
				case	CMethodCommand::RunState_MethodStarted	:
							{
								if (!m_PeerStatusListHandler)
								{	MC->RunState = CMethodCommand::RunState_MethodFinished;
									break;
								}

								MC->Success = m_PeerStatusListHandler(MC->ErrorText, MC->ErrorCode);
								if (!MC->Success)
								{	
									MC->RunState = CMethodCommand::RunState_MethodError;
								}
								else
								{	// TODO: use status codes instead of text
									if (MC->ErrorText==L"Idle")
									{	MC->RunState = CMethodCommand::RunState_MethodFinished;
										MC->ErrorCode = 0;
										MC->ErrorText.Empty();
									}
									else if (MC->ErrorText==L"Busy")
									{
									}
									else
									{	MC->RunState = CMethodCommand::RunState_MethodError;
									}
								}
							}
							break;
				case	CMethodCommand::RunState_MethodFinished	:
							{
								MC->Success = true;
								SendCommandReply(i->second->Command, false);
								i->second->State = CReceivedCommandOnTheFly::CommandAckReplySent;
								i = m_ReceivedCommands.erase(i);
								continue;
							}
							break;
				case	CMethodCommand::RunState_MethodError	:
							{
								MC->Success = false;
								if (!MC->ErrorCode) MC->ErrorCode = 9;
								if (MC->ErrorText.IsEmpty()) MC->ErrorText = L"TODO";
								SendCommandReply(i->second->Command, false);
								i->second->State = CReceivedCommandOnTheFly::CommandAckReplySent;
								i = m_ReceivedCommands.erase(i);
								continue;
							}
							break;
				default	:
							{	_ASSERTE(false);
							}
							break;
			}
		}
		else if (CBaseResourceCommand* RC = dynamic_cast<CBaseResourceCommand*>(i->second->Command.get()))
		{	if (CResourceStatusCommand* RSC = dynamic_cast<CResourceStatusCommand*>(RC))
			{	std::shared_ptr<CBaseResourceStatusInfo> BRSI1(new CBaseResourceStatusInfo(L"Instrument", L"Idle", 5));
				// TODO: remove the next line
				std::shared_ptr<CBaseResourceStatusInfo> BRSI2(new CBaseResourceStatusInfo(L"Rudolf", L"Busy", 5));
				if (m_PeerStatusListHandler)
				{
					RSC->Success = m_PeerStatusListHandler(BRSI1->StatusAsText, BRSI1->StatusCode);
					if (!RSC->Success)
					{
						RSC->ErrorText = BRSI1->StatusAsText;
						RSC->ErrorCode = BRSI1->StatusCode;
					}
				}
				if (RSC->List.empty()) RSC->List.push_back(BRSI1);
				else RSC->List.front() = BRSI1;
				RSC->List.push_back(BRSI2);
				SendCommandAck(i->second->Command, true);
				i->second->State = CReceivedCommandOnTheFly::CommandAckReplySent;
			}
			else if (CResourceProjectsCommand* RPC = dynamic_cast<CResourceProjectsCommand*>(RC))
			{	std::shared_ptr<CBaseResourceProjectInfo> BRPI(new CBaseResourceProjectInfo(L"Instrument"));
				if (m_PeerProjectListHandler)
				{
					RPC->Success = m_PeerProjectListHandler(BRPI->Projects);
					if (!RPC->Success)
					{
						if (!BRPI->Projects.empty())
						{	RPC->ErrorText = BRPI->Projects.front();
							RPC->ErrorCode = 1;
							BRPI->Projects.clear();
						}
					}
				}
				if (RPC->List.empty()) RPC->List.push_back(BRPI);
				else RPC->List.front() = BRPI;
				SendCommandAck(i->second->Command, true);
				i->second->State = CReceivedCommandOnTheFly::CommandAckReplySent;
			}
			else if (CResourceMethodsCommand* RMC = dynamic_cast<CResourceMethodsCommand*>(RC))
			{	CString Project;
				if (!RMC->List.empty())
				{	
					CBaseResourceMethodInfo* MI = dynamic_cast<CBaseResourceMethodInfo*>(RMC->List.front().get());
					if (MI) Project = MI->Project;
				}
				else Project = L"Missing project name";
				std::shared_ptr<CBaseResourceMethodInfo> BRMI(new CBaseResourceMethodInfo(L"Instrument", Project));
				if (m_PeerMethodListHandler)
				{
					RMC->Success = m_PeerMethodListHandler(BRMI->Project, BRMI->Methods);
					if (!RMC->Success)
					{
						if (!BRMI->Methods.empty())
						{	RMC->ErrorText = BRMI->Methods.front();
							RMC->ErrorCode = 1;
							BRMI->Methods.clear();
						}
					}
				}
				if (RMC->List.empty()) RMC->List.push_back(BRMI);
				else RMC->List.front() = BRMI;
				SendCommandAck(i->second->Command, true);
				i->second->State = CReceivedCommandOnTheFly::CommandAckReplySent;
			}
			else if (CResourceMasterBatchesCommand* RMBC = dynamic_cast<CResourceMasterBatchesCommand*>(RC))
			{	CString Project;
				if (!RMBC->List.empty())
				{	
					CBaseResourceMasterBatchInfo* MBI = dynamic_cast<CBaseResourceMasterBatchInfo*>(RMBC->List.front().get());
					if (MBI) Project = MBI->Project;
				}
				else Project = L"Missing project name";
				std::shared_ptr<CBaseResourceMasterBatchInfo> BRMBI( new CBaseResourceMasterBatchInfo(L"Instrument", Project));
				if (m_PeerMasterBatchListHandler)
				{
					RMBC->Success = m_PeerMasterBatchListHandler(BRMBI->Project, BRMBI->MasterBatches);
					if (!RMBC->Success)
					{
						if (!BRMBI->MasterBatches.empty())
						{	RMBC->ErrorText = BRMBI->MasterBatches.front();
							RMBC->ErrorCode = 1;
							BRMBI->MasterBatches.clear();
						}
					}
				}
				if (RMBC->List.empty()) RMBC->List.push_back(BRMBI);
				else RMBC->List.front() = BRMBI;
				SendCommandAck(i->second->Command, true);
				i->second->State = CReceivedCommandOnTheFly::CommandAckReplySent;
			}
			else if (CResourceCommand* R = dynamic_cast<CResourceCommand*>(RC))
			{	// what? a test?
				SendCommandAck(i->second->Command, false);
				i->second->State = CReceivedCommandOnTheFly::CommandAckReplySent;
			}
			i = m_ReceivedCommands.erase(i);
			continue;
		}
		else if (CBaseCommand* BC = dynamic_cast<CBaseCommand*>(i->second->Command.get()))
		{	// last chance if we failed to decode the inner command
			std::shared_ptr<CBaseCommand> E(new CBaseError(*BC, L"Unknown Command", 2));
			SendCommandAck(E, false);
			i->second->State = CReceivedCommandOnTheFly::CommandAckReplySent;
			i = m_ReceivedCommands.erase(i);
			continue;
		}
		else
		{
			_ASSERTE(false);
		}
#endif
		++i;
	}
	if (!m_ReceivedCommands.empty())
	{	m_CommandProcessingTimer.expires_from_now(posix_time::milliseconds(NextProcessingInMS));
		m_CommandProcessingTimer.async_wait(std::bind(&CMassHunterPeerConnection::DoReceivedCommands, shared_from_this()));
	}
}

void CMassHunterPeerConnection::SendCommandToName(const CString& ClientName,
										std::shared_ptr<CBaseCommand>& Command, 
										posix_time::time_duration Timeout,
										CommandHandler::CommandResponseHandler CommandCompleteHandler)
{	
	if (m_PeerVersion.Client!=ClientName && !m_PeerStatus.HasEntryWithName(ClientName))
	{	if (CommandCompleteHandler)
		{	system::error_code Error(system::errc::operation_canceled, system::generic_category());
			m_IoService.post(bind(CommandCompleteHandler, Error, (CCommandOnTheFly*)NULL));
		}
		return;
	}
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);	// TODO: remove if we get to know the real Status
	SendCommand(Command, Timeout, CommandCompleteHandler);
	m_CurrentCommands.rbegin()->second->ClientName = ClientName; // TODO: remove if we get to know the real Status
}

bool CMassHunterPeerConnection::RunMethod(CString ProjectPath, CString MasterBatch, CString MethodName,
												CString DataPath,
												std::list<CommandHandler::CPositionData>& Labware,
												boost::posix_time::time_duration Timeout)
{
//	TRACE(L"CMassHunterPeerConnection::RunMethod\n");
	std::shared_ptr<CBaseCommand> TC(new CMethodCommand(ProjectPath, MasterBatch, MethodName, DataPath, Labware));
	return SendCommand(TC, Timeout,
						std::bind(&CMassHunterPeerConnection::RunMethodCompleteHandler, shared_from_this(),
						std::placeholders::_1, std::placeholders::_2));
}

void CMassHunterPeerConnection::RunMethodCompleteHandler(system::error_code Error, CCommandOnTheFly* Command)
{
	TRACE(L"CMassHunterPeerConnection::RunMethodCompleteHandler: %u\n", Error.value());
	if (Error)
	{	m_ErrorText = Utf8ToCString(Error.message());
		return;
	}
	_ASSERTE(Command);
	_ASSERTE(!Command->Responses.empty());
	// TODO: RunMethod Command Response
	CMethodCommand* MC = dynamic_cast<CMethodCommand*>(Command->Responses.back().get());
	//_ASSERTE(MC);
}

void CMassHunterPeerConnection::SetPeerTimeoutHandler(PeerTimeoutHandler Handler)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_PeerTimeoutHandler = Handler;
}

void CMassHunterPeerConnection::SetPeerErrorHandler(PeerErrorHandler Handler)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_PeerErrorHandler = Handler;
}

void CMassHunterPeerConnection::SetPeerActivateHandler(PeerActivateHandler Handler)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_PeerActivateHandler = Handler;
}

void CMassHunterPeerConnection::SetPeerProjectListHandler(PeerProjectListHandler Handler)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_PeerProjectListHandler = Handler;
}

void CMassHunterPeerConnection::SetPeerMethodListHandler(PeerMethodListHandler Handler)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_PeerMethodListHandler = Handler;
}

void CMassHunterPeerConnection::SetPeerMasterBatchListHandler(PeerMasterBatchListHandler Handler)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_PeerMasterBatchListHandler = Handler;
}

void CMassHunterPeerConnection::SetPeerStatusListHandler(PeerStatusListHandler Handler)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_PeerStatusListHandler = Handler;
}

void CMassHunterPeerConnection::SetPeerRunMethodHandler(PeerRunMethodHandler Handler)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_PeerRunMethodHandler = Handler;
}

void CMassHunterPeerConnection::SetPeerAbortHandler(PeerAbortHandler Handler)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_PeerAbortHandler = Handler;
}


//------------------------------------------------------------------------------------------------

CMassHunterServer::CMassHunterServer(asio::io_service& IoService, CString IP, unsigned short Port, CString ClientName)
	: m_ErrorText(L"CMassHunterServer: Unknown Error")
	, m_IoService(IoService)
	, m_Abort(false)
	, m_Logger(L"")
	, m_Acceptor(IoService, tcp::endpoint(address::from_string(CStringToUtf8(IP)), Port))
	, m_LastSecondTick(posix_time::microsec_clock::local_time())
	, m_Timer(IoService)
	, m_ClientName(ClientName)
	, m_PeerTimeoutHandler(NULL)
	, m_PeerErrorHandler(NULL)
	, m_PeerProjectListHandler(NULL)
	, m_PeerMethodListHandler(NULL)
	, m_PeerMasterBatchListHandler(NULL)
	, m_PeerStatusListHandler(NULL)
	, m_PeerRunMethodHandler(NULL)
	, m_PeerAbortHandler(NULL)
{
	TRACE(L"CMassHunterServer::CMassHunterServer\n");
	m_Logger.NewLogFileName(L"MassHunter.Server." + m_ClientName + L".log.txt");
	m_Logger.LogText(L"Server object created");
}

CMassHunterServer::~CMassHunterServer(void)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	TRACE(L"CMassHunterServer::~CMassHunterServer\n");
	for (std::list<CMassHunterPeerConnectionPtr>::iterator i(m_Connections.begin());i!=m_Connections.end();++i)
	{
		i->reset();
	}
	m_Logger.LogText(L"Server object destructed");
}

void CMassHunterServer::Start()
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	CMassHunterPeerConnectionPtr X(new CMassHunterPeerConnection(shared_from_this(), m_ClientName));
	m_Connections.push_back(X);
	m_Acceptor.async_accept(X->Socket(), bind(&CMassHunterServer::HandleAccept, shared_from_this(),
							asio::placeholders::error));
	m_Timer.expires_from_now(posix_time::seconds(1));
	m_Timer.async_wait(bind(&CMassHunterServer::SecondTick, shared_from_this(), placeholders::_1, true));
}

void CMassHunterServer::ClearHandler()
{
	m_PeerTimeoutHandler = NULL;
	m_PeerErrorHandler = NULL;
	m_PeerProjectListHandler = NULL;
	m_PeerMethodListHandler = NULL;
	m_PeerMasterBatchListHandler = NULL;
	m_PeerStatusListHandler = NULL;
	m_PeerRunMethodHandler = NULL;
	m_PeerAbortHandler = NULL;
}

void CMassHunterServer::HandleAccept(const system::error_code& error)
{
	TRACE(L"CMassHunterServer::HandleAccept = %d\n", error.value());
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	if (error)
  {
		m_ErrorText = Utf8ToCString(error.message());
		// Aborted/Socket closed
		if (error.value()==ERROR_OPERATION_ABORTED || m_Abort) return;
	}
	else
	{	if (m_Logger.IsEnabled()) m_Logger.LogText(L"Accept connection from " + Utf8ToCString(m_Connections.back()->Socket().remote_endpoint().address().to_string()));
		CMassHunterPeerConnection& X = *m_Connections.back();
		X.Start(false);
		X.m_IsConnecting = false;
		if (m_PeerTimeoutHandler) X.SetPeerTimeoutHandler(m_PeerTimeoutHandler);
		if (m_PeerErrorHandler) X.SetPeerErrorHandler(m_PeerErrorHandler);
		if (m_PeerProjectListHandler) X.SetPeerProjectListHandler(m_PeerProjectListHandler);
		if (m_PeerMethodListHandler) X.SetPeerMethodListHandler(m_PeerMethodListHandler);
		if (m_PeerMasterBatchListHandler) X.SetPeerMasterBatchListHandler(m_PeerMasterBatchListHandler);
		if (m_PeerStatusListHandler) X.SetPeerStatusListHandler(m_PeerStatusListHandler);
		if (m_PeerRunMethodHandler) X.SetPeerRunMethodHandler(m_PeerRunMethodHandler);
		if (m_PeerAbortHandler) X.SetPeerAbortHandler(m_PeerAbortHandler);

		CMassHunterPeerConnectionPtr Peer(new CMassHunterPeerConnection(shared_from_this(), m_ClientName));
		m_Connections.push_back(Peer);
	}
	m_Acceptor.async_accept(m_Connections.back()->Socket(), bind(&CMassHunterServer::HandleAccept, shared_from_this(),
															asio::placeholders::error));
}

bool CMassHunterServer::Abort()
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	TRACE(L"CMassHunterServer::Abort\n");
	m_ErrorText.Empty();
	m_Abort = true;
	m_Timer.cancel();
	m_Acceptor.close();
	for (std::list<CMassHunterPeerConnectionPtr>::iterator i(m_Connections.begin());i!=m_Connections.end();++i)
	{
		(*i)->Abort();
	}
	m_Connections.clear();
	return true;
}

CMassHunterPeerConnectionPtr CMassHunterServer::Connect(CString Ip, unsigned short Port, CString ClientName)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	CMassHunterPeerConnectionPtr X(new CMassHunterPeerConnection(shared_from_this(), Ip, Port, ClientName));
	m_Connections.push_back(X);
	X->Connect();
	return X;
}

void CMassHunterServer::RemovePeer(const boost::system::error_code& Error, CMassHunterPeerConnection* Peer)
{
	TRACE(L"CMassHunterServer::RemovePeer\n");
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	for (std::list<CMassHunterPeerConnectionPtr>::iterator i(m_Connections.begin());i!=m_Connections.end();++i)
	{	
		CMassHunterPeerConnectionPtr X(*i);
		if (X.get()==Peer)
		{	m_Connections.erase(i);
			X->Abort();
			if (X->m_PeerErrorHandler)
			{	X->m_PeerErrorHandler(Error, X.get());
			}
			break;
		}
	}
}

void CMassHunterServer::SecondTick(system::error_code Error, bool SelfTiming)
{
	if (Error.value()==ERROR_OPERATION_ABORTED || m_Abort) return;
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	if (SelfTiming)
	{	m_Timer.expires_from_now(posix_time::seconds(1));
		m_Timer.async_wait(bind(&CMassHunterServer::SecondTick, shared_from_this(), placeholders::_1, SelfTiming));
	}
	posix_time::ptime Now(posix_time::microsec_clock::local_time());
	posix_time::time_duration Delta = Now - m_LastSecondTick;
	m_LastSecondTick = Now;
	for (std::list<CMassHunterPeerConnectionPtr>::iterator i(m_Connections.begin());i!=m_Connections.end();)
	{
		if ((*i)->IsConnecting())
		{	++i;
			continue;
		}
		(*i)->SecondTick(Now, Delta, false);
		if ((*i)->IsTimeout(Now))
		{	if ((*i)->m_PeerTimeoutHandler)
			{	system::error_code Err(system::errc::timed_out, system::generic_category());
				(*i)->m_PeerTimeoutHandler(Err, i->get());
			}
			// this will result in a call to RemovePeer
			(*i)->Abort();
			// TODO: reconnect?
			i = m_Connections.erase(i);
		}
		if ((*i)->NeedsKeepAlive(Now))
		{	std::shared_ptr<CBaseCommand> KALC(new CKeepAliveCommand());
			(*i)->SendCommand(KALC, posix_time::seconds(5));
			(*i)->KeepAlive();
			++i;
		}
		else ++i;
	}
}

void CMassHunterServer::DataFeedback(char* /*Data*/, size_t /*DatenLength*/, CMassHunterPeerConnectionPtr /*Peer*/)
{
//	TRACE(L"CMassHunterServer::DataFeedback\n");
}

std::list<CString> CMassHunterServer::GetMassHunterNameList()
{	
	std::list<CString> L;
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	for (std::list<CMassHunterPeerConnectionPtr>::iterator i(m_Connections.begin());i!=m_Connections.end();++i)
	{	if (!(*i)->m_IsConnected)
		{	continue;
		}
		L.push_back((*i)->PeerName());
	}
	return L;
}

CMassHunterPeerConnectionPtr CMassHunterServer::GetClientByName(CString ClientName)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	for (std::list<CMassHunterPeerConnectionPtr>::iterator i(m_Connections.begin());i!=m_Connections.end();++i)
	{	if (ClientName==(*i)->PeerName()) return *i;
	}
	return CMassHunterPeerConnectionPtr();
}

void CMassHunterServer::SendCommandToName(const CString& ClientName,
										std::shared_ptr<CommandHandler::CBaseCommand>& Command, 
										posix_time::time_duration Timeout,
										CommandResponseHandler CommandCompleteHandler)
{
	CMassHunterPeerConnectionPtr X(GetClientByName(ClientName));
	if (X) X->SendCommandToName(ClientName, Command, Timeout, CommandCompleteHandler);
	else
	{	if (CommandCompleteHandler)
		{	system::error_code Error(system::errc::operation_canceled, system::generic_category());
			IoService().post(std::bind(CommandCompleteHandler, Error, (CCommandOnTheFly*)NULL));
		}
	}
}

void CMassHunterServer::SetPeerTimeoutHandler(PeerTimeoutHandler Handler)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_PeerTimeoutHandler = Handler;
	for (std::list<CMassHunterPeerConnectionPtr>::iterator i(m_Connections.begin());i!=m_Connections.end();++i)
	{	(*i)->SetPeerTimeoutHandler(m_PeerTimeoutHandler);
	}
}

void CMassHunterServer::SetPeerErrorHandler(PeerErrorHandler Handler)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_PeerErrorHandler = Handler;
	for (std::list<CMassHunterPeerConnectionPtr>::iterator i(m_Connections.begin());i!=m_Connections.end();++i)
	{	(*i)->SetPeerErrorHandler(m_PeerErrorHandler);
	}
}

void CMassHunterServer::SetPeerProjectListHandler(PeerProjectListHandler Handler)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_PeerProjectListHandler = Handler;
	for (std::list<CMassHunterPeerConnectionPtr>::iterator i(m_Connections.begin());i!=m_Connections.end();++i)
	{	(*i)->SetPeerProjectListHandler(m_PeerProjectListHandler);
	}
}

void CMassHunterServer::SetPeerMethodListHandler(PeerMethodListHandler Handler)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_PeerMethodListHandler = Handler;
	for (std::list<CMassHunterPeerConnectionPtr>::iterator i(m_Connections.begin());i!=m_Connections.end();++i)
	{	(*i)->SetPeerMethodListHandler(m_PeerMethodListHandler);
	}
}

void CMassHunterServer::SetPeerMasterBatchListHandler(PeerMasterBatchListHandler Handler)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_PeerMasterBatchListHandler = Handler;
	for (std::list<CMassHunterPeerConnectionPtr>::iterator i(m_Connections.begin());i!=m_Connections.end();++i)
	{	(*i)->SetPeerMasterBatchListHandler(m_PeerMasterBatchListHandler);
	}
}

void CMassHunterServer::SetPeerStatusListHandler(PeerStatusListHandler Handler)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_PeerStatusListHandler = Handler;
	for (std::list<CMassHunterPeerConnectionPtr>::iterator i(m_Connections.begin());i!=m_Connections.end();++i)
	{	(*i)->SetPeerStatusListHandler(m_PeerStatusListHandler);
	}
}

void CMassHunterServer::SetPeerRunMethodHandler(PeerRunMethodHandler Handler)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_PeerRunMethodHandler = Handler;
	for (std::list<CMassHunterPeerConnectionPtr>::iterator i(m_Connections.begin());i!=m_Connections.end();++i)
	{	(*i)->SetPeerRunMethodHandler(m_PeerRunMethodHandler);
	}
}

void CMassHunterServer::SetPeerAbortHandler(PeerAbortHandler Handler)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_PeerAbortHandler = Handler;
	for (std::list<CMassHunterPeerConnectionPtr>::iterator i(m_Connections.begin());i!=m_Connections.end();++i)
	{	(*i)->SetPeerAbortHandler(m_PeerAbortHandler);
	}
}


CMassHunterPeerConnectionSimulation::CMassHunterPeerConnectionSimulation(std::shared_ptr<MassHunter::CMassHunterServer> MassHunter, CString IP, unsigned short Port, CString ClientName)
	: CMassHunterPeerConnection(MassHunter, IP, Port, ClientName)
{
	TRACE(L"CMassHunterPeerConnectionSimulation::CMassHunterPeerConnectionSimulation: A\n");
}

CMassHunterPeerConnectionSimulation::CMassHunterPeerConnectionSimulation(std::shared_ptr<asio::io_service> IoService, CString IP, unsigned short Port, CString ClientName, bool SilasLog)
	: CMassHunterPeerConnection(IoService, IP, Port, ClientName, SilasLog)
{
	TRACE(L"CMassHunterPeerConnectionSimulation::CMassHunterPeerConnectionSimulation: B\n");
}

CMassHunterPeerConnectionSimulation::CMassHunterPeerConnectionSimulation(std::shared_ptr<MassHunter::CMassHunterServer> MassHunter, CString ClientName)
	: CMassHunterPeerConnection(MassHunter, ClientName)
{
	TRACE(L"CMassHunterPeerConnectionSimulation::CMassHunterPeerConnectionSimulation: C\n");
}

bool CMassHunterPeerConnectionSimulation::SendCommand(std::shared_ptr<CommandHandler::CBaseCommand>& Command, 
										posix_time::time_duration Timeout,
										CommandHandler::CommandResponseHandler CommandCompleteHandler)
{
	if (CommandCompleteHandler)
	{	// TODO alles
		boost::system::error_code Error(boost::system::errc::success, boost::system::generic_category());
		std::list<std::shared_ptr<CBaseCommand>> CL;
		CL.push_back(Command);
		std::shared_ptr<CCommandOnTheFly> COTF(new CCommandOnTheFly(CL, Timeout, CommandCompleteHandler));
		for (std::list<std::shared_ptr<CBaseCommand>>::iterator C(COTF->Commands.begin());C!=COTF->Commands.end();++C)
		{	if (dynamic_cast<MassHunter::CResourceProjectsCommand*>(C->get()))
			{	std::shared_ptr<CBaseCommand> X(new MassHunter::CResourceProjectsCommand());
				MassHunter::CResourceProjectsCommand& RPC(*static_cast<MassHunter::CResourceProjectsCommand*>(X.get()));
				std::shared_ptr<CBaseResourceInfo> PI(new CBaseResourceProjectInfo(L"MH"));
				CBaseResourceProjectInfo& RPI(*static_cast<CBaseResourceProjectInfo*>(PI.get()));
				RPI.Projects.push_back(L"Sim Project A");
				RPI.Projects.push_back(L"Sim Project Z");
				RPC.List.push_back(PI);
				RPC.Success = true;
				COTF->Responses.push_back(X);
			}
			else if (MassHunter::CResourceMasterBatchesCommand* RMBC = dynamic_cast<MassHunter::CResourceMasterBatchesCommand*>(C->get()))
			{	CString ProjectName(L"Sim Default");
				if (RMBC->List.size())
				{	std::shared_ptr<CBaseResourceInfo> P = RMBC->List.front();
					if (CBaseResourceMasterBatchInfo* Y = dynamic_cast<CBaseResourceMasterBatchInfo*>(P.get()))
					{	ProjectName = Y->Project;
					}
				}
				std::shared_ptr<CBaseCommand> X(new MassHunter::CResourceMasterBatchesCommand());
				MassHunter::CResourceMasterBatchesCommand& MBC(*static_cast<MassHunter::CResourceMasterBatchesCommand*>(X.get()));
				std::shared_ptr<CBaseResourceInfo> MBI(new CBaseResourceMasterBatchInfo(L"MH", ProjectName));
				CBaseResourceMasterBatchInfo& RMBI(*static_cast<CBaseResourceMasterBatchInfo*>(MBI.get()));
				RMBI.MasterBatches.push_back(L"Sim Master Batch B");
				RMBI.MasterBatches.push_back(L"Sim Master Batch Y");
				MBC.List.push_back(MBI);
				MBC.Success = true;
				COTF->Responses.push_back(X);
			}
			// CResourceMasterBatchesCommand has to be tested before CMethodCommand
			else if (MassHunter::CMasterBatchCommand* MBC = dynamic_cast<MassHunter::CMasterBatchCommand*>(C->get()))
			{	std::shared_ptr<CBaseCommand> X(new MassHunter::CMasterBatchCommand(*MBC));
				MassHunter::CMasterBatchCommand& Y(*static_cast<MassHunter::CMasterBatchCommand*>(X.get()));
				Y.Success = true;
				COTF->Responses.push_back(X);
			}
			else if (MassHunter::CMethodCommand* MC = dynamic_cast<MassHunter::CMethodCommand*>(C->get()))
			{	std::shared_ptr<CBaseCommand> X(new MassHunter::CMethodCommand(*MC));
				MassHunter::CMethodCommand& Y(*static_cast<MassHunter::CMethodCommand*>(X.get()));
				Y.Success = false;
				Y.ErrorText = L"Method command is not supported";
				Y.ErrorCode = 3;
				COTF->Responses.push_back(X);
			}
			else if (dynamic_cast<MassHunter::CResourceStatusCommand*>(C->get()))
			{	std::shared_ptr<CBaseCommand> X(new MassHunter::CResourceStatusCommand());
				MassHunter::CResourceStatusCommand& RSC(*static_cast<MassHunter::CResourceStatusCommand*>(X.get()));
				std::shared_ptr<CBaseResourceInfo> SI(new CBaseResourceStatusInfo(L"MH"));
				CBaseResourceStatusInfo& RSI(*static_cast<CBaseResourceStatusInfo*>(SI.get()));
				RSI.StatusAsText = L"Idle";
				RSI.StatusCode = 0;
				RSC.List.push_back(SI);
				RSC.Success = true;
				COTF->Responses.push_back(X);
			}
			else
			{	std::shared_ptr<CBaseCommand> X(new CBaseError(**C, L"Unknown simulation command", 1));
				COTF->Responses.push_back(X);
			}
		}
		COTF->ProcessedCommands = COTF->Commands;
		COTF->Commands.clear();
		m_IoService.post(std::bind(&CMassHunterPeerConnectionSimulation::CallbackHandler,
							  this, COTF, Error, CommandCompleteHandler));
		if (m_IoService.stopped())
		{	m_IoService.restart();
			m_IoService.run();
		}
	}
	return true;
}

void CMassHunterPeerConnectionSimulation::CallbackHandler(std::shared_ptr<CCommandOnTheFly> PCOTF,
															system::error_code Error,
															CommandResponseHandler CommandCompleteHandler)
{	_ASSERTE(CommandCompleteHandler);
	CommandCompleteHandler(Error, PCOTF.get());
}


}	// namespace