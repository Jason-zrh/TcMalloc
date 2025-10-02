#pragma once
#include "Common.h"

class ThreadCache
{
public:
    // ������ͷ��ڴ����
    void* Allocate(size_t size);
    void Deallocate(void* ptr, size_t size);

    // �����Ļ�ȡ�ڴ����
    void* FetchFromCentralCache(size_t index, size_t size);




private:
    FreeList _freeLists[NFREE_LISTS];
};

// TLS (thread local storage )��һ�ֱ������淽������������������ڵ��߳�����ȫ�ֿ��Է��ʵģ����ǲ��ܱ������̷߳��ʵ�
// �Ӷ����������ݵ��̶߳�����, ʵ�����̵߳�TLS��������
static thread_local ThreadCache* pTLSThreadCache = nullptr;  // �����һ��static���Ա���ֻ�ڵ�ǰ�ļ��ɼ�

