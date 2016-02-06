//
// Created by Alex on 1/31/2016.
//

#ifndef SCAN_TAILOR_BACKGROUNDCLEANER_H
#define SCAN_TAILOR_BACKGROUNDCLEANER_H

#include <QImage>

namespace imageproc
{
    void whitenBackground(QImage& cg_image, const unsigned int threshold, const int brightness,
                           const int contrast);
}

#endif //SCAN_TAILOR_BACKGROUNDCLEANER_H
