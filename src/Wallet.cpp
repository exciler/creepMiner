﻿#include "Wallet.hpp"
#include "MinerUtil.hpp"
#include <Poco/Net/HTTPClientSession.h>
#include "MinerConfig.hpp"
#include "Request.hpp"
#include <Poco/Net/HTTPRequest.h>
#include <Poco/JSON/Parser.h>
#include <cassert>
#include <Poco/NestedDiagnosticContext.h>
#include "MinerLogger.hpp"
#include "Account.hpp"

using namespace Poco::Net;

Burst::Wallet::Wallet()
{}

Burst::Wallet::Wallet(const Url& url)
	: url_(url)
{
	poco_ndc(Wallet(const Url& url));
	walletSession_ = url_.createSession();
}

Burst::Wallet::~Wallet()
{}

bool Burst::Wallet::getWinnerOfBlock(uint64_t block, AccountId& winnerId)
{
	poco_ndc(Wallet::getWinnerOfBlock);
	winnerId = 0;

	if (url_.empty())
		return false;

	Poco::JSON::Object::Ptr json;

	Poco::URI uri;
	uri.setPath("/burst");
	uri.addQueryParameter("requestType", "getBlock");
	uri.addQueryParameter("height", std::to_string(block));

	if (sendWalletRequest(uri, json))
	{
		if (json->has("generator"))
		{
			winnerId = json->get("generator").convert<uint64_t>();
			return true;
		}

		return false;
	}

	log_debug(MinerLogger::wallet, "Could not get last winner!");
	return false;
}

bool Burst::Wallet::getNameOfAccount(AccountId account, std::string& name)
{
	poco_ndc(Wallet::getNameOfAccount);
	Poco::JSON::Object::Ptr json;
	name = "";

	if (url_.empty())
		return false;

	Poco::URI uri;
	uri.setPath("/burst");
	uri.addQueryParameter("requestType", "getAccount");
	uri.addQueryParameter("account", std::to_string(account));

	if (sendWalletRequest(uri, json))
	{
		if (json->has("name"))
		{
			name = json->get("name").convert<std::string>();
			return true;
		}

		return false;
	}

	log_debug(MinerLogger::wallet, "Could not get name of account!");
	return false;
}

bool Burst::Wallet::getRewardRecipientOfAccount(AccountId account, AccountId& rewardRecipient)
{
	poco_ndc(Wallet::getRewardRecipientOfAccount);
	Poco::JSON::Object::Ptr json;
	rewardRecipient = 0;

	if (url_.empty())
		return false;

	Poco::URI uri;
	uri.setPath("/burst");
	uri.addQueryParameter("requestType", "getRewardRecipient");
	uri.addQueryParameter("account", std::to_string(account));

	if (sendWalletRequest(uri, json))
	{
		if (json->has("rewardRecipient"))
		{
			rewardRecipient = json->get("rewardRecipient");
			return true;
		}

		return false;
	}

	log_debug(MinerLogger::wallet, "Could not get name of account!");
	return false;
}

bool Burst::Wallet::getLastBlock(uint64_t& block)
{
	poco_ndc(Wallet::getLastBlock);
	Poco::JSON::Object::Ptr json;
	block = 0;

	if (url_.empty())
	{
		return false;
	}

	Poco::URI uri;
	uri.setPath("/burst");
	uri.addQueryParameter("requestType", "getBlock");

	if (sendWalletRequest(uri, json))
	{
		if (json->has("height"))
		{
			block = json->get("height");
			return true;
		}

		return false;
	}

	log_debug(MinerLogger::wallet, "Could not get last blockheight!");
	return false;
}

void Burst::Wallet::getAccount(AccountId id, Account& account)
{
	if (!url_.empty())
		account = {*this, id};
}

bool Burst::Wallet::sendWalletRequest(const Poco::URI& uri, Poco::JSON::Object::Ptr& json)
{
	poco_ndc(Wallet::sendWalletRequest);

	HTTPRequest request{ HTTPRequest::HTTP_GET, uri.getPathAndQuery(), HTTPRequest::HTTP_1_1};
	request.setKeepAlive(true);

	if (walletSession_ == nullptr)
		walletSession_ = url_.createSession();
	
	Request req{std::move(walletSession_)};
	auto resp = req.send(request);
	std::string data;

	if (resp.receive(data))
	{
		try
		{
			Poco::JSON::Parser parser;
			json = parser.parse(data).extract<Poco::JSON::Object::Ptr>();
			transferSession(resp, walletSession_);
			return true;
		}
		catch (Poco::Exception& exc)
		{
			log_error(MinerLogger::wallet, "Got invalid json response from server\n"
				"\tURI: %s\n"
				"\tResponse: %s\n"
				"\t%s",
				uri.getPathAndQuery(), data, exc.displayText()
			);

			log_current_stackframe(MinerLogger::wallet);
			
			transferSession(resp, walletSession_);
			return false;
		}
	}

	transferSession(resp, walletSession_);
	transferSession(req, walletSession_);

	assert(walletSession_ != nullptr);

	return false;
}
