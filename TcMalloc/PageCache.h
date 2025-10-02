#pragma once
#include "Common.h"


// PageCacheҲʹ�õ���ģʽ
class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_sInst;
	}

	Span* NewSpan(size_t size);

private:
	PageCache()
	{ };
	PageCache(const PageCache&) = delete;
	static PageCache _sInst;

private:
	SpanList _spanLists[NPAGES];
	std::mutex _pageMtx;
};