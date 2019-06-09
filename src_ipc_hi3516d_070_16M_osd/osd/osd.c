/* Copyright (C) 
 * 2019 - wangkaichao
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */
/**
 * @file osd.c
 * @brief 
 * @author wangkaichao
 * @version 1.0
 * @date 2019-06-02
 */
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <fcntl.h>

#include "hi_comm_vgs.h"
#include "hi_comm_video.h"
#include "hi_comm_vpss.h"
#include "mpi_vgs.h"

#include "osd.h"

#define DEBUG_VGS_TIME  (0)

#define ASSERT_PARAM(exp) do { \
    if (!(exp)) { \
        printf("%s %d: err\n", __FUNCTION__, __LINE__); \
        return ERR_INVALID_PARAM; \
    } \
} while (0)

#define PARAM_USE(p)    (p = p)

#define ALIGN_UP(x, a)              ((x+a-1)&(~(a-1)))
#define ALIGN_BACK(x, a)            ((a) * (((x) / (a))))

static int gs32OsdThreadRunning = 0;
static pthread_t gstOsdthreadId = 0;

static pthread_mutex_t gstOsdMutex = PTHREAD_MUTEX_INITIALIZER;

static OSD_ST gastOsd[MAX_OSD_NUM];
static OSD_ST gastOsdTmp[MAX_OSD_NUM];
static FILE *gstFile = NULL;

static void     osd_cfg_init(const char *path);
static void *   osd_thread(void *arg);

static void osd_cfg_init(const char *path)
{
    int i = 0;
 
    memset(gastOsd, 0, sizeof(gastOsd));

    if (-1 == access(path, F_OK)) {
        gstFile = fopen(path, "wb");

        if (NULL == gstFile) {
            printf("%s %d err: fopen failed\n", __FUNCTION__, __LINE__);
            return;
        }

        for (; i < MAX_POLYGON_NUM; ++i) {
            gastOsd[i].enType = OSD_POLYGON;
            gastOsd[i].unData.stPolygon.u32Id = i;
        }

        for (; i < MAX_POLYGON_NUM + MAX_HOTSPOT_NUM; ++i) {
            gastOsd[i].enType = OSD_HOTSPOT;
            gastOsd[i].unData.stHotspot.u32Id = i;
        }
       
        fwrite(gastOsd, 1, sizeof(gastOsd), gstFile);
        fsync(fileno(gstFile));
        printf("init osd default cfg\n");
        return;
    }

    gstFile = fopen(path, "r+b");
    if (NULL == gstFile) {
        printf("%s %d err: fopen failed\n", __FUNCTION__, __LINE__);
        return;
    }

    fread(gastOsd, 1, sizeof(gastOsd), gstFile);
    for (i = 0; i < MAX_OSD_NUM; ++i) {
        OSD_Dump(&gastOsd[i]);
    }
}

