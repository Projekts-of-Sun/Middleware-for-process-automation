#pragma once
#include "..\Logger\File Logger.h"
#include "..\Command Handler\Command On The Fly.h"
#include "..\Command Handler\Base Abort Command.h"
#include "..\Command Handler\Base Method Command.h"

#include <boost/date_time.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/system/error_code.hpp>

#include <memory>
#include <string>
#include <list>
#include <map>

namespace CommandHandler
{

class CBasePeerConnectionSettings
{
public:
	unsigned int MaxReplyRetries;
	boost::posix_time::seconds ReplyRetryInterval;
	boost::posix_time::seconds KeepAliveTimeout;
	boost::posix_time::seconds Timeout;

	CBasePeerConnectionSettings()
		: MaxReplyRetries(3)
		, ReplyRetryInterval(5)
		, KeepAliveTimeout(180)
		, Timeout(300)
	{	}
};

typedef	std::map<unsigned int, std::shared_ptr<CommandHandler::CCommandOnTheFly>> CommandMap;
typedef	std::map<unsigned int, std::shared_ptr<CommandHandler::CReceivedCommandOnTheFly>> ReceivedCommandMap;

class CBasePeerConnection : public std::enable_shared_from_this<CBasePeerConnection>
{
public:
	CBasePeerConnection(boost::asio::io_service& IoService, CString Client = L"", bool SilasLog = false)
		: m_ErrorText(L"CBasePeerConnection: Unknown Error")
		, m_IoService(IoService)
		, m_Abort(false)
		, m_Logger(L"Peer." + Client + L".log.txt", SilasLog)
		, m_NextMessageID(rand())
		, m_LastSecondTick(boost::posix_time::microsec_clock::local_time())
		, m_Timer(IoService)
		, m_CommandProcessingTimer(IoService)
		, m_ClientName(Client)
		, m_TotalBytesReceived(0)
		, m_TotalBytesSent(0)
		, m_SupportsZLIB(false)
	{
		TRACE(L"CBasePeerConnection::CBasePeerConnection\n");
	}

	virtual ~CBasePeerConnection()
	{
		TRACE(L"CBasePeerConnection::~CBasePeerConnection\n");
		m_Logger.LogText(L"Base Command interpreter object destructed");
	}

	CString& ErrorText()
	{
		return m_ErrorText;
	}

	virtual void Start(bool SelfTiming)
	{
		TRACE(L"CBasePeerConnection::Start\n");
		if (SelfTiming)
		{	m_Timer.expires_from_now(boost::posix_time::seconds(1));
			m_Timer.async_wait(boost::bind(&CBasePeerConnection::SecondTick, shared_from_this(), boost::placeholders::_1, SelfTiming));
		}
	}

	virtual void SecondTick(boost::system::error_code Error, bool SelfTiming)
	{
		if (Error.value()==ERROR_OPERATION_ABORTED || m_Abort) return;

		boost::interprocess::scoped_lock<boost::recursive_mutex> SL(m_Mutex);
		if (SelfTiming)
		{	m_Timer.expires_from_now(boost::posix_time::seconds(1));
			m_Timer.async_wait(boost::bind(&CBasePeerConnection::SecondTick, shared_from_this(), boost::placeholders::_1, SelfTiming));
		}
		boost::posix_time::ptime Now(boost::posix_time::microsec_clock::local_time());
		boost::posix_time::time_duration Delta = Now - m_LastSecondTick;
		m_LastSecondTick = Now;
		// check timeouts for commands which we have sent
		for (CommandMap::iterator i(m_CurrentCommands.begin());i!=m_CurrentCommands.end();)
		{	_ASSERTE(i->second);
			i->second->SecondTick(Now, Delta);
			if (i->second->IsTimeout(Now))
			{	TRACE(L"\tCommand timeout\n");
				if (i->second->CommandCompleteHandler) 
				{	boost::system::error_code Err(boost::system::errc::timed_out, boost::system::generic_category());
					i->second->CommandCompleteHandler(Err, i->second.get());
				}
				i = m_CurrentCommands.erase(i);
				continue;
			}
			// TODO
			++i;
		}
		// check timeouts for commands which we have received
		for (std::map<unsigned int, std::shared_ptr<CReceivedCommandOnTheFly>>::iterator i(m_ReceivedCommands.begin());i!=m_ReceivedCommands.end();)
		{
			i->second->SecondTick(Now, Delta);
			if (i->second->IsTimeout(Now))
			{	// send Reply again
				CReceivedCommandOnTheFly& RCOTF(*i->second);
				if (RCOTF.State==CReceivedCommandOnTheFly::CommandReplySent)
				{	if (RCOTF.Command->NumberOfRepliesSends<m_Settings.MaxReplyRetries)
					{	RCOTF.Timeout+=m_Settings.ReplyRetryInterval;
						SendReplyToName(RCOTF.ClientName, RCOTF.Command, m_Settings.ReplyRetryInterval, false, RCOTF.Handler);
						continue;
					}
				}
				// TODO: Timeout feedback to whatever
				//TRACE(L"\t Received command timeout\n");
				i = m_ReceivedCommands.erase(i);
				continue;
			}
			// TODO
			++i;
		}
	}

