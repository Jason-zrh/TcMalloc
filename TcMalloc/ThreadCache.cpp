#include "ThreadCache.h"
#include "CentralCache.h"


// ThreadCache����size��С���ڴ�
void* ThreadCache::Allocate(size_t size)
{
    assert(size <= MAX_BYTES);
    // �ȶ����ڴ�, ����Ӧ�ø�sizeʲô���뷽ʽ
    size_t alginSize = SizeClass::RoundUp(size);
    // ��������������Ͱ��λ��
    size_t index = SizeClass::Index(size);

    // ���������в�Ϊ�գ�ֱ��ȡһ���ڴ���󷵻�
    if (!_freeLists[index].Empty())
    {
        return _freeLists[index].Pop();
    }
    // ��������Ϊ�գ���CentralCaChe�����ڴ����
    else
    {
        return FetchFromCentralCache(index, size);
    }
}

void ThreadCache::Deallocate(void* ptr, size_t size)
{
    assert(size <= MAX_BYTES);
    assert(ptr);
    // ����Ͱ��λ��
    size_t index = SizeClass::Index(size);
    // ͷ�嵽��������
    _freeLists[index].Push(ptr);
}

// CentralCache��ȡ�ڴ����
// CentralCache�ĽṹҲ�ǹ�ϣͰ������ÿ��Ͱ��װ���Ǻ����к��ڴ�����Span����Щspan��˫������Ľṹ����һ��ӳ���ϵ��threadcache��ͬ�����Կ���ֱ�Ӱ�index������
void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
    // ����ʹ������ʼ���������㷨
    // 1.�ʼ����һ��Ҫ̫�࣬��Ϊ����Ҫ̫���˿����ò���
    // 2.����в�������ڴ��С�����󣬾ͻ᲻��������ֱ������
    // 3.����ԽС������Խ�󣬶���Խ������Խ��

    // ����ThreadCache��ȻֻҪ��һ�������ǻ��Ǹ���һ�������������Լ���������
    size_t batchNum = std::min(_freeLists[index].maxSize(), SizeClass::NumMovSize(size));
    if (batchNum == _freeLists[index].maxSize())
    {
        // ��1������ʼ��ȡ��ֱ����ȡ�����ֵ
        // ��������Ե��������ٶȿ���
        _freeLists[index].maxSize() += 1;
    }


    // ����Ͳ���
    void* start = nullptr;
    void* end = nullptr;
    // �ж����ڴ�ͷ��ض��٣����centralcache����Ĳ����ˣ��ͷ��ظ��˶��ٸ�
    // ��Ϊ�п����Ƕ���߳�ͬʱ��centralcache������������������Ϊ����ģʽ������ÿ��Ҫ��Ͱ��
    size_t actulNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size);
    assert(actulNum > 0);

    // ���ֻ������һ������ֱ�ӷ���
    if (actulNum == 1)
    {
        assert(start == end);
        return start;
    }
    // ����ֵ���Ȳ�ֹһ�������ó�һ��������
    else
    {
        _freeLists[index].PushRange(NextObj(start), end);
        return start;
    }
}