static void *osd_thread(void *arg)
{
    HI_S32 s32Ret;
    VIDEO_FRAME_INFO_S stExtFrmInfo;
    VPSS_GRP VpssGrp = 0;
    VPSS_CHN VpssChn = 4;
    HI_S32 s32MilliSec = 40;

    VGS_HANDLE VgsHandle = -1;
    VGS_TASK_ATTR_S stVgsTask;
    VGS_DRAW_LINE_S astVgsDrawLine[MAX_POLYGON_NUM][MAX_POLYGON_POINT_NUM];

    int i, j, s32PolygonCnt = 0;
 
#if DEBUG_VGS_TIME 
    struct timeval tm[2];
#endif

    printf("%s(%p) running ...\n", __FUNCTION__, arg);
    prctl(PR_SET_NAME, __FUNCTION__);

    s32Ret = HI_MPI_VPSS_SetDepth(VpssGrp, VpssChn, 3);
    if (s32Ret != HI_SUCCESS) {
        printf("%s %d err: HI_MPI_VPSS_SetDepth\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    while (gs32OsdThreadRunning) {

        if (OSD_GetAll(gastOsdTmp) != ERR_SUCCESS) {
            printf("%s %d err: OSD_GetAll failed", __FUNCTION__, __LINE__);
            continue;
        }

        s32Ret = HI_MPI_VPSS_GetChnFrame(VpssGrp, VpssChn, &stExtFrmInfo, s32MilliSec);
        if (HI_SUCCESS != s32Ret) {
            printf("%s %d err:HI_MPI_VPSS_GetChnFrame failed, VPSS_GRP(%d), VPSS_CHN(%d), Error(%#x)!\n",
                    __FUNCTION__, __LINE__, VpssGrp, VpssChn, s32Ret);
            continue;
        }

#if DEBUG_VGS_TIME
        gettimeofday(&tm[0], NULL);
#endif

        s32Ret = HI_MPI_VGS_BeginJob(&VgsHandle);
        if (s32Ret != HI_SUCCESS) {
            printf("%s %d err:Vgs begin job fail,Error(%#x)\n", __FUNCTION__, __LINE__, s32Ret);
            goto EXT_RELEASE;
        }

        memcpy(&stVgsTask.stImgIn, &stExtFrmInfo, sizeof(VIDEO_FRAME_INFO_S));
        memcpy(&stVgsTask.stImgOut, &stExtFrmInfo, sizeof(VIDEO_FRAME_INFO_S));
        
        s32PolygonCnt = 0;

        for (i = 0; i < MAX_OSD_NUM; ++i) {
            if (OSD_POLYGON == gastOsdTmp[i].enType) {
                POLYGON_ST *pst = (POLYGON_ST *)&gastOsdTmp[i].unData;

                if (!pst->u32Enable) {
                    continue;
                }

                if (pst->u32PointNum > MAX_POLYGON_POINT_NUM) {
                    printf("%s %d err: Polygon point num:%d > %d\n", __FUNCTION__, __LINE__, pst->u32PointNum, MAX_POLYGON_POINT_NUM);
                    continue;
                }

                // 边框
                for (j = 0; j < pst->u32PointNum; ++j) {
                    astVgsDrawLine[s32PolygonCnt][j].stStartPoint.s32X = (int)ALIGN_UP(pst->astPoint[j].u32X, 2);
                    astVgsDrawLine[s32PolygonCnt][j].stStartPoint.s32Y = (int)ALIGN_UP(pst->astPoint[j].u32Y, 2);

                    astVgsDrawLine[s32PolygonCnt][j].stEndPoint.s32X = (int)ALIGN_UP(pst->astPoint[(j + 1) % pst->u32PointNum].u32X, 2);
                    astVgsDrawLine[s32PolygonCnt][j].stEndPoint.s32Y = (int)ALIGN_UP(pst->astPoint[(j + 1) % pst->u32PointNum].u32Y, 2);

                    astVgsDrawLine[s32PolygonCnt][j].u32Thick = ALIGN_UP(pst->u32Thick, 2) > 8 ? 8 : ALIGN_UP(pst->u32Thick, 2);
                    astVgsDrawLine[s32PolygonCnt][j].u32Color = pst->u32Color & 0xFFFFFF;
                }

                s32Ret = HI_MPI_VGS_AddDrawLineTaskArray(VgsHandle, &stVgsTask, astVgsDrawLine[s32PolygonCnt], pst->u32PointNum);  
                if (s32Ret != HI_SUCCESS) {
                    printf("%s %d err:HI_MPI_VGS_AddDrawLineTaskArray fail,Error(%#x)\n", __FUNCTION__, __LINE__, s32Ret);
                    HI_MPI_VGS_CancelJob(VgsHandle);
                    goto EXT_RELEASE;
                }

                // Text
                
                
                ++s32PolygonCnt;
            }
            else if (OSD_HOTSPOT == gastOsdTmp[i].enType) {
                HOTSPOT_ST* pst = (HOTSPOT_ST *)&gastOsdTmp[i].unData;

                if (!pst->u32Enable) {
                    continue;
                }

                // Cross

                // Text

            }
            else {
                continue;
            }
        }

        s32Ret = HI_MPI_VGS_EndJob(VgsHandle);
        if (s32Ret != HI_SUCCESS) {
            printf("%s %d err:HI_MPI_VGS_EndJob fail,Error(%#x)\n", __FUNCTION__, __LINE__, s32Ret);
            HI_MPI_VGS_CancelJob(VgsHandle);
            goto EXT_RELEASE;
        }
 
#if DEBUG_VGS_TIME
        gettimeofday(&tm[1], NULL);
        printf("%d ms\n", ((tm[1].tv_sec * 1000000 + tm[1].tv_usec) - (tm[0].tv_sec * 1000000 + tm[0].tv_usec))/1000);
#endif

EXT_RELEASE:
        s32Ret = HI_MPI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, &stExtFrmInfo);
        if (HI_SUCCESS != s32Ret) {
            printf("%s %d err:HI_MPI_VPSS_ReleaseChnFrame fail,Grp(%d) chn(%d),Error(%#x)\n",
                    __FUNCTION__, __LINE__, VpssGrp, VpssChn, s32Ret);
        }
    }

    printf("%s exit ...\n", __FUNCTION__);
    return NULL;
}

