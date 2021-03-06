//  cryptoport.io Burst Pool Miner
//
//  Created by Uray Meiviar < uraymeiviar@gmail.com > 2014
//  donation :
//
//  [Burst  ] BURST-8E8K-WQ2F-ZDZ5-FQWHX
//  [Bitcoin] 1UrayjqRjSJjuouhJnkczy5AuMqJGRK4b

#include "Miner.hpp"
#include "MinerLogger.hpp"
#include "MinerConfig.hpp"
#include "MinerUtil.hpp"
#include <Poco/Net/SSLManager.h>
#include "Poco/Net/ConsoleCertificateHandler.h"
#include "Poco/Net/HTTPSStreamFactory.h"
#include "Poco/Net/AcceptCertificateHandler.h"
#include <Poco/Net/HTTPSSessionInstantiator.h>
#include <Poco/Net/PrivateKeyPassphraseHandler.h>
#include <Poco/NestedDiagnosticContext.h>
#include "MinerServer.hpp"
#include <Poco/Logger.h>

class SSLInitializer
{
public:
	SSLInitializer()
	{
		Poco::Net::initializeSSL();
	}

	~SSLInitializer()
	{
		Poco::Net::uninitializeSSL();
	}
};

int main(int argc, const char* argv[])
{
	poco_ndc(main);

	Burst::MinerLogger::setup();
	
	auto& general = Poco::Logger::get("general");
	
	log_information(general, Burst::Settings::Project.nameAndVersionAndOs);
	log_information(general, "----------------------------------------------");
	log_information(general, "Github:   https://github.com/Creepsky/creepMiner");
	log_information(general, "Author:   Creepsky [creepsky@gmail.com]");
	log_information(general, "Burst :   BURST-JBKL-ZUAV-UXMB-2G795");
	log_information(general, "----------------------------------------------");
	log_unimportant(general, "Based on http://github.com/uraymeiviar/burst-miner");
	log_unimportant(general, "author : uray meiviar [ uraymeiviar@gmail.com ]");
	log_unimportant(general, "please donate to support developments :");
	log_unimportant(general, " [ Burst   ] BURST-8E8K-WQ2F-ZDZ5-FQWHX");
	log_unimportant(general, " [ Bitcoin ] 1UrayjqRjSJjuouhJnkczy5AuMqJGRK4b");
	log_unimportant(general, "----------------------------------------------");

	std::string configFile = "mining.conf";

	if (argc > 1)
	{
		if (argv[1][0] == '-')
		{
			log_information(general, "usage : burstminer <config-file>");
			log_information(general, "if no config-file specified, program will look for mining.conf file inside current directory");
		}
		configFile = std::string(argv[1]);
	}

	log_system(general, "using config file %s", configFile);
	
	try
	{
		using namespace Poco;
		using namespace Net;
		
		SSLInitializer sslInitializer;
		HTTPSStreamFactory::registerFactory();

		SharedPtr<InvalidCertificateHandler> ptrCert = new AcceptCertificateHandler(false); // ask the user via console
		Context::Ptr ptrContext = new Context(Context::CLIENT_USE, "", "", "",
			Context::VERIFY_RELAXED, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
		SSLManager::instance().initializeClient(nullptr, ptrCert, ptrContext);

		HTTPSessionInstantiator::registerInstantiator();
		HTTPSSessionInstantiator::registerInstantiator();

		if (Burst::MinerConfig::getConfig().readConfigFile(configFile))
		{
			Burst::Miner miner;
			Burst::MinerServer server{miner};

			if (Burst::MinerConfig::getConfig().getStartServer())
			{
				server.connectToMinerData(miner.getData());
				server.run(Burst::MinerConfig::getConfig().getServerUrl().getPort());
			}

			miner.run();
			server.stop();
		}
		else
		{
			log_error(general, "Aborting program due to invalid configuration");
		}
	}
	catch (Poco::Exception& exc)
	{
		log_fatal(general, "Aborting program due to exceptional state: %s", exc.displayText());
		log_exception(general, exc);
	}
	catch (std::exception& exc)
	{
		log_fatal(general, "Aborting program due to exceptional state: %s", exc.what());
	}

	return 0;
}