	virtual bool RunMethod(CString ProjectPath, CString MasterBatch, CString MethodName,
							CString DataPath,
							std::list<CommandHandler::CPositionData>& Labware,
							boost::posix_time::time_duration Timeout) = 0;
	virtual void RunMethodCompleteHandler(boost::system::error_code Error, CommandHandler::CCommandOnTheFly* Command) = 0;
	
	virtual bool Abort()
	{
		m_ErrorText.Empty();
		m_Abort = true;
		m_Timer.cancel();
		m_CommandProcessingTimer.cancel();
		return true;
	}

	virtual bool Status(const CString& /*ClientName*/, boost::posix_time::time_duration /*Timeout*/)
	{
		return true;
	}

	virtual bool StatusIsIdle(const CString& ClientName, boost::posix_time::time_duration /*Timeout*/)
	{
		//TRACE(L"StatusIsIdle for '%s' Commands: %u\n", ClientName, m_CurrentCommands.size());
		boost::interprocess::scoped_lock<boost::recursive_mutex> SL(m_Mutex);
		for (CommandMap::iterator i(m_CurrentCommands.begin());i!=m_CurrentCommands.end();++i)
		{	_ASSERTE(i->second);
			if (ClientName==i->second->ClientName)
			{	for (std::list<std::shared_ptr<CBaseCommand>>::iterator j(i->second->Commands.begin());j!=i->second->Commands.end();++j)
				{	if (dynamic_cast<CBaseMethodCommand*>(j->get()))
					{	//TRACE(L"\tfor'%s' is 'Busy'\n", ClientName);
						return false;
					}
				}
			}
		}
		//TRACE(L"\tfor'%s' is 'Idle'\n", ClientName);
		return true;
	}

	virtual CString StatusAsText(const CString& ClientName)
	{
		if (StatusIsIdle(ClientName, boost::posix_time::seconds(5)))
		{	return L"Idle";
		}
		return L"Busy";
	}


	virtual int GetCommandCountForPeer(const CString& Name)
	{
		boost::interprocess::scoped_lock<boost::recursive_mutex> SL(m_Mutex);
		int n = 0;
		for (CommandMap::iterator i(m_CurrentCommands.begin());i!=m_CurrentCommands.end();++i)
		{
			if (i->second->ClientName==Name) ++n;
		}
		return n;
	}

	virtual std::list<std::shared_ptr<CommandHandler::CCommandOnTheFly>> GetCommandList(const CString& Name)
	{
		boost::interprocess::scoped_lock<boost::recursive_mutex> SL(m_Mutex);
		std::list<std::shared_ptr<CCommandOnTheFly>> L;
		for (CommandMap::iterator i(m_CurrentCommands.begin());i!=m_CurrentCommands.end();++i)
		{	if (i->second->ClientName==Name) L.push_back(i->second);
		}
		return L;
	}

	virtual void AbortCommandList(const CString& Name, const CCommandOnTheFly* COTF)
	{
		boost::interprocess::scoped_lock<boost::recursive_mutex> SL(m_Mutex);
		for (CommandMap::iterator i(m_CurrentCommands.begin());i!=m_CurrentCommands.end();)
		{	if (i->second->ClientName!=Name)
			{	++i;
				continue;
			}
			std::shared_ptr<CCommandOnTheFly> Command(i->second);
			if (Command.get()==COTF)
			{	// we need to keep this Command alive for it gets deleted after this handler returns
				++i;
				continue;
			}
			// the command must be erased from the map before the handler gets called
			// because it might be an Abort command which would call this AbortCommandList() again
			i = m_CurrentCommands.erase(i);
			if (Command->CommandCompleteHandler) 
			{	boost::system::error_code Error(boost::system::errc::success, boost::system::generic_category());
				Command->State = CCommandOnTheFly::CommandFailed;
				Command->CommandCompleteHandler(Error, Command.get());
			}
		}
	}