OSD_ERR_EN OSD_GetBuildVersion(unsigned char *pu8Version)
{
    printf("%s(pu8Version:%p)\n", __FUNCTION__, pu8Version);
    ASSERT_PARAM(pu8Version != NULL);
    //TODO
    const unsigned char au8Months[12][4] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    unsigned char au8TmpDate[16] = {0};
    unsigned char au8Month[4] = {0};
    int s32Year, s32Month, s32Day;

    snprintf(au8TmpDate, sizeof(au8TmpDate), "%s", __DATE__); //"Sep 18 2010"
    sscanf(au8TmpDate,"%s %d %d", au8Month, &s32Day, &s32Year);

    int i;
    for (i = 0; i < 12; i++) {
        if (strncmp(au8Month, au8Months[i], 3) == 0) {
            s32Month = i + 1;
            break;
        }
    }

    snprintf(pu8Version, 32, "%04d-%02d-%02d %s", s32Year, s32Month, s32Day, __TIME__);
    printf("%s\n", pu8Version);

    return ERR_SUCCESS;
}

OSD_ERR_EN OSD_Start(void *p)
{
    OSD_ERR_EN enRet = ERR_SUCCESS;

    printf("%s(p:%p)\n", __FUNCTION__, p);
    PARAM_USE(p);
    //TODO
    osd_cfg_init("/opt/user/osd.cfg");

    gs32OsdThreadRunning = 1;
    if (pthread_create(&gstOsdthreadId, NULL, osd_thread, NULL) < 0) {
        printf("%s %d err: create thread failedi\n", __FUNCTION__, __LINE__);
        enRet = ERR_IO_FAILED;
        return enRet;
    }

    return enRet;
}

OSD_ERR_EN OSD_Stop(void *p)
{
    printf("%s(p:%p)\n", __FUNCTION__, p);
    PARAM_USE(p);
    //TODO
    if (gstFile != NULL) {
        fsync(fileno(gstFile));
        fclose(gstFile);
        gstFile = NULL;
    }

    gs32OsdThreadRunning = 0;
    pthread_join(gstOsdthreadId, NULL);

    return ERR_SUCCESS;
}

OSD_ERR_EN OSD_GetActiveId(OSD_TYPE_EN enType, unsigned int *pId)
{
    OSD_ERR_EN enRet = ERR_SUCCESS;
    int i = 0;

    printf("%s(enType:%d, pId:%p)\n", __FUNCTION__, enType, pId);
    ASSERT_PARAM(enType >= 0 && enType < OSD_TYPE_NUM);
    ASSERT_PARAM(pId != NULL);
    //TODO
    ASSERT_PARAM(0 == pthread_mutex_lock(&gstOsdMutex));

    for (i = 0; i < MAX_OSD_NUM; ++i) {
        if (gastOsd[i].enType == enType) {
            if (OSD_POLYGON == enType && !gastOsd[i].unData.stPolygon.u32Enable) {
                *pId = gastOsd[i].unData.stPolygon.u32Id;
                break;
            }
            else if (OSD_HOTSPOT == enType && !gastOsd[i].unData.stHotspot.u32Enable) {
                *pId = gastOsd[i].unData.stPolygon.u32Id;
                break;
            }
        }
    }
 
    if (i >= MAX_OSD_NUM) {
        printf("%s %d err: no useful osd id", __FUNCTION__, __LINE__);
        enRet = ERR_UNKNOWN;
    }

    ASSERT_PARAM(0 == pthread_mutex_unlock(&gstOsdMutex));

    return enRet;
}

