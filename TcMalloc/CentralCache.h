#pragma once
#include "Common.h"

// ����ģʽ
class CentralCache
{
public:

	// ��ȡһ���ǿ�Span
	Span* GetOneSpan(SpanList& spanList, size_t size);
	 

	size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size);
private:
	// �����͵���ģʽ
	// ���캯���Ϳ���������˽��
	CentralCache()
	{ }

	CentralCache(const CentralCache&) = delete;

	// ����������
	static CentralCache _sInst;

public:
	static CentralCache* GetInstance()
	{
		return &_sInst;
	}

private:
	// centralcache�Ĺ�ϣͰ
	SpanList _spanList[NFREE_LISTS];
};