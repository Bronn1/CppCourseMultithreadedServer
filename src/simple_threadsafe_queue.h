#ifndef SIMPLE_THREADSAFE_QUEUE_H_INCLUDED
#define SIMPLE_THREADSAFE_QUEUE_H_INCLUDED

#include <mutex>
#include <condition_variable>
#include <queue>


#include <memory>

namespace server
{
/*
Simple(and slow a bit) queue using locks
for one reader/writer could be lock free but for the final 
course exam i'll do very simple queue
*/
template<typename T>
class threadsafe_queue
{
public:
    using value_type = T;
    using value_ptr = std::shared_ptr<T>;

    void push(const T& new_data)
    {
        auto data_ptr = std::make_shared<T>(new_data);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push(data_ptr);
        m_cond_var.notify_one();
    }

    value_ptr wait_and_pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond_var.wait(lock,[this](){ return !m_queue.empty();} );
        auto data_ptr = m_queue.front();
        m_queue.pop();

        return data_ptr;
    }

    bool is_empty() const
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.is_empty();
    }

private:
    std::queue<value_ptr> m_queue{};
    mutable std::mutex m_mutex;
    std::condition_variable m_cond_var;
};
} // namespace server

#endif //SIMPLE_THREADSAFE_QUEUE_H_INCLUDED