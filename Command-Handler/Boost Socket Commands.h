#pragma once
#include "..\Command Handler\Base Socket Commands.hpp"
#include "Version Command.h"
#include "Resource Command.h"

#include <memory>


namespace MassHunter
{
class CMassHunterServer;
class CMassHunterPeerConnection;

typedef std::function<void (boost::system::error_code, CMassHunterPeerConnection*)> PeerTimeoutHandler;
typedef std::function<void (boost::system::error_code, CMassHunterPeerConnection*)> PeerErrorHandler;
typedef std::function<void (boost::system::error_code, CMassHunterPeerConnection*)> PeerActivateHandler;

typedef std::function<bool (std::list<CString>&)> PeerProjectListHandler;
typedef std::function<bool (const CString& Project, std::list<CString>&)> PeerMethodListHandler;
typedef std::function<bool (const CString& Project, std::list<CString>&)> PeerMasterBatchListHandler;
typedef std::function<bool (CString&, int&)> PeerStatusListHandler;
typedef std::function<bool ()> PeerAbortHandler;

// ProjectPath, MasterBatch, MethodName, DataPath, Labware, Timeout
// ProjectPath can contain the error text on return
typedef	boost::function<bool (CString&, const CString&, const CString&, const CString&,
						const std::list<CommandHandler::CPositionData>&, const boost::posix_time::time_duration)> PeerRunMethodHandler;


class CMassHunterPeerConnection : public CommandHandler::CBasePeerConnection
{
	friend CMassHunterServer;
public:

	CMassHunterPeerConnection(std::shared_ptr<MassHunter::CMassHunterServer> MassHunter, CString IP, unsigned short Port, CString ClientName);
	CMassHunterPeerConnection(std::shared_ptr<boost::asio::io_service> IoService, CString IP, unsigned short Port, CString ClientName, bool SilasLog = false);
	CMassHunterPeerConnection(std::shared_ptr<MassHunter::CMassHunterServer> MassHunter, CString ClientName);
	virtual ~CMassHunterPeerConnection();

	std::shared_ptr<CMassHunterPeerConnection> shared_from_this()
	{
		return std::dynamic_pointer_cast<CMassHunterPeerConnection>(CBasePeerConnection::shared_from_this());
	}

	virtual bool DataInterpreter(char* Data, size_t DatenLength);
	
	CString& ClientName()
	{
		return m_ClientName;
	}
	
	CString& PeerName()
	{
		return m_PeerVersion.Client;
	}
	
	boost::asio::ip::tcp::socket& Socket()
	{
		return *m_Socket;
	}
	
	bool IsConnected()
	{
		return m_IsConnected && !m_IsConnecting;
	}
	
	bool IsConnecting()
	{
		return m_IsConnecting;
	}

	CVersionCommand& ClientVersion()
	{	
		return m_PeerVersion;
	}

	boost::posix_time::ptime& LastContactTime()
	{ 
		return m_LastReceiveTime;
	}

	bool IsTimeout(boost::posix_time::ptime& Now)
	{
		return Now - m_LastReceiveTime > m_Settings.Timeout;
	}

	void KeepAlive()
	{
		m_LastKeepAliveTime = boost::posix_time::microsec_clock::local_time();
	}

	bool NeedsKeepAlive(boost::posix_time::ptime& Now)
	{
		return  (Now - m_LastKeepAliveTime > m_Settings.KeepAliveTimeout) 
					&& (Now - m_LastReceiveTime > m_Settings.KeepAliveTimeout);
	}

	virtual void Start(bool SelfTiming);
	virtual void Connect();
	virtual void ReConnect();
	virtual void SecondTick(boost::posix_time::ptime& Now, boost::posix_time::time_duration& Delta, bool SelfTiming);
	
	void SetPeerTimeoutHandler(PeerTimeoutHandler Handler);
	void SetPeerErrorHandler(PeerErrorHandler Handler);
	void SetPeerActivateHandler(PeerActivateHandler Handler);

	void SetPeerProjectListHandler(PeerProjectListHandler Handler);
	void SetPeerMethodListHandler(PeerMethodListHandler Handler);
	void SetPeerMasterBatchListHandler(PeerMasterBatchListHandler Handler);
	void SetPeerStatusListHandler(PeerStatusListHandler Handler);
	void SetPeerRunMethodHandler(PeerRunMethodHandler Handler);
	void SetPeerAbortHandler(PeerAbortHandler Handler);

