/**
* Copyright (c) 2018 Zilliqa 
* This source code is being disclosed to you solely for the purpose of your participation in 
* testing Zilliqa. You may view, compile and run the code for that purpose and pursuant to 
* the protocols and algorithms that are programmed into, and intended by, the code. You may 
* not do anything else with the code without express permission from Zilliqa Research Pte. Ltd., 
* including modifying or publishing the code (or any part of it), and developing or forming 
* another public or private blockchain network. This source code is provided ‘as is’ and no 
* warranties are given as to title or non-infringement, merchantability or fitness for purpose 
* and, to the extent permitted by law, all liability for your use of the code is disclaimed. 
* Some programs in this code are governed by the GNU General Public License v3.0 (available at 
* https://www.gnu.org/licenses/gpl-3.0.en.html) (‘GPLv3’). The programs that are governed by 
* GPLv3.0 are those programs that are located in the folders src/depends and tests/depends 
* and which include a reference to GPLv3 in their program files.
**/

#ifndef __FIXEDSIZEQUEUE_H__
#define __FIXEDSIZEQUEUE_H__

#include <deque>
#include <stdexcept>

#include "libUtils/Logger.h"

template<class T> class FixedSizeQueue
{
    std::deque<T> m_queue;
    unsigned int m_capacity;

public:
    /// Default constructor.
    FixedSizeQueue() { m_capacity = 0; }

    /// Changes the array maximum capacity
    void reset(unsigned int capacity)
    {
        m_queue.clear();
        m_capacity = capacity;
    }

    FixedSizeQueue(const FixedSizeQueue<T>& FixedSizeQueue) = delete;

    FixedSizeQueue& operator=(const FixedSizeQueue<T>& FixedSizeQueue) = delete;

    /// Destructor.
    ~FixedSizeQueue() {}

    /// Index operator.
    T& operator[](unsigned int index)
    {
        if(!m_capacity)
        {
            LOG_GENERAL(WARNING, "m_capacity not initialized");
            throw;
        }
        return m_queue.at(index);
    }

    /// Returns the element at the back of the array.
    T& back()
    {
        if(!m_capacity)
        {
            LOG_GENERAL(WARNING, "m_capacity not initialized");
            throw;
        }
        if(m_queue.empty())
        {
            LOG_GENERAL(WARNING,"Back called on empty queue");
            throw;
        }
        return m_queue.back();
    }

    /// Adds an element to the end of the array.
    void push_back(T element)
    {
        if (!m_capacity)
        {
            LOG_GENERAL(WARNING, "m_capacity not initialized");
            throw;
        }
        if (m_queue.size() == m_capacity)
        {
            m_queue.pop_front();
        }
        m_queue.push_back(element);
    }

    /// Returns the storage capacity of the queue.
    int capacity() { return m_capacity; }

    ///Returns the current size of the queue
    int size() { return m_queue.size(); }
};

#endif // __FixedSizeQueue_H__