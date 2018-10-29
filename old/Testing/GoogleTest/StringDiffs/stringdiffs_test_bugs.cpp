#include <gtest/gtest.h>
#include <windows.h>
#include <tchar.h>
#include <vector>
#include "stringdiffs.h"


using std::vector;

namespace
{
	// The fixture for testing String differencing functions.
	// These tests are from actually reported bugs.
	class StringDiffsBugsTest : public testing::Test
	{
	protected:
		// You can remove any or all of the following functions if its body
		// is	empty.

		StringDiffsBugsTest()
		{
			// You can do set-up work for each test	here.
			sd_Init();
		}

		virtual ~StringDiffsBugsTest()
		{
			// You can do clean-up work	that doesn't throw exceptions here.
			sd_Close();
		}

		// If	the	constructor	and	destructor are not enough for setting up
		// and cleaning up each test, you can define the following methods:

		virtual void SetUp()
		{
			// Code	here will be called	immediately	after the constructor (right
			// before each test).
		}

		virtual void TearDown()
		{
			// Code	here will be called	immediately	after each test	(right
			// before the destructor).
		}

		// Objects declared here can be used by all tests in the test case for Foo.
	};

	// sd_ComputeWordDiffs() parameters are:
	// String & str1 - the first string to compare
	// String & str2 - the second string to compare
	// bool case_sensitive - is the compare case-sensitive?
	// int whitespace - do we ignore whitespace and how
	// int breakType - Space (0) or punctuations (1) break
	// bool byte_level - are we word (false) or byte-level (true) diffing
	// std::vector<wdiff*> * pDiffs - resultting diff list


	// Sf.net bug #1685466 - char level diff
	// (ENUMRESLANGPROC) added to middle
	TEST_F(StringDiffsBugsTest, Bug_1685466_1)
	{
		std::vector<wdiff*> diffs;
		sd_ComputeWordDiffs(
				_T("if (EnumResourceLanguages(hinstLang, RT_VERSION, MAKEINTRESOURCE(VS_VERSION_INFO), (ENUMRESLANGPROC)FindNextResLang, (LPARAM)&wLangID) == 0)"),
				_T("if (EnumResourceLanguages(hinstLang, RT_VERSION, MAKEINTRESOURCE(VS_VERSION_INFO), FindNextResLang, (LPARAM)&wLangID) == 0)"),
				false, 0, 1, true, &diffs);
		EXPECT_EQ(1, diffs.size());
		if (diffs.size() > 0)
		{
			wdiff *pDiff = diffs[0];
			EXPECT_EQ(83, pDiff->start[0]);
			EXPECT_EQ(83, pDiff->start[1]);
			EXPECT_EQ(99, pDiff->end[0]);
			EXPECT_EQ(82, pDiff->end[1]);
		}
	}

	// Sf.net bug #1685466 - word level diff
	// (ENUMRESLANGPROC) added to middle
	TEST_F(StringDiffsBugsTest, Bug_1685466_2)
	{
		std::vector<wdiff*> diffs;
		sd_ComputeWordDiffs(_T("if (EnumResourceLanguages(hinstLang, RT_VERSION, MAKEINTRESOURCE(VS_VERSION_INFO), (ENUMRESLANGPROC)FindNextResLang, (LPARAM)&wLangID) == 0)"),
			_T("if (EnumResourceLanguages(hinstLang, RT_VERSION, MAKEINTRESOURCE(VS_VERSION_INFO), FindNextResLang, (LPARAM)&wLangID) == 0)"),
				false, 0, 1, false, &diffs);
		EXPECT_EQ(1, diffs.size());
		if (diffs.size() > 0)
		{
			wdiff *pDiff = diffs[0];
			EXPECT_EQ(83, pDiff->start[0]);
			EXPECT_EQ(83, pDiff->start[1]);
			EXPECT_EQ(114, pDiff->end[0]);
			EXPECT_EQ(97, pDiff->end[1]);
		}
	}

