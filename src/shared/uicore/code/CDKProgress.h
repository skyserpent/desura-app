/*
Desura is the leading indie game distribution platform
Copyright (C) 2011 Mark Chandler (Desura Net Pty Ltd)

$LicenseInfo:firstyear=2014&license=lgpl$
Copyright (C) 2014, Linden Research, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License only.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see <http://www.gnu.org/licenses/>
or write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
$/LicenseInfo$
*/

#ifndef DESURA_CDKPROGRESS_H
#define DESURA_CDKPROGRESS_H
#ifdef _WIN32
#pragma once
#endif

#include "BasePage.h"
#include "wx_controls/gcControls.h"

#include "usercore/MCFThreadI.h"
#include "usercore/CDKeyManagerI.h"

class CDKProgress : public BasePage, public UserCore::Misc::CDKeyCallBackI
{
public:
	CDKProgress(wxWindow* parent, bool launch);
	~CDKProgress();

	void dispose();
	void run();

protected:
	friend class LanguageTestDialog;

	Event<gcString> onCompleteEvent;
	Event<gcException> onErrorEvent;

	void onComplete(gcString& cdKey);
	void onError(gcException& e);

	void onCDKeyComplete(DesuraId id, gcString &cdKey) override;
	void onCDKeyError(DesuraId id, gcException& e) override;

private:
	wxStaticText* m_labInfo;
	bool m_bLaunch;

	gcSpinningBar* m_pbProgress;
	gcButton* m_butClose;

	bool m_bOutstandingRequest = false;
};


#endif //DESURA_CDKPROGRESS_H
