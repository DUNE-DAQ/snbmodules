/*
    The MIT License (MIT)
    Copyright (c) 2016 Gagan Kumar(scope_infinity)
    Complete License at https://raw.githubusercontent.com/scope_infinity/NaturalSort/master/LICENSE.md
*/

/**********************************************************************
Calling Methods :
    //For Natural Sorting
    void SI::natural::sort(Container<String>);
    void SI::natural::sort(IteratorBegin<String>,IteratorEnd<String>);
    void SI::natural::sort<String,CArraySize>(CArray<String>);
    //For Natural Comparision
    bool SI::natural::compare<String>(String lhs,String rhs);
    bool SI::natural::compare<String>(char *const lhs,char *const rhs);
    Here we can have
        std::vector<std::string> 	as Container<String>
        String 						as std::string
        CArray<String>				as std::string[CArraySize]
***********************************************************************/
#ifndef SNBMODULES_INCLUDE_SNBMODULES_SISortHPP_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_SISortHPP_HPP_
#include <cctype>
#include <algorithm>
#include <vector>

namespace SI
{
    namespace natural
    {
        namespace detail
        {
            /********** Compare Two Character CaseInsensitive ********/
            template <typename ElementType>
            bool natural_less(const ElementType &lhs, const ElementType &rhs)
            {
                if (tolower(lhs) < tolower(rhs))
                    return true;
                return false;
            }

            template <typename ElementType>
            bool is_not_digit(const ElementType &x)
            {
                return !isdigit(x);
            }

            /********** Compare Two Iterators CaseInsensitive ********/
            template <typename ElementType, typename Iterator>
            struct comp_over_iterator
            {
                int operator()(const Iterator &lhs, const Iterator &rhs) const
                {
                    if (natural_less<ElementType>(*lhs, *rhs))
                        return -1;
                    if (natural_less<ElementType>(*rhs, *lhs))
                        return +1;
                    return 0;
                }
            };

            /****************************************************
            Comparing two SubString from (Begin,End Iterator each)
            with only digits.
            Usage :
                int compare_number()(\
                First_number_beginIterator,First_number_endIterator,is_first_number_fractional_part\
                Second_number_beginIterator,Second_number_endIterator,is_second_number_fractional_part\
                );
            Returns :
                -1  - Number1  <   Number2
                 0  - Number1  ==  Number2
                 1  - Number1  >   Number2
            ***************************************************/

            template <typename ValueType, typename Iterator>
            struct compare_number
            {
            private:
                // If Number is Itself fractional Part
                int fractional(Iterator lhs_begin, Iterator lhs_end, Iterator rhs_begin, Iterator rhs_end)
                {
                    while (lhs_begin < lhs_end && rhs_begin < rhs_end)
                    {
                        int local_compare = comp_over_iterator<ValueType, Iterator>()(lhs_begin, rhs_begin);
                        if (local_compare != 0)
                            return local_compare;
                        lhs_begin++;
                        rhs_begin++;
                    }
                    while (lhs_begin < lhs_end && *lhs_begin == '0')
                        lhs_begin++;
                    while (rhs_begin < rhs_end && *rhs_begin == '0')
                        rhs_begin++;
                    if (lhs_begin == lhs_end && rhs_begin != rhs_end)
                        return -1;
                    else if (lhs_begin != lhs_end && rhs_begin == rhs_end)
                        return +1;
                    else // lhs_begin==lhs_end && rhs_begin==rhs_end
                        return 0;
                }
                int non_fractional(Iterator lhs_begin, Iterator lhs_end, Iterator rhs_begin, Iterator rhs_end)
                {
                    // Skip Inital Zero's
                    while (lhs_begin < lhs_end && *lhs_begin == '0')
                        lhs_begin++;
                    while (rhs_begin < rhs_end && *rhs_begin == '0')
                        rhs_begin++;

                    // Comparing By Length of Both String
                    if (lhs_end - lhs_begin < rhs_end - rhs_begin)
                        return -1;
                    if (lhs_end - lhs_begin > rhs_end - rhs_begin)
                        return +1;

                    // Equal In length
                    while (lhs_begin < lhs_end)
                    {
                        int local_compare = comp_over_iterator<ValueType, Iterator>()(lhs_begin, rhs_begin);
                        if (local_compare != 0)
                            return local_compare;
                        lhs_begin++;
                        rhs_begin++;
                    }
                    return 0;
                }

