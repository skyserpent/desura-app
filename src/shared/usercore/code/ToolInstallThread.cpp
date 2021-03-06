/*
Copyright (C) 2011 Mark Chandler (Desura Net Pty Ltd)
Copyright (C) 2014 Bad Juju Games, Inc.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA.

Contact us at legal@badjuju.com.

*/

#include "Common.h"
#include "ToolInstallThread.h"

#ifdef WIN32
#include "IPCToolMain.h"
#include "ToolIPCPipeClient.h"
#endif

#include "ToolTransaction.h"

using namespace UserCore::Misc;


void ToolInstallThread::run()
{
	const uint32 timeout = 5*60; //five mins

	while (!isStopped())
	{
		if (m_CurrentInstall != -1)
			doNextInstall();
		else
			doFirstInstall();

		if (isStopped())
			break;

		m_InstallWait.wait(0, 300);

		if (m_bStillInstalling)
			continue;

		m_InstallLock.lock();
		uint32 size = m_mTransactions.size();
		m_InstallLock.unlock();

		if (size == 0)
		{
			bool timedout = m_InstallWait.wait(timeout);

			if (!timedout)
				continue;

			//timeout happened. Stop
#ifdef WIN32
			safe_delete(m_pIPCClient);
#endif
			nonBlockStop();
			onFailedToRunEvent();
		}
	}
}

void ToolInstallThread::onStop()
{
	m_InstallWait.notify();
}

void ToolInstallThread::doFirstInstall()
{
	gcTrace("");

	{
		std::lock_guard<std::mutex> guard(m_InstallLock);

		if (m_dvInstallQue.empty())
			return;

		m_CurrentInstall = m_dvInstallQue.front();
		m_dvInstallQue.pop_front();
	}

	if (!preInstallStart())
		return;

	if (!getToolMain())
	{
		gcException e(ERR_PIPE, "Pipe to Tool Install Helper not running. Failed to install tools.");
		onINError(e);
	}

	ToolStartRes startRes = ToolStartRes::Failed;

	{
		std::lock_guard<std::mutex> guard(m_MapLock);

		auto it = m_mTransactions.find(m_CurrentInstall);

		if (it != m_mTransactions.end())
			startRes = it->second->startNextInstall(getToolMain(), m_CurrentInstallId);
	}

	switch (startRes)
	{
	case ToolStartRes::NoToolsLeft:
	case ToolStartRes::Failed:
		m_CurrentInstall = -1;
		doFirstInstall();
		break;

	case ToolStartRes::Success:
		m_bStillInstalling = true;
		break;
	};
}

bool ToolInstallThread::hasToolMain()
{
	return !!getToolMain();
}

void ToolInstallThread::doNextInstall()
{
	gcTrace("");

	if (m_bStillInstalling)
		return;

	if (m_CurrentInstall == -1)
		return;

	std::lock_guard<std::mutex> guard(m_MapLock);
	auto it = m_mTransactions.find(m_CurrentInstall);

	if (it != m_mTransactions.end() && hasToolMain())
	{
		it->second->onINComplete();

		if (it->second->startNextInstall(getToolMain(), m_CurrentInstallId) == ToolStartRes::Success)
			m_bStillInstalling = true;
		else
			m_CurrentInstall = -1;
	}
	else
	{
		//install must be canceled
		m_CurrentInstall = -1;
	}
}

void ToolInstallThread::onINComplete(int32 &result)
{
	gcTrace("Result {0}", result);

	m_bStillInstalling = false;

	auto tool = m_pToolManager->findItem(m_CurrentInstallId.toInt64());

	bool installError = false;

	if (tool)
	{
		installError = tool->checkExpectedResult(result) == false;

		if (!installError)
			tool->setInstalled();
	}

	if (installError)
	{
		std::lock_guard<std::mutex> guard(m_MapLock);
		auto it = m_mTransactions.find(m_CurrentInstall);

		if (it != m_mTransactions.end() && hasToolMain())
		{
			int32 r = result;

			Warning("The tool install [{3}] result didnt match what was expected [Actual: {0}, Expected: {1}]", r, tool->getResultString(), tool->getName());
			gcException e(ERR_BADRESPONSE, gcString("The tool {0} failed to install (Bad result)", tool->getName()));
			it->second->onINError(e);
			m_CurrentInstall = -1;
		}
	}

	m_pToolManager->saveItems();
	m_InstallWait.notify();
}

void ToolInstallThread::onINError(gcException &error)
{
	m_bStillInstalling = false;

	std::lock_guard<std::mutex> guard(m_MapLock);
	auto it = m_mTransactions.find(m_CurrentInstall);

	if (it != m_mTransactions.end())
		it->second->onINError(error);

	m_CurrentInstall = -1;
	m_InstallWait.notify();
}

void ToolInstallThread::startInstall(ToolTransactionId ttid)
{
	gcTrace("TTI: {0}", (uint32)ttid);

	std::lock_guard<std::mutex> guard(m_InstallLock);
	m_dvInstallQue.push_back(ttid);
	m_InstallWait.notify();
}

void ToolInstallThread::cancelInstall(ToolTransactionId ttid)
{
	gcTrace("TTI: {0}", (uint32)ttid);

	std::lock_guard<std::mutex> guard(m_InstallLock);
	if (m_CurrentInstall == ttid)
	{

	}
	else
	{
		for (size_t x=0; x<m_dvInstallQue.size(); x++)
		{
			if (ttid != m_dvInstallQue[x])
				continue;

			m_dvInstallQue.erase(m_dvInstallQue.begin()+x);
			break;
		}
	}
}
