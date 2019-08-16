// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PageId.h"
#include <cassert>

PageId::PageId() : m_subPage(SINGLE_PAGE) {}

PageId::PageId(const ImageId& image_id, SubPage subpage) : m_imageId(image_id), m_subPage(subpage) {}

QString PageId::subPageToString(const SubPage sub_page) {
  const char* str = nullptr;

  switch (sub_page) {
    case SINGLE_PAGE:
      str = "single";
      break;
    case LEFT_PAGE:
      str = "left";
      break;
    case RIGHT_PAGE:
      str = "right";
      break;
  }

  assert(str);

  return QString::fromLatin1(str);
}

PageId::SubPage PageId::subPageFromString(const QString& string, bool* ok) {
  bool recognized = true;
  SubPage sub_page = SINGLE_PAGE;

  if (string == "single") {
    sub_page = SINGLE_PAGE;
  } else if (string == "left") {
    sub_page = LEFT_PAGE;
  } else if (string == "right") {
    sub_page = RIGHT_PAGE;
  } else {
    recognized = false;
  }

  if (ok) {
    *ok = recognized;
  }

  return sub_page;
}

bool operator==(const PageId& lhs, const PageId& rhs) {
  return ((lhs.subPage() == rhs.subPage()) && (lhs.imageId() == rhs.imageId()));
}

bool operator!=(const PageId& lhs, const PageId& rhs) {
  return !(lhs == rhs);
}

bool operator<(const PageId& lhs, const PageId& rhs) {
  if (lhs.imageId() < rhs.imageId()) {
    return true;
  } else if (rhs.imageId() < lhs.imageId()) {
    return false;
  } else {
    return lhs.subPage() < rhs.subPage();
  }
}