	void SendCommandToName(const CString& ClientName,
										std::shared_ptr<CommandHandler::CBaseCommand>& Command, 
										boost::posix_time::time_duration Timeout,
										CommandHandler::CommandResponseHandler CommandCompleteH = 0);

	bool Activate(boost::posix_time::time_duration Timeout);
	void VersionCompleteHandler(boost::system::error_code Error, CommandHandler::CCommandOnTheFly* Command);
	void ResourcesCompleteHandler(boost::system::error_code Error, CommandHandler::CCommandOnTheFly* Command);
	void ActivateCompleteHandler(boost::system::error_code Error, CommandHandler::CCommandOnTheFly* Command);

	bool Deactivate(boost::posix_time::time_duration Timeout);

	bool RunMethod(CString ProjectPath, CString MasterBatch, CString MethodName,
									CString DataPath,
									std::list<CommandHandler::CPositionData>& Labware,
									boost::posix_time::time_duration Timeout);
	void RunMethodCompleteHandler(boost::system::error_code Error, CommandHandler::CCommandOnTheFly* Command);
	
	virtual bool Abort();

	bool Status(const CString& ClientName, boost::posix_time::time_duration Timeout);
	bool StatusIsIdle(const CString& ClientName, boost::posix_time::time_duration Timeout);
	bool StatusIsIdle(boost::posix_time::time_duration Timeout);
	CString StatusAsText(const CString& ClientName);
	CString StatusAsText();

	enum
	{
		ReceiveBufferSize = 1500 * 20
	};

	virtual bool SendCommand(std::shared_ptr<CommandHandler::CBaseCommand>& Command, 
										boost::posix_time::time_duration Timeout,
										CommandHandler::CommandResponseHandler CommandCompleteHandler = 0);
	virtual bool SendCommandList(std::list<std::shared_ptr<CommandHandler::CBaseCommand>>& Commands,
										boost::posix_time::time_duration Timeout,
										CommandHandler::CommandResponseHandler CommandCompleteHandler = 0);
	virtual bool SendCommandReply(std::shared_ptr<CommandHandler::CBaseCommand>& Command,
										bool AckReply = true,
										CommandHandler::CommandResponseHandler Handler = 0);
	virtual bool SendCommandAck(std::shared_ptr<CommandHandler::CBaseCommand>& Command,
										bool Ack = true, bool ReplyWillFollow = false,
										CommandHandler::CommandResponseHandler Handler = 0);

private:
	PeerProjectListHandler m_PeerProjectListHandler;
	PeerMethodListHandler m_PeerMethodListHandler;
	PeerMasterBatchListHandler m_PeerMasterBatchListHandler;
	PeerStatusListHandler m_PeerStatusListHandler;
	PeerRunMethodHandler m_PeerRunMethodHandler;
	PeerAbortHandler m_PeerAbortHandler;


	enum
	{
		ReadLength = 1,
		ReadData = 2
	};
	CVersionCommand m_PeerVersion;
	CResourceStatusCommand m_PeerStatus;

	std::shared_ptr<boost::asio::ip::tcp::socket> m_Socket;
	std::shared_ptr<MassHunter::CMassHunterServer> m_MassHunter;
	boost::asio::ip::tcp::endpoint m_Peer;
	PeerTimeoutHandler m_PeerTimeoutHandler;
	PeerErrorHandler m_PeerErrorHandler;
	PeerActivateHandler m_PeerActivateHandler;
	bool m_StartupComplete;


	char* m_ReceiveBuffer;
	int m_ReadState;
	size_t m_PacketLength;
	size_t m_BytesReceived;
	bool m_IsConnecting;
	bool m_IsConnected;
	boost::posix_time::ptime m_LastReceiveTime;
 	boost::posix_time::ptime m_LastKeepAliveTime;
 	boost::posix_time::ptime m_LastResourceRequest;

	void DoWrite(std::string* Message);

	void DoReceivedCommands();

