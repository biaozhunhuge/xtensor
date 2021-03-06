/***************************************************************************
* Copyright (c) 2016, Johan Mabille, Sylvain Corlay and Wolf Vollprecht    *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#include "gtest/gtest.h"
#include "xtensor/xarray.hpp"
#include "xtensor/xtensor.hpp"
#include "xtensor/xfixed.hpp"
#include "xtensor/xbuilder.hpp"
#include "xtensor/xmath.hpp"
#include "xtensor/xreducer.hpp"
#include "xtensor/xview.hpp" 
#include "xtensor/xstrided_view.hpp" 
#include "xtensor/xrandom.hpp" 

namespace xt
{
    struct xreducer_features
    {
        using axes_type = std::array<std::size_t, 2>;
        axes_type m_axes;
        xarray<double> m_a;
        using shape_type = xarray<double>::shape_type;

        using func = xreducer_functors<std::plus<double>>;
        xreducer<func, const xarray<double>&, axes_type> m_red;

        xreducer_features();
    };

    xreducer_features::xreducer_features()
        : m_axes({1, 3}), m_a(ones<double>({3, 2, 4, 6, 5})),
          m_red(func(), m_a, m_axes)
    {
        for (std::size_t i = 0; i < 2; ++i)
        {
            for (std::size_t j = 0; j < 6; ++j)
            {
                m_a(1, i, 1, j, 1) = 2;
            }
        }
    }

    TEST(xreducer, functor_type)
    {
        auto sum = [](auto const& left, auto const& right) { return left + right; };
        auto sum_functor = xt::make_xreducer_functor(sum);
        xt::xarray<int> a = {{1, 2, 3}, {4, 5, 6}};
        xt::xarray<int> a_sums = xt::reduce(std::move(sum_functor), a, {1});
        xt::xarray<int> a_sums2 = xt::reduce(sum_functor, a, {1});
        xt::xarray<int> expect = {6, 15};
        EXPECT_EQ(a_sums, expect);
        EXPECT_EQ(a_sums2, expect);
    }

    TEST(xreducer, errors)
    {
        xt::xarray<int> a = {{1, 2, 3}, {4, 5, 6}};
        EXPECT_THROW(xt::sum(a, {1, 0}), std::runtime_error);
        EXPECT_THROW(xt::sum(a, {0, 2}), std::runtime_error);
        EXPECT_THROW(xt::sum(a, {1, 0}, evaluation_strategy::immediate()), std::runtime_error);
        EXPECT_THROW(xt::sum(a, {0, 2}, evaluation_strategy::immediate()), std::runtime_error);
    }

    TEST(xreducer, shape)
    {
        xreducer_features features;
        xreducer_features::shape_type s = {3, 4, 5};
        EXPECT_EQ(s, features.m_red.shape());
        EXPECT_EQ(features.m_red.layout(), layout_type::dynamic);
    }

    TEST(xreducer, access)
    {
        xreducer_features features;
        EXPECT_EQ(12, features.m_red(0, 0, 0));
        EXPECT_EQ(24, features.m_red(1, 1, 1));
        EXPECT_EQ(features.m_red(0, 1), features.m_red(0, 0, 1));
        EXPECT_EQ(features.m_red(1, 2, 1, 0, 1), features.m_red(1, 0, 1));
    }

    TEST(xreducer, unchecked)
    {
        xreducer_features features;
        EXPECT_EQ(12, features.m_red.unchecked(0, 0, 0));
        EXPECT_EQ(24, features.m_red.unchecked(1, 1, 1));
    }

    TEST(xreducer, indexed_access)
    {
        xreducer_features features;
        EXPECT_EQ(12, (features.m_red[{0, 0, 0}]));
        EXPECT_EQ(24, (features.m_red[{1, 1, 1}]));
    }

    TEST(xreducer, at)
    {
        xreducer_features features;
        EXPECT_EQ(12, features.m_red.at(0, 0, 0));
        EXPECT_EQ(24, features.m_red.at(1, 1, 1));
        EXPECT_ANY_THROW(features.m_red.at(10, 10, 10));
        EXPECT_ANY_THROW(features.m_red.at(0, 0, 0, 0));
    }

    TEST(xreducer, iterator)
    {
        xreducer_features features;
        auto iter = features.m_red.cbegin();
        auto iter_end = features.m_red.cend();
        const xreducer_features::shape_type& s = features.m_red.shape();
        std::size_t nb_iter = 1;
        nb_iter = std::accumulate(s.cbegin(), s.cend(), nb_iter, std::multiplies<std::size_t>());
        std::advance(iter, nb_iter);
        EXPECT_EQ(iter_end, iter);
    }

    TEST(xreducer, assign)
    {
        xreducer_features features;
        xarray<double> res = features.m_red;
        xarray<double> expected = 12 * ones<double>({3, 4, 5});
        expected(1, 1, 1) = 24;
        EXPECT_EQ(expected, res);
    }

    TEST(xreducer, sum)
    {
        xreducer_features features;
        xarray<double> res = sum(features.m_a, features.m_axes);
        xarray<double> expected = 12 * ones<double>({3, 4, 5});
        expected(1, 1, 1) = 24;
        EXPECT_EQ(expected, res);
    }

    TEST(xreducer, sum_tensor)
    {
        xtensor<double, 2> m = {{1, 2}, {3, 4}};
        xarray<double> res = xt::sum(m, {0});
        EXPECT_EQ(res.dimension(), 1);
        EXPECT_EQ(res(0), 4.0);
        EXPECT_EQ(res(1), 6.0);
    }

    TEST(xreducer, sum2)
    {
        xarray<double> u = ones<double>({2, 4});
        xarray<double> expectedu0 = 2 * ones<double>({4});
        xarray<double> resu0 = sum(u, {0});
        EXPECT_EQ(expectedu0, resu0);
        xarray<double> expectedu1 = 4 * ones<double>({2});
        xarray<double> resu1 = sum(u, {1});
        EXPECT_EQ(expectedu1, resu1);
        xarray<double> v = ones<double>({4, 2});
        xarray<double> expectedv0 = 4 * ones<double>({2});
        xarray<double> resv0 = sum(v, {0});
        EXPECT_EQ(expectedv0, resv0);
        xarray<double> expectedv1 = 2 * ones<double>({4});
        xarray<double> resv1 = sum(v, {1});
        EXPECT_EQ(expectedv1, resv1);

        // check that there is no overflow
        xarray<uint8_t> c = ones<uint8_t>({1000});
        EXPECT_EQ(1000u, sum(c)());
    }

    TEST(xreducer, sum_all)
    {
        xreducer_features features;
        auto res = sum(features.m_a);
        double expected = 732;
        EXPECT_EQ(res(), expected);
    }

    TEST(xreducer, prod)
    {
        // check that there is no overflow
        xarray<uint8_t> c = 2 * ones<uint8_t>({34});
        EXPECT_EQ(1ULL << 34, prod(c)());
    }

    TEST(xreducer, mean)
    {
        xtensor<double, 2> input
            {{-1.0, 0.0}, {1.0, 0.0}};
        auto mean_all = mean(input);
        auto mean0 = mean(input, {0});
        auto mean1 = mean(input, {1});

        xtensor<double, 0> expect_all = 0.0;
        xtensor<double, 1> expect0 = {0.0, 0.0};
        xtensor<double, 1> expect1 = {-0.5, 0.5};

        EXPECT_EQ(mean_all(), expect_all());
        EXPECT_TRUE(all(equal(mean0, expect0)));
        EXPECT_TRUE(all(equal(mean1, expect1)));

        xarray<uint8_t> c = {1, 2};
        EXPECT_EQ(mean(c)(), 1.5);
    }

    TEST(xreducer, minmax)
    {
        using A = std::array<double, 2>;

        xtensor<double, 2> input
            {{-1.0, 0.0}, {1.0, 0.0}};
        EXPECT_EQ(minmax(input)(), (A{-1.0, 1.0}));
    }

    TEST(xreducer, immediate)
    {
        xarray<double> a = xt::arange(27);
        a.resize({3, 3, 3});

        xarray<double> a_lz = sum(a);
        auto a_gd = sum(a, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd);

        a_lz = sum(a, {1});
        a_gd = sum(a, {1}, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd);

        a_lz = sum(a, {0, 2});
        a_gd = sum(a, {0, 2}, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd);

        a_lz = sum(a, {1, 2});
        a_gd = sum(a, {1, 2}, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd);

        a = xt::arange(4 * 3 * 6 * 2 * 7);
        a.resize({4, 3, 6, 2, 7});

        a_lz = sum(a);
        a_gd = sum(a, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd);

        a_lz = sum(a, {1});
        a_gd = sum(a, {1}, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd);

        a_lz = sum(a, {0, 2});
        a_gd = sum(a, {0, 2}, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd);

        a_lz = sum(a, {1, 2});
        a_gd = sum(a, {1, 2}, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd);

        a_lz = sum(a, {1, 3, 4});
        a_gd = sum(a, {1, 3, 4}, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd);

        a_lz = sum(a, {0, 1, 4});
        a_gd = sum(a, {0, 1, 4}, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd);

        a_lz = sum(a, {0, 1, 3});
        a_gd = sum(a, {0, 1, 3}, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd);

        a_lz = sum(a, {0, 2, 3});
        a_gd = sum(a, {0, 2, 3}, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd);

        a_lz = sum(a, {1, 2, 3});
        a_gd = sum(a, {1, 2, 3}, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd);

        xtensor<short, 3> ct = xt::random::randint<short>({1, 5, 3});
        EXPECT_EQ(sum(ct, {0, 2}), sum(ct, {0, 2}, evaluation_strategy::immediate()));

        xtensor<short, 5> ct2 = xt::random::randint<short>({1, 5, 1, 2, 3});
        // EXPECT_EQ(sum(ct2, {0, 1, 2}) * 2, sum(xt::cast<short>(ct2) * 2, {0, 1, 2}, evaluation_strategy::immediate()));
        EXPECT_EQ(sum(ct2, {0, 1, 2}), sum(ct2, {0, 1, 2}, evaluation_strategy::immediate()));
        EXPECT_EQ(sum(ct2, {2, 3}), sum(ct2, {2, 3}, evaluation_strategy::immediate()));
        EXPECT_EQ(sum(ct2, {1, 3}), sum(ct2, {1, 3}, evaluation_strategy::immediate()));
    }

    TEST(xreducer, chaining_reducers)
    {
        xt::xarray<double> a = {{ 1., 2. },
                                { 3., 4. }};

        auto b = a - xt::sum(a, { 0 });
        auto c = xt::sum(b, { 0 });
        EXPECT_EQ(c(0), -4.);
        EXPECT_EQ(c(1), -6.);
    }

    TEST(xreducer, immediate_shape)
    {
        xtensor<double, 2> c = {{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}};
        auto xa = xt::sum(c, {0}, evaluation_strategy::immediate());
        auto is_arr = [](const auto& c)
        {
            bool istrue = detail::is_array<std::decay_t<decltype(c)>>::value;
            return istrue;
        };

    #ifndef X_OLD_CLANG
        EXPECT_TRUE(is_arr(xa.shape()));
    #endif

        xtensor<double, 3> a;
        a.resize({3, 3, 3});
        std::iota(a.storage().begin(), a.storage().end(), 0);

        xarray<double> a_lz = sum(a);
        auto a_gd = sum(a, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd);
        EXPECT_TRUE(is_arr(a_gd.shape()));

        a_lz = sum(a, {1});
        auto a_gd_1 = sum(a, {1}, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd_1);

        a_lz = sum(a, {0, 2});
        auto a_gd_2 = sum(a, {0, 2}, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd_2);

    #ifndef X_OLD_CLANG
        EXPECT_TRUE(is_arr(a_gd_1.shape()));
        EXPECT_TRUE(is_arr(a_gd_2.shape()));
    #endif

        a_lz = sum(a, {1, 2});
        a_gd_2 = sum(a, {1, 2}, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd_2);
    }

    TEST(xreducer, xfixed_reduction)
    {
        xtensor_fixed<double, xshape<3, 3, 3>> a;
        std::iota(a.storage().begin(), a.storage().end(), 0);

        xtensor<double, 3> b;
        b.resize({3, 3, 3});
        std::iota(b.storage().begin(), b.storage().end(), 0);

        auto is_arr = [](const auto& c)
        {
            bool istrue = detail::is_array<std::decay_t<decltype(c)>>::value;
            return istrue;
        };

        xarray<double> a_lz = sum(a);
        auto a_gd = sum(a, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd);
        EXPECT_TRUE(is_arr(a_gd.shape()));

        a_lz = sum(a, {1});
        auto a_gd_1 = sum(a, {1}, evaluation_strategy::immediate());
        auto b_gd_1 = sum(b, {1}, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd_1);
        EXPECT_EQ(a_lz, b_gd_1);

        a_lz = sum(a, {0, 2});
        auto a_gd_2 = sum(a, {0, 2}, evaluation_strategy::immediate());
        auto b_gd_2 = sum(b, {0, 2}, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd_2);
        EXPECT_EQ(b_gd_2, a_gd_2);
        EXPECT_EQ(a_gd_2.dimension(), 1);

    #ifndef X_OLD_CLANG
        EXPECT_TRUE(is_arr(a_gd_1.shape()));
        EXPECT_TRUE(is_arr(a_gd_2.shape()));
    #endif

        a_lz = sum(a, {1, 2});
        a_gd_2 = sum(a, {1, 2}, evaluation_strategy::immediate());
        EXPECT_EQ(a_lz, a_gd_2);
    }

    TEST(xreducer, view_steppers)
    {
        xt::xtensor<double, 2> X({10, 20});
        xt::xtensor<double, 2> Y(X.shape());

        X = xt::random::randn<double>(X.shape());

        xt::xtensor<double, 2> vx0 = xt::view(xt::sum(X, {1}), xt::all(), xt::newaxis());
        xt::xtensor<double, 2> vx1 = xt::expand_dims(xt::sum(X, {1}), 1);

        EXPECT_EQ(vx0, vx1);
    }

    TEST(xreducer, wrong_number_of_indices)
    {
        xt::xtensor<double, 4> a = xt::random::rand<double>({5, 5, 5, 5});
        double e = xt::sum(a)();
        double s1 = xt::sum(a)(0);
        EXPECT_EQ(s1, e);
        double s2 = xt::sum(a)(0, 1, 2, 3, 4, 5);
        EXPECT_EQ(s2, e);

        auto red = xt::sum(a, {0});
        EXPECT_EQ(red(2), red(0, 0, 2));
        EXPECT_EQ(red(1, 2), red(0, 1, 2));
        EXPECT_EQ(red(1, 2), red(1, 1, 1, 1, 1, 0, 1, 2));
    }
}