            public:
                int operator()(
                    Iterator lhs_begin, Iterator lhs_end, bool is_fractional_part1,
                    Iterator rhs_begin, Iterator rhs_end, bool is_fractional_part2)
                {
                    if (is_fractional_part1 && !is_fractional_part2)
                        return true; // 0<num1<1 && num2>=1
                    if (!is_fractional_part1 && is_fractional_part2)
                        return false; // 0<num2<1 && num1>=1

                    // is_fraction_part1 == is_factional_part2
                    if (is_fractional_part1)
                        return fractional(lhs_begin, lhs_end, rhs_begin, rhs_end);
                    else
                        return non_fractional(lhs_begin, lhs_end, rhs_begin, rhs_end);
                }
            };

        } // namespace detail

        /***********************************************************************
        Natural Comparision of Two String using both's (Begin and End Iterator)
        Returns :
            -1  - String1  <   String2
             0  - String1  ==  String2
             1  - String1  >   String2
        Suffix 1 represents for components of 1st String
        Suffix 2 represents for components of 2nd String
        ************************************************************************/

        template <typename ElementType, typename Iterator>
        bool _compare(
            const Iterator &lhs_begin, const Iterator &lhs_end,
            const Iterator &rhs_begin, const Iterator &rhs_end)
        {
            Iterator current1 = lhs_begin, current2 = rhs_begin;

            // Flag for Space Found Check
            bool flag_found_space1 = false, flag_found_space2 = false;

            while (current1 != lhs_end && current2 != rhs_end)
            {
                // Ignore More than One Continous Space

                /******************************************
                For HandlingComparision Like
                    Hello   9
                    Hello  10
                    Hello 123
                ******************************************/
                while (flag_found_space1 && current1 != lhs_end && *current1 == ' ')
                    current1++;
                flag_found_space1 = false;
                if (*current1 == ' ')
                    flag_found_space1 = true;

                while (flag_found_space2 && current2 != rhs_end && *current2 == ' ')
                    current2++;
                flag_found_space2 = false;
                if (*current2 == ' ')
                    flag_found_space2 = true;

                if (!isdigit(*current1) || !isdigit(*current2))
                {
                    // Normal comparision if any of character is non digit character
                    if (detail::natural_less<ElementType>(*current1, *current2))
                        return true;
                    if (detail::natural_less<ElementType>(*current2, *current1))
                        return false;
                    current1++;
                    current2++;
                }
                else
                {
                    /*********************************
                    Capture Numeric Part of Both String
                    and then using it to compare Both
                    ***********************************/
                    Iterator last_nondigit1 = std::find_if(current1, lhs_end, detail::is_not_digit<ElementType>);
                    Iterator last_nondigit2 = std::find_if(current2, rhs_end, detail::is_not_digit<ElementType>);

                    int result = detail::compare_number<ElementType, Iterator>()(
                        current1, last_nondigit1, (current1 > lhs_begin && *(current1 - 1) == '.'),
                        current2, last_nondigit2, (current2 > rhs_begin && *(current2 - 1) == '.'));
                    if (result < 0)
                        return true;
                    if (result > 0)
                        return false;
                    current1 = last_nondigit1;
                    current2 = last_nondigit2;
                }
            }

            if (current1 == lhs_end && current2 == rhs_end)
            {
                return false;
            }
            else
            {
                return current1 == lhs_end;
            }
        }

        template <typename String>
        inline bool compare(const String &first, const String &second)
        {
            return _compare<typename String::value_type, typename String::const_iterator>(first.begin(), first.end(), second.begin(), second.end());
        }
        template <>
        inline bool compare(char *const &first, char *const &second)
        {
            char *it1 = first;
            while (*it1 != '\0')
                it1++;
            char *it2 = second;
            while (*it2 != '\0')
                it2++;
            return _compare<char, char *>(first, it1, second, it2);
        }

        template <typename Container>
        inline void sort(Container &container)
        {
            std::sort(container.begin(), container.end(), compare<typename Container::value_type>);
        }

        template <typename Iterator>
        inline void sort(const Iterator &first, const Iterator &end)
        {
            std::sort(first, end, compare<typename Iterator::value_type>);
        }

        template <typename ValueType>
        inline void sort(ValueType *const first, ValueType *const end)
        {
            std::sort(first, end, compare<ValueType>);
        }

        template <typename ValueType, int N>
        inline void sort(ValueType container[N])
        {
            std::sort(&container[0], &container[0] + N, compare<ValueType>);
        }

    } // namespace natural
} // namespace SI

#endif // SI_NATURAL_SORT_HPP