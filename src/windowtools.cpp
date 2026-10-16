#include <QtCore>

#include "windowtools.h"
#include "windowtools_x11.h"


WindowTools::WindowTools()
    : QObject()
{
}

WindowTools::~WindowTools()
{
}


WindowTools *WindowTools::create()
{
    static WindowTools * tools;

    if ( !tools )
        tools = new WindowTools_X11();

    return tools;
}
