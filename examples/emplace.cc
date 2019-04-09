#include <map>
#include <unordered_map>
#include <string>
#include <iostream>
#include <chrono>
#include <vector>
#include <parallel_hashmap/phmap.h>

#include <sstream>

namespace patch
{
    template <typename T> std::string to_string(const T& n)
    {
        std::ostringstream stm;
        stm << n;
        return stm.str();
    }
}

template <typename T>
using milliseconds = std::chrono::duration<T, std::milli>;

class custom_type
{
    std::string one = "one";
    std::string two = "two";
    std::uint32_t three = 3;
    std::uint64_t four = 4;
    std::uint64_t five = 5;
public:
    custom_type() = default;

    // Make object movable and non-copyable
    custom_type(custom_type &&) = default;
    custom_type& operator=(custom_type &&) = default;

    // should be automatically deleted per http://www.slideshare.net/ripplelabs/howard-hinnant-accu2014
    //custom_type(custom_type const&) = delete;
    //custom_type& operator=(custom_type const&) = delete;
};

class custom_type_2
{
    std::uint32_t three = 3;
    std::uint64_t four  = 4;
    std::uint64_t five  = 5;
    std::uint64_t six   = 6;
public:
    custom_type_2() = default;

    // Make object movable and non-copyable
    custom_type_2(custom_type_2 &&) = default;
    custom_type_2& operator=(custom_type_2 &&) = default;

    // should be automatically deleted per http://www.slideshare.net/ripplelabs/howard-hinnant-accu2014
    //custom_type_2(custom_type_2 const&) = delete;
    //custom_type_2& operator=(custom_type_2 const&) = delete;
};

// emplace string + large struct
// -----------------------------
template <class Map, class V, class T> struct _emplace
{
    void operator()(Map &m, std::size_t j);
};

// "void" template parameter -> use emplace
template <class Map, class V> struct _emplace<Map, V, void>
{
    void operator()(Map &m, std::size_t j)
    {
        m.emplace(patch::to_string(j), V());
    }
};

// "int" template parameter -> use emplace_back for std::vector
template <class Map, class V> struct _emplace<Map, V, int>
{
    void operator()(Map &m, std::size_t j)
    {
        m.emplace_back(patch::to_string(j), V());
    }
};

// insert <int, custom_type_2>
// ---------------------------
template <class Map, class V, class T> struct emplace_2_ints
{
    void operator()(Map &m, std::size_t j);
};

// "void" template parameter -> use emplace
template <class Map, class V> struct emplace_2_ints<Map, V, void>
{
    void operator()(Map &m, std::size_t j)
    {
        m.emplace(j, V());
    }
};

// "int" template parameter -> use emplace_back for std::vector
template <class Map, class V> struct emplace_2_ints<Map, V, int> 
{
    void operator()(Map &m, std::size_t j)
    {
        m.emplace_back(j, V());
    }
};

// The test itself
// ---------------
template <class Map, class V, class T, template <class, class, class> class INSERT>
void _test(std::size_t iterations, std::size_t container_size, const char *map_name)
{
    std::size_t count = 0;
    auto t1 = std::chrono::high_resolution_clock::now();
    INSERT<Map, V, T> insert;
    for (std::size_t i=0; i<iterations; ++i)
    {
        Map m;
        for (std::size_t j=0; j<container_size; ++j)
            insert(m, j);
        count += m.size();
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    auto elapsed = milliseconds<double>(t2 - t1).count();
    if (count != iterations*container_size)
        std::clog << "  invalid count: " << count << "\n";
    std::clog << map_name << std::fixed << int(elapsed) << " ms\n";
}


template <class K, class V, template <class, class, class> class INSERT>
void test(std::size_t iterations, std::size_t container_size)
{
    std::clog << "bench: iterations: " << iterations <<  " / container_size: "  << container_size << "\n";

    _test<std::map<K, V>,               V, void, INSERT>(iterations, container_size, "  std::map:               ");
    _test<std::unordered_map<K, V>,     V, void, INSERT>(iterations, container_size, "  std::unordered_map:     ");
    _test<phmap::flat_hash_map<K, V>,   V, void, INSERT>(iterations, container_size, "  phmap::flat_hash_map:   ");
    _test<std::vector<std::pair<K, V>>, V, int, INSERT> (iterations, container_size, "  std::vector<std::pair>: ");
    std::clog << "\n";
    
}

int main()
{
    std::size_t iterations = 100000;

    // test with custom_type_2 (int key + 32 byte value). This is representative 
    // of the hash table insertion speed.
    // -------------------------------------------------------------------------
    std::clog << "\n\n" << "testing with <int, custom_type_2>"   "\n";
    std::clog << "---------------------------------"   "\n";
    test<int, custom_type_2, emplace_2_ints>(iterations,10);
    test<int, custom_type_2, emplace_2_ints>(iterations,100);
    test<int, custom_type_2, emplace_2_ints>(iterations,500);

    // test with custom_type, which contains two std::string values, and use 
    // a generated string key. This is not very indicative of the speed of the 
    // hash itself, as a good chunk of the time is spent creating the keys and 
    // values (as shown by the long times even for std::vector).
    // -----------------------------------------------------------------------
    std::clog << "\n" << "testing with <string, custom_type>"   "\n";
    std::clog << "---------------------------------"   "\n";
    test<std::string, custom_type, _emplace>(iterations,1);
    test<std::string, custom_type, _emplace>(iterations,10);
    test<std::string, custom_type, _emplace>(iterations,50);

}
