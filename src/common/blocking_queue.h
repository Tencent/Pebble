/*
 * Tencent is pleased to support the open source community by making Pebble available.
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
 * Licensed under the MIT License (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 *
 */


#ifndef _PEBBLE_COMMON_BLOCKING_QUEUE_H_
#define _PEBBLE_COMMON_BLOCKING_QUEUE_H_

#include <assert.h>
#include <cstddef>
#include <deque>

#include "common/condition_variable.h"
#include "common/mutex.h"

namespace pebble {


template <typename T>
class BlockingQueue
{
public:
    typedef T ValueType;
    typedef std::deque<T> UnderlyContainerType;

    BlockingQueue(size_t max_elements = static_cast<size_t>(-1))
        : m_max_elements(max_elements)
    {
        assert(m_max_elements > 0);
    }

    /// @brief push element in to front of queue
    /// @param value to be pushed
    /// @note if queue is full, block and wait for non-full
    void PushFront(const T& value)
    {
        {
            AutoLocker locker(m_mutex);
            while (UnlockedIsFull())
            {
                m_cond_not_full.Wait(&m_mutex);
            }
            m_queue.push_front(value);
        }
        m_cond_not_empty.Signal();
    }

    /// @brief try push element in to front of queue
    /// @param value to be pushed
    /// @note if queue is full, return false
    bool TryPushFront(const T& value)
    {
        {
            AutoLocker locker(&m_mutex);
            if (UnlockedIsFull())
                return false;
            m_queue.push_front(value);
        }
        m_cond_not_empty.Signal();
        return true;
    }

    /// @brief push element in to back of queue
    /// @param value to be pushed
    /// @note if queue is full, block and wait for non-full
    void PushBack(const T& value)
    {
        {
            AutoLocker locker(&m_mutex);
            while (UnlockedIsFull())
            {
                m_cond_not_full.Wait(&m_mutex);
            }
            m_queue.push_back(value);
        }
        m_cond_not_empty.Signal();
    }

    /// @brief Try push element in to back of queue
    /// @param value to be pushed
    /// @note if queue is full, return false
    bool TryPushBack(const T& value)
    {
        {
            AutoLocker locker(&m_mutex);
            if (UnlockedIsFull())
                return false;
            m_queue.push_back(value);
        }
        m_cond_not_empty.Signal();
        return true;
    }

    /// @brief popup from front of queue.
    /// @param value to hold the result
    /// @note if queue is empty, block and wait for non-empty
    void PopFront(T* value)
    {
        {
            AutoLocker locker(&m_mutex);
            while (m_queue.empty())
            {
                m_cond_not_empty.Wait(&m_mutex);
            }
            *value = m_queue.front();
            m_queue.pop_front();
        }
        m_cond_not_full.Signal();
    }

    /// @brief Try popup from front of queue.
    /// @param value to hold the result
    /// @note if queue is empty, return false
    bool TryPopFront(T* value)
    {
        {
            AutoLocker locker(&m_mutex);
            if (m_queue.empty())
                return false;
            *value = m_queue.front();
            m_queue.pop_front();
        }
        m_cond_not_full.Signal();
        return true;
    }

    /// @brief popup from back of queue
    /// @param value to hold the result
    /// @note if queue is empty, block and wait for non-empty
    void PopBack(T* value)
    {
        {
            AutoLocker locker(&m_mutex);
            while (m_queue.empty())
            {
                m_cond_not_empty.Wait(&m_mutex);
            }
            *value = m_queue.back();
            m_queue.pop_back();
        }
        m_cond_not_full.Signal();
    }

    /// @brief Try popup from back of queue
    /// @param value to hold the result
    /// @note if queue is empty, return false
    bool TryPopBack(T* value)
    {
        {
            AutoLocker locker(&m_mutex);
            if (m_queue.empty())
                return false;
            *value = m_queue.back();
            m_queue.pop_back();
        }
        m_cond_not_full.Signal();
        return true;
    }

    /// @brief push front with time out
    /// @param value to be pushed
    /// @param timeout_in_ms timeout, in milliseconds
    /// @return whether pushed
    bool TimedPushFront(const T& value, int timeout_in_ms)
    {
        bool success = false;
        {
            AutoLocker locker(&m_mutex);

            if (UnlockedIsFull())
                m_cond_not_full.TimedWait(&m_mutex, timeout_in_ms);

            if (!UnlockedIsFull())
            {
                m_queue.push_front(value);
                success = true;
            }
        }

        if (success)
            m_cond_not_empty.Signal();

        return success;
    }

