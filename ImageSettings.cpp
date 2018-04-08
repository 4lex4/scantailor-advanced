
#include "ImageSettings.h"
#include "AbstractRelinker.h"
#include "RelinkablePath.h"
#include "Utils.h"

using namespace imageproc;

void ImageSettings::clear() {
    QMutexLocker locker(&mutex);
    perPageParams.clear();
}

void ImageSettings::performRelinking(const AbstractRelinker& relinker) {
    QMutexLocker locker(&mutex);
    PerPageParams newParams;
    for (const auto& kv : perPageParams) {
        const RelinkablePath oldPath(kv.first.imageId().filePath(), RelinkablePath::File);
        PageId newPageId(kv.first);
        newPageId.imageId().setFilePath(relinker.substitutionPathFor(oldPath));
        newParams.insert(PerPageParams::value_type(newPageId, kv.second));
    }

    perPageParams.swap(newParams);
}

void ImageSettings::setPageParams(const PageId& page_id, const PageParams& params) {
    QMutexLocker locker(&mutex);
    Utils::mapSetValue(perPageParams, page_id, params);
}

std::unique_ptr<ImageSettings::PageParams> ImageSettings::getPageParams(const PageId& page_id) const {
    QMutexLocker locker(&mutex);
    const auto it(perPageParams.find(page_id));
    if (it != perPageParams.end()) {
        return std::make_unique<PageParams>(it->second);
    } else {
        return nullptr;
    }
}

/*=============================== ImageSettings::Params ==================================*/

ImageSettings::PageParams::PageParams() : bwThreshold(0) {
}

ImageSettings::PageParams::PageParams(const BinaryThreshold& bwThreshold) : bwThreshold(bwThreshold) {
}

ImageSettings::PageParams::PageParams(const QDomElement& el) : bwThreshold(el.attribute("bwThreshold").toInt()) {
}

QDomElement ImageSettings::PageParams::toXml(QDomDocument& doc, const QString& name) const {
    QDomElement el(doc.createElement(name));
    el.setAttribute("bwThreshold", bwThreshold);

    return el;
}

const BinaryThreshold& ImageSettings::PageParams::getBwThreshold() const {
    return bwThreshold;
}

void ImageSettings::PageParams::setBwThreshold(const BinaryThreshold& bwThreshold) {
    PageParams::bwThreshold = bwThreshold;
}