	// Sf.net bug #1939279 - char level diff
	// various changes
	TEST_F(StringDiffsBugsTest, Bug_1939279_1)
	{
		std::vector<wdiff*> diffs;
		sd_ComputeWordDiffs(_T("[overlay_oid_origin, overlay_oid_target], [nil, nil]"),
			_T("[overlay_oid_origin, overlay_oid_target, origin_file_name, target_file_name], [nil, nil, \"origin.txt\"), \"target.txt\"]"),
				false, 0, 1, true, &diffs);
		EXPECT_EQ(6, diffs.size());
		if (diffs.size() > 0)
		{
			wdiff *pDiff = diffs[0];
			EXPECT_EQ(19, pDiff->start[0]);
			EXPECT_EQ(19, pDiff->start[1]);
			EXPECT_EQ(18, pDiff->end[0]);
			EXPECT_EQ(81, pDiff->end[1]);
		}
	}

	// Sf.net bug #1939279 - word level diff
	// various changes
	TEST_F(StringDiffsBugsTest, Bug_1939279_2)
	{
		std::vector<wdiff*> diffs;
		sd_ComputeWordDiffs(_T("[overlay_oid_origin, overlay_oid_target], [nil, nil]"),
			_T("[overlay_oid_origin, overlay_oid_target, origin_file_name, target_file_name], [nil, nil, \"origin.txt\"), \"target.txt\"]"),
				false, 0, 1, false, &diffs);
		EXPECT_EQ(6, diffs.size());
		if (diffs.size() > 0)
		{
			wdiff *pDiff = diffs[0];
			EXPECT_EQ(19, pDiff->start[0]);
			EXPECT_EQ(19, pDiff->start[1]);
			EXPECT_EQ(18, pDiff->end[0]);
			EXPECT_EQ(81, pDiff->end[1]);
		}
	}

	// Sf.net bug #2022935 - char level diff
	// Chars added to end
	TEST_F(StringDiffsBugsTest, Bug_2022935)
	{
		std::vector<wdiff*> diffs;
		sd_ComputeWordDiffs(_T("N42=Import"), _T("N42=Importuj"),
				false, 0, 1, true, &diffs);
		EXPECT_EQ(1, diffs.size());
		if (diffs.size() > 0)
		{
			wdiff *pDiff = diffs[0];
			EXPECT_EQ(0, pDiff->start[0]);
			EXPECT_EQ(10, pDiff->start[1]);
			EXPECT_EQ(-1, pDiff->end[0]);
			EXPECT_EQ(11, pDiff->end[1]);
		}
	}

	// Sf.net bug #2638045 - word diff
	// Char added to end
	TEST_F(StringDiffsBugsTest, Bug_2638045)
	{
		std::vector<wdiff*> diffs;
		sd_ComputeWordDiffs(_T("LIB_PHP4_DIR=$(RPM_NAME)-$(RPM_VER)/usr/lib/php"),
				_T("LIB_PHP4_DIR=$(RPM_NAME)-$(RPM_VER)/usr/lib/php4"),
				false, 0, 0, false, &diffs);
		EXPECT_EQ(1, diffs.size());
		if (diffs.size() > 0)
		{
			wdiff *pDiff = diffs[0];
			EXPECT_EQ(0, pDiff->start[0]);
			EXPECT_EQ(0, pDiff->start[1]);
			EXPECT_EQ(46, pDiff->end[0]);
			EXPECT_EQ(47, pDiff->end[1]);
		}
	}

	// Sf.net bug #2638045 - word diff with puncts
	// Char added to end
	TEST_F(StringDiffsBugsTest, Bug_2638045_2)
	{
		std::vector<wdiff*> diffs;
		sd_ComputeWordDiffs(_T("LIB_PHP4_DIR=$(RPM_NAME)-$(RPM_VER)/usr/lib/php"),
				_T("LIB_PHP4_DIR=$(RPM_NAME)-$(RPM_VER)/usr/lib/php4"),
				false, 0, 1, false, &diffs);
		EXPECT_EQ(1, diffs.size());
		if (diffs.size() > 0)
		{
			wdiff *pDiff = diffs[0];
			EXPECT_EQ(0, pDiff->start[0]);
			EXPECT_EQ(0, pDiff->start[1]);
			EXPECT_EQ(46, pDiff->end[0]);
			EXPECT_EQ(47, pDiff->end[1]);
		}
	}

