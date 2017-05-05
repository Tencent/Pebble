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


#ifndef _PEBBLE_COMMON_MUTEX_H_
#define _PEBBLE_COMMON_MUTEX_H_

#include <pthread.h>

namespace pebble {


class Mutex
{
public:
    Mutex()
    {
        pthread_mutex_init(&m_mutex, NULL);
    }

    ~Mutex()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    void Lock()
    {
        pthread_mutex_lock(&m_mutex);
    }

    void UnLock()
    {
        pthread_mutex_unlock(&m_mutex);
    }

    pthread_mutex_t* GetMutex() {
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex;
};

class AutoLocker
{
public:
    explicit AutoLocker(Mutex* mutex)
    {
        m_mutex = mutex;
        m_mutex->Lock();
    }

    ~AutoLocker()
    {
        m_mutex->UnLock();
    }

private:
    Mutex* m_mutex;
};


class ReadWriteLock
{
public:
    ReadWriteLock()
    {
        pthread_rwlock_init(&m_lock, NULL);
    }

    ~ReadWriteLock()
    {
        pthread_rwlock_destroy(&m_lock);
    }

    void ReadLock()
    {
        pthread_rwlock_rdlock(&m_lock);
    }

    void WriteLock()
    {
        pthread_rwlock_wrlock(&m_lock);
    }

    void UnLock()
    {
        pthread_rwlock_unlock(&m_lock);
    }

private:
    pthread_rwlock_t m_lock;
};

class ReadAutoLocker
{
public:
    explicit ReadAutoLocker(ReadWriteLock* lock)
    {
        m_lock = lock;
        m_lock->ReadLock();
    }

    ~ReadAutoLocker()
    {
        m_lock->UnLock();
    }
private:
    ReadWriteLock* m_lock;
};

class WriteAutoLocker
{
public:
    explicit WriteAutoLocker(ReadWriteLock* lock)
    {
        m_lock = lock;
        m_lock->WriteLock();
    }

    ~WriteAutoLocker()
    {
        m_lock->UnLock();
    }
private:
    ReadWriteLock* m_lock;
};

class SpinLock
{
public:
    SpinLock()
    {
        pthread_spin_init(&_lock, 0);
    }
    ~SpinLock()
    {
        pthread_spin_destroy(&_lock);
    }
    void Lock()
    {
        pthread_spin_lock(&_lock);
    }
    void Unlock()
    {
        pthread_spin_unlock(&_lock);
    }
private:
    pthread_spinlock_t  _lock;
};

class AutoSpinLock
{
public:
    explicit AutoSpinLock(SpinLock* spin_lock)
        :   _spin_lock(spin_lock)
    {
        if (NULL != _spin_lock)
        {
            _spin_lock->Lock();
        }
    }
    ~AutoSpinLock()
    {
        if (NULL != _spin_lock)
        {
            _spin_lock->Unlock();
        }
    }
private:
    SpinLock*   _spin_lock;
};

} // namespace pebble

#endif // _PEBBLE_COMMON_MUTEX_H_