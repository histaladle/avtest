#include "r2d.h"
double r2d(AVRational r)
{
    if(r.den)
    {
        return (double)r.num/(double)r.den;
    }
    return 0;
}
