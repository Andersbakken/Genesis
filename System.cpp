#include "System.h"

QVector<System*> System::sInsts;

System::System(int scrn)
    : mScreen(scrn)
{
}

System* System::instance(int scrn)
{
    System* s = sInsts.value(scrn, 0);
    if (!s) {
        s = new System(scrn);
        sInsts[scrn] = s;
    }
    return s;
}
