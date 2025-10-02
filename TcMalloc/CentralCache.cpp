#include "CentralCache.h"
#include "PageCache.h"


// ����ģʽ������
CentralCache CentralCache::_sInst;

Span* CentralCache::GetOneSpan(SpanList& spanlist, size_t size)
{
	// ���������1.��spanlist����span 2.û���ˣ�ȥcentralCache������
	Span* it = spanlist.Begin();
	while (it != spanlist.End())
	{
		if (it->_freeList != nullptr)
		{
			return it;
		}
		else
		{
			it = it->_next;
		}
	}

	// ����PageCacheҪ��
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));

	// �Ѿ����뵽span�ˣ�������Ҫ��span�к��Ժ������
	// ����Ҫͨ��ҳ�����ҳ����ʼ��ַ
	char* start = (char*)(span->_pageId << PAGE_SHIFT);
	// ����һ��span�ж����ڴ�
	size_t bytes = span->_n << PAGE_SHIFT;
	char* end = start + bytes;
	// �Ѵ���ڴ��г��������������
	// ����һ����ͷ������β��
	span->_freeList = start;
	start += size;
	void* tail = span->_freeList;
	while (start < end)
	{
		// �����Ƽ���β����ΪС���ַ���ڴ��������ģ��������бȽϸ�
		NextObj(tail) = start;
		// ����β
		tail = start;
		start += size;
	}

	spanlist.PushFront(span);

	return span;
}

// threadcache��centralcache����������󣬷���ֵ�ǳɹ��������ĸ�����ǰ��������������Ͳ���
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size)
{
	size_t index = SizeClass::Index(size);
	// ����Ͱ��
	_spanList[index]._mtx.lock();

	Span* span = GetOneSpan(_spanList[index], size);
	// spanȡ���ˣ���span���治Ϊ��
	assert(span);
	assert(span->_freeList);

	// ���ҵ�span��ȡ��batchNum������
	start = span->_freeList;
	end = start;
	// ������end������span
	size_t i = 0;
	size_t actualNum = 1;
	// ��span�л�ȡbatchNum����������������м����ü���
	while (i < batchNum - 1 && NextObj(end) != nullptr)
	{
		end = NextObj(end);
		i++;
		actualNum++;
	}
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;

	// ������һ��Ҫ����
	_spanList[index]._mtx.unlock();
	return actualNum;
}