	// Sf.net bug #2638045 - char diff
	// Char added to end
	TEST_F(StringDiffsBugsTest, Bug_2638045_3)
	{
		std::vector<wdiff*> diffs;
		sd_ComputeWordDiffs(_T("LIB_PHP4_DIR=$(RPM_NAME)-$(RPM_VER)/usr/lib/php"),
				_T("LIB_PHP4_DIR=$(RPM_NAME)-$(RPM_VER)/usr/lib/php4"),
				false, 0, 0, true, &diffs);
		EXPECT_EQ(1, diffs.size());
		if (diffs.size() > 0)
		{
			wdiff *pDiff = diffs[0];
			EXPECT_EQ(0, pDiff->start[0]);
			EXPECT_EQ(47, pDiff->start[1]);
			EXPECT_EQ(-1, pDiff->end[0]);
			EXPECT_EQ(47, pDiff->end[1]);
		}
	}

	// Sf.net bug #2638045 - custom punct word diff
	// Char added to end
	TEST_F(StringDiffsBugsTest, Bug_2638045_4)
	{
		std::vector<wdiff*> diffs;
		sd_SetBreakChars(_T("/"));
		sd_ComputeWordDiffs(_T("LIB_PHP4_DIR=$(RPM_NAME)-$(RPM_VER)/usr/lib/php"),
				_T("LIB_PHP4_DIR=$(RPM_NAME)-$(RPM_VER)/usr/lib/php4"),
				false, 0, 1, false, &diffs);
		EXPECT_EQ(1, diffs.size());
		if (diffs.size() > 0)
		{
			wdiff *pDiff = diffs[0];
			EXPECT_EQ(44, pDiff->start[0]);
			EXPECT_EQ(44, pDiff->start[1]);
			EXPECT_EQ(46, pDiff->end[0]);
			EXPECT_EQ(47, pDiff->end[1]);
		}
	}

	// Sf.net bug #2779834
	// (int) added to right-side
	// word-diff
	TEST_F(StringDiffsBugsTest, Bug_2779834_1)
	{
		std::vector<wdiff*> diffs;
		sd_ComputeWordDiffs(_T("if (nDiff < m_diffs.size())"),
				_T("if (nDiff < (int) m_diffs.size())"),
				false, 0, 0, false, &diffs);
		EXPECT_EQ(1, diffs.size());
		if (diffs.size() > 0)
		{
			wdiff *pDiff = diffs[0];
			EXPECT_EQ(11, pDiff->start[0]);
			EXPECT_EQ(11, pDiff->start[1]);
			EXPECT_EQ(10, pDiff->end[0]);
			EXPECT_EQ(16, pDiff->end[1]);
		}
	}

	// Sf.net bug #2779834
	// (int) added to right-side
	// char-diff
	TEST_F(StringDiffsBugsTest, Bug_2779834_2)
	{
		std::vector<wdiff*> diffs;
		sd_ComputeWordDiffs(_T("if (nDiff < m_diffs.size())"),
				_T("if (nDiff < (int) m_diffs.size())"),
				false, 0, 0, true, &diffs);
		EXPECT_EQ(1, diffs.size());
		if (diffs.size() > 0)
		{
			wdiff *pDiff = diffs[0];
			EXPECT_EQ(11, pDiff->start[0]);
			EXPECT_EQ(11, pDiff->start[1]);
			EXPECT_EQ(10, pDiff->end[0]);
			EXPECT_EQ(16, pDiff->end[1]);
		}
	}

