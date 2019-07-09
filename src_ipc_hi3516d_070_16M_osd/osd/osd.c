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
#include <stdlib.h>
#include <unistd.h>

#include "hi_common.h"
#include "hi_comm_sys.h"
#include "hi_comm_vpss.h"
#include "hi_comm_vgs.h"
#include "hi_comm_video.h"
#include "hi_comm_venc.h"

#include "mpi_sys.h"
#include "mpi_vpss.h"
#include "mpi_vgs.h"
#include "mpi_venc.h"

#include "osd.h"

#define DEBUG_VGS_TIME              (0)         // 测试VGS画图耗时，3~5 ms/fps

#define ASSERT_PARAM(exp)           do { \
    if (!(exp)) { \
        printf("%s %d: err\n", __FUNCTION__, __LINE__); \
        return ERR_INVALID_PARAM; \
    } \
} while (0)

#define PARAM_USE(p)                ((void)p)
#define ALIGN_UP(x, a)              ((x+a-1)&(~(a-1)))
#define ALIGN_BACK(x, a)            ((a) * (((x) / (a))))

#define FONT_16CH_FILE              "/saferlib/font/HZK16"
#define FONT_16ASC_FILE             "/saferlib/font/ASC16"
#define FONT_ASCI_SIZE              (16 * 8 / 8)
#define BIT_I(ch, i)                ((ch)&(1<<i))
#define PIX_SIZE                    (4)

static int              gs32OsdThreadRunning = 0;
static pthread_t        gstOsdthreadId = 0;
static pthread_mutex_t  gstOsdMutex = PTHREAD_MUTEX_INITIALIZER;

static char             gacPath[128] = "/opt/user/osd.cfg";

static OSD_ST           gastOsd[MAX_OSD_NUM];

static VGS_ADD_OSD_S    gastVgsText[MAX_OSD_NUM][MAX_TEXT_LINE_NUM];
static HI_VOID *        gavpVgsTextVirtualAddr[MAX_OSD_NUM][MAX_TEXT_LINE_NUM];

static VGS_ADD_OSD_S    gastVgsPolygon[MAX_POLYGON_NUM];
static HI_VOID *        gavpVgsPolygonVirtualAddr[MAX_POLYGON_NUM];

static void     osd_cfg_init(const char *path);
static void *   osd_thread(void *arg);
static void     osd_createText(const unsigned char *pu8Str, VGS_ADD_OSD_S *pstVgsOsd, HI_VOID **ppvVirtualAddr);
static void     osd_createPolygon(const POINT_S *pstPoint, unsigned int u32Size, VGS_ADD_OSD_S *pstVgsOsd, HI_VOID **ppvVirtualAddr);
static HI_BOOL  osd_is_inside(const POINT_S *pstSrc, const POINT_S *pstPoint, unsigned int u32Size);

static void osd_cfg_init(const char *path)
{
    int i = 0, j = 0;
    FILE *fp = NULL;

    memset(gastOsd, 0, sizeof(gastOsd));

    if (path != NULL) {
        strcpy(gacPath, path);
    }

    if (-1 == access(gacPath, F_OK)) {
        fp = fopen(path, "wb");

        if (NULL == fp) {
            printf("%s %d err: fopen failed\n", __FUNCTION__, __LINE__);
            return;
        }

        for (i = 0; i < MAX_POLYGON_NUM; ++i) {
            gastOsd[i].enType = OSD_POLYGON;
            gastOsd[i].unData.stPolygon.u32Id = i;
        }

        for (; i < MAX_OSD_NUM; ++i) {
            gastOsd[i].enType = OSD_HOTSPOT;
            gastOsd[i].unData.stHotspot.u32Id = i;
        }

        fwrite(gastOsd, 1, sizeof(gastOsd), fp);
        fsync(fileno(fp));
        fclose(fp); fp = NULL;
        printf("init osd default cfg\n");
        return;
    }

    fp = fopen(gacPath, "r+b");
    if (NULL == fp) {
        printf("%s %d err: fopen failed\n", __FUNCTION__, __LINE__);
        return;
    }

    fread(gastOsd, 1, sizeof(gastOsd), fp);
    fclose(fp);
    fp = NULL;

    for (i = 0; i < MAX_OSD_NUM; ++i) {
        OSD_Dump(&gastOsd[i]);
    }

    pthread_mutex_lock(&gstOsdMutex);
    for (i = 0; i < MAX_OSD_NUM; ++i) {
        if (OSD_POLYGON == gastOsd[i].enType && gastOsd[i].unData.stPolygon.u32Enable) {
            POINT_S astPoint[MAX_POLYGON_POINT_NUM];

            // malloc new data polygon
            for (j = 0; j < gastOsd[i].unData.stPolygon.u32PointNum; ++j) {
                astPoint[j].s32X = (int)ALIGN_UP(gastOsd[i].unData.stPolygon.astPoint[j].u32X, 2);
                astPoint[j].s32Y = (int)ALIGN_UP(gastOsd[i].unData.stPolygon.astPoint[j].u32Y, 2);
            }
            gastVgsPolygon[i].u32BgColor = gastOsd[i].unData.stPolygon.u32BgColor;

            osd_createPolygon(astPoint, gastOsd[i].unData.stPolygon.u32PointNum,
                            &gastVgsPolygon[i],
                            &gavpVgsPolygonVirtualAddr[i]);

            // malloc new data text
            for (j = 0; j < gastOsd[i].unData.stPolygon.stText.u32LineNum; ++j) {
                gastVgsText[i][j].stRect.s32X = (int)ALIGN_UP(gastOsd[i].unData.stPolygon.stText.astStartPoint[j].u32X, 2);
                gastVgsText[i][j].stRect.s32Y = (int)ALIGN_UP(gastOsd[i].unData.stPolygon.stText.astStartPoint[j].u32Y, 2);
                gastVgsText[i][j].u32BgColor = gastOsd[i].unData.stPolygon.stText.u32Color[j];

                osd_createText(gastOsd[i].unData.stPolygon.stText.au8TextCode[j],
                           &gastVgsText[i][j], &gavpVgsTextVirtualAddr[i][j]);

            }
        }
        else if (OSD_HOTSPOT == gastOsd[i].enType && gastOsd[i].unData.stHotspot.u32Enable) {
            // malloc new data txt
            for (j = 0; j < gastOsd[i].unData.stHotspot.stText.u32LineNum; ++j) {
                gastVgsText[i][j].stRect.s32X = (int)ALIGN_UP(gastOsd[i].unData.stHotspot.stText.astStartPoint[j].u32X, 2);
                gastVgsText[i][j].stRect.s32Y = (int)ALIGN_UP(gastOsd[i].unData.stHotspot.stText.astStartPoint[j].u32Y, 2);
                gastVgsText[i][j].u32BgColor = gastOsd[i].unData.stHotspot.stText.u32Color[j];

                osd_createText(gastOsd[i].unData.stHotspot.stText.au8TextCode[j],
                           &gastVgsText[i][j], &gavpVgsTextVirtualAddr[i][j]);

            }
        }
    }
    pthread_mutex_unlock(&gstOsdMutex);
}

static void *osd_thread(void *arg)
{
    HI_S32 s32Ret;
    VIDEO_FRAME_INFO_S stExtFrmInfo;
    VPSS_GRP VpssGrp = 0;
    VPSS_CHN VpssChn = 4;
    VENC_CHN VeChn   = 0;
    HI_S32 s32MilliSec = -1; //1000;

    VGS_HANDLE VgsHandle = -1;
    VGS_TASK_ATTR_S stVgsTask;
    OSD_ST   astOsd[MAX_OSD_NUM];
    OSD_ST   astOsdOld[MAX_OSD_NUM];
    VGS_DRAW_LINE_S astVgsDrawLine[MAX_POLYGON_NUM][MAX_POLYGON_POINT_NUM];
    VGS_DRAW_LINE_S astVgsDrawCrossStar[MAX_HOTSPOT_NUM][MAX_HOTSPOT_POINT_NUM * 2];
    int i, j, s32PolygonCnt = 0, s32HotspotCnt = 0, s32JobInit = 0;

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

    if (OSD_GetAll(astOsdOld) != ERR_SUCCESS) {
        printf("%s %d err: OSD_GetAll failed", __FUNCTION__, __LINE__);
        return NULL;
    }

    while (gs32OsdThreadRunning) {

        if (OSD_GetAll(astOsd) != ERR_SUCCESS) {
            printf("%s %d err: OSD_GetAll failed", __FUNCTION__, __LINE__);
            continue;
        }

        // if osd parameter change, VGS_ADD_OSD_S need modify.
        for (i = 0; i < MAX_OSD_NUM; ++i) {
            // need modify osd
            if (!memcmp(&astOsd[i], &astOsdOld[i], sizeof(OSD_ST)))
                continue;

            if (OSD_POLYGON == astOsd[i].enType) {
                if (astOsd[i].unData.stPolygon.u32BgColor != astOsdOld[i].unData.stPolygon.u32BgColor) {
                    gastVgsPolygon[i].u32BgColor = astOsd[i].unData.stPolygon.u32BgColor;
                }

                // Polygon
                if (memcmp(astOsd[i].unData.stPolygon.astPoint, astOsdOld[i].unData.stPolygon.astPoint,
                    astOsdOld[i].unData.stPolygon.u32PointNum) != 0) {
                    // free old data
                    if (astOsdOld[i].unData.stPolygon.u32Enable) {
                            s32Ret = HI_MPI_SYS_MmzFree(gastVgsPolygon[i].u32PhyAddr, gavpVgsPolygonVirtualAddr[i]);
                            if (s32Ret != HI_SUCCESS) {
                                printf("%s %d err:HI_MPI_SYS_MmzFree fail,Error(%#x)\n", __FUNCTION__, __LINE__, s32Ret);
                            }
                            memset(&gastVgsPolygon[i], 0, sizeof(VGS_ADD_OSD_S));
                            gavpVgsPolygonVirtualAddr[i] = NULL;
                    }

                    // malloc new data
                    if (astOsd[i].unData.stPolygon.u32Enable) {
                        POINT_S astPoint[MAX_POLYGON_POINT_NUM];

                        for (j = 0; j < astOsd[i].unData.stPolygon.u32PointNum; ++j) {
                            astPoint[j].s32X = (int)ALIGN_UP(astOsd[i].unData.stPolygon.astPoint[j].u32X, 2);
                            astPoint[j].s32Y = (int)ALIGN_UP(astOsd[i].unData.stPolygon.astPoint[j].u32Y, 2);
                        }

                        osd_createPolygon(astPoint, astOsd[i].unData.stPolygon.u32PointNum,
                            &gastVgsPolygon[i],
                            &gavpVgsPolygonVirtualAddr[i]);
                    }
                }

                // Text
                if (memcmp(&astOsd[i].unData.stPolygon.stText, &astOsdOld[i].unData.stPolygon.stText, sizeof(TEXT_ST)) != 0) {
                    // free old data
                    if (astOsdOld[i].unData.stPolygon.u32Enable) {
                        for (j = 0; j < astOsdOld[i].unData.stPolygon.stText.u32LineNum; ++j) {
                            s32Ret = HI_MPI_SYS_MmzFree(gastVgsText[i][j].u32PhyAddr, gavpVgsTextVirtualAddr[i][j]);
                            if (s32Ret != HI_SUCCESS) {
                                printf("%s %d err:HI_MPI_SYS_MmzFree fail,Error(%#x)\n", __FUNCTION__, __LINE__, s32Ret);
                            }
                            memset(&gastVgsText[i][j], 0, sizeof(VGS_ADD_OSD_S));
                            gavpVgsTextVirtualAddr[i][j] = NULL;
                        }
                    }

                    // malloc new data
                    if (astOsd[i].unData.stPolygon.u32Enable) {
                        for (j = 0; j < astOsd[i].unData.stPolygon.stText.u32LineNum; ++j) {
                            gastVgsText[i][j].stRect.s32X = (int)ALIGN_UP(astOsd[i].unData.stPolygon.stText.astStartPoint[j].u32X, 2);
                            gastVgsText[i][j].stRect.s32Y = (int)ALIGN_UP(astOsd[i].unData.stPolygon.stText.astStartPoint[j].u32Y, 2);
                            gastVgsText[i][j].u32BgColor = astOsd[i].unData.stPolygon.stText.u32Color[j];

                            osd_createText(astOsd[i].unData.stPolygon.stText.au8TextCode[j],
                                       &gastVgsText[i][j], &gavpVgsTextVirtualAddr[i][j]);
                        }
                    }
                }
            }
            else if (OSD_HOTSPOT == astOsd[i].enType) {
                // Text
                if (memcmp(&astOsd[i].unData.stHotspot.stText, &astOsdOld[i].unData.stHotspot.stText, sizeof(TEXT_ST)) != 0) {
                    // free old data
                    if (astOsdOld[i].unData.stHotspot.u32Enable) {
                        for (j = 0; j < astOsdOld[i].unData.stHotspot.stText.u32LineNum; ++j) {
                            s32Ret = HI_MPI_SYS_MmzFree(gastVgsText[i][j].u32PhyAddr, gavpVgsTextVirtualAddr[i][j]);
                            if (s32Ret != HI_SUCCESS) {
                                printf("%s %d err:HI_MPI_SYS_MmzFree fail,Error(%#x)\n", __FUNCTION__, __LINE__, s32Ret);
                            }

                            memset(&gastVgsText[i][j], 0, sizeof(VGS_ADD_OSD_S));
                            gavpVgsTextVirtualAddr[i][j] = NULL;
                        }
                    }

                    // malloc new data
                    if (astOsd[i].unData.stHotspot.u32Enable) {
                        for (j = 0; j < astOsd[i].unData.stHotspot.stText.u32LineNum; ++j) {
                            gastVgsText[i][j].stRect.s32X = (int)ALIGN_UP(astOsd[i].unData.stHotspot.stText.astStartPoint[j].u32X, 2);
                            gastVgsText[i][j].stRect.s32Y = (int)ALIGN_UP(astOsd[i].unData.stHotspot.stText.astStartPoint[j].u32Y, 2);
                            gastVgsText[i][j].u32BgColor = astOsd[i].unData.stHotspot.stText.u32Color[j];

                            osd_createText(astOsd[i].unData.stHotspot.stText.au8TextCode[j],
                                       &gastVgsText[i][j], &gavpVgsTextVirtualAddr[i][j]);
                        }
                    }
                }
            }

            astOsdOld[i] = astOsd[i];
        }

        s32Ret = HI_MPI_VPSS_GetChnFrame(VpssGrp, VpssChn, &stExtFrmInfo, s32MilliSec);
        if (s32Ret != HI_SUCCESS) {
            if (s32Ret != HI_ERR_VPSS_BUF_EMPTY) {
                printf("%s %d err:HI_MPI_VPSS_GetChnFrame failed, VPSS_GRP(%d), VPSS_CHN(%d), Error(%#x)!\n",
                    __FUNCTION__, __LINE__, VpssGrp, VpssChn, s32Ret);
            }
            continue;
        }

#if DEBUG_VGS_TIME
        gettimeofday(&tm[0], NULL);
#endif

        s32PolygonCnt = 0;
        s32HotspotCnt = 0;

        for (i = 0; i < MAX_OSD_NUM; ++i) {
            if (OSD_POLYGON == astOsd[i].enType) {
                POLYGON_ST *pst = (POLYGON_ST *)&astOsd[i].unData;

                if (!pst->u32Enable) {
                    continue;
                }

                if (!s32JobInit) {
                    s32Ret = HI_MPI_VGS_BeginJob(&VgsHandle);
                    if (s32Ret != HI_SUCCESS) {
                        printf("%s %d err:Vgs begin job fail,Error(%#x)\n", __FUNCTION__, __LINE__, s32Ret);
                        goto EXT_RELEASE;
                    }

                    memcpy(&stVgsTask.stImgIn, &stExtFrmInfo, sizeof(VIDEO_FRAME_INFO_S));
                    memcpy(&stVgsTask.stImgOut, &stExtFrmInfo, sizeof(VIDEO_FRAME_INFO_S));
                    s32JobInit = 1;
                }

                // OSD polygon
                s32Ret = HI_MPI_VGS_AddOsdTask(VgsHandle, &stVgsTask, &gastVgsPolygon[i]);
                if (s32Ret != HI_SUCCESS) {
                    printf("%s %d err:HI_MPI_VGS_AddOsdTask fail,Error(%#x)\n", __FUNCTION__, __LINE__, s32Ret);
                    HI_MPI_VGS_CancelJob(VgsHandle);
                    goto EXT_RELEASE;
                }

                // 边框
                unsigned int u32Thick = ALIGN_UP(pst->u32Thick, 2);
                if (u32Thick != 0) {
                    for (j = 0; j < pst->u32PointNum; ++j) {
                        astVgsDrawLine[s32PolygonCnt][j].stStartPoint.s32X = (int)ALIGN_UP(pst->astPoint[j].u32X, 2);
                        astVgsDrawLine[s32PolygonCnt][j].stStartPoint.s32Y = (int)ALIGN_UP(pst->astPoint[j].u32Y, 2);

                        astVgsDrawLine[s32PolygonCnt][j].stEndPoint.s32X = (int)ALIGN_UP(pst->astPoint[(j + 1) % pst->u32PointNum].u32X, 2);
                        astVgsDrawLine[s32PolygonCnt][j].stEndPoint.s32Y = (int)ALIGN_UP(pst->astPoint[(j + 1) % pst->u32PointNum].u32Y, 2);

                        astVgsDrawLine[s32PolygonCnt][j].u32Thick = u32Thick > 8 ? 8 : u32Thick;
                        astVgsDrawLine[s32PolygonCnt][j].u32Color = pst->u32Color & 0xFFFFFF;
                    }

                    s32Ret = HI_MPI_VGS_AddDrawLineTaskArray(VgsHandle, &stVgsTask, astVgsDrawLine[s32PolygonCnt], pst->u32PointNum);
                    if (s32Ret != HI_SUCCESS) {
                        printf("%s %d err:HI_MPI_VGS_AddDrawLineTaskArray fail,Error(%#x)\n", __FUNCTION__, __LINE__, s32Ret);
                        HI_MPI_VGS_CancelJob(VgsHandle);
                        goto EXT_RELEASE;
                    }

                    ++s32PolygonCnt;
                }

                // Text
                s32Ret = HI_MPI_VGS_AddOsdTaskArray(VgsHandle, &stVgsTask, gastVgsText[i], pst->stText.u32LineNum);
                if (s32Ret != HI_SUCCESS) {
                    printf("%s %d err:HI_MPI_VGS_AddOsdTaskArray fail,Error(%#x)\n", __FUNCTION__, __LINE__, s32Ret);
                    HI_MPI_VGS_CancelJob(VgsHandle);
                    goto EXT_RELEASE;
                }
            }
            else if (OSD_HOTSPOT == astOsd[i].enType) {
                HOTSPOT_ST* pst = (HOTSPOT_ST *)&astOsd[i].unData;

                if (!pst->u32Enable) {
                    continue;
                }

                if (!s32JobInit) {
                    s32Ret = HI_MPI_VGS_BeginJob(&VgsHandle);
                    if (s32Ret != HI_SUCCESS) {
                        printf("%s %d err:Vgs begin job fail,Error(%#x)\n", __FUNCTION__, __LINE__, s32Ret);
                        goto EXT_RELEASE;
                    }

                    memcpy(&stVgsTask.stImgIn, &stExtFrmInfo, sizeof(VIDEO_FRAME_INFO_S));
                    memcpy(&stVgsTask.stImgOut, &stExtFrmInfo, sizeof(VIDEO_FRAME_INFO_S));
                    s32JobInit = 1;
                }

                // Cross Star
                for (j = 0; j < pst->u32PointNum; ++j) {
                    unsigned int u32X = ALIGN_UP(pst->astPoint[j].u32X, 2);
                    unsigned int u32Y = ALIGN_UP(pst->astPoint[j].u32Y, 2);

                    // -
                    astVgsDrawCrossStar[s32HotspotCnt][j * 2].stStartPoint.s32X = (int)(u32X >= 2 ? u32X - 2 : u32X);
                    astVgsDrawCrossStar[s32HotspotCnt][j * 2].stStartPoint.s32Y = (int)u32Y;
                    astVgsDrawCrossStar[s32HotspotCnt][j * 2].stEndPoint.s32X = (int)(u32X + 4);
                    astVgsDrawCrossStar[s32HotspotCnt][j * 2].stEndPoint.s32Y = (int)u32Y;
                    astVgsDrawCrossStar[s32HotspotCnt][j * 2].u32Thick = 2;
                    astVgsDrawCrossStar[s32HotspotCnt][j * 2].u32Color = pst->u32Color[j] & 0xFFFFFF;

                    // |
                    astVgsDrawCrossStar[s32HotspotCnt][j * 2 + 1].stStartPoint.s32X = (int)u32X;
                    astVgsDrawCrossStar[s32HotspotCnt][j * 2 + 1].stStartPoint.s32Y = (int)(u32Y >= 2 ? u32Y -2 : u32Y);
                    astVgsDrawCrossStar[s32HotspotCnt][j * 2 + 1].stEndPoint.s32X = (int)u32X;
                    astVgsDrawCrossStar[s32HotspotCnt][j * 2 + 1].stEndPoint.s32Y = (int)u32Y + 4;
                    astVgsDrawCrossStar[s32HotspotCnt][j * 2 + 1].u32Thick = 2;
                    astVgsDrawCrossStar[s32HotspotCnt][j * 2 + 1].u32Color = pst->u32Color[j] & 0xFFFFFF;
                }

                s32Ret = HI_MPI_VGS_AddDrawLineTaskArray(VgsHandle, &stVgsTask, astVgsDrawCrossStar[s32HotspotCnt], pst->u32PointNum * 2);
                if (s32Ret != HI_SUCCESS) {
                    printf("%s %d err:HI_MPI_VGS_AddDrawLineTaskArray fail,Error(%#x)\n", __FUNCTION__, __LINE__, s32Ret);
                    HI_MPI_VGS_CancelJob(VgsHandle);
                    goto EXT_RELEASE;
                }

                ++s32HotspotCnt;

                // Text
                s32Ret = HI_MPI_VGS_AddOsdTaskArray(VgsHandle, &stVgsTask, gastVgsText[i], pst->stText.u32LineNum);
                if (s32Ret != HI_SUCCESS) {
                    printf("%s %d err:HI_MPI_VGS_AddOsdTaskArray fail,Error(%#x)\n", __FUNCTION__, __LINE__, s32Ret);
                    HI_MPI_VGS_CancelJob(VgsHandle);
                    goto EXT_RELEASE;
                }
            }
            else {
                printf("%s %d err: unknow type(%d)\n", __FUNCTION__, __LINE__, astOsd[i].enType);
                continue;
            }
        }

        if (s32JobInit) {
            s32Ret = HI_MPI_VGS_EndJob(VgsHandle);
            if (s32Ret != HI_SUCCESS) {
                printf("%s %d err:HI_MPI_VGS_EndJob fail,Error(%#x)\n", __FUNCTION__, __LINE__, s32Ret);
                HI_MPI_VGS_CancelJob(VgsHandle);
                goto EXT_RELEASE;
            }
        }

#if DEBUG_VGS_TIME
        gettimeofday(&tm[1], NULL);
        printf("%d ms\n", ((tm[1].tv_sec * 1000000 + tm[1].tv_usec) - (tm[0].tv_sec * 1000000 + tm[0].tv_usec))/1000);
#endif

EXT_RELEASE:
        s32Ret = HI_MPI_VENC_SendFrame(VeChn, &stExtFrmInfo, 40);
        if (HI_SUCCESS != s32Ret) {
            printf("%s %d err:HI_MPI_VENC_SendFrame fail, chn(%d),Error(%#x)\n",
                    __FUNCTION__, __LINE__, VeChn, s32Ret);
        }

        s32Ret = HI_MPI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, &stExtFrmInfo);
        if (HI_SUCCESS != s32Ret) {
            printf("%s %d err:HI_MPI_VPSS_ReleaseChnFrame fail,Grp(%d) chn(%d),Error(%#x)\n",
                    __FUNCTION__, __LINE__, VpssGrp, VpssChn, s32Ret);
        }

        s32JobInit = 0;
    }

    printf("%s exit ...\n", __FUNCTION__);
    return NULL;
}

