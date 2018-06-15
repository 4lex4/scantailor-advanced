/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef intrusive_ptr_H_
#define intrusive_ptr_H_

#include <cstddef>
#include <utility>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

template <typename T>
class intrusive_ptr {
  template <typename>
  friend class intrusive_ptr;

 public:
  struct hash;

  using pointer = T*;

  template <typename OT>
  using __enable_if_convertible =
      typename std::enable_if<std::is_convertible<typename intrusive_ptr<OT>::pointer, pointer>::value>::type;

 public:
  constexpr intrusive_ptr(std::nullptr_t = nullptr) noexcept;

  explicit intrusive_ptr(T* obj) noexcept;

  intrusive_ptr(const intrusive_ptr& other) noexcept;

  intrusive_ptr(intrusive_ptr&& other) noexcept;

  template <typename OT, typename = __enable_if_convertible<OT>>
  intrusive_ptr(const intrusive_ptr<OT>& other) noexcept;

  template <typename OT, typename = __enable_if_convertible<OT>>
  intrusive_ptr(intrusive_ptr<OT>&& other) noexcept;

  ~intrusive_ptr() noexcept;

  intrusive_ptr& operator=(std::nullptr_t) noexcept;

  intrusive_ptr& operator=(const intrusive_ptr& rhs) noexcept;

  intrusive_ptr& operator=(intrusive_ptr&& rhs) noexcept;

  template <typename OT, typename = __enable_if_convertible<OT>>
  intrusive_ptr& operator=(const intrusive_ptr<OT>& rhs) noexcept;

  template <typename OT, typename = __enable_if_convertible<OT>>
  intrusive_ptr& operator=(intrusive_ptr<OT>&& rhs) noexcept;

  T& operator*() const;

  T* operator->() const noexcept;

  T* get() const noexcept;

  T* release() noexcept;

  void reset(std::nullptr_t = nullptr) noexcept;

  void reset(T* obj) noexcept;

  void swap(intrusive_ptr& other) noexcept;

  explicit operator bool() const noexcept;

 private:
  T* fork() const noexcept;

  void intrusive_ref(const T& obj) const noexcept;

  void intrusive_unref(const T& obj) const noexcept;


  T* m_obj;
};

/**
 * \brief Default implementation of intrusive referencing.
 *
 * May be specialized or overloaded.
 */
template <typename T>
inline void intrusive_ptr<T>::intrusive_ref(const T& obj) const noexcept {
  obj.ref();
}

/**
 * \brief Default implementation of intrusive unreferencing.
 *
 * May be specialized or overloaded.
 */
template <typename T>
inline void intrusive_ptr<T>::intrusive_unref(const T& obj) const noexcept {
  obj.unref();
}

template <typename T>
constexpr inline intrusive_ptr<T>::intrusive_ptr(std::nullptr_t) noexcept : m_obj(nullptr) {}

template <typename T>
inline intrusive_ptr<T>::intrusive_ptr(T* obj) noexcept : m_obj(obj) {
  if (obj) {
    intrusive_ref(*obj);
  }
}

template <typename T>
inline intrusive_ptr<T>::intrusive_ptr(const intrusive_ptr& other) noexcept : m_obj(other.fork()) {}

template <typename T>
inline intrusive_ptr<T>::intrusive_ptr(intrusive_ptr&& other) noexcept : m_obj(other.release()) {}

template <typename T>
template <typename OT, typename>
inline intrusive_ptr<T>::intrusive_ptr(const intrusive_ptr<OT>& other) noexcept : m_obj(other.fork()) {}

template <typename T>
template <typename OT, typename>
inline intrusive_ptr<T>::intrusive_ptr(intrusive_ptr<OT>&& other) noexcept : m_obj(other.release()) {}

template <typename T>
inline intrusive_ptr<T>::~intrusive_ptr() noexcept {
  if (m_obj) {
    intrusive_unref(*m_obj);
  }
}

template <typename T>
inline intrusive_ptr<T>& intrusive_ptr<T>::operator=(std::nullptr_t) noexcept {
  reset();

  return *this;
}

