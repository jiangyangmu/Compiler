#ifndef UTIL_H
#define UTIL_H

#include <cassert>
#include <map>
#include <set>
#include <vector>

#include <iostream>
template <class T>
void DebugPrintValue(T & container)
{
    std::cout << '[';
    for (auto & i : container)
    {
        std::cout << i.second << ',';
    }
    std::cout << ']' << std::endl;
}

template <class T>
void DebugPrint(T & container)
{
    std::cout << '[';
    for (auto & i : container)
    {
        std::cout << i << ',';
    }
    std::cout << ']' << std::endl;
}


#define CHECK(x) assert(x)
#define CHECK_GT(a, b) assert((a) > (b))
#define CHECK_EQ(a, b) assert((a) == (b))

template <class T>
class TopoMap {
public:
    void add(T t)
    {
        if (dependents_.find(t) == dependents_.end())
            dependents_[t] = {};
        if (indegree_.find(t) == indegree_.end())
            indegree_[t] = 0;
    }
    void add_dependency(T t, T dependent)
    {
        assert(t != dependent);
        dependents_[t].insert(dependent);
        indegree_[dependent]++;
        if (indegree_.find(t) == indegree_.end())
            indegree_[t] = 0;
    }
    std::set<T> & dependents(T t)
    {
        return dependents_[t];
    }
    // dependent last
    std::vector<T> sort()
    {
        std::vector<T> result;
        std::map<T, int> d1 = indegree_;
        std::map<T, int> d2 = indegree_;

        while (result.size() < dependents_.size())
        {
            size_t count = result.size();
            // DebugPrintValue(d1);
            for (auto & kv : d1)
            {
                T t = kv.first;
                int in = kv.second;
                // still has dependency not resolved.
                if (in != 0)
                    continue;

                result.emplace_back(t);

                for (T dependent : dependents_[t])
                {
                    assert(d2[dependent] > 0);
                    d2[dependent]--;
                }

                d2[t] = -1;
            }

            // circular dependency detected if false.
            assert(result.size() > count);

            d1 = d2;
        }

        return result;
    }
private:
    std::map<T, std::set<T>> dependents_;
    std::map<T, int> indegree_;
};

#endif
