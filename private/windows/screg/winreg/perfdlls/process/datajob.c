/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    datajob.c

Abstract:

    a file containing the constant data structures used by the Performance
    Monitor data for the Job Performance data objects

Created:

    Bob Watson  10-Oct-97

Revision History:

    None.

--*/

//  Include Files


#include <windows.h>
#include <winperf.h>
#include <ntprfctr.h>
#include <perfutil.h>
#include "datajob.h"

// dummy variable for field sizing.
static JOB_COUNTER_DATA        jcd;
static JOB_DETAILS_COUNTER_DATA        jdd;


//  Constant structure initializations
//      defined in datajob.h


JOB_DATA_DEFINITION JobDataDefinition = {
    {   0,  // depends on number of instances found
        sizeof(JOB_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        JOB_OBJECT_TITLE_INDEX,
        0,
        JOB_OBJECT_TITLE_INDEX + 1,
        0,
        PERF_DETAIL_ADVANCED,
        (sizeof(JOB_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/
        sizeof(PERF_COUNTER_DEFINITION),
        0,
        0,
        UNICODE_CODE_PAGE,
        {0L,0L},
        {10000000L,0L}
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        JOB_FIRST_COUNTER_INDEX + 2,
        0,
        JOB_FIRST_COUNTER_INDEX + 3,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_100NSEC_TIMER,
        sizeof(jcd.CurrentProcessorTime),
        (DWORD)(ULONG_PTR)&((PJOB_COUNTER_DATA)0)->CurrentProcessorTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        JOB_FIRST_COUNTER_INDEX + 4,
        0,
        JOB_FIRST_COUNTER_INDEX + 5,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_100NSEC_TIMER,
        sizeof(jcd.CurrentUserTime),
        (DWORD)(ULONG_PTR)&((PJOB_COUNTER_DATA)0)->CurrentUserTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        JOB_FIRST_COUNTER_INDEX + 6,
        0,
        JOB_FIRST_COUNTER_INDEX + 7,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_100NSEC_TIMER,
        sizeof(jcd.CurrentKernelTime),
        (DWORD)(ULONG_PTR)&((PJOB_COUNTER_DATA)0)->CurrentKernelTime
    },
#ifdef _DATAJOB_INCLUDE_TOTAL_COUNTERS
    {   sizeof(PERF_COUNTER_DEFINITION),
        JOB_FIRST_COUNTER_INDEX + 8,
        0,
        JOB_FIRST_COUNTER_INDEX + 9,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(jcd.TotalProcessorTime),
        (DWORD)(ULONG_PTR)&((PJOB_COUNTER_DATA)0)->TotalProcessorTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        JOB_FIRST_COUNTER_INDEX + 10,
        0,
        JOB_FIRST_COUNTER_INDEX + 11,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(jcd.TotalUserTime),
        (DWORD)(ULONG_PTR)&((PJOB_COUNTER_DATA)0)->TotalUserTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        JOB_FIRST_COUNTER_INDEX + 12,
        0,
        JOB_FIRST_COUNTER_INDEX + 13,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(jcd.TotalKernelTime),
        (DWORD)(ULONG_PTR)&((PJOB_COUNTER_DATA)0)->TotalKernelTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        JOB_FIRST_COUNTER_INDEX + 22,
        0,
        JOB_FIRST_COUNTER_INDEX + 23,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(jcd.CurrentProcessorUsage),
        (DWORD)(ULONG_PTR)&((PJOB_COUNTER_DATA)0)->CurrentProcessorUsage
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        JOB_FIRST_COUNTER_INDEX + 24,
        0,
        JOB_FIRST_COUNTER_INDEX + 25,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(jcd.CurrentUserUsage),
        (DWORD)(ULONG_PTR)&((PJOB_COUNTER_DATA)0)->CurrentUserUsage
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        JOB_FIRST_COUNTER_INDEX + 26,
        0,
        JOB_FIRST_COUNTER_INDEX + 27,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(jcd.CurrentKernelUsage),
        (DWORD)(ULONG_PTR)&((PJOB_COUNTER_DATA)0)->CurrentKernelUsage
    },
#endif //_DATAJOB_INCLUDE_TOTAL_COUNTERS
    {   sizeof(PERF_COUNTER_DEFINITION),
        JOB_FIRST_COUNTER_INDEX + 14,
        0,
        JOB_FIRST_COUNTER_INDEX + 15,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(jcd.PageFaults),
        (DWORD)(ULONG_PTR)&((PJOB_COUNTER_DATA)0)->PageFaults
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        JOB_FIRST_COUNTER_INDEX + 16,
        0,
        JOB_FIRST_COUNTER_INDEX + 17,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(jcd.TotalProcessCount),
        (DWORD)(ULONG_PTR)&((PJOB_COUNTER_DATA)0)->TotalProcessCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        JOB_FIRST_COUNTER_INDEX + 18,
        0,
        JOB_FIRST_COUNTER_INDEX + 19,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(jcd.ActiveProcessCount),
        (DWORD)(ULONG_PTR)&((PJOB_COUNTER_DATA)0)->ActiveProcessCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        JOB_FIRST_COUNTER_INDEX + 20,
        0,
        JOB_FIRST_COUNTER_INDEX + 21,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(jcd.TerminatedProcessCount),
        (DWORD)(ULONG_PTR)&((PJOB_COUNTER_DATA)0)->TerminatedProcessCount
    }
};

JOB_DETAILS_DATA_DEFINITION JobDetailsDataDefinition = {
    {   0,  // depends on number of instanced found
        sizeof(JOB_DETAILS_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        JOB_DETAILS_OBJECT_TITLE_INDEX,
        0,
        JOB_DETAILS_OBJECT_TITLE_INDEX + 1,
        0,
        PERF_DETAIL_ADVANCED,
        (sizeof(JOB_DETAILS_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/
        sizeof(PERF_COUNTER_DEFINITION),
        0,
        0,
        UNICODE_CODE_PAGE,
        {0L,0L},
        {10000000L,0L}
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        6,
        0,
        189,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_100NSEC_TIMER,
        sizeof(jdd.ProcessorTime),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->ProcessorTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        142,
        0,
        157,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_100NSEC_TIMER,
        sizeof(jdd.UserTime),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->UserTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        144,
        0,
        159,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_100NSEC_TIMER,
        sizeof(jdd.KernelTime),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->KernelTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        172,
        0,
        173,
        0,
        -6,
        PERF_DETAIL_EXPERT,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(jdd.PeakVirtualSize),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->PeakVirtualSize
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        174,
        0,
        175,
        0,
        -6,
        PERF_DETAIL_EXPERT,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(jdd.VirtualSize),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->VirtualSize
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        28,
        0,
        177,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(jdd.PageFaults),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->PageFaults
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        178,
        0,
        179,
        0,
        -5,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(jdd.PeakWorkingSet),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->PeakWorkingSet
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        180,
        0,
        181,
        0,
        -5,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(jdd.TotalWorkingSet),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->TotalWorkingSet
    },
#ifdef _DATAPROC_PRIVATE_WS_
    {   sizeof(PERF_COUNTER_DEFINITION),
        1478,
        0,
        1479,
        0,
        -5,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(jdd.PrivateWorkingSet),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->PrivateWorkingSet
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1480,
        0,
        1481,
        0,
        -5,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(jdd.SharedWorkingSet),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->SharedWorkingSet
    },
#endif
    {   sizeof(PERF_COUNTER_DEFINITION),
        182,
        0,
        183,
        0,
        -6,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(jdd.PeakPageFile),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->PeakPageFile
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        184,
        0,
        185,
        0,
        -6,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(jdd.PageFile),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->PageFile
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        186,
        0,
        187,
        0,
        -5,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(jdd.PrivatePages),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->PrivatePages
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        680,
        0,
        681,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(jdd.ThreadCount),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->ThreadCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        682,
        0,
        683,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(jdd.BasePriority),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->BasePriority
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        684,
        0,
        685,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_ELAPSED_TIME,
        sizeof(jdd.ElapsedTime),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->ElapsedTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        784,
        0,
        785,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(jdd.ProcessId),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->ProcessId
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1410,
        0,
        1411,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(jdd.CreatorProcessId),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->CreatorProcessId
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        56,
        0,
        57,
        0,
        -5,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(jdd.PagedPool),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->PagedPool
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        58,
        0,
        59,
        0,
        -5,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(jdd.NonPagedPool),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->NonPagedPool
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        952,
        0,
        953,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(jdd.HandleCount),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->HandleCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1412,
        0,
        1413,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(jdd.ReadOperationCount),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->ReadOperationCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1414,
        0,
        1415,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(jdd.WriteOperationCount),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->WriteOperationCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1416,
        0,
        1417,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(jdd.DataOperationCount),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->DataOperationCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1418,
        0,
        1419,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(jdd.OtherOperationCount),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->OtherOperationCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1420,
        0,
        1421,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(jdd.ReadTransferCount),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->ReadTransferCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1422,
        0,
        1423,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(jdd.WriteTransferCount),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->WriteTransferCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1424,
        0,
        1425,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(jdd.DataTransferCount),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->DataTransferCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1426,
        0,
        1427,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(jdd.OtherTransferCount),
        (DWORD)(ULONG_PTR)&((PJOB_DETAILS_COUNTER_DATA)0)->OtherTransferCount
    }
};