	virtual std::list<std::shared_ptr<CommandHandler::CReceivedCommandOnTheFly>> GetReceivedCommandList(const CString& Name)
	{
		boost::interprocess::scoped_lock<boost::recursive_mutex> SL(m_Mutex);
		std::list<std::shared_ptr<CReceivedCommandOnTheFly>> L;
		for (ReceivedCommandMap::iterator i(m_ReceivedCommands.begin());i!=m_ReceivedCommands.end();++i)
		{	if (i->second->ClientName==Name) L.push_back(i->second);
		}
		return L;
	}

	virtual boost::recursive_mutex& GetMutex()
	{	return m_Mutex;
	}

	Logger::CFileLogger m_Logger;

	unsigned __int64 GetAndResetBytesSent(const CString& /*ClientName*/)
	{
		boost::interprocess::scoped_lock<boost::recursive_mutex> SL(m_Mutex);
		// TODO: sort by peer
		unsigned __int64 X = m_TotalBytesSent;
		m_TotalBytesSent = 0;
		return X;
	}

	unsigned __int64 GetAndResetBytesReceived(const CString& /*ClientName*/)
	{
		boost::interprocess::scoped_lock<boost::recursive_mutex> SL(m_Mutex);
		// TODO: sort by peer
		unsigned __int64 X = m_TotalBytesReceived;
		m_TotalBytesReceived = 0;
		return X;
	}

	bool m_SupportsZLIB;
	bool m_Abort;
	CString m_ErrorText;
	CString m_ClientName;
	CommandMap m_CurrentCommands;
	ReceivedCommandMap m_ReceivedCommands;

	unsigned __int64 m_TotalBytesReceived;
	unsigned __int64 m_TotalBytesSent;
	unsigned int m_NextMessageID;

	boost::posix_time::ptime m_LastSecondTick;
	boost::recursive_mutex m_Mutex;
	boost::asio::deadline_timer m_Timer;
	boost::asio::deadline_timer m_CommandProcessingTimer;
	boost::asio::io_context& m_IoService;

	CBasePeerConnectionSettings m_Settings;

	// inform all command sources about error state
	// complete all commands with error
	virtual void DispatchError(const boost::system::error_code& Error)
	{
		TRACE(L"CBasePeerConnection::DispatchError: %u\n", Error.value());
		while (!m_CurrentCommands.empty())
		{	CCommandOnTheFly& X = *m_CurrentCommands.begin()->second;
			X.State = CCommandOnTheFly::CommandFailed;
			if (X.CommandCompleteHandler) X.CommandCompleteHandler(Error, &X);
			m_CurrentCommands.erase(m_CurrentCommands.begin());
		}
		while (!m_ReceivedCommands.empty())
		{	CReceivedCommandOnTheFly& X = *m_ReceivedCommands.begin()->second;
			X.State = CReceivedCommandOnTheFly::CommandFailed;
			if (X.Handler) X.Handler(Error, &X);
			m_ReceivedCommands.erase(m_ReceivedCommands.begin());
		}
	}

	virtual void SendCommandToName(const CString& /*ClientName*/,
									std::shared_ptr<CommandHandler::CBaseCommand>& Command, 
									boost::posix_time::time_duration /*Timeout*/,
									CommandHandler::CommandResponseHandler /*CommandCompleteHandler = 0*/)
	{
		++Command->NumberOfSends;
	}

	virtual void SendReplyToName(const CString& /*ClientName*/,
									std::shared_ptr<CommandHandler::CBaseCommand>& Command, 
									boost::posix_time::time_duration /*Timeout*/,
									bool /*Success = true*/,
									CommandHandler::ReceivedCommandCompleteHandler Handler = 0)
	{
		++Command->NumberOfRepliesSends;
	}

};

typedef std::shared_ptr<CBasePeerConnection> CBasePeerConnectionPtr;


}	// namespace
