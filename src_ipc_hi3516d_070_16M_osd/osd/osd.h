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
 * @brief draw ploygon and text on the picture.
 * @author wangkaichao
 * @version 1.0
 * @date 2019-06-01
 */

#ifndef _OSD_H_
#define _OSD_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef struct {
    unsigned int    u32X;
    unsigned int    u32Y;
} POINT_ST;

/**
 * @brief 文字属性
 */
typedef struct {
//    unsigned int    u32Id;
    unsigned int    u32Color;       //ARGB8888
    unsigned int    u32LineNum;

#define MAX_TEXT_LINE_NUM        (5)             /**< 文字最大行数上限 */
    POINT_ST        astStartPoint[MAX_TEXT_LINE_NUM];

#define MAX_LINE_CHAR_NUM        (32)            /**< 一行最大英文上限，汉字减倍 */
    unsigned char   au8TextCode[MAX_TEXT_LINE_NUM][MAX_LINE_CHAR_NUM + 1];
} TEXT_ST;

/**
 * @brief 多边形
 */
typedef struct {
#define MAX_POLYGON_NUM         (5)
    unsigned int    u32Id;
    unsigned int    u32Enable;              /**< 显示控制 */
    unsigned int    u32PointNum;

#define MAX_POLYGON_POINT_NUM   (10)        /**< 多边形最大边数上限 */
    POINT_ST        astPoint[MAX_POLYGON_POINT_NUM];

    //unsigned int    u32BgColor;             /**< RGB888 */
    //unsigned int    u32Alpha;               /**< [0, 255] */
    unsigned int    u32Color;
    unsigned int    u32Thick; 

    TEXT_ST         stText;
} POLYGON_ST;

/**
 * @brief 热点属性
 */
typedef struct {
#define MAX_HOTSPOT_NUM               (5)
    unsigned int    u32Id;
    unsigned int    u32Enable;
    unsigned int    u32PointNum;

#define MAX_HOTSPOT_POINT_NUM         (8)   /**< 十字星最大上限 */
    POINT_ST        astPoint[MAX_HOTSPOT_POINT_NUM];

    unsigned int    u32Color;

    TEXT_ST         stText;
} HOTSPOT_ST;

/**
 * @brief 接口类型
 */
typedef enum {
    OSD_POLYGON,
    OSD_HOTSPOT,
    OSD_TYPE_NUM,
} OSD_TYPE_EN;

/**
 * @brief 接口数据
 */
typedef union {
    POLYGON_ST  stPolygon;
    HOTSPOT_ST  stHotspot;
} OSD_DATA_UN;

#define MAX_OSD_NUM (MAX_POLYGON_NUM+ MAX_HOTSPOT_NUM)
/**
 * @brief 最终接口
 */
typedef struct {
    OSD_TYPE_EN enType;
    OSD_DATA_UN unData;
} OSD_ST;

typedef enum {
    ERR_SUCCESS         = 0,
    ERR_INVALID_PARAM,
    ERR_MALLOC_FAILED,
    ERR_TIMEOUT,
    ERR_IO_FAILED,
    ERR_UNKNOWN
} OSD_ERR_EN;

/**
 * @brief 获取库的编译版本信息
 *
 * @param[out] pu8Version 调用至少要32字节空间
 *
 * @return 0:Success, !0:Failed
 */
OSD_ERR_EN OSD_GetBuildVersion(unsigned char *pu8Version);

/**
 * @brief OSD开始
 *
 * @param[in] p 预留，暂不用
 *
 * @return 0:Success, !0:Failed
 */
OSD_ERR_EN OSD_Start(void *p);

/**
 * @brief OSD关闭
 *
 * @param[in] p 预留，暂不用
 *
 * @return 0:Success, !0:Failed
 */
OSD_ERR_EN OSD_Stop(void *p);


/**
 * @brief 获取可用的id.
 *
 * @param[in]  enType OSD type
 * @param[out] pId id
 *
 * @return 0:Success, !0:Failed
 */
OSD_ERR_EN OSD_GetActiveId(OSD_TYPE_EN enType, unsigned int *pId);

/**
 * @brief 通过Id获取OSD的数据
 *
 * @param[in]  pstOsd->enType OSD type.
 * @param[in]  pstOsd->unData.stPolygon.u32Id  OSD data id.
 * @param[out] pstOsd->unData All other members of pstOsd->unData.
 *
 * @Exp: Get OSD polygon data at id = 0.
 *
 *  OSD_ST stOsd;
 *  stOsd.enType = OSD_POLYGON;
 *  stOsd.unData.stPolygon.u32Id = 0;
 *  if (!OSD_Get(&stOsd))
 *  {
 *      POLYGON_ST *p = (POLYGON_ST *)&stOsd.unData;
 *      printf("TextLineNum:%d\n", p->stText.u32LineNum);
 *  }
 *
 * @return 0:Success, !0:Failed
 */
OSD_ERR_EN OSD_Get(OSD_ST *pstOsd);

/**
 * @brief Draw osd on the picture.
 *
 * @param[in] pstOsd
 *
 * @return 0:Success, !0:Failed
 */
OSD_ERR_EN OSD_Set(const OSD_ST *pstOsd);

/**
 * @brief get all OSD data.
 *
 * @param[out] apstOsd the osd lib will copy all the data to astOsd.
 *
 * @return 0:Success, !0:Failed
 */
OSD_ERR_EN OSD_GetAll(OSD_ST astOsd[MAX_OSD_NUM]);


/**
 * @brief print osd log
 *
 * @param[in] pstOsd 打印指针所指结构数据
 *
 * @return 0:Success, !0:Failed
 */
OSD_ERR_EN OSD_Dump(const OSD_ST *pstOsd);

#ifdef __cplusplus
}
#endif

#endif

