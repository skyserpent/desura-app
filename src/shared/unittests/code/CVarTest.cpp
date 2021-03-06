/*
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
#include "managers/Managers.h"
#include "managers/CVar.h"

namespace UnitTest
{
	using namespace ::testing;

	class CVarRegTargetMock : public CVarRegTargetI
	{
	public:
		MOCK_METHOD1(RegCVar, bool(const gcRefPtr<CVar> &var));
		MOCK_METHOD1(UnRegCVar, void(const gcRefPtr<CVar> &var));
	};

	class CVarTestFixture : public ::testing::Test
	{
	public:
		void load(CVar& var, const char* szVal)
		{
			var.setValueOveride(szVal, true);
		}

		const char* getExitString(CVar& var)
		{
			return var.getExitString();
		}

		CVarRegTargetMock rt;
	};

	TEST_F(CVarTestFixture, noCallbackOnLoad)
	{
		EXPECT_CALL(rt, RegCVar(_)).Times(1).WillOnce(Return(true));
		EXPECT_CALL(rt, UnRegCVar(_)).Times(1);

		std::function<bool(const CVar*, const char*)> callback = [](const CVar* pCVar, const char* szVal) -> bool
		{
			EXPECT_TRUE(false);
			return true;
		};

		{
			CVar test("test", "0", CFLAG_NOCALLBACKONLOAD, callback, &rt);
			load(test, "1");

			ASSERT_TRUE(test.getBool());
		}
	}

	TEST_F(CVarTestFixture, callbackOnLoad)
	{
		EXPECT_CALL(rt, RegCVar(_)).Times(1).WillOnce(Return(true));
		EXPECT_CALL(rt, UnRegCVar(_)).Times(1);

		std::function<bool(const CVar*, const char*)> callback = [](const CVar* pCVar, const char* szVal) -> bool
		{
			EXPECT_TRUE(true);
			return true;
		};

		{
			CVar test("test", "0", 0, callback, &rt);
			load(test, "1");

			ASSERT_TRUE(test.getBool());
		}
	}

	TEST_F(CVarTestFixture, ignoreChange)
	{
		EXPECT_CALL(rt, RegCVar(_)).Times(1).WillOnce(Return(true));
		EXPECT_CALL(rt, UnRegCVar(_)).Times(1);

		std::function<bool(const CVar*, const char*)> callback = [](const CVar* pCVar, const char* szVal) -> bool
		{
			EXPECT_TRUE(true);
			return false;
		};

		{
			CVar test("test", "0", 0, callback, &rt);
			load(test, "1");

			ASSERT_FALSE(test.getBool());
		}
	}

	TEST_F(CVarTestFixture, saveOnExit)
	{
		EXPECT_CALL(rt, RegCVar(_)).Times(1).WillOnce(Return(true));
		EXPECT_CALL(rt, UnRegCVar(_)).Times(1);

		{
			CVar test("test", "0", CFLAG_SAVEONEXIT, nullptr, &rt);
			test.setValue("1");

			ASSERT_FALSE(test.getBool());
			ASSERT_STREQ("1", getExitString(test));
		}
	}

	TEST_F(CVarTestFixture, OSIgnored)
	{
#ifdef WIN32
		uint32 flags = CVAR_LINUX_ONLY;
#else
		uint32 flags = CVAR_WINDOWS_ONLY;
#endif

		EXPECT_CALL(rt, RegCVar(_)).Times(0);
		EXPECT_CALL(rt, UnRegCVar(_)).Times(0);

		{
			CVar test("test", "0", flags, nullptr, &rt);
		}
	}

	TEST_F(CVarTestFixture, deregister)
	{
		InSequence s;

		EXPECT_CALL(rt, RegCVar(_)).Times(1).WillOnce(Return(true));
		EXPECT_CALL(rt, UnRegCVar(_)).Times(1).RetiresOnSaturation();

		CVar test("test", "0", 0, nullptr, &rt);
		test.deregister();

		EXPECT_CALL(rt, UnRegCVar(_)).Times(0);
	}
}
