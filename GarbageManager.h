#ifndef ART_EPOCHE_H
#define ART_EPOCHE_H

#include <atomic>
#include <array>
#include "tbb/enumerable_thread_specific.h"
#include "tbb/combinable.h"

namespace ART
{

    struct LabelDelete
    {
        std::array<void *, 32> nodes;
        uint64_t epoche;
        std::size_t nodesCount;
        LabelDelete *next;
    };

    class DeletionList
    {
        LabelDelete *headDeletionList = nullptr;
        LabelDelete *freeLabelDeletes = nullptr;
        std::size_t deletitionListCount = 0;

    public:
        std::atomic<uint64_t> localEpoche;
        size_t thresholdCounter{0};

        ~DeletionList();
        LabelDelete *head();

        void add(void *n, uint64_t globalEpoch);

        void remove(LabelDelete *label, LabelDelete *prev);

        std::size_t size();

        std::uint64_t deleted = 0;
        std::uint64_t added = 0;
    };

    class GarbageManager;
    class GarbageGuard;

    class ThreadInfo
    {
        friend class GarbageManager;
        friend class GarbageGuard;
        GarbageManager &epoche;
        DeletionList &deletionList;

        DeletionList &getDeletionList() const;

    public:
        ThreadInfo(GarbageManager &epoche);

        ThreadInfo(const ThreadInfo &ti) : epoche(ti.epoche), deletionList(ti.deletionList)
        {
        }

        ~ThreadInfo();

        GarbageManager &getGarbageManager() const;
    };

    class GarbageManager
    {
        friend class ThreadInfo;
        std::atomic<uint64_t> currentEpoche{0};

        tbb::enumerable_thread_specific<DeletionList> deletionLists;

        size_t startGCThreshhold;

    public:
        GarbageManager(size_t startGCThreshhold) : startGCThreshhold(startGCThreshhold) {}

        ~GarbageManager();

        void enterEpoche(ThreadInfo &threadInfo);

        void markNodeForDeletion(void *n, ThreadInfo &threadInfo);

        void exitEpocheAndCleanup(ThreadInfo &info);

        void showDeleteRatio();
    };

    class GarbageGuard
    {
        ThreadInfo &threadEpocheInfo;

    public:
        GarbageGuard(ThreadInfo &threadEpocheInfo) : threadEpocheInfo(threadEpocheInfo)
        {
            threadEpocheInfo.getGarbageManager().enterEpoche(threadEpocheInfo);
        }

        ~GarbageGuard()
        {
            threadEpocheInfo.getGarbageManager().exitEpocheAndCleanup(threadEpocheInfo);
        }
    };

    class GarbageGuardReadonly
    {
    public:
        GarbageGuardReadonly(ThreadInfo &threadEpocheInfo)
        {
            threadEpocheInfo.getGarbageManager().enterEpoche(threadEpocheInfo);
        }

        ~GarbageGuardReadonly()
        {
        }
    };

    inline ThreadInfo::~ThreadInfo()
    {
        deletionList.localEpoche.store(std::numeric_limits<uint64_t>::max());
    }

}

#endif //ART_EPOCHE_H
