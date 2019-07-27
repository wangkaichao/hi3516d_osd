#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include "osd.h"

#define COLOR_RED           (0xFF0000)
#define COLOR_GREEN         (0x00FF00)
#define COLOR_BLUE          (0x0000FF)

#define ARGB888_RED         (0xFF000000 | 0xFF0000)
#define ARGB888_GREEN       (0xFF000000 | 0x00FF00)
#define ARGB888_BLUE        (0xFF000000 | 0x0000FF)
#define ARGB888_WHITE       (0xFF000000 | 0xFFFFFF)
#define ARGB888_BLACK       (0xFF000000 | 0x000000)


//#define A1555_RED           (0x8000 | 0x7C00)
//#define A1555_GREEN         (0x8000 | 0x03E0)
//#define A1555_BLUE          (0x8000 | 0x001F)

#if 0
static int Sample_main(int argc, char *argv[])
{
    unsigned char au8Ver[32];
    OSD_ST stOsd;
    OSD_ST astOsdRead[10];
    OSD_ST astOsd[10] =
    {
        //polygon
        {
            .enType = OSD_POLYGON,
            .unData.stPolygon =
            {
                0, 1, 10,
                {
                    {70, 10}, {100, 10}, {130, 40}, {160, 70}, {130, 100},
                    {100, 130}, {70, 130}, {40, 100}, {10, 70}, {40, 40}
                },
                0x10000000 | COLOR_BLUE, COLOR_RED, 2,
                {5,
                    {{300, 0}, {300, 30}, {300, 60}, {300, 90}, {300, 120}},
                    {ARGB888_RED, ARGB888_GREEN, ARGB888_BLUE, ARGB888_RED, ARGB888_RED},
                    {"line1~1", "line1_2", "line1_3", "line1_4", "line1_5"}
                }
            }
        },
        {
            OSD_POLYGON,
            .unData.stPolygon =
            {
                1, 1, 10,
                {
                    {270, 10}, {300, 10}, {330, 40}, {360, 70}, {330, 100},
                    {300, 130}, {270, 130}, {240, 100}, {210, 70}, {240, 40}
                },
                0x10000000 | COLOR_GREEN, COLOR_GREEN, 0,
                {1,
                    {{400, 400}},
                    {ARGB888_BLUE},
                    {"一二三四五六七八九十"}
                }
            }
        },
        {
            OSD_POLYGON,
            .unData.stPolygon =
            {
                2, 0, 10,
                {
                    {0, 0}
                },
                0x20 | COLOR_BLUE, COLOR_BLUE, 2,
                {0,
                    {{0, 0}},
                    {ARGB888_RED},
                    {"line1"}
                }
            }
        },
        {
            OSD_POLYGON,
            .unData.stPolygon =
            {
                3, 0, 10,
                {
                    {0, 0}
                },
                0x20 | COLOR_BLUE, COLOR_BLUE, 2,
                {0,
                    {{0, 0}},
                    {ARGB888_RED},
                    {"line1"}
                }
            }
        },
        {
            OSD_POLYGON,
            .unData.stPolygon =
            {
                4, 0, 10,
                {
                    {0, 0}
                },
                0x20 | COLOR_BLUE, COLOR_BLUE, 2,
                {0,
                    {{0, 0}},
                    {ARGB888_RED},
                    {"line1"}
                }
            }
        },

        // hotspot
        {
            OSD_HOTSPOT,
            .unData.stHotspot =
            {
                5, 1, 8,
                {
                    {70, 40}, {70, 70}, {70, 100}, {40, 70},
                    {100, 40}, {100, 70}, {100, 100}, {130, 70}
                },
                {
                    COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_RED,
                    COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_RED,
                },
                {1,
                    {{500, 30}},
                    {ARG8888_WHITE},
                    {"一二三abc"}
                }
            }
        },
        {
            OSD_HOTSPOT,
            .unData.stHotspot =
            {
                6, 1, 8,
                {
                    {270, 40}, {270, 70}, {270, 100}, {240, 70},
                    {300, 40}, {300, 70}, {300, 100}, {330, 70}
                },
                {
                    COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_RED,
                    COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_RED,
                },
                {1,
                    {{240, 240}},
                    {ARG8888_BLACK},
                    {"温度报警: 100"}
                }
            }
        },
        {
            OSD_HOTSPOT,
            .unData.stHotspot =
            {
                7, 0, 8,
                {
                    {0, 0}
                },
                {
                    COLOR_BLUE
                },
                {0,
                    {{0, 0}},
                    {ARGB888_RED},
                    {"line1"}
                }
            }
        },
        {
            OSD_HOTSPOT,
            .unData.stHotspot =
            {
                8, 0, 8,
                {
                    {0, 0}
                },
                {
                    COLOR_RED
                },
                {0,
                    {{0, 0}},
                    {ARGB888_RED},
                    {"line1"}
                }
            }
        },
        {
            OSD_HOTSPOT,
            .unData.stHotspot =
            {
                9, 0, 8,
                {
                    {0, 0}
                },
                {
                    COLOR_RED
                },
                {0,
                    {{0, 0}},
                    {ARGB888_RED},
                    {"line1"}
                }
            }
        },
    };

    OSD_GetBuildVersion(au8Ver);
    OSD_Start(NULL);

    OSD_GetAll(astOsdRead);

    unsigned int u32Id;
    OSD_Set(&astOsd[0]);

    u32Id = 0;
    stOsd.enType = OSD_POLYGON;
    stOsd.unData.stPolygon.u32Id = u32Id;
    OSD_Get(&stOsd);
    OSD_Dump(&stOsd);

    OSD_Set(&astOsd[1]);

    u32Id = 1;
    stOsd.enType = OSD_POLYGON;
    stOsd.unData.stPolygon.u32Id = u32Id;
    OSD_Get(&stOsd);
    OSD_Dump(&stOsd);

    OSD_Set(&astOsd[0 + 5]);

    u32Id = 0 + 5;
    stOsd.enType = OSD_HOTSPOT;
    stOsd.unData.stHotspot.u32Id = u32Id;
    OSD_Get(&stOsd);
    OSD_Dump(&stOsd);

    OSD_Set(&astOsd[1 + 5]);

    u32Id = 1 + 5;
    stOsd.enType = OSD_HOTSPOT;
    stOsd.unData.stHotspot.u32Id = u32Id;
    OSD_Get(&stOsd);
    OSD_Dump(&stOsd);

    pause();
    OSD_Stop(NULL);

    return 0;
}
#endif

