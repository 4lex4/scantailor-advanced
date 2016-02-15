
#ifndef SCAN_TAILOR_BACKGROUNDCLEANER_H
#define SCAN_TAILOR_BACKGROUNDCLEANER_H

#include <QImage>

namespace imageproc
{
    void whitenBackground(QImage& cg_image, const unsigned int threshold, const int brightness, const int contrast);
}

#endif
