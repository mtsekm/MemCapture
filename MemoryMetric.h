/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2023 Sky UK
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#pragma once

#include "IMetric.h"

#include <thread>
#include <condition_variable>
#include <map>
#include <mutex>
#include "Platform.h"
#include "GroupManager.h"

#include "Procrank.h"
#include "JsonReportGenerator.h"


class MemoryMetric : public IMetric
{
public:
    MemoryMetric(Platform platform, bool enableZram, std::shared_ptr<JsonReportGenerator> reportGenerator);

    ~MemoryMetric() override;

    void StartCollection(std::chrono::seconds frequency) override;

    void StopCollection() override;

    void SaveResults() override;

private:
    void CollectData(std::chrono::seconds frequency);

    void GetLinuxMemoryUsage();

    void GetCmaMemoryUsage();

    void GetGpuMemoryUsage();

    void GetContainerMemoryUsage();

    void GetMemoryBandwidth();

    void GetBroadcomBmemUsage();

    void CalculateFragmentation();

    // GPU measurements per platform
    void GetGpuMemoryUsageBroadcom();

    void GetGpuMemoryUsageAmlogic();

    void GetGpuMemoryUsageRealtek();

    void GetZramMetrics();

    pid_t tidToParentPid(pid_t tid);

private:
    struct cmaMeasurement
    {
        cmaMeasurement(long _size, Measurement &_used, Measurement &unused)
                : sizeKb(_size),
                  Used(std::move(_used)),
                  Unused(std::move(unused))
        {

        }

        long sizeKb;
        Measurement Used;
        Measurement Unused;
    };

    struct memoryFragmentation
    {
        memoryFragmentation(Measurement &_freePages, Measurement &fragmentation)
                : FreePages(std::move(_freePages)),
                  Fragmentation(std::move(fragmentation))
        {

        }

        Measurement FreePages;
        Measurement Fragmentation;
    };


    struct gpuMeasurement
    {
        gpuMeasurement(Process _process, Measurement _used)
                : ProcessInfo(std::move(_process)),
                  Used(std::move(_used))
        {

        }

        Process ProcessInfo;
        Measurement Used;
    };

    struct zramMeasurement {
        zramMeasurement(Measurement &_uncompressedSize, Measurement &_compressedSize, Measurement &_sysMemUsed,
                        Measurement &_migrated, Measurement &_totalFragmentation, Measurement &_zeroPages,
                        Measurement &_concurrentCompressOps)
            : uncompressedSize(std::move(_uncompressedSize)), compressedSize(std::move(_compressedSize)),
              sysMemUsed(std::move(_sysMemUsed)), migrated(std::move(_migrated)),
              totalFragmentation(std::move(_totalFragmentation)), zeroPages(std::move(_zeroPages)),
              concurrentCompressOps(std::move(_concurrentCompressOps)) {}

        Measurement uncompressedSize;
        Measurement compressedSize;
        Measurement sysMemUsed;         // Actual system memory in use
        Measurement migrated;           // No. of objects migrated by compaction
        Measurement totalFragmentation; // Total fragmentation + meta overhead
        Measurement zeroPages;          // Empty pages with no allocated memory
        Measurement concurrentCompressOps;
    };

    std::thread mCollectionThread;
    bool mQuit;
    std::condition_variable mCv;
    std::mutex mLock;

    size_t mPageSize;

    std::map<std::string, cmaMeasurement> mCmaMeasurements;
    std::map<std::string, Measurement> mLinuxMemoryMeasurements;
    std::map<pid_t, gpuMeasurement> mGpuMeasurements;
    std::map<std::string, Measurement> mContainerMeasurements;
    std::map<std::string, zramMeasurement> mZramMeasurements;
    std::map<std::string, Measurement> mBroadcomBmemMeasurements;

    Measurement mCmaFree;
    Measurement mCmaBorrowed;
    Measurement mMemoryBandwidth;

    bool mMemoryBandwidthSupported;
    bool mGPUMemorySupported;
    bool mZramSupported;

    // Position in vector reflects order
    std::map<std::string, std::vector<memoryFragmentation>> mMemoryFragmentation;

    Platform mPlatform;

    std::map<std::string, std::string> mCmaNames;

    std::shared_ptr<JsonReportGenerator> mReportGenerator;
};
