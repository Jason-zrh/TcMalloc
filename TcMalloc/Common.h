#pragma once
#include <iostream>
#include <vector>
#include <time.h>
#include <assert.h>
#include <thread>
#include <mutex>
#include <algorithm>

#ifdef _WIN64
typedef unsigned long long PAGE_ID;
#elif _WIN32
typedef size_t PAGE_ID;
#endif // _WIN64

using std::cin;
using std::cout;
using std::endl;

// threadcache��󻺴�Ϊ256kb
static const size_t MAX_BYTES = 256 * 1024;
static const size_t NFREE_LISTS = 208;
static const size_t NPAGES = 128;
// ���ﶨ��һҳ��8kb
static const size_t PAGE_SHIFT = 13;

// ���һ���ֽڶ���һ�εĻ�������20w+���������������ǲ�������Ķ��뷽ʽ
// ���ֶ��뷽ʽ��������10%���ҵ�����Ƭ�˷�
// ���뷽ʽ                  �����ڴ���С��Χ                    ����������±�
// ---------------------------------------------------------------------
// 8bits          |      [1, 128]                         |    [0, 16)
// 16bits         |      [128 + 1, 1024]                  |    [16, 72)
// 128bits        |      [1024 + 1, 8 * 1024]             |    [72, 128)
// 1024bits       |      [8 * 1024 + 1, 64 * 1024]        |    [128, 184)
// 8 * 1024bits   |      [64 * 1024 + 1, 256 * 1024]      |    [184, 208) 


class SizeClass
{
public:
    // �ڴ�����Ӻ���
    // size_t _RoundUp(size_t size, size_t alignNum) // ����1Ϊ�������ڴ��С�� ����2Ϊ�����ڴ�
    // {
    //     size_t alignSize = 0; 
    //     if(size % alignNum != 0)
    //     {
    //         // ��Ҫ�����ֶ�����
    //         alignSize = (size / alignNum + 1) * alignNum;
    //     }
    //     else
    //     {
    //         // ���Ա�������������˵����������size�Ѿ��Զ�������
    //         alignSize = size;
    //     }
    //     return alignSize;
    // }

    // Դ����ʵ�ַ�ʽ -> λ���㣬���ܸ��ã�Ч�ʸ���
    static size_t _RoundUp(size_t size, size_t alignNum)
    {
        // �ǳ�����
        return (size + alignNum - 1) & ~(alignNum - 1);
    }


    // �ڴ����ʵ�� 
    static inline size_t RoundUp(size_t size)
    {
        if (size <= 128)
        {
            // 8bits���뷽ʽ
            return _RoundUp(size, 8);
        }
        else if (size <= 1024)
        {
            // 16bits���뷽ʽ
            return _RoundUp(size, 16);
        }
        else if (size <= 8 * 1024)
        {
            // 128bits���뷽ʽ
            return _RoundUp(size, 128);
        }
        else if (size <= 64 * 1024)
        {
            // 1024bits���뷽ʽ
            return _RoundUp(size, 1024);
        }
        else if (size <= 256 * 1024)
        {
            // 8 * 1024bits���뷽ʽ
            return _RoundUp(size, 8 * 1024);
        }
        else
        {
            assert(false);
            return -1;
        }
    }


    // �Լ�ʵ�֣������е�����
    // size_t _Index(size_t size, size_t alignNum)
    // {
    //     if(size % alignNum == 0)
    //     {
    //         return size / alignNum - 1;
    //     }
    //     else
    //     {
    //         return size / alignNum;
    //     }
    // }


    // Դ����ʵ�ַ�ʽ ����ʹ��λ����
    static inline size_t _Index(size_t byte, size_t alignNum_shift)
    {
        return ((byte + (1 << alignNum_shift) - 1) >> alignNum_shift) - 1;
    }