static pthread_t gstTextThreadId = 0;
static int       gs32TextRunning = 0;

static pthread_t gstStarThreadId = 0;
static int       gs32StarRunning = 0;


static void PrintUsage(void)
{
    printf("Usage:\n");
    printf("    input 1 :Display a area\n");
    printf("    input 2 :Display a line text, change(1~10)\n");
    printf("    input 3 :Display a cross star, move in the area. \n");
    printf("    input 4 :Delete the area. \n");
    printf("    input q :Quit.\n");
}

static void *text_thread(void *pArg)
{
    int s32PolygonId = (int)pArg;
    OSD_ST stOsd;
    int s32Text = 0;
    POINT_ST stPoint = {70, 40};

    printf("%s run ...\n", __FUNCTION__);

    stOsd.enType = OSD_POLYGON;
    stOsd.unData.stPolygon.u32Id = s32PolygonId;

    while (gs32TextRunning)
    {
        OSD_Get(&stOsd);

        stOsd.unData.stPolygon.stText.u32LineNum = 1;
        stOsd.unData.stPolygon.stText.astStartPoint[0] = stPoint;
        stOsd.unData.stPolygon.stText.u32Color[0] = ARGB888_BLACK;  // argb
        snprintf((char *)stOsd.unData.stPolygon.stText.au8TextCode[0], MAX_LINE_CHAR_NUM, "温度监控:%d", s32Text++ % 10);

        OSD_Set(&stOsd);
        usleep(1000 * 1000);
    }

    printf("%s quit ...\n", __FUNCTION__);

    return NULL;
}

static void *star_thread(void *pArg)
{
    int s32HotspotId = (int)pArg;
    OSD_ST stOsd;
    POINT_ST stPoint = {40, 70};
    POINT_ST stTextPoint;

    unsigned int u32Left = 40, u32Right = 130, u32Top = 70, u32Bottom = 100;

    printf("%s run ...\n", __FUNCTION__);

    stOsd.enType = OSD_HOTSPOT;
    stOsd.unData.stHotspot.u32Id = s32HotspotId;

    while (gs32StarRunning)
    {
        OSD_Get(&stOsd);

        stOsd.unData.stHotspot.u32Enable = 1;
        stOsd.unData.stHotspot.u32PointNum = 1;
        stOsd.unData.stHotspot.u32Thick = 4;
        stOsd.unData.stHotspot.u32Radius = 8;
        stOsd.unData.stHotspot.astPoint[0] = stPoint;
        stOsd.unData.stHotspot.u32Color[0] = COLOR_RED;    // rgb

        stTextPoint = stPoint;
        stTextPoint.u32Y -= (16 + stOsd.unData.stHotspot.u32Radius + 2);
        stOsd.unData.stHotspot.stText.u32LineNum = 1;
        stOsd.unData.stHotspot.stText.astStartPoint[0] = stTextPoint;
        stOsd.unData.stHotspot.stText.u32Color[0] = ARGB888_RED;  // argb
        snprintf((char *)stOsd.unData.stHotspot.stText.au8TextCode[0], MAX_LINE_CHAR_NUM, "%u,%u", stPoint.u32X, stPoint.u32Y);

        OSD_Set(&stOsd);

        if (++stPoint.u32X % u32Right == 0)
        {
            stPoint.u32X = u32Left;
            if (++stPoint.u32Y % u32Bottom == 0)
            {
                stPoint.u32Y = u32Top;
            }
        }

        usleep(200 * 1000);
    }

    printf("%s quit ...\n", __FUNCTION__);

    return NULL;
}

