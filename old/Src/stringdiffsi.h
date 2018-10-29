/** 
 * @file  stringdiffsi.h
 *
 * @brief Declaration file for class stringdiffs
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef stringdiffsi_h_included
#define stringdiffsi_h_included

#include <vector>
/**
 * @brief kind of diff blocks.
 */
enum
{
	dlword = 0,
	dlspace,
	dlbreak, 
	dlinsert,
};
/**
 * @brief kind of synchronaction
 */
enum
{
	synbegin1 = 0, 
	synbegin2,
	synend1, 
	synend2 
};

struct wdiff;

/**
 * @brief Class to hold together data needed to implement sd_ComputeWordDiffs
 */
class stringdiffs
{
public:
	stringdiffs(const String & str1, const String & str2,
		bool case_sensitive, int whitespace, int breakType,
		std::vector<wdiff*> * pDiffs);

	~stringdiffs();

	void BuildWordDiffList();
	void PopulateDiffs();

// Implementation types
private:
	struct word {
		int start; // index of first character of word in original string
		int end;   // index of last character of word in original string
		int hash;
		int bBreak; // Is it a isWordBreak 0 = word -1= whitespace -2 = empty 1 = breakWord
		word(int s = 0, int e = 0, int b = 0, int h = 0) : start(s), end(e), bBreak(b),hash(h) { }
		int length() const { return end+1-start; }
	};

// Implementation methods
private:

	void BuildWordsArray(const String & str, std::vector<word*> * words);
	void InsertInWords(std::vector<word*> &words, int bw);
	int FindPreMatchInWords(const std::vector<word*> &words,const word & needword, int bw, int side) const;
	int FindNextMatchInWords(const std::vector<word*> &words,const word & needword, int bw, int side) const;
	int FindPreSpaceInWords(const std::vector<word*> &words, int bw) const;
	int FindNextSpaceInWords(const std::vector<word*> &words, int bw) const;
	int FindPreNoInsertInWords(const std::vector<word*> &words, int bw) const;
	int FindNextInsertInWords(const std::vector<word*> &words, int bw) const;
	int FindNextNoInsertInWords(const std::vector<word*> &words, int bw) const;
	void MoveInWordsUp(std::vector<word*> &words, int source, int target) const;
	void MoveInWordsDown(std::vector<word*> &words, int source, int target) const;
	UINT Hash(const String & str, int begin, int end, UINT h ) const;
	bool AreWordsSame(const word & word1, const word & word2) const;
	bool IsWord(const word & word1) const;
	bool IsSpace(const word & word1) const;
	bool IsBreak(const word & word1) const;
	bool IsInsert(const word & word1) const;
	bool caseMatch(TCHAR ch1, TCHAR ch2) const;
	bool RemoveItem(std::vector<word*> &words, int bw) const;

// Implementation data
private:
	const String & m_str1;
	const String & m_str2;
	bool m_case_sensitive;
	int m_whitespace;
	int m_breakType;
	bool m_matchblock;
	std::vector<wdiff*> * m_pDiffs;
	std::vector<word*> m_words1;
	std::vector<word*> m_words2;
	std::vector<wdiff*> m_wdiffs;
};


#endif // stringdiffsi_h_included