	// Sf.net bug #2783185
	// 'private' added to right-side
	// char-diff
	TEST_F(StringDiffsBugsTest, Bug_2783185_1)
	{
		std::vector<wdiff*> diffs;
		sd_ComputeWordDiffs(_T("const string ManualFolder = \"Manual\";"),
				_T("private const string ManualFolder = \"Manual\";"),
				false, 0, 0, true, &diffs);
		EXPECT_EQ(1, diffs.size());
		if (diffs.size() > 0)
		{
			wdiff *pDiff = diffs[0];
			EXPECT_EQ(0, pDiff->start[0]);
			EXPECT_EQ(0, pDiff->start[1]);
			EXPECT_EQ(-1, pDiff->end[0]);
			EXPECT_EQ(7, pDiff->end[1]);
		}
	}

	// Sf.net bug #2783185
	// 'private' added to right-side
	// word-diff
	TEST_F(StringDiffsBugsTest, Bug_2783185_2)
	{
		std::vector<wdiff*> diffs;
		sd_ComputeWordDiffs(_T("const string ManualFolder = \"Manual\";"),
				_T("private const string ManualFolder = \"Manual\";"),
				false, 0, 0, false, &diffs);
		EXPECT_EQ(1, diffs.size());
		if (diffs.size() > 0)
		{
			wdiff *pDiff = diffs[0];
			EXPECT_EQ(0, pDiff->start[0]);
			EXPECT_EQ(0, pDiff->start[1]);
			EXPECT_EQ(-1, pDiff->end[0]);
			EXPECT_EQ(7, pDiff->end[1]);
		}
	}

	// Sf.net bug #2791879
	// 'x' added in right side at end of second and fourth words
	// char-diff
	TEST_F(StringDiffsBugsTest, Bug_2791879_1)
	{
		std::vector<wdiff*> diffs;
		sd_ComputeWordDiffs(_T("abc def ghi jkl mno"),
				_T("abc defx ghi jklx mno"),
				false, 0, 0, true, &diffs);
		EXPECT_EQ(2, diffs.size());
		if (diffs.size() > 1)
		{
			wdiff *pDiff = diffs[0];
			EXPECT_EQ(4, pDiff->start[0]);
			EXPECT_EQ(7, pDiff->start[1]);
			EXPECT_EQ(3, pDiff->end[0]);
			EXPECT_EQ(7, pDiff->end[1]);
			pDiff = diffs[1];
			EXPECT_EQ(12, pDiff->start[0]);
			EXPECT_EQ(16, pDiff->start[1]);
			EXPECT_EQ(11, pDiff->end[0]);
			EXPECT_EQ(16, pDiff->end[1]);
		}
	}

	// Sf.net bug #2791879
	// 'x' added in right side at end of second and fourth words
	// word-diff
	TEST_F(StringDiffsBugsTest, Bug_2791879_2)
	{
		std::vector<wdiff*> diffs;
		sd_ComputeWordDiffs(_T("abc def ghi jkl mno"),
				_T("abc defx ghi jklx mno"),
				false, 0, 0, false, &diffs);
		EXPECT_EQ(2, diffs.size());
		if (diffs.size() > 1)
		{
			wdiff *pDiff = diffs[0];
			EXPECT_EQ(4, pDiff->start[0]);
			EXPECT_EQ(4, pDiff->start[1]);
			EXPECT_EQ(6, pDiff->end[0]);
			EXPECT_EQ(7, pDiff->end[1]);
			pDiff = diffs[1];
			EXPECT_EQ(12, pDiff->start[0]);
			EXPECT_EQ(13, pDiff->start[1]);
			EXPECT_EQ(14, pDiff->end[0]);
			EXPECT_EQ(16, pDiff->end[1]);
		}
	}

