#include <stdio.h>

#include "osd.h"

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