OSD_ERR_EN OSD_Get(OSD_ST *pstOsd)
{
    OSD_ERR_EN enRet = ERR_SUCCESS;
    int i = 0;

    printf("%s(pstOsd:%p)\n", __FUNCTION__, pstOsd);
    ASSERT_PARAM(pstOsd != NULL);
    ASSERT_PARAM(OSD_POLYGON == pstOsd->enType || OSD_HOTSPOT == pstOsd->enType);

    //TODO
    ASSERT_PARAM(0 == pthread_mutex_lock(&gstOsdMutex));

    for (i = 0; i < MAX_OSD_NUM; ++i) {
        if (pstOsd->enType == gastOsd[i].enType) {
            if (OSD_POLYGON == pstOsd->enType
                    && pstOsd->unData.stPolygon.u32Id == gastOsd[i].unData.stPolygon.u32Id) {
                break;
            }
            else if (OSD_HOTSPOT == pstOsd->enType
                    && pstOsd->unData.stHotspot.u32Id == gastOsd[i].unData.stHotspot.u32Id) {
                break;
            }
        }
    }
    
    if (i < MAX_OSD_NUM) {
        *pstOsd = gastOsd[i];
        enRet = ERR_SUCCESS;
    }
    else {
        enRet = ERR_UNKNOWN;
    }

    ASSERT_PARAM(0 == pthread_mutex_unlock(&gstOsdMutex));
    return enRet;
}

OSD_ERR_EN OSD_Set(const OSD_ST *pstOsd)
{
    OSD_ERR_EN enRet = ERR_SUCCESS;
    int i = 0;

    printf("%s(pstOsd:%p)\n", __FUNCTION__, pstOsd);
    ASSERT_PARAM(pstOsd != NULL);
    ASSERT_PARAM(OSD_POLYGON == pstOsd->enType || OSD_HOTSPOT == pstOsd->enType);
    //TODO
    ASSERT_PARAM(0 == pthread_mutex_lock(&gstOsdMutex));

    for (i = 0; i < MAX_OSD_NUM; ++i) {
        if (pstOsd->enType == gastOsd[i].enType) {
            if (OSD_POLYGON == pstOsd->enType
                    && pstOsd->unData.stPolygon.u32Id == gastOsd[i].unData.stPolygon.u32Id) {
                break;
            }
            else if (OSD_HOTSPOT == pstOsd->enType
                    && pstOsd->unData.stHotspot.u32Id == gastOsd[i].unData.stHotspot.u32Id) {
                break;
            }
        }
    }
    
    if (i < MAX_OSD_NUM) {
        gastOsd[i] = *pstOsd;
        enRet = ERR_SUCCESS;
    }
    else {
        enRet = ERR_UNKNOWN;
    }

    ASSERT_PARAM(0 == pthread_mutex_unlock(&gstOsdMutex));

    fwrite(gastOsd, 1, sizeof(gastOsd), gstFile);
    fsync(fileno(gstFile));

    return ERR_SUCCESS;
}

OSD_ERR_EN OSD_GetAll(OSD_ST astOsd[MAX_OSD_NUM])
{
    int i;

    printf("%s(pstOsd:%p)\n", __FUNCTION__, astOsd);
    ASSERT_PARAM(astOsd != NULL);

    //TODO
    ASSERT_PARAM(0 == pthread_mutex_lock(&gstOsdMutex));

    memcpy(astOsd, gastOsd, sizeof(OSD_ST) * MAX_OSD_NUM);
 
    ASSERT_PARAM(0 == pthread_mutex_unlock(&gstOsdMutex));
    return ERR_SUCCESS;
}

