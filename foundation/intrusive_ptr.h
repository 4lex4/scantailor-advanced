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

template<typename T>
class intrusive_ptr {
public:
    class hash;

public:
    constexpr intrusive_ptr() noexcept
            : m_pObj(nullptr) {
    }

    constexpr intrusive_ptr(std::nullptr_t) noexcept
            : intrusive_ptr() {
    }

    explicit intrusive_ptr(T* obj) noexcept;

    intrusive_ptr(intrusive_ptr const& other) noexcept;

    template<typename OT>
    intrusive_ptr(intrusive_ptr<OT> const& other) noexcept;

    intrusive_ptr(intrusive_ptr&& other) noexcept;

    template<typename OT>
    explicit intrusive_ptr(intrusive_ptr<OT>&& other) noexcept;

    ~intrusive_ptr() noexcept;

    intrusive_ptr& operator=(intrusive_ptr const& rhs) noexcept;

    template<typename OT>
    intrusive_ptr& operator=(intrusive_ptr<OT> const& rhs) noexcept;

    intrusive_ptr& operator=(intrusive_ptr&& rhs) noexcept;

    template<typename OT>
    intrusive_ptr& operator=(intrusive_ptr<OT>&& rhs) noexcept;

    T& operator*() const {
        return *m_pObj;
    }

    T* operator->() const noexcept {
        return m_pObj;
    }

    T* get() const noexcept {
        return m_pObj;
    }

    void reset(T* obj = nullptr) noexcept;

    void swap(intrusive_ptr& other) noexcept;

    /**
     * Used for boolean tests, like:
     * \code
     * intrusive_ptr<T> ptr = ...;
     * if (ptr) {
     *   ...
     * }
     * if (!ptr) {
     *   ...
     * }
     * \endcode
     */
    explicit operator bool() const noexcept;

private:
    T* m_pObj;
};

/**
 * \brief Default implementation of intrusive referencing.
 *
 * May be specialized or overloaded.
 */
template<typename T>
inline void intrusive_ref(T& obj) {
    obj.ref();
}

/**
 * \brief Default implementation of intrusive unreferencing.
 *
 * May be specialized or overloaded.
 */
template<typename T>
inline void intrusive_unref(T& obj) {
    obj.unref();
}

template<typename T>
inline
intrusive_ptr<T>::intrusive_ptr(T* obj) noexcept
        : m_pObj(obj) {
    if (obj) {
        intrusive_ref(*obj);
    }
}

template<typename T>
inline
intrusive_ptr<T>::intrusive_ptr(intrusive_ptr const& other) noexcept
        : m_pObj(other.m_pObj) {
    if (m_pObj) {
        intrusive_ref(*m_pObj);
    }
}

template<typename T>
template<typename OT>
inline
intrusive_ptr<T>::intrusive_ptr(intrusive_ptr<OT> const& other) noexcept
        : m_pObj(other.get()) {
    if (m_pObj) {
        intrusive_ref(*m_pObj);
    }
}

template<typename T>
inline
intrusive_ptr<T>::intrusive_ptr(intrusive_ptr&& other) noexcept
        : m_pObj(other.m_pObj) {
    other.m_pObj = nullptr;
}

template<typename T>
template<typename OT>
inline
intrusive_ptr<T>::intrusive_ptr(intrusive_ptr<OT>&& other) noexcept
        : m_pObj(other.get()) {
    other.reset();
}

template<typename T>
inline
intrusive_ptr<T>::~intrusive_ptr() noexcept {
    if (m_pObj) {
        intrusive_unref(*m_pObj);
    }
}

template<typename T>
inline intrusive_ptr<T>& intrusive_ptr<T>::operator=(intrusive_ptr const& rhs) noexcept {
    intrusive_ptr(rhs).swap(*this);

    return *this;
}

template<typename T>
template<typename OT>
inline intrusive_ptr<T>& intrusive_ptr<T>::operator=(intrusive_ptr<OT> const& rhs) noexcept {
    intrusive_ptr(rhs).swap(*this);

    return *this;
}

template<typename T>
inline intrusive_ptr<T>& intrusive_ptr<T>::operator=(intrusive_ptr&& rhs) noexcept {
    if (m_pObj != rhs.m_pObj) {
        m_pObj = rhs.m_pObj;
        rhs.m_pObj = nullptr;
    }

    return *this;
}

template<typename T>
template<typename OT>
inline intrusive_ptr<T>& intrusive_ptr<T>::operator=(intrusive_ptr<OT>&& rhs) noexcept {
    if (m_pObj != rhs.get()) {
        m_pObj = rhs.get();
        rhs.reset();
    }

    return *this;
}

template<typename T>
inline void intrusive_ptr<T>::reset(T* obj) noexcept {
    intrusive_ptr(obj).swap(*this);
}

template<typename T>
inline void intrusive_ptr<T>::swap(intrusive_ptr& other) noexcept {
    T* obj = other.m_pObj;
    other.m_pObj = m_pObj;
    m_pObj = obj;
}

template<typename T>
inline void swap(intrusive_ptr<T>& o1, intrusive_ptr<T>& o2) {
    o1.swap(o2);
}

template<typename T>
inline
intrusive_ptr<T>::operator bool() const noexcept {
    return (m_pObj != nullptr);
}

#define INTRUSIVE_PTR_OP(op) \
    template <typename T> \
    inline bool operator op(intrusive_ptr<T> const& lhs, intrusive_ptr<T> const& rhs) { \
        return (lhs.get() op rhs.get()); \
    } \
    \
    template <typename T, typename OT> \
    inline bool operator op(intrusive_ptr<T> const& lhs, intrusive_ptr<OT> const& rhs) { \
        return (lhs.get() op rhs.get()); \
    } \
    \
    template <typename T> \
    inline bool operator op(std::nullptr_t, intrusive_ptr<T> const& rhs) { \
        return (nullptr op rhs.get()); \
    } \
    \
    template <typename T> \
    inline bool operator op(intrusive_ptr<T> const& lhs, std::nullptr_t) { \
        return (lhs.get() op nullptr); \
    }

INTRUSIVE_PTR_OP(==)

INTRUSIVE_PTR_OP(!=)

INTRUSIVE_PTR_OP(<)

INTRUSIVE_PTR_OP(>)

INTRUSIVE_PTR_OP(<=)

INTRUSIVE_PTR_OP(>=)

template<typename T>
struct intrusive_ptr<T>::hash {
    std::size_t operator()(const intrusive_ptr<T>& __p) const noexcept {
        return reinterpret_cast<std::size_t>(__p.get());
    }
};

#endif  // ifndef intrusive_ptr_H_