template <typename T>
inline intrusive_ptr<T>& intrusive_ptr<T>::operator=(const intrusive_ptr& rhs) noexcept {
  intrusive_ptr(rhs).swap(*this);

  return *this;
}

template <typename T>
inline intrusive_ptr<T>& intrusive_ptr<T>::operator=(intrusive_ptr&& rhs) noexcept {
  intrusive_ptr(std::move(rhs)).swap(*this);

  return *this;
}

template <typename T>
template <typename OT, typename>
inline intrusive_ptr<T>& intrusive_ptr<T>::operator=(const intrusive_ptr<OT>& rhs) noexcept {
  intrusive_ptr(rhs).swap(*this);

  return *this;
}

template <typename T>
template <typename OT, typename>
inline intrusive_ptr<T>& intrusive_ptr<T>::operator=(intrusive_ptr<OT>&& rhs) noexcept {
  intrusive_ptr(std::move(rhs)).swap(*this);

  return *this;
}

template <typename T>
inline T& intrusive_ptr<T>::operator*() const {
  return *get();
}

template <typename T>
inline T* intrusive_ptr<T>::operator->() const noexcept {
  return get();
}

template <typename T>
inline T* intrusive_ptr<T>::get() const noexcept {
  return m_obj;
}

template <typename T>
inline T* intrusive_ptr<T>::release() noexcept {
  T* obj = m_obj;
  m_obj = nullptr;

  return obj;
}

template <typename T>
inline void intrusive_ptr<T>::reset(std::nullptr_t) noexcept {
  intrusive_ptr().swap(*this);
}

template <typename T>
inline void intrusive_ptr<T>::reset(T* obj) noexcept {
  intrusive_ptr(obj).swap(*this);
}

template <typename T>
inline void intrusive_ptr<T>::swap(intrusive_ptr& other) noexcept {
  T* obj = other.m_obj;
  other.m_obj = m_obj;
  m_obj = obj;
}

template <typename T>
inline intrusive_ptr<T>::operator bool() const noexcept {
  return (get() != nullptr);
}

template <typename T>
inline T* intrusive_ptr<T>::fork() const noexcept {
  T* obj = m_obj;
  if (obj) {
    intrusive_ref(*obj);
  }

  return obj;
}

template <typename T, typename... Args>
inline intrusive_ptr<T> make_intrusive(Args&&... args) {
  return intrusive_ptr<T>(new T(std::forward<Args>(args)...));
};

template <typename T>
struct intrusive_ptr<T>::hash {
  std::size_t operator()(const intrusive_ptr<T>& __p) const noexcept {
    return reinterpret_cast<std::size_t>(__p.get());
  }
};


#define INTRUSIVE_PTR_OP(op)                                                           \
  template <typename T>                                                                \
  inline bool operator op(const intrusive_ptr<T>& lhs, const intrusive_ptr<T>& rhs) {  \
    return (lhs.get() op rhs.get());                                                   \
  }                                                                                    \
                                                                                       \
  template <typename T, typename OT>                                                   \
  inline bool operator op(const intrusive_ptr<T>& lhs, const intrusive_ptr<OT>& rhs) { \
    return (lhs.get() op rhs.get());                                                   \
  }                                                                                    \
                                                                                       \
  template <typename T>                                                                \
  inline bool operator op(std::nullptr_t, const intrusive_ptr<T>& rhs) {               \
    return (nullptr op rhs.get());                                                     \
  }                                                                                    \
                                                                                       \
  template <typename T>                                                                \
  inline bool operator op(const intrusive_ptr<T>& lhs, std::nullptr_t) {               \
    return (lhs.get() op nullptr);                                                     \
  }

INTRUSIVE_PTR_OP(==)

INTRUSIVE_PTR_OP(!=)

INTRUSIVE_PTR_OP(<)

INTRUSIVE_PTR_OP(>)

INTRUSIVE_PTR_OP(<=)

INTRUSIVE_PTR_OP(>=)

template <typename T>
inline void swap(intrusive_ptr<T>& lhs, intrusive_ptr<T>& rhs) {
  lhs.swap(rhs);
}

#endif  // ifndef intrusive_ptr_H_