	// Sf.net bug #2793479
	// ' ' (space) added before '(TCHAR' in left
	// 'const' added before 'BYTE' in right
	// char-diff
	TEST_F(StringDiffsBugsTest, Bug_2793479_1)
	{
		std::vector<wdiff*> diffs;
		sd_ComputeWordDiffs(_T("static int iTranslateBytesToBC (TCHAR* pd, BYTE* src, int srclen);"),
				_T("static int iTranslateBytesToBC(TCHAR* pd, const BYTE* src, int srclen);"),
				false, 0, 0, true, &diffs);
		EXPECT_EQ(2, diffs.size());
		if (diffs.size() > 1)
		{
			wdiff *pDiff = diffs[0];
			EXPECT_EQ(30, pDiff->start[0]);
			EXPECT_EQ(30, pDiff->start[1]);
			EXPECT_EQ(30, pDiff->end[0]);
			EXPECT_EQ(29, pDiff->end[1]);
			pDiff = diffs[1];
			EXPECT_EQ(42, pDiff->start[0]);
			EXPECT_EQ(41, pDiff->start[1]);
			EXPECT_EQ(41, pDiff->end[0]);
			EXPECT_EQ(46, pDiff->end[1]);
		}
	}

	// Sf.net bug #2793479
	// ' ' (space) added before '(TCHAR' in left
	// 'const' added before 'BYTE' in right
	// word-diff
	TEST_F(StringDiffsBugsTest, Bug_2793479_2)
	{
		std::vector<wdiff*> diffs;
		sd_ComputeWordDiffs(_T("static int iTranslateBytesToBC (TCHAR* pd, BYTE* src, int srclen);"),
				_T("static int iTranslateBytesToBC(TCHAR* pd, const BYTE* src, int srclen);"),
				false, 0, 0, false, &diffs);
		EXPECT_EQ(2, diffs.size());
		wdiff *pDiff;
		if (diffs.size() > 0)
		{
			pDiff = diffs[0];
			EXPECT_EQ(11, pDiff->start[0]);
			EXPECT_EQ(11, pDiff->start[1]);
			EXPECT_EQ(37, pDiff->end[0]);
			EXPECT_EQ(36, pDiff->end[1]);
		}
		if (diffs.size() > 1)
		{
			pDiff = diffs[1];
			EXPECT_EQ(42, pDiff->start[0]);
			EXPECT_EQ(41, pDiff->start[1]);
			EXPECT_EQ(41, pDiff->end[0]);
			EXPECT_EQ(46, pDiff->end[1]);
		}
		if (diffs.size() > 2)
		{
			pDiff = diffs[2];
			EXPECT_EQ(43, pDiff->start[0]);
			EXPECT_EQ(42, pDiff->start[1]);
			EXPECT_EQ(42, pDiff->end[0]);
			EXPECT_EQ(57, pDiff->end[1]);
		}
	}

	// Sf.net bug #2797067
	// '123' and '456  added in right side and 'x' separated from 'vwx'
	// char-diff
	TEST_F(StringDiffsBugsTest, Bug_2797067_1)
	{
		std::vector<wdiff*> diffs;
		sd_ComputeWordDiffs(_T("abc def ghi jkl mno pqr stu vwx yz"),
				_T("abc def 123 ghi jkl mno pqr stu 456 vw x yz"),
				false, 0, 0, true, &diffs);
		EXPECT_EQ(3, diffs.size());
		if (diffs.size() > 2)
		{
			wdiff *pDiff = diffs[0];
			EXPECT_EQ(7, pDiff->start[0]);
			EXPECT_EQ(7, pDiff->start[1]);
			EXPECT_EQ(6, pDiff->end[0]);
			EXPECT_EQ(10, pDiff->end[1]);
			pDiff = diffs[1];
			EXPECT_EQ(27, pDiff->start[0]);
			EXPECT_EQ(31, pDiff->start[1]);
			EXPECT_EQ(26, pDiff->end[0]);
			EXPECT_EQ(37, pDiff->end[1]);
			pDiff = diffs[2];
			EXPECT_EQ(28, pDiff->start[0]);
			EXPECT_EQ(39, pDiff->start[1]);
			EXPECT_EQ(29, pDiff->end[0]);
			EXPECT_EQ(38, pDiff->end[1]);
		}
	}

	// Sf.net bug #2797067
	// '123' and '456  added in right side and 'x' separated from 'vwx'
	// word-diff
	TEST_F(StringDiffsBugsTest, Bug_2797067_2)
	{
		std::vector<wdiff*> diffs;
		sd_ComputeWordDiffs(_T("abc def ghi jkl mno pqr stu vwx yz"),
				_T("abc def 123 ghi jkl mno pqr stu 456 vw x yz"),
				false, 0, 0, false, &diffs);
		EXPECT_EQ(3, diffs.size());
		if (diffs.size() > 2)
		{
			wdiff *pDiff = diffs[0];
			EXPECT_EQ(7, pDiff->start[0]);
			EXPECT_EQ(7, pDiff->start[1]);
			EXPECT_EQ(6, pDiff->end[0]);
			EXPECT_EQ(10, pDiff->end[1]);
			pDiff = diffs[1];
			EXPECT_EQ(27, pDiff->start[0]);
			EXPECT_EQ(31, pDiff->start[1]);
			EXPECT_EQ(26, pDiff->end[0]);
			EXPECT_EQ(37, pDiff->end[1]);
			pDiff = diffs[2];
			EXPECT_EQ(28, pDiff->start[0]);
			EXPECT_EQ(39, pDiff->start[1]);
			EXPECT_EQ(30, pDiff->end[0]);
			EXPECT_EQ(39, pDiff->end[1]);
		}
	}

	// Sf.net bug #2797067
	// '123', '456' and last 'r' added to right side
	// char-diff
	TEST_F(StringDiffsBugsTest, Bug_2797067_3)
	{
		std::vector<wdiff*> diffs;
		sd_ComputeWordDiffs(_T("abc def ghi jkl mno pqr stu vwx yzr"),
				_T("abc def 123ghi jkl mno pqr stu 456vwx yzrr"),
				false, 0, 0, true, &diffs);
		EXPECT_EQ(3, diffs.size());
		if (diffs.size() > 2)
		{
			wdiff *pDiff = diffs[0];
			EXPECT_EQ(8, pDiff->start[0]);
			EXPECT_EQ(8, pDiff->start[1]);
			EXPECT_EQ(7, pDiff->end[0]);
			EXPECT_EQ(10, pDiff->end[1]);
			pDiff = diffs[1];
			EXPECT_EQ(28, pDiff->start[0]);
			EXPECT_EQ(31, pDiff->start[1]);
			EXPECT_EQ(27, pDiff->end[0]);
			EXPECT_EQ(33, pDiff->end[1]);
			pDiff = diffs[2];
			EXPECT_EQ(32, pDiff->start[0]);
			EXPECT_EQ(41, pDiff->start[1]);
			EXPECT_EQ(31, pDiff->end[0]);
			EXPECT_EQ(41, pDiff->end[1]);
		}
	}

	// Sf.net bug #2797067
	// '123', '456' and last 'r' added to right side
	// word-diff
	TEST_F(StringDiffsBugsTest, Bug_2797067_4)
	{
		std::vector<wdiff*> diffs;
		sd_ComputeWordDiffs(_T("abc def ghi jkl mno pqr stu vwx yzr"),
				_T("abc def 123ghi jkl mno pqr stu 456vwx yzrr"),
				false, 0, 0, false, &diffs);
		EXPECT_EQ(3, diffs.size());
		if (diffs.size() > 2)
		{
			wdiff *pDiff = diffs[0];
			EXPECT_EQ(8, pDiff->start[0]);
			EXPECT_EQ(8, pDiff->start[1]);
			EXPECT_EQ(10, pDiff->end[0]);
			EXPECT_EQ(13, pDiff->end[1]);
			pDiff = diffs[1];
			EXPECT_EQ(28, pDiff->start[0]);
			EXPECT_EQ(31, pDiff->start[1]);
			EXPECT_EQ(30, pDiff->end[0]);
			EXPECT_EQ(36, pDiff->end[1]);
			pDiff = diffs[2];
			EXPECT_EQ(32, pDiff->start[0]);
			EXPECT_EQ(38, pDiff->start[1]);
			EXPECT_EQ(34, pDiff->end[0]);
			EXPECT_EQ(41, pDiff->end[1]);
		}
	}
}  // namespace
