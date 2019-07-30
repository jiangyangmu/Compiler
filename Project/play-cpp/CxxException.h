#pragma once

#include <exception>
#include <iostream>

// ISO:
// https://en.cppreference.com/w/cpp/error
// https://en.cppreference.com/w/cpp/language/exceptions
//      throw...try...catch...RAII
//      throw...terminate
//      exception object: nested
//      exception object: save for later
//      noexcept

// WIN:
// https://docs.microsoft.com/en-us/cpp/cpp/exception-handling-in-visual-cpp
//  SEH: for non-MFC C
//      __try...__catch
//      __try...__finally
//  MFC exceptions: for MFC

void Cxx_Terminate()
{
    std::cout << "Terminate!" << std::endl;
}
void Cxx_BadCast()
{
    class BadCastFrom { public: virtual ~BadCastFrom() = default; };
    class BadCastTo { public: virtual ~BadCastTo() = default; };
    BadCastFrom from;
    BadCastTo & to = dynamic_cast<BadCastTo &>(from);
}
void Test_CxxISO_ErrorHandling()
{
    // throw...try...catch...RAII
    for (int i = 0; i < 11; ++i)
    {
        try
        {
            class FinallyRAII {
            public:
                FinallyRAII(int i) : id(i) {}
                ~FinallyRAII() { std::cout << "Finally " << id << " called." << std::endl; }
            private:
                int id;
            } f(i);

            switch (i)
            {
                case 0: throw "somethine is wrong!"; break;
                case 1: throw 100; break;
                
                // List: https://en.cppreference.com/w/cpp/error/exception
                // Common exceptions
                case 2: throw std::runtime_error("runtime_error!"); break;

                case 3: throw std::out_of_range("out_of_range!"); break;
                case 4: throw std::overflow_error("overflow!"); break;
                case 5: throw std::underflow_error("underflow!"); break;
                
                case 6: throw std::invalid_argument("invalid_argument!"); break;

                case 7: throw std::bad_alloc(); break;
                case 8: Cxx_BadCast(); break;

                default: break;
            }
        }
        catch (const char * str)
        {
            std::cerr << "Caught string: " << str << std::endl;
        }
        catch (int integer)
        {
            std::cerr << "Caught integer: " << integer << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Caught exception: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "Uncaught exception." << std::endl;
            std::abort();
        }
    }

    // throw...terminate
    std::set_terminate(Cxx_Terminate);
    throw "Throw for termination!";
}
