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

#include "osd.h"


#define ASSERT_PARAM(exp) do { \
    if (!(exp)) { \
        printf("%s %d: err\n", __FUNCTION__, __LINE__); \
        return ERR_INVALID_PARAM; \
    } \
} while (0)

#define PARAM_USE(p)    (p = p)


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
    printf("%s(p:%p)\n", __FUNCTION__, p);
    PARAM_USE(p);
    //TODO
    return ERR_SUCCESS;
}

OSD_ERR_EN OSD_Stop(void *p)
{
    printf("%s(p:%p)\n", __FUNCTION__, p);
    PARAM_USE(p);
    //TODO
    return ERR_SUCCESS;
}

OSD_ERR_EN OSD_GetActiveId(OSD_TYPE_EN enType, unsigned int *pId)
{
    printf("%s(enType:%d, pId:%p)\n", __FUNCTION__, enType, pId);
    ASSERT_PARAM(enType >= 0 && enType < OSD_TYPE_NUM);
    ASSERT_PARAM(pId != NULL);
    //TODO
    return ERR_SUCCESS;
}

OSD_ERR_EN OSD_Get(OSD_ST *pstOsd)
{
    printf("%s(pstOsd:%p)\n", __FUNCTION__, pstOsd);
    ASSERT_PARAM(pstOsd != NULL);
    //TODO
    return ERR_SUCCESS;
}

OSD_ERR_EN OSD_Set(const OSD_ST *pstOsd)
{
    printf("%s(pstOsd:%p)\n", __FUNCTION__, pstOsd);
    ASSERT_PARAM(pstOsd != NULL);
    //TODO
    return ERR_SUCCESS;
}

OSD_ERR_EN OSD_GetAll(OSD_ST astOsd[MAX_POLYGON_POINT_NUM + MAX_HOTSPOT_POINT_NUM])
{
    printf("%s(pstOsd:%p)\n", __FUNCTION__, astOsd);
    ASSERT_PARAM(astOsd != NULL);
    //TODO
    return ERR_SUCCESS;
}

int main(int argc, char *argv[])
{
    unsigned char au8Ver[32];
    OSD_ST astOsd[10];

    OSD_GetBuildVersion(au8Ver);
    OSD_Start(NULL);

    OSD_GetAll(astOsd);
    OSD_Get(astOsd);
    OSD_Set(astOsd);

    OSD_Stop(NULL);

    return 0;
}
