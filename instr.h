#ifndef INSTR_H
#define INSTR_H

#include <memory>
#include <random>
#include <vector>
#include <type_traits>
#include <utility>

#ifndef MAX_BOGUS_IMPLEMENTATIONS
#define MAX_BOGUS_IMPLEMENTATIONS 3
#endif

namespace obf
{

// random generator reusing from https://github.com/andrivet/ADVobfuscator
// Copyright (c) 2010-2014, Sebastien Andrivet
// All rights reserved.
constexpr char time[] = __TIME__;
constexpr int DigitToInt(char c) { return c - '0'; }
const int seed = DigitToInt(time[7]) +
                 DigitToInt(time[6]) * 10 +
                 DigitToInt(time[4]) * 60 +
                 DigitToInt(time[3]) * 600 +
                 DigitToInt(time[1]) * 3600 +
                 DigitToInt(time[0]) * 36000;

template<int N>
struct MetaRandomGenerator
{
private:
    static constexpr unsigned a = 16807;
    static constexpr unsigned m = 2147483647;
    static constexpr unsigned s = MetaRandomGenerator<N - 1>::value;
    static constexpr unsigned lo = a * (s & 0xFFFF);
    static constexpr unsigned hi = a * (s >> 16);
    static constexpr unsigned lo2 = lo + ((hi & 0x7FFF) << 16);
    static constexpr unsigned hi2 = hi >> 15;
    static constexpr unsigned lo3 = lo2 + hi;

public:
    static constexpr unsigned max = m;
    static constexpr unsigned value = lo3 > m ? lo3 - m : lo3;
};

template<>
struct MetaRandomGenerator<0>
{
    static constexpr unsigned value = seed;
};

template<int N, int M>
struct MetaRandom
{
    static const int value = MetaRandomGenerator<N + 1>::value % M;
};

#define COMP_ASSIGNMENT_OPERATOR(x) \
    refholder<T>& operator x##= (const refholder<T>& ov) { v x##= ov.v; return *this;}  \
    refholder<T>& operator x##= (const refholder<T>&& ov) { v x##= ov.v; return *this;} \
    refholder<T>& operator x##= (const T& ov) { v x##= ov; return *this;}               \
    refholder<T>& operator x##= (const T&& ov) { v x##= ov; return *this;}              \
    refholder<T>& operator x##= (T& ov) { v x##= ov; return *this;}


#define COMPARISON_OPERATOR(x) \
    bool operator x (const T& ov) { return (v x ov); }


/* simple reference holder class, mostly for dealing with numbers */
template <typename T>
class refholder final
{
public:
    /* Construction, destruction */
    refholder() = delete;
    refholder(T& pv) : v(pv) {}
    refholder(T&&) = delete;

    ~refholder() = default;

    /* Assignment */
    refholder<T>& operator = (const T& ov) { v = ov; return *this;}
    refholder<T>& operator = (const refholder<T>& ov ) { v = ov.v; return *this; }

    /* Comparison */
    bool operator == (const T& ov) { return !(v ^ ov); }
    bool operator != (const T& ov) { return !operator ==(ov); }
    COMPARISON_OPERATOR(>=)
    COMPARISON_OPERATOR(<=)
    COMPARISON_OPERATOR(>)
    COMPARISON_OPERATOR(<)

    /* Conversion to the real type */
    operator T() {return v;}

    /* Pre increment/decrement operators */
    refholder<T>& operator++() { ++ v; return *this; }
    refholder<T>& operator--() { -- v; return *this; }


    /* post increment/decrement */
    refholder<T> operator++(int) { refholder<T> rv(*this); operator ++(); return rv; }
    refholder<T> operator--(int) { refholder<T> rv(*this); operator --(); return rv; }

    /* Compound assignments */
    COMP_ASSIGNMENT_OPERATOR(+)
    COMP_ASSIGNMENT_OPERATOR(-)
    COMP_ASSIGNMENT_OPERATOR(*)
    COMP_ASSIGNMENT_OPERATOR(/)
    COMP_ASSIGNMENT_OPERATOR(%)
    COMP_ASSIGNMENT_OPERATOR(<<)
    COMP_ASSIGNMENT_OPERATOR(>>)
    COMP_ASSIGNMENT_OPERATOR(&)
    COMP_ASSIGNMENT_OPERATOR(|)
    COMP_ASSIGNMENT_OPERATOR(^)

private:

    /* The root of all evil */
    T& v;
};

/* Binary operators for the value wrappers */
#define DEFINE_BINARY_OPERATOR(x) \
template <class T> refholder<T> operator x (refholder<T>& ls, const T& rs) {refholder<T> rv = ls; ls x##= rs; return rv; } \
template <class T> refholder<T> operator x (refholder<T>& ls, T& rs) {refholder<T> rv = ls; ls x##= rs; return rv; } \
template <class T> refholder<T> operator x (refholder<T>& ls, T&& rs) {refholder<T> rv = ls; ls x##= rs; return rv; } \
template <class T> refholder<T> operator x (refholder<T>& ls, const T&& rs) {refholder<T> rv = ls; ls x##= rs; return rv; } \
template <class T> refholder<T> operator x (refholder<T>& ls, const refholder<T>&& rs) {refholder<T> rv = ls; ls x##= rs; return rv; } \
template <class T> refholder<T> operator x (refholder<T>& ls, refholder<T>&& rs) {refholder<T> rv = ls; ls x##= rs; return rv; } \
template <class T> refholder<T> operator x (refholder<T>& ls, refholder<T>& rs) {refholder<T> rv = ls; ls x##= rs; return rv; } \
template <class T> refholder<T> operator x (refholder<T>& ls, const refholder<T>& rs) {refholder<T> rv = ls; ls x##= rs; return rv; }

DEFINE_BINARY_OPERATOR(+)
DEFINE_BINARY_OPERATOR(-)
DEFINE_BINARY_OPERATOR(*)
DEFINE_BINARY_OPERATOR(/)
DEFINE_BINARY_OPERATOR(%)
DEFINE_BINARY_OPERATOR(&)
DEFINE_BINARY_OPERATOR(|)
DEFINE_BINARY_OPERATOR(<<)
DEFINE_BINARY_OPERATOR(>>)


/* Helping stuff */

struct base_rvholder
{
    virtual ~base_rvholder() = default;

    template<class T>
    operator T () const
    {
        return *reinterpret_cast<const T*>(get());
    }

    template<class T>
    bool operator == (const T& o) const
    {
        return o == operator T ();
    }

    template<class T>
    bool equals(const T& o) const
    {
        return o == *reinterpret_cast<const T*>(get());
    }

    virtual const void* get() const = 0;
};

template<class T>
class rvholder : public base_rvholder
{
public:
    rvholder(T t, T c) :base_rvholder(), v(t), check(c) {}
    ~rvholder() = default;
    virtual const void* get() const override {return reinterpret_cast<const void*>(&v);}
private:
    T v;
    T check;
};


/* what the RETURN/BREAK/CONTINUE will return while running from inside a loop block*/
enum class next_step
{
    ns_break,
    ns_continue,
    ns_return,
    ns_done
};

struct next_step_functor_base
{
    virtual next_step run() = 0;

};

template <class T>
struct next_step_functor final : public next_step_functor_base {

    next_step_functor(T r) : runner(r) {}
    virtual next_step run() {return runner();}

private:
    T runner;
};

struct bool_functor_base
{
    virtual bool run() = 0;
};

template <class T>
struct bool_functor final : public bool_functor_base
{
    bool_functor(T r) : runner(r) {}
    virtual bool run() {return runner();}

private:
    T runner;
};

struct any_functor_base
{
    virtual void run(void*) = 0;
};

template <class T>
struct any_functor final : public any_functor_base
{
    any_functor(T r) : runner(r) {}

    virtual void run(void* retv)
    {
        auto r = runner();
        *reinterpret_cast<decltype(r)*>(retv) = r;
    }


private:
    T runner;

};

/* c++ control structures implementation */

/* supporting implementation for the REPEAT/UNTIL macros*/

class repeat_wrapper
{
public:
    repeat_wrapper():body(nullptr), condition(nullptr) {}

    void run()
    {
        do
        {
            next_step c = body->run();
            if(c == next_step::ns_break) break;
            if(c == next_step::ns_continue) continue;
        }
        while( condition->run() );
    }

    template<class T>
    repeat_wrapper& set_body(T lambda) { body.reset(new next_step_functor<T>(lambda)); return *this; }


    template<class T>
    repeat_wrapper& set_condition(T lambda) { condition.reset(new bool_functor<T>(lambda)); return *this; }

private:

    std::unique_ptr<next_step_functor_base> body;
    std::unique_ptr<bool_functor_base> condition;
};

/* supporting implementation for the FOR macro */

class for_wrapper final
{
public:

    template<class INIT, class COND, class INCR>
    explicit for_wrapper(INIT lambda_init, COND lambda_cond, INCR lambda_incr)
    {
        condition.reset(new bool_functor<COND>(lambda_cond));
        initializer.reset(new next_step_functor<INIT>(lambda_init));
        increment.reset(new next_step_functor<INCR>(lambda_incr));
    }

    void run()
    {
        for( initializer->run(); condition->run(); increment->run())
        {
            next_step c = body->run();
            if(c == next_step::ns_break) break;
            if(c == next_step::ns_continue) continue;
        }
    }

    ~for_wrapper() noexcept = default;

    template<class T>
    for_wrapper& set_body(T lambda) { body.reset(new next_step_functor<T>(lambda)); return *this; }
private:
    std::unique_ptr<next_step_functor_base> initializer;
    std::unique_ptr<bool_functor_base> condition;
    std::unique_ptr<next_step_functor_base> increment;
    std::unique_ptr<next_step_functor_base> body;

};

/* supporting implementation for the WHILE macro */

class while_wrapper final
{
public:
    template<class T>
    explicit while_wrapper(T lambda) :body(nullptr), condition(nullptr)
    {condition.reset(new bool_functor<T>(lambda));}

    void run()
    {
        while( condition->run() )
        {
            next_step c = body->run();
            if(c == next_step::ns_break) break;
            if(c == next_step::ns_continue) continue;
        }
    }

    ~while_wrapper() noexcept = default;

    template<class T>
    while_wrapper& set_body(T lambda) { body.reset(new next_step_functor<T>(lambda)); return *this; }

private:
    std::unique_ptr<next_step_functor_base> body;
    std::unique_ptr<bool_functor_base> condition;
};

/* supporting implementation for the IF macro */

class if_wrapper final
{
public:
    template<class T>
    if_wrapper(T lambda) {condition.reset(new bool_functor<T>(lambda));}

    void run()
    {
        if(condition->run()) { if(thens) {
            thens->run();
        }}
        else { if(elses) {
            elses->run();
        }}

    }

    ~if_wrapper() noexcept = default;

    template<class T>
    if_wrapper& set_then(T lambda) { thens.reset(new next_step_functor<T>(lambda)); return *this; }
    template<class T>
    if_wrapper& set_else(T lambda) { elses.reset(new next_step_functor<T>(lambda)); return *this; }

private:
    std::unique_ptr<bool_functor_base> condition;
    std::unique_ptr<next_step_functor_base> thens;
    std::unique_ptr<next_step_functor_base> elses;
};

/* select ... case helper */

class case_instruction
{
public:
    case_instruction() = default;
    virtual ~case_instruction() = default;
    virtual next_step execute(const base_rvholder&) const = 0;
};

template<class CT>
class branch final : public case_instruction
{
public:
    template<class T>
    branch(T lambda) {condition.reset(new any_functor<T>(lambda));}

    bool equals(const base_rvholder& rv, CT lv) const
    {
        return rv.equals(lv);
    }

    virtual next_step execute(const base_rvholder& against) const override
    {
        CT retv;
        condition->run( reinterpret_cast<void*>(&retv) );
        // this looks funny, however we cannot use the operator ==
        // due to a visual C++ 2015 compiler bug, which fails to compile and crashes
        return equals(against,retv) ? next_step::ns_done : next_step::ns_continue;
    }

private:
    std::unique_ptr<any_functor_base> condition;
};

class body final : public case_instruction
{
public:
    template<class T>
    body(T lambda) {instructions.reset(new next_step_functor<T>(lambda));}


    virtual next_step execute(const base_rvholder&) const override
    {
        return instructions->run();
    }
private:
    std::unique_ptr<next_step_functor_base> instructions;
};

template<class CT>
class case_wrapper final
{
public:
    explicit case_wrapper(const CT& v) : check(v) {}

    case_wrapper& add_entry(const case_instruction& lambda_holder)
    {
        steps.push_back(&lambda_holder);
        return *this;
    }

    case_wrapper& join()
    {
        return *this;
    }

    void run()
    {
        auto it = steps.begin();
        while(it != steps.end())
        {
            bool increased = false;
            // see if  this is a branch or body
            if(dynamic_cast<const branch<CT>*>(*it))
            {
                // branch. Execute it, see if it returns true or false
                next_step enter = (*it)->execute(rvholder<CT>(check,check));
                if(enter == next_step::ns_continue)
                {
                    // step to the next branch
                    ++it;
                    increased = true;
                }
                else
                {
                    // now fast forward and find the first body,
                    // and from that point on run all the bodies, unless one of them breaks
                    while(! dynamic_cast<const body*>(*it)  && it != steps.end() )
                    {
                        ++it;
                        increased = true;
                    }

                    // found the first body.
                    while(it != steps.end())
                    {
                        if(dynamic_cast<const body*>(*it))
                        {
                            next_step leave_switch = (*it)->execute(rvholder<CT>(check,check));
                            if(leave_switch == next_step::ns_break)
                            {
                                return;
                            }
                        }
                        increased = true;
                        ++it;
                    }
                }
            }
            else
            {
                // shouldn't really get in here
            }


            // just for safety
            if(!increased)
            {
                ++it;
            }
        }
    }

private:
    std::vector<const case_instruction*> steps;
    CT check;
};

/* syntactic sugar */

template<typename X>
refholder<X> RH (X& a)
{
    return refholder<X>(a);
}

class stream_helper {};
template <typename T>
refholder<T> operator << (stream_helper, T& a)
{
    return refholder<T>(a);
}

/* The bogus operations allowed */
class basic_extra
{
public:
    basic_extra() = default;
    ~basic_extra() = default;
    basic_extra(const basic_extra&) = delete;
    basic_extra& operator = (const basic_extra&) = delete;
    basic_extra(const basic_extra&&) = delete;
    basic_extra& operator = (const basic_extra&&) = delete;
};

template <class T>
class extra_xor : public basic_extra
{
public:
    extra_xor(T& a) : v(a)
    {
        v ^= MetaRandom<16, 4096>::value;
    }
    virtual ~extra_xor() { v ^= MetaRandom<16, 4096>::value; }

private:
    T& v;
};

template <class T>
class extra_addition : public basic_extra
{
public:
    extra_addition(T& a) : v(a) { v += 1; }
    virtual ~extra_addition() { v -= 1; }


private:
    T& v;
};


template <class T>
class extra_substraction : public basic_extra
{
public:
    extra_substraction(T& a) : v(a) {v -= 1; }
    virtual ~extra_substraction() { v += 1; }

private:
    T& v;
};


template <typename T, int N>
class extra_chooser
{
    using type=basic_extra; // intentionally private
};

/* Number obfuscation implementer */

template<typename T, T n>
struct Num
{
    enum { value = ( (n & 0x01)  | ( Num < T , (n >> 1)>::value << 1) ) };
    T v;
    Num() : v(0)
    {
        v = value ^  MetaRandom<32, 4096>::value;
    }
    T get() const { return v ^ MetaRandom<32, 4096>::value;}
};

template <> struct Num<int,0>
{
    enum {value = 0};
    int v = value;
};

template <> struct Num<int,1>
{
    enum {value = 1};
    int v = value;
};

#define N(a) (obf::Num<decltype(a), obf::MetaRandom<__COUNTER__, 4096>::value ^ a>().get() ^ obf::MetaRandom<__COUNTER__ - 1, 4096>::value)
#define DEFINE_EXTRA(N,implementer) template <typename T> struct extra_chooser<T,N> { using type = implementer<T>; }
DEFINE_EXTRA(0, extra_xor);
DEFINE_EXTRA(1, extra_substraction);
DEFINE_EXTRA(2, extra_addition);
#define V(a) ([&](){obf::extra_chooser<decltype(a), obf::MetaRandom<__COUNTER__, MAX_BOGUS_IMPLEMENTATIONS>::value >::type _ec_##__COUNTER__(a);\
            return obf::stream_helper();}() << a)
#define IF(x) { std::shared_ptr<obf::base_rvholder> rvlocal; obf::if_wrapper(( [&]()->bool{ return (x); })).set_then( [&]() {
#define ELSE return obf::next_step::ns_done;}).set_else( [&]() {
#define FOR(init,cond,inc) { std::shared_ptr<obf::base_rvholder> rvlocal; obf::for_wrapper( [&]()\
           {init;return obf::next_step::ns_done;}, [&]()->bool{return cond;}, [&](){inc;return obf::next_step::ns_done;}).set_body( [&]() {
#define END return obf::next_step::ns_done;}).run(); }
#define ENDIF END
#define ENDWHILE END
#define ENDFOR END
#define BREAK return obf::next_step::ns_break;
#define RETURN(x) rvlocal.reset(new obf::rvholder<decltype(x)>(x,x));  throw rvlocal;
#define CONTINUE return obf::next_step::ns_continue;
#define WHILE(x) {std::shared_ptr<obf::base_rvholder> rvlocal; obf::while_wrapper([&]()->bool{ return (x); }).set_body( [&]() {
#define REPEAT { std::shared_ptr<obf::base_rvholder> rvlocal; obf::repeat_wrapper().set_body( [&]() {
#define UNTIL(x) return obf::next_step::ns_done;}).set_condition([&]()->bool{ return (x); }).run(); }
#define OBF_BEGIN try { std::shared_ptr<obf::base_rvholder> rvlocal;
#define OBF_END } catch(std::shared_ptr<obf::base_rvholder>& r) { return *r; }

#define CASE(a) {std::shared_ptr<obf::base_rvholder> rvlocal; obf::case_wrapper<decltype(a)>(a).
#define ENDCASE run(); }
#define WHEN(c) add_entry(obf::branch<decltype(c)>( [](){return (c);})).
#define DO add_entry( obf::body([](){
#define DONE return obf::next_step::ns_continue;})).
#define OR join().

} // namespace obf

#endif // INSTR_H