	void HandleConnect(const boost::system::error_code& error);
	void HandleRead(const boost::system::error_code& error, size_t BytesTransferred);
  void HandleWrite(const boost::system::error_code& error, size_t BytesTransferred, std::string* Message);
};

typedef std::shared_ptr<CMassHunterPeerConnection> CMassHunterPeerConnectionPtr;


class CMassHunterServer : public std::enable_shared_from_this<CMassHunterServer>
{
public:
	CMassHunterServer(boost::asio::io_service& IoService, CString Ip, unsigned short Port = 1969, CString ClientName = L"MassHunter");
	virtual ~CMassHunterServer();

	CString& ErrorText()
	{
		return m_ErrorText;
	}
	boost::asio::io_service& IoService()
	{
		return m_IoService;
	}

	std::list<CString> GetMassHunterNameList();
	CMassHunterPeerConnectionPtr GetClientByName(CString ClientName);

	virtual bool Abort();
	virtual void Start();
	virtual void ClearHandler();
	virtual CMassHunterPeerConnectionPtr Connect(CString Ip, unsigned short Port = 1969, CString ClientName = L"");
	virtual void RemovePeer(const boost::system::error_code& Error, CMassHunterPeerConnection* Peer);
	virtual void SecondTick(boost::system::error_code Error, bool SelfTiming);
	virtual void DataFeedback(char* Data, size_t DatenLength, CMassHunterPeerConnectionPtr Peer);

	virtual void SetPeerTimeoutHandler(PeerTimeoutHandler Handler);
	virtual void SetPeerErrorHandler(PeerErrorHandler Handler);

	virtual void SetPeerProjectListHandler(PeerProjectListHandler Handler);
	virtual void SetPeerMethodListHandler(PeerMethodListHandler Handler);
	virtual void SetPeerMasterBatchListHandler(PeerMasterBatchListHandler Handler);
	virtual void SetPeerStatusListHandler(PeerStatusListHandler Handler);
	virtual void SetPeerRunMethodHandler(PeerRunMethodHandler Handler);
	virtual void SetPeerAbortHandler(PeerAbortHandler Handler);

	void SendCommandToName(const CString& ClientName,
										std::shared_ptr<CommandHandler::CBaseCommand>& Command, 
										boost::posix_time::time_duration Timeout,
										CommandHandler::CommandResponseHandler CommandCompleteH = 0);

	Logger::CFileLogger m_Logger;

private:
	PeerProjectListHandler m_PeerProjectListHandler;
	PeerMethodListHandler m_PeerMethodListHandler;
	PeerMasterBatchListHandler m_PeerMasterBatchListHandler;
	PeerStatusListHandler m_PeerStatusListHandler;
	PeerRunMethodHandler m_PeerRunMethodHandler;
	PeerTimeoutHandler m_PeerTimeoutHandler;
	PeerErrorHandler m_PeerErrorHandler;
	PeerAbortHandler m_PeerAbortHandler;

	CString m_ClientName;
	bool m_Abort;
	CString m_ErrorText;
	boost::posix_time::ptime m_LastSecondTick;
	boost::recursive_mutex m_Mutex;
	boost::asio::deadline_timer m_Timer;

	boost::asio::io_service& m_IoService;
	boost::asio::ip::tcp::acceptor m_Acceptor;
	std::list<CMassHunterPeerConnectionPtr> m_Connections;

	void HandleAccept(const boost::system::error_code& Error);
};

typedef std::shared_ptr<CMassHunterServer> CMassHunterServerPtr;

class CMassHunterPeerConnectionSimulation : public CMassHunterPeerConnection
{
public:
	CMassHunterPeerConnectionSimulation(std::shared_ptr<MassHunter::CMassHunterServer> MassHunter, CString IP, unsigned short Port, CString ClientName);
	CMassHunterPeerConnectionSimulation(std::shared_ptr<boost::asio::io_service> IoService, CString IP, unsigned short Port, CString ClientName, bool SilasLog = false);
	CMassHunterPeerConnectionSimulation(std::shared_ptr<MassHunter::CMassHunterServer> MassHunter, CString ClientName);

	virtual bool SendCommand(std::shared_ptr<CommandHandler::CBaseCommand>& Command, 
										boost::posix_time::time_duration Timeout,
										CommandHandler::CommandResponseHandler CommandCompleteHandler = 0) override;

	void CallbackHandler(std::shared_ptr<CommandHandler::CCommandOnTheFly> PCOTF, boost::system::error_code Error,
						 CommandHandler::CommandResponseHandler CommandCompleteHandler);
};

}