static void Sample_DisplayArea(int s32PolygonId)
{
    //printf("%s(Pid:%d)\n", __FUNCTION__, s32PolygonId);
    OSD_ST stOsd;
    /*POINT_ST astPoints[10] =
    {
        {70, 10}, {100, 10}, {130, 40}, {160, 70}, {130, 100},
        {100, 130}, {70, 130}, {40, 100}, {10, 70}, {40, 40}
    };*/
    
    POINT_ST astPoints[4] =
    {
        {30, 30},   {430, 30}, {430, 430}, {30, 430}
    };

    memset(&stOsd, 0, sizeof(stOsd));

    stOsd.enType = OSD_POLYGON;
    stOsd.unData.stPolygon.u32Id = s32PolygonId;
    OSD_Get(&stOsd);

    stOsd.unData.stPolygon.u32Enable = 1;
    stOsd.unData.stPolygon.u32PointNum = sizeof(astPoints) / sizeof(astPoints[0]);
    memcpy(stOsd.unData.stPolygon.astPoint, astPoints, sizeof(astPoints));
    stOsd.unData.stPolygon.u32BgColor = (0x10 << 24) | COLOR_GREEN; // ARGB8888
    stOsd.unData.stPolygon.u32Color = COLOR_GREEN;
    stOsd.unData.stPolygon.u32Thick = 8; // no bord

    OSD_Set(&stOsd);
    OSD_Dump(&stOsd);
}

static void Sample_DisplayLineText(int s32PolygonId)
{
    //printf("%s(Pid:%d)\n", __FUNCTION__, s32PolygonId);
    if (gstTextThreadId)
        return;

    gs32TextRunning = 1;
    if (pthread_create(&gstTextThreadId, NULL, text_thread, (void *)s32PolygonId) < 0) {
        printf("%s %d err: create thread failedi\n", __FUNCTION__, __LINE__);
        return;
    }
}

static void Sample_DisplayCrossStar(int s32HotspotId)
{
    //printf("%s(Hid:%d)\n", __FUNCTION__, s32HotspotId);
    if (gstStarThreadId)
        return;

    gs32StarRunning = 1;
    if (pthread_create(&gstStarThreadId, NULL, star_thread, (void *)s32HotspotId) < 0) {
        printf("%s %d err: create thread failedi\n", __FUNCTION__, __LINE__);
        return;
    }
}

static void Sample_Quit(void)
{
    if (gstTextThreadId) {
        gs32TextRunning = 0;
        pthread_join(gstTextThreadId, NULL);
        gstTextThreadId = 0;
    }

    if (gstStarThreadId) {
        gs32StarRunning = 0;
        pthread_join(gstStarThreadId, NULL);
        gstStarThreadId = 0;
    }
}

static void Sample_DeleteArea(int s32PolygonId, int s32HotspotId)
{
    //printf("%s(Pid:%d, Hid:%d)\n", __FUNCTION__, s32PolygonId, s32HotspotId);
    OSD_ST stOsd;
    int isBakOldCfg = 0;

    Sample_Quit();

    // clear s32PolygonId
    memset(&stOsd, 0, sizeof(stOsd));
    stOsd.enType = OSD_POLYGON;
    stOsd.unData.stPolygon.u32Id = s32PolygonId;
    if (isBakOldCfg)
    {
        OSD_Get(&stOsd);
    }
    else
    {
        // the memset(&stOsd, 0, sizeof(stOsd)) clear all cfg parameter.
        ;
    }
    stOsd.unData.stPolygon.u32Enable = 0;
    OSD_Set(&stOsd);

    // clear s32HotspotId
    memset(&stOsd, 0, sizeof(stOsd));
    stOsd.enType = OSD_HOTSPOT;
    stOsd.unData.stHotspot.u32Id = s32HotspotId;
    if (isBakOldCfg)
    {
        OSD_Get(&stOsd);
    }
    else
    {
        // the memset(&stOsd, 0, sizeof(stOsd)) clear all cfg parameter.
        ;
    }
    stOsd.unData.stHotspot.u32Enable = 0;
    OSD_Set(&stOsd);
}


int main(int argc, char *args[])
{
    char as8Buff[256];
    int isQuit = 0;
    int s32PolygonId = 0; // [0, 4]
    int s32HotspotId = 5; // [5, 9]

    // start send frame to venc
    OSD_Start(NULL);

    do
    {
        PrintUsage();
        fgets(as8Buff, sizeof(as8Buff), stdin);

        switch (as8Buff[0])
        {
            case '1':
                Sample_DisplayArea(s32PolygonId);
                break;
            case '2':
                Sample_DisplayLineText(s32PolygonId);
                break;
            case '3':
                Sample_DisplayCrossStar(s32HotspotId);
                break;
            case '4':
                Sample_DeleteArea(s32PolygonId, s32HotspotId);
                break;
            case 'q':
                Sample_Quit();
                isQuit = 1;
                break;
            default:
                break;
        }

    } while (!isQuit);

   // stop send frame to venc
   OSD_Stop(NULL);

   return 0;
}