    /// @brief push back with time out
    /// @param value to be pushed
    /// @param timeout_in_ms timeout, in milliseconds
    /// @return whether pushed
    bool TimedPushBack(const T& value, int timeout_in_ms)
    {
        bool success = false;
        {
            AutoLocker locker(&m_mutex);
            if (UnlockedIsFull())
                m_cond_not_full.TimedWait(&m_mutex, timeout_in_ms);

            if (!UnlockedIsFull())
            {
                m_queue.push_back(value);
                success = true;
            }
        }
        if (success)
            m_cond_not_empty.Signal();
        return success;
    }

    /// @brief pop front with time out
    /// @param value to hold the result
    /// @param timeout_in_ms timeout, in milliseconds
    /// @return whether poped
    bool TimedPopFront(T* value, int timeout_in_ms)
    {
        bool success = false;
        {
            AutoLocker locker(&m_mutex);

            if (m_queue.empty())
                m_cond_not_empty.TimedWait(&m_mutex, timeout_in_ms);

            if (!m_queue.empty())
            {
                *value = m_queue.front();
                m_queue.pop_front();
                success = true;
            }
        }

        if (success)
            m_cond_not_full.Signal();

        return success;
    }

    /// @brief pop back with time out
    /// @param value to hold the result
    /// @param timeout_in_ms timeout, in milliseconds
    /// @return whether poped
    bool TimedPopBack(T* value, int timeout_in_ms)
    {
        bool success = false;
        {
            AutoLocker locker(&m_mutex);

            if (m_queue.empty())
                m_cond_not_empty.TimedWait(&m_mutex, timeout_in_ms);

            if (!m_queue.empty())
            {
                *value = m_queue.back();
                m_queue.pop_back();
                success = true;
            }
        }

        if (success)
            m_cond_not_full.Signal();

        return success;
    }

    /// @brief pop all from the queue and guarantee the output queue contains at
    /// least one element.
    /// @param values to hold the results, it would be cleared firstly by the
    /// function.
    /// @note if queue is empty, block and wait for non-empty.
    void PopAll(UnderlyContainerType* values)
    {
        values->clear();

        {
            AutoLocker locker(&m_mutex);
            while (m_queue.empty())
                m_cond_not_empty.Wait(&m_mutex);
            values->swap(m_queue);
        }

        m_cond_not_full.Broadcast();
    }

    /// @brief Try pop all from the queue and guarantee the output queue
    /// contains at least one element.
    /// @param values to hold the results, it would be cleared firstly by the
    /// function.
    /// @note if queue is empty, return false
    bool TryPopAll(UnderlyContainerType* values)
    {
        values->clear();

        {
            AutoLocker locker(&m_mutex);
            if (m_queue.empty())
                return false;
            values->swap(m_queue);
        }
        m_cond_not_full.Broadcast();
        return true;
    }

    /// @brief pop all from the queue with time out.
    /// @param values to hold the results, it would be cleared firstly by the
    /// function.
    /// @param timeout_in_ms timeout, in milliseconds.
    /// @return whether poped.
    bool TimedPopAll(UnderlyContainerType* values, int timeout_in_ms)
    {
        bool success = false;
        values->clear();

        {
            AutoLocker locker(&m_mutex);
            if (m_queue.empty())
                m_cond_not_empty.TimedWait(&m_mutex, timeout_in_ms);

            if (!m_queue.empty())
            {
                values->swap(m_queue);
                m_cond_not_full.Broadcast();
                success = true;
            }
        }

        if (success)
            m_cond_not_full.Signal();

        return success;
    }

    /// @brief number of elements in the queue.
    /// @return number of elements in the queue.
    size_t Size()
    {
        AutoLocker locker(&m_mutex);
        return m_queue.size();
    }

    /// @brief whether the queue is empty
    /// @return whether empty
    bool IsEmpty()
    {
        AutoLocker locker(&m_mutex);
        return m_queue.empty();
    }

    /// @brief whether the queue is full
    /// @return whether full
    bool IsFull()
    {
        AutoLocker locker(&m_mutex);
        return UnlockedIsFull();
    }

    /// @brief clear the queue
    void Clear()
    {
        AutoLocker locker(&m_mutex);
        m_queue.clear();
        m_cond_not_full.Broadcast();
    }

private:
    bool UnlockedIsFull() const
    {
        return m_queue.size() >= m_max_elements;
    }

private:
    size_t m_max_elements;
    UnderlyContainerType m_queue;

    Mutex m_mutex;
    ConditionVariable m_cond_not_empty;
    ConditionVariable m_cond_not_full;
};

} // namespace pebble

#endif // _PEBBLE_COMMON_BLOCKING_QUEUE_H_