OSD_ERR_EN OSD_Dump(const OSD_ST *pstOsd)
{
    OSD_ERR_EN enRet = ERR_SUCCESS;
    unsigned char au8Buf[512];
    unsigned int u32Index = 0;
    size_t size = sizeof(au8Buf);

    ASSERT_PARAM(pstOsd != NULL);

    switch (pstOsd->enType)
    {
        unsigned char *pu8Str;
        unsigned int u32Tmp;

        case OSD_POLYGON:
        {
            POLYGON_ST *pst = (POLYGON_ST *)&pstOsd->unData;

            pu8Str = "OSD_POLYGON";
            u32Index += snprintf(au8Buf + u32Index, size, "Type:%s\n", pu8Str);
            u32Index += snprintf(au8Buf + u32Index, size, "Id:%u\n", pst->u32Id);
            u32Index += snprintf(au8Buf + u32Index, size, "Enable:%u\n", pst->u32Enable);
            u32Index += snprintf(au8Buf + u32Index, size, "PointNum:%u\n", pst->u32PointNum);
            for (u32Tmp = 0; u32Tmp < pst->u32PointNum && u32Tmp < MAX_POLYGON_POINT_NUM; ++u32Tmp)
                u32Index += snprintf(au8Buf + u32Index, size, "[%u,%u] ", 
                        pst->astPoint[u32Tmp].u32X, pst->astPoint[u32Tmp].u32Y);
            //u32Index += snprintf(au8Buf + u32Index, size, "\nBgColor:0x%03X\n", pst->u32BgColor);
            //u32Index += snprintf(au8Buf + u32Index, size, "Alpha:%u\n", pst->u32Alpha);

            u32Index += snprintf(au8Buf + u32Index, size, "SolidColor:0x%03X\n", pst->u32Color);
            u32Index += snprintf(au8Buf + u32Index, size, "SolidThick:%u\n", pst->u32Thick);

            u32Index += snprintf(au8Buf + u32Index, size, "TextColor:0x%03X\n", pst->stText.u32Color);
            u32Index += snprintf(au8Buf + u32Index, size, "TextLineNum:%u\n", pst->stText.u32LineNum);
            for (u32Tmp = 0; u32Tmp < pst->stText.u32LineNum && u32Tmp < MAX_TEXT_LINE_NUM; ++u32Tmp)
            {
                u32Index += snprintf(au8Buf + u32Index, size, "Line:%d [%u,%u] %s\n", u32Tmp,
                        pst->stText.astStartPoint[u32Tmp].u32X, pst->stText.astStartPoint[u32Tmp].u32Y, 
                        pst->stText.au8TextCode[u32Tmp]);
            }
            break;
        }
        case OSD_HOTSPOT:
        {
            HOTSPOT_ST *pst = (HOTSPOT_ST *)&pstOsd->unData;

            pu8Str = "OSD_HOTSPOT";
            u32Index += snprintf(au8Buf + u32Index, size, "Type:%s\n", pu8Str);
            u32Index += snprintf(au8Buf + u32Index, size, "Id:%u\n", pst->u32Id);
            u32Index += snprintf(au8Buf + u32Index, size, "Enable:%u\n", pst->u32Enable);
            u32Index += snprintf(au8Buf + u32Index, size, "PointNum:%u\n", pst->u32PointNum);
            for (u32Tmp = 0; u32Tmp < pst->u32PointNum && u32Tmp < MAX_HOTSPOT_POINT_NUM; ++u32Tmp)
                u32Index += snprintf(au8Buf + u32Index, size, "[%u,%u] ", 
                        pst->astPoint[u32Tmp].u32X, pst->astPoint[u32Tmp].u32Y);
            u32Index += snprintf(au8Buf + u32Index, size, "\nColor:0x%03X\n", pst->u32Color);

            u32Index += snprintf(au8Buf + u32Index, size, "TextColor:0x%03X\n", pst->stText.u32Color);
            u32Index += snprintf(au8Buf + u32Index, size, "TextLineNum:%u\n", pst->stText.u32LineNum);
            for (u32Tmp = 0; u32Tmp < pst->stText.u32LineNum && u32Tmp < MAX_TEXT_LINE_NUM; ++u32Tmp)
            {
                u32Index += snprintf(au8Buf + u32Index, size, "Line:%d [%u,%u] %s\n", u32Tmp,
                        pst->stText.astStartPoint[u32Tmp].u32X, pst->stText.astStartPoint[u32Tmp].u32Y, 
                        pst->stText.au8TextCode[u32Tmp]);
            }
            break;
        }
        default:
            pu8Str = "err:unknow osd type\n";
            enRet = ERR_INVALID_PARAM;
            break; 
    }

    printf("%s\n", au8Buf);
    printf("---------------------------------------------------------------n");
    return enRet;
}