    static size_t Index(size_t bytes)
    {
        assert(bytes <= MAX_BYTES);
        static int array[4] = { 16, 56, 56, 56 }; // ���ÿ�������ж��ٸ����� ʵ������16��56��56��56��34����������
        if (bytes <= 128)
        {
            // 8bits���룬�ڵ�һ������,2^3
            return _Index(bytes, 3);
        }
        else if (bytes <= 1024)
        {
            // 16bits���룬�ڵڶ�������, 2^4
            return _Index(bytes - 128, 4) + array[0];
        }
        else if (bytes <= 8 * 1024)
        {
            // 256bits���룬�ڵ���������, 2^7
            return _Index(bytes - 1024, 7) + array[0] + array[1];

        }
        else if (bytes <= 64 * 1024)
        {
            // 1024bits���룬�ڵ��ĸ�����, 2^10
            return _Index(bytes - 64 * 1024, 10) + array[0] + array[1] + array[2];

        }
        else if (bytes <= 256 * 1024)
        {
            // 8 * 1024bits���룬�ڵ��������
            return _Index(bytes - 64 * 1024, 13) + array[0] + array[1] + array[2] + array[3];
        }
        else
        {
            assert(false);
            return -1;
        }
    }


    // ����ʼ�㷨������threadCacheһ�δ�centralCache��ö��ٸ�����, ʹ�ô��㷨���Կ���һ�λ��[2, 512]������
    static size_t NumMovSize(size_t size)
    {
        if (size == 0)
        {
            return 0;
        }

        // ���һ���ڴ�ռ�ϴ������ٷ���2��
        size_t num = MAX_BYTES / size;
        if (num < 2)
        {
            num = 2;
        }

        // ���һ���ڴ�ռ��С����������512��
        if (num > 512)
        {
            num = 512;
        }
        return num;
    }   

    // ����һ����ϵͳҪ��ҳ
    static size_t NumMovePage(size_t size)
    {
        size_t num = NumMovSize(size);
        size_t npage = num * size;
        npage >>= PAGE_SHIFT;
        if (npage == 0)
        {
            npage = 1;
        }
        return npage;
    }
};




// ����ȡ����������ǰ4 / 8���ֽ�
static void*& NextObj(void* obj)
{
    return *((void**)obj);
}


// ��������Ķ���
class FreeList
{
public:
    void Push(void* obj)
    {
        assert(obj);
        // ͷ��
        NextObj(obj) = _freeList;
        _freeList = obj;
    }

    // ����һ��ֵ
    void PushRange(void* start, void* end)
    {
        NextObj(end) = _freeList;
        _freeList = start;
    }

    void* Pop()
    {
        assert(_freeList);
        // ͷɾ
        void* obj = _freeList;
        _freeList = NextObj(obj);
        return obj;
    }

    bool Empty()
    {
        return _freeList == nullptr;
    }

    size_t& maxSize()
    {
        return _maxSize;
    }

private:
    void* _freeList = nullptr;
    size_t _maxSize = 1;
};

// �������ȵ��ڴ�ռ�
struct Span
{
    PAGE_ID _pageId = 0; // �������ڴ����ʼҳ��ҳ��
    size_t _n = 0;       // ҳ������

    Span* _next = nullptr;     // ˫������Ľṹ
    Span* _prev = nullptr;

    size_t _useCount = 0; // �кõ�С���ڴ汻�ָ�threadCache�ļ���

    void* _freeList = nullptr; // ��������
};


class SpanList
{
public:
    // Ĭ�Ϲ��캯��
    SpanList()
    {
        // ͷ�ڵ��ʼ��
        _head = new Span;
        _head->_next = _head;
        _head->_prev = _head;
    }

    Span* Begin()
    {
        return _head->_next;
    }
    Span* End()
    {
        return _head->_prev;
    }


    void Insert(Span* pos, Span* newSpan)
    {
        assert(pos);
        assert(newSpan);
        // ˫���ͷѭ������Ĳ���
        Span* prev = pos->_prev;
        prev->_next = newSpan;
        newSpan->_prev = prev;
        newSpan->_next = pos;
        pos->_prev = newSpan;
    }

    void Erase(Span* pos)
    {
        assert(pos);
        assert(pos != _head);
        Span* prev = pos->_prev;
        Span* next = pos->_next;
        prev->_next = next;
        next->_prev = prev;
        // ����������Ҫ��ɾ����Span���л���
        // ...
    }

    void PushFront(Span* span)
    {
        Insert(Begin(), span);
    }
private:
    Span* _head;
public:
    std::mutex _mtx; // Ͱ��
};