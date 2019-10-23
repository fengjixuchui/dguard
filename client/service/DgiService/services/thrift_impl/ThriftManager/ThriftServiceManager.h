//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

//
// There is a implementation of basic thrift service-manager.
//
// Abilities:
//	1. Set size of thread pool.
//	2. Start single-threaded server in a calling thread - startInCurrentThread()
//	3. Start multi-threaded server - start() methood.
//	4. Let stop thrift::service correctrly only if ~destructor of the service frees everything correctly.
//	all early acquired resources.
//

#pragma once

#include <memory>
#include "../../../stdafx.h"
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/concurrency/StdThreadFactory.h>
#include <thrift/concurrency/ThreadManager.h>
#include <transport/TServerSocket.h>
#include <transport/TBufferTransports.h>
#include <boost/make_shared.hpp>
#include "../../../helpers/internal/log.h"


namespace service
{
	namespace thrift_impl
	{
		template <class TService,
		class TSrvProcessor,
		class TProtocolFactory_ = apache::thrift::protocol::TBinaryProtocolFactory,
		class TTransportFactory_ = apache::thrift::transport::TBufferedTransportFactory>
		class ServiceManager
		{
		public:

			ServiceManager(int networkPortToDeployServerOn, const std::string& _log, int countOfWorkerThreads = 20) :
				m_log(_log),
				m_port(networkPortToDeployServerOn),
				m_handler(new TService(_log)),
				m_processor(new TSrvProcessor(m_handler)),
				m_serverTransport(new apache::thrift::transport::TServerSocket(networkPortToDeployServerOn)),
				m_transportFactory(new TTransportFactory_()),
				m_protocolFactory(new TProtocolFactory_()),
				m_threadManager(apache::thrift::concurrency::ThreadManager::newSimpleThreadManager(countOfWorkerThreads)),
				m_threadFactory(std::make_shared<apache::thrift::concurrency::StdThreadFactory>()),
				m_server(m_processor,
				m_serverTransport,
				m_transportFactory,
				m_protocolFactory,
				m_threadManager)
			{
				m_threadManager->threadFactory(m_threadFactory);
			}

			void stop()
			{
				m_log.print(std::string(__FUNCTION__) + ": Called.");

				//
				// I'm not sure about sequance of using these .stop() methoods.
				//
				m_threadManager->stop();
				m_server.stop();

				if (m_serverWorker.joinable())
				{
					m_serverWorker.join();

					//
					// Send signal for terminating...
					// .. wait for a while ..
					//
				}
			}

			//
			// Runs thrift-service in a background thread.
			//
			bool start()
			{
				m_log.print(std::string(__FUNCTION__) + ": Called.");

				if (!isRun())
				{
					return false;
				}

				m_threadManager->start();
				m_serverWorker = std::thread(ServiceManager::serviceProc, std::ref(*this));
				return true;
			}

			//
			// Returns execution only when main server worker-thread finishes.
			// True as a result means that thrift-service is completely stopped.
			//
			bool waitForCompletion()
			{
				m_log.print(std::string(__FUNCTION__) + ": Called.");

				if (!isRun())
				{
					m_serverWorker.join();
					return true;
				}

				return false;
			}

			bool isRun() const
			{
				return m_serverWorker.joinable() == false;
			}

			bool startInCurrentThread()
			{
				m_log.print(std::string(__FUNCTION__) + ": Called.");

				m_threadManager->start();

				try
				{
					m_server.serve();
				}
				catch (...)
				{
					m_log.print(std::string(__FUNCTION__) + ": Error - could not start the service.");

					//m_threadManager->stop();
					return false;
				}

				return true;
			}

			std::shared_ptr<TService> getService() const
			{
				return m_handler;
			}

		private:
			logfile m_log;
			int m_port;

			std::shared_ptr<TService> m_handler;
			std::shared_ptr<apache::thrift::TProcessor> m_processor;
			std::shared_ptr<apache::thrift::transport::TServerTransport> m_serverTransport;
			std::shared_ptr<apache::thrift::transport::TTransportFactory> m_transportFactory;
			std::shared_ptr<apache::thrift::protocol::TProtocolFactory> m_protocolFactory;
			std::shared_ptr<apache::thrift::concurrency::ThreadManager> m_threadManager;
			std::shared_ptr<apache::thrift::concurrency::StdThreadFactory> m_threadFactory;

			apache::thrift::server::TThreadPoolServer m_server;
			std::thread m_serverWorker;


			static void serviceProc(ServiceManager& _manager)
			{
				try
				{
					_manager.m_server.serve();
				}
				catch (...)
				{
					_manager.m_log.printEx("%s : critical error - the service (worked on %d port) is stopped! An exception was caught. Note: the port could be occupied by another process.",
						__FUNCTION__,
						_manager.m_port);
				}
			}
		};
	}
}