static void osd_createText(const unsigned char *pu8Str, VGS_ADD_OSD_S *pstVgsOsd, HI_VOID **ppvVirtualAddr)
{
    if (pu8Str == NULL || pstVgsOsd == NULL || ppvVirtualAddr == NULL) {
        printf("%s %d err: invailed param.\n", __FUNCTION__, __LINE__);
        return;
    }

#if (2 == PIX_SIZE)
        unsigned short **ppu32Buffer = (unsigned short **)ppvVirtualAddr;
#elif (4 == PIX_SIZE)
        unsigned int **ppu32Buffer = (unsigned int **)ppvVirtualAddr;
#endif

    // 获取字模到pu8Font
    int s32Len = strlen((const char *)pu8Str);
    printf("%s %d str:%s, len:%d.\n", __FUNCTION__, __LINE__, pu8Str, s32Len);
    if (s32Len <= 0) {
        printf("%s %d err: u8Str len=0.\n", __FUNCTION__, __LINE__);
        return;
    }

    unsigned char *pu8Font = (unsigned char *)malloc(s32Len * FONT_ASCI_SIZE);

    if (pu8Font == NULL) {
        printf("%s %d err: invailed param.\n", __FUNCTION__, __LINE__);
        return;
    }

    FILE *fpAsc, *fpCn;

    if ((fpAsc = fopen(FONT_16ASC_FILE, "rb")) == NULL) {
        printf("%s %d err: fopen err.\n", __FUNCTION__, __LINE__);
        free(pu8Font);
        return;
    }

    if ((fpCn = fopen(FONT_16CH_FILE, "rb")) == NULL) {
        printf("%s %d err: fopen err.\n", __FUNCTION__, __LINE__);
        free(pu8Font);
        fclose(fpAsc);
        return;
    }

    int i, j, k;
    unsigned char ch1, ch2;

    for (i = 0; i < s32Len; ++i) {
        ch1 = *(pu8Str + i);
        ch2 = i + 1 < s32Len ? *(pu8Str + i + 1) : 0;

        if (ch1 < 0xa1) {
            int pos = ch1 * FONT_ASCI_SIZE;
            if (fseek(fpAsc, pos, SEEK_SET) != 0) {
                printf("%s %d err: fseek err.\n", __FUNCTION__, __LINE__);

                free(pu8Font);
                fclose(fpAsc);
                fclose(fpCn);
                return;
            }

            size_t rdblk = fread(pu8Font + i * FONT_ASCI_SIZE, FONT_ASCI_SIZE, 1, fpAsc);

            if (rdblk != 1) {
                printf("%s %d err: fread err.\n", __FUNCTION__, __LINE__);
                free(pu8Font);
                fclose(fpAsc);
                fclose(fpCn);
                return;
            }
        }
        else if (ch2 >= 0xa1) {
            int pos = (94 * (ch1 - 0xa1) + (ch2 - 0xa1)) * FONT_ASCI_SIZE * 2;
            if (fseek(fpCn, pos, SEEK_SET) != 0) {
                printf("%s %d err: fseek err.\n", __FUNCTION__, __LINE__);

                free(pu8Font);
                fclose(fpAsc);
                fclose(fpCn);
                return;
            }

            unsigned char tmp[FONT_ASCI_SIZE * 2];
            size_t rdblk = fread(tmp, FONT_ASCI_SIZE * 2, 1, fpCn);

            if (rdblk != 1) {
                printf("%s %d err: fread err.\n", __FUNCTION__, __LINE__);
                free(pu8Font);
                fclose(fpAsc);
                fclose(fpCn);
                return;
            }

            for (j = 0; j < FONT_ASCI_SIZE; j++) {
              *(pu8Font + i * FONT_ASCI_SIZE + j) = tmp[j * 2];
              *(pu8Font + i * FONT_ASCI_SIZE + FONT_ASCI_SIZE + j) = tmp[j * 2 + 1];
            }
            ++i;
        }
    }

    fclose(fpAsc);
    fclose(fpCn);

    // 生成OSD
    pstVgsOsd->stRect.u32Height = ALIGN_UP(FONT_ASCI_SIZE, 2);
    pstVgsOsd->stRect.u32Width  = ALIGN_UP(s32Len * 8, 2);
    pstVgsOsd->enPixelFmt       = (PIX_SIZE == 2 ? PIXEL_FORMAT_RGB_1555 : PIXEL_FORMAT_RGB_8888);
    pstVgsOsd->u32Stride        = pstVgsOsd->stRect.u32Width * PIX_SIZE;
    pstVgsOsd->u32BgAlpha       = 0;
    pstVgsOsd->u32FgAlpha       = 0xFF;

    HI_S32 s32Ret = HI_MPI_SYS_MmzAlloc(&pstVgsOsd->u32PhyAddr, ppvVirtualAddr, NULL, NULL,
        pstVgsOsd->stRect.u32Height * pstVgsOsd->u32Stride);
    if (s32Ret != HI_SUCCESS) {
        printf("%s %d err: HI_MPI_SYS_MmzAlloc err(%x).\n", __FUNCTION__, __LINE__, s32Ret);
        free(pu8Font);
        return;
    }

    // 遍历单个字模
    for (i = 0; i < s32Len; i++) {
        int offset_x = i * 8;

        for (j = 0; j < FONT_ASCI_SIZE; j++) {
            unsigned char ch = *(pu8Font + i * FONT_ASCI_SIZE + j);

            for (k = 0; k < 8; k++) {
                if (2 == PIX_SIZE) {
                    *(*ppu32Buffer + offset_x + j*pstVgsOsd->stRect.u32Width + k) = BIT_I(ch, (7 - k)) ? pstVgsOsd->u32BgColor : 0x7FFF;
                }
                else if (4 == PIX_SIZE) {
                    *(*ppu32Buffer + offset_x + j*pstVgsOsd->stRect.u32Width + k) = BIT_I(ch, (7 - k)) ? pstVgsOsd->u32BgColor : 0;
                }
            }
        }
    }

    free(pu8Font);
}

static void osd_createPolygon(const POINT_S *pstPoint, unsigned int u32Size, VGS_ADD_OSD_S *pstVgsOsd, HI_VOID **ppvVirtualAddr)
{
    if (pstPoint == NULL || u32Size < 4 || pstVgsOsd == NULL || ppvVirtualAddr == NULL) {
        printf("%s %d err: invailed param.\n", __FUNCTION__, __LINE__);
        return;
    }

#if (2 == PIX_SIZE)
    unsigned short **ppu32Buffer = (unsigned short **)ppvVirtualAddr;
#elif (4 == PIX_SIZE)
    unsigned int **ppu32Buffer = (unsigned int **)ppvVirtualAddr;
#endif

    HI_S32 s32Left, s32Top, s32Right, s32Bottom;
    HI_S32 i, j;
    POINT_S stPoint;

    s32Left  = pstPoint[0].s32X;
    s32Right = s32Left;
    s32Top   = pstPoint[0].s32Y;
    s32Bottom = s32Top;

    for (i = 0; i < (int)u32Size; i++)
    {
        if (s32Left > pstPoint[i].s32X)
            s32Left = pstPoint[i].s32X;
        if (s32Right < pstPoint[i].s32X)
            s32Right = pstPoint[i].s32X;
        if (s32Top > pstPoint[i].s32Y)
            s32Top = pstPoint[i].s32Y;
        if (s32Bottom < pstPoint[i].s32Y)
            s32Bottom = pstPoint[i].s32Y;
    }

    printf("%s %d info:[%d %d] %dx%d.\n", __FUNCTION__, __LINE__, s32Left, s32Top, s32Right - s32Left, s32Bottom - s32Top);

    pstVgsOsd->stRect.s32X      = s32Left;
    pstVgsOsd->stRect.s32Y      = s32Top;
    pstVgsOsd->stRect.u32Height = s32Bottom - s32Top;
    pstVgsOsd->stRect.u32Width  = s32Right - s32Left;
    pstVgsOsd->enPixelFmt       = (PIX_SIZE == 2 ? PIXEL_FORMAT_RGB_1555 : PIXEL_FORMAT_RGB_8888);
    pstVgsOsd->u32Stride        = (s32Right - s32Left) * PIX_SIZE;
    pstVgsOsd->u32BgAlpha       = 0;
    pstVgsOsd->u32FgAlpha       = 0xFF;

    HI_S32 s32Ret = HI_MPI_SYS_MmzAlloc(&pstVgsOsd->u32PhyAddr, ppvVirtualAddr, NULL, NULL,
        pstVgsOsd->stRect.u32Height * pstVgsOsd->u32Stride);
    if (s32Ret != HI_SUCCESS) {
        printf("%s %d err: HI_MPI_SYS_MmzAlloc err(%x).\n", __FUNCTION__, __LINE__, s32Ret);
        return;
    }

    for (i = s32Top; i < s32Bottom; ++i) {
        for (j = s32Left; j < s32Right; ++j) {
            stPoint.s32X = j;
            stPoint.s32Y = i;

            if (2 == PIX_SIZE) {
                *(*ppu32Buffer + (stPoint.s32Y - s32Top) * pstVgsOsd->stRect.u32Width + (stPoint.s32X - s32Left)) = (osd_is_inside(&stPoint, pstPoint, u32Size) ? pstVgsOsd->u32BgColor : 0x7FFF);
            }
            else if (4 == PIX_SIZE) {
                *(*ppu32Buffer + (stPoint.s32Y - s32Top) * pstVgsOsd->stRect.u32Width + (stPoint.s32X - s32Left)) = (osd_is_inside(&stPoint, pstPoint, u32Size) ? pstVgsOsd->u32BgColor : 0);
            }
        }
    }
}

static inline double osd_multi(const POINT_S *pst1, const POINT_S *pst2, const POINT_S *pst0)
{
    return (pst1->s32X - pst0->s32X) * (pst2->s32Y - pst0->s32Y) - (pst2->s32X - pst0->s32X) * (pst1->s32Y - pst0->s32Y);
}

static HI_BOOL osd_is_inside(const POINT_S *pstSrc, const POINT_S *pstPoint, unsigned int u32Size)
{
    double pre, now;
    unsigned int i;

    for (i = 0; i < u32Size; ++i) {
        now = osd_multi(pstSrc, &pstPoint[i], &pstPoint[(i + 1) % u32Size]);
        if (i > 0) {
            if (pre * now <= 0) {
                return HI_FALSE;
            }
        }

        pre = now;
    }

    return HI_TRUE;
}

OSD_ERR_EN OSD_GetBuildVersion(unsigned char *pu8Version)
{
    printf("%s(pu8Version:%p)\n", __FUNCTION__, pu8Version);
    ASSERT_PARAM(pu8Version != NULL);
    //TODO
    unsigned char au8Months[12][4] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    unsigned char au8TmpDate[16] = {0};
    unsigned char au8Month[4] = {0};
    int s32Year, s32Month, s32Day;

    snprintf((char *)au8TmpDate, sizeof(au8TmpDate), "%s", __DATE__); //"Sep 18 2010"
    sscanf((char *)au8TmpDate, "%s %d %d", au8Month, &s32Day, &s32Year);

    int i;
    for (i = 0; i < 12; i++) {
        if (strncmp((char *)au8Month, (char *)au8Months[i], 3) == 0) {
            s32Month = i + 1;
            break;
        }
    }

    snprintf((char *)pu8Version, 32, "%04d-%02d-%02d %s", s32Year, s32Month, s32Day, __TIME__);
    printf("%s\n", pu8Version);

    return ERR_SUCCESS;
}

OSD_ERR_EN OSD_Start(void *p)
{
    OSD_ERR_EN enRet = ERR_SUCCESS;

    printf("%s(p:%p)\n", __FUNCTION__, p);
    PARAM_USE(p);

    //TODO
    if (gstOsdthreadId)
        return enRet;

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
    if (gstOsdthreadId) {
        gs32OsdThreadRunning = 0;
        pthread_join(gstOsdthreadId, NULL);
        gstOsdthreadId = 0;
    }

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
    int i = 0, j = 0;
    HI_S32 s32Ret;

    printf("%s(pstOsd:%p)\n", __FUNCTION__, pstOsd);

    ASSERT_PARAM(pstOsd != NULL);
    ASSERT_PARAM(OSD_POLYGON == pstOsd->enType || OSD_HOTSPOT == pstOsd->enType);
    if (OSD_POLYGON == pstOsd->enType) {
        ASSERT_PARAM(pstOsd->unData.stPolygon.u32Id >= 0 && pstOsd->unData.stPolygon.u32Id <= 4);
        ASSERT_PARAM(pstOsd->unData.stPolygon.u32PointNum <= MAX_POLYGON_POINT_NUM);
        ASSERT_PARAM(pstOsd->unData.stPolygon.stText.u32LineNum <= MAX_TEXT_LINE_NUM);
    }
    else if (OSD_HOTSPOT == pstOsd->enType) {
        ASSERT_PARAM(pstOsd->unData.stHotspot.u32Id >= 5 && pstOsd->unData.stHotspot.u32Id <= 9);
        ASSERT_PARAM(pstOsd->unData.stHotspot.u32PointNum <= MAX_HOTSPOT_POINT_NUM);
        ASSERT_PARAM(pstOsd->unData.stHotspot.stText.u32LineNum <= MAX_TEXT_LINE_NUM);
    }

    //TODO
    ASSERT_PARAM(0 == pthread_mutex_lock(&gstOsdMutex));

    for (i = 0; i < MAX_OSD_NUM; ++i) {
        if (pstOsd->enType == OSD_POLYGON && pstOsd->unData.stPolygon.u32Id == i) {
            break;
        }
        else if (pstOsd->enType == OSD_HOTSPOT && pstOsd->unData.stHotspot.u32Id == i) {
            break;
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

    if (ERR_SUCCESS == enRet) {
        FILE *fp = fopen(gacPath, "r+b");
        if (NULL == fp) {
            printf("%s %d err: fopen failed\n", __FUNCTION__, __LINE__);
            return ERR_IO_FAILED;
        }

        fseek(fp, i * sizeof(OSD_ST), SEEK_SET);
        fwrite(pstOsd, 1, sizeof(OSD_ST), fp);
        fsync(fileno(fp));
        fclose(fp); fp = NULL;
    }

    return ERR_SUCCESS;
}

OSD_ERR_EN OSD_GetAll(OSD_ST astOsd[MAX_OSD_NUM])
{
    //printf("%s(pstOsd:%p)\n", __FUNCTION__, astOsd);
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

            pu8Str = (unsigned char *)"OSD_POLYGON";
            u32Index += snprintf((char *)au8Buf + u32Index, size, "Type:%s\n", pu8Str);
            u32Index += snprintf((char *)au8Buf + u32Index, size, "Id:%u\n", pst->u32Id);
            u32Index += snprintf((char *)au8Buf + u32Index, size, "Enable:%u\n", pst->u32Enable);
            u32Index += snprintf((char *)au8Buf + u32Index, size, "PointNum:%u\n", pst->u32PointNum);
            for (u32Tmp = 0; u32Tmp < pst->u32PointNum && u32Tmp < MAX_POLYGON_POINT_NUM; ++u32Tmp)
                u32Index += snprintf((char *)au8Buf + u32Index, size, "[%u,%u] ",
                        pst->astPoint[u32Tmp].u32X, pst->astPoint[u32Tmp].u32Y);
            u32Index += snprintf(au8Buf + u32Index, size, "\nBgColor:0x%08X\n", pst->u32BgColor);
            u32Index += snprintf((char *)au8Buf + u32Index, size, "SolidColor:0x%06X\n", pst->u32Color);
            u32Index += snprintf((char *)au8Buf + u32Index, size, "SolidThick:%u\n", pst->u32Thick);

            u32Index += snprintf((char *)au8Buf + u32Index, size, "TextLineNum:%u\n", pst->stText.u32LineNum);
            for (u32Tmp = 0; u32Tmp < pst->stText.u32LineNum && u32Tmp < MAX_TEXT_LINE_NUM; ++u32Tmp)
            {
                u32Index += snprintf((char *)au8Buf + u32Index, size, "Line:%d [%u,%u](0x%08X) %s\n", u32Tmp,
                        pst->stText.astStartPoint[u32Tmp].u32X, pst->stText.astStartPoint[u32Tmp].u32Y,
                        pst->stText.u32Color[u32Tmp], pst->stText.au8TextCode[u32Tmp]);
            }
            break;
        }
        case OSD_HOTSPOT:
        {
            HOTSPOT_ST *pst = (HOTSPOT_ST *)&pstOsd->unData;

            pu8Str = (unsigned char *)"OSD_HOTSPOT";
            u32Index += snprintf((char *)au8Buf + u32Index, size, "Type:%s\n", pu8Str);
            u32Index += snprintf((char *)au8Buf + u32Index, size, "Id:%u\n", pst->u32Id);
            u32Index += snprintf((char *)au8Buf + u32Index, size, "Enable:%u\n", pst->u32Enable);
            u32Index += snprintf((char *)au8Buf + u32Index, size, "PointNum:%u\n", pst->u32PointNum);
            for (u32Tmp = 0; u32Tmp < pst->u32PointNum && u32Tmp < MAX_HOTSPOT_POINT_NUM; ++u32Tmp)
            {
                u32Index += snprintf((char *)au8Buf + u32Index, size, "[%u,%u](0x%06X) ",
                        pst->astPoint[u32Tmp].u32X, pst->astPoint[u32Tmp].u32Y, pst->u32Color[u32Tmp]);
            }
            u32Index += snprintf((char *)au8Buf + u32Index, size, "\nTextLineNum:%u\n", pst->stText.u32LineNum);
            for (u32Tmp = 0; u32Tmp < pst->stText.u32LineNum && u32Tmp < MAX_TEXT_LINE_NUM; ++u32Tmp)
            {
                u32Index += snprintf((char *)au8Buf + u32Index, size, "Line:%d [%u,%u](0x%08X) %s\n", u32Tmp,
                        pst->stText.astStartPoint[u32Tmp].u32X, pst->stText.astStartPoint[u32Tmp].u32Y,
                        pst->stText.u32Color[u32Tmp], pst->stText.au8TextCode[u32Tmp]);
            }
            break;
        }
        default:
            pu8Str = (unsigned char *)"err:unknow osd type\n";
            enRet = ERR_INVALID_PARAM;
            break;
    }

    printf("%s\n", au8Buf);
    printf("---------------------------------------------------------------\n");
    return enRet;
}


