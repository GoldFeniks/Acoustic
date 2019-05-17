#pragma once
#include <tuple>
#include <cstddef>
#include <istream>
#include "normal_modes.h"
#include "utils/types.hpp"
#include "utils/interpolation.hpp"

namespace acstc {

    template<typename T>
    class config;

    namespace __impl {

        template<typename T>
        auto read_coords(std::istream& stream, types::vector1d_t<T>& v) {
            for (size_t i = 0; i < v.size(); ++i)
                stream >> v[i];
        }

        template<typename T, typename V>
        struct modes_reader {

            static auto read(std::istream& stream, types::vector3d_t<V>& res) {
                for (size_t i = 0; i < res.size(); ++i)
                    read(stream, res[i]);
            }

            static auto read(std::istream& stream, types::vector2d_t<V>& res) {
                T a, b;
                for (size_t i = 0; i < res.size(); ++i)
                    for (size_t j = 0; j < res[i].size(); ++j) {
                        stream >> a >> b;
                        res[i][j] = V(a, b);
                    }
            }

        };

        template<typename T>
        struct modes_reader<T, T> {

            static auto read(std::istream& stream, types::vector3d_t<T>& res) {
                for (size_t i = 0; i < res.size(); ++i)
                    read(stream, res[i]);
            }

            static auto read(std::istream& stream, types::vector2d_t<T>& res) {
                for (size_t i = 0; i < res.size(); ++i)
                    for (size_t j = 0; j < res[i].size(); ++j)
                        stream >> res[i][j];
            }

        };

        template<typename T, typename V>
        auto from_text(std::istream& stream) {
            size_t n, m, k;
            stream >> n >> m >> k;
            types::vector1d_t<T> x(n), y(m);
            types::vector3d_t<V> k_j(k, types::vector2d_t<V>(n, types::vector1d_t<V>(m)));
            types::vector3d_t<T> phi_j(k, types::vector2d_t<T>(n, types::vector1d_t<T>(m)));
            read_coords(stream, x);
            read_coords(stream, y);
            modes_reader<T, V>::read(stream, k_j);
            modes_reader<T, T>::read(stream, phi_j);
            return std::make_tuple(
                    utils::linear_interpolated_data_2d<T, V>(x, y, std::move(k_j)),
                    utils::linear_interpolated_data_2d<T, T>(x, y, std::move(phi_j)));
        }

        template<typename T, typename V>
        auto const_from_text(std::istream& stream) {
            size_t n, k;
            stream >> n >> k;
            types::vector1d_t<T> y(n);
            types::vector2d_t<V> k_j(k, types::vector1d_t<V>(n));
            types::vector2d_t<T> phi_j(k, types::vector1d_t<T>(n));
            read_coords(stream, y);
            modes_reader<T, V>::read(stream, k_j);
            modes_reader<T, T>::read(stream, phi_j);
            return std::make_tuple(
                    utils::linear_interpolated_data_1d<T, V>(y, std::move(k_j)),
                    utils::linear_interpolated_data_1d<T, T>(y, std::move(phi_j)));
        }

    }// namespace __impl

    template<typename T = types::real_t, typename V = T>
    class modes {

    public:

        modes() = delete;

        static auto create(const config<T>& config) {
            return _create(config, config.bathymetry().x(), config.bathymetry().y(), config.bathymetry().data());
        }

        static auto create(const config<T>& config,
                const T& x0, const T& x1, const size_t& nx,
                const T& y0, const T& y1, const size_t& ny) {
            return _create(config, utils::mesh_1d(x0, x1, nx), utils::mesh_1d(y0, y1, ny),
                    config.bathymetry().field(x0, x1, nx, y0, y1, ny));
        }

        static auto create(const config<T>& config, const size_t& nx, const size_t& ny) {
            const auto [x0, x1, y0, y1] = config.bounds();
            return create(config, x0, x1, nx, y0, y1, ny);
        }

        static auto create(const config<T>& config, const T& y0, const T& y1, const size_t& ny) {
            return _create(config, utils::mesh_1d(y0, y1, ny),
                    config.bathymetry().line(config.bathymetry().x().front(), y0, y1, ny));
        }

        static auto create(const config<T>& config, const size_t& ny) {
            const auto [y0, y1] = config.y_bounds();
            return create(config, y0, y1, ny);
        }

        static auto from_text(std::istream& stream) {
            __impl::from_text<T, V>(stream);
        }

        static auto from_text(std::istream&& stream) {
            return from_text(stream);
        }

        template<typename S = uint32_t>
        static auto from_binary(std::istream& stream) {
            S n, m, k;
            stream.read(reinterpret_cast<char*>(&n), sizeof(S));
            stream.read(reinterpret_cast<char*>(&m), sizeof(S));
            stream.read(reinterpret_cast<char*>(&k), sizeof(S));
            types::vector1d_t<T> x(n), y(m);
            types::vector3d_t<V> k_j(k, types::vector2d_t<V>(n, types::vector1d_t<V>(m)));
            types::vector3d_t<T> phi_j(k, types::vector2d_t<T>(n, types::vector1d_t<T>(m)));
            for (size_t j = 0; j < k; ++j)
                for (size_t i = 0; i < n; ++i)
                    stream.read(reinterpret_cast<char*>(k_j[j][i].data()), sizeof(V) * m);
            for (size_t j = 0; j < k; ++j)
                for (size_t i = 0; i < n; ++i)
                    stream.read(reinterpret_cast<char*>(phi_j[j][i].data()), sizeof(V) * m);
            return std::make_tuple(
                    utils::linear_interpolated_data_2d<T, V>(x, y, std::move(k_j)),
                    utils::linear_interpolated_data_2d<T, T>(x, y, std::move(phi_j)));
        }

        template<typename S = uint32_t>
        static auto from_binary(std::istream&& stream) {
            return from_binary<S>(stream);
        }

        static auto const_from_text(std::istream& stream) {
            return __impl::const_from_text<T, V>(stream);
        }

        static auto const_from_text(std::istream&& stream) {
            return const_from_text(stream);
        }

        template<typename S = uint32_t>
        static auto const_from_binary(std::istream& stream) {
            S n, k;
            stream.read(reinterpret_cast<char*>(&n), sizeof(S));
            stream.read(reinterpret_cast<char*>(&k), sizeof(S));
            types::vector1d_t<T> y(n);
            types::vector2d_t<V> k_j(k, types::vector1d_t<V>(n));
            types::vector2d_t<T> phi_j(k, types::vector1d_t<T>(n));
            for (size_t j = 0; j < k; ++j)
                stream.read(reinterpret_cast<char*>(k_j[j].data()), sizeof(V) * n);
            for (size_t j = 0; j < k; ++j)
                stream.read(reinterpret_cast<char*>(phi_j[j].data()), sizeof(V) * n);
            return std::make_tuple(
                    utils::linear_interpolated_data_1d<T, V>(y, std::move(k_j)),
                    utils::linear_interpolated_data_1d<T, T>(y, std::move(phi_j)));
        }

        template<typename S = uint32_t>
        static auto const_from_binary(std::istream&& stream) {
            return const_from_binary(stream);
        }

    private:

        template<typename XV, typename YV, typename DV>
        static auto _create(const config<T>& config, const XV& x, const YV& y, const DV& data) {
            types::vector3d_t<V> k_j;
            types::vector3d_t<T> phi_j;
            const auto nx = x.size();
            const auto ny = y.size();
            for (size_t i = 0; i < nx; ++i) {
                size_t m = 0;
                for (size_t j = 0; j < ny; ++j)
                    _fill_data(_calc_modes(config, data[i][j]), k_j, phi_j, nx, ny, i, j, m);
            }
            return std::make_tuple(
                    utils::linear_interpolated_data_2d<T, V>(x, y, std::move(k_j)),
                    utils::linear_interpolated_data_2d<T, T>(x, y, std::move(phi_j)));
        }

        template<typename YV, typename DV>
        static auto _create(const config<T>& config, const YV& y, const DV& data) {
            types::vector2d_t<V> k_j;
            types::vector2d_t<T> phi_j;
            const auto ny = y.size();
            size_t m = 0;
            for (size_t i = 0; i < ny; ++i)
                _fill_data(_calc_modes(config, data[i]), k_j, phi_j, ny, i, m);
            return std::make_tuple(
                    utils::linear_interpolated_data_1d<T, V>(y, std::move(k_j)),
                    utils::linear_interpolated_data_1d<T, T>(y, std::move(phi_j)));
        }

        static auto _calc_modes(const config<T>& config, const T& depth) {
            NormalModes n_m;
            n_m.iModesSubset = config.mode_subset();
            n_m.ppm = static_cast<unsigned int>(config.ppm());
            n_m.ordRich = static_cast<unsigned int>(config.ordRich());
            n_m.zr.push_back(config.zr());
            n_m.f = config.f();
            n_m.M_c1s = { T(1500), T(1700) };
            n_m.M_c2s = { T(1500), T(1700) };
            n_m.M_rhos = { T(1), T(2) };
            n_m.M_depths = { depth, T(500) };
            n_m.M_Ns_points.resize(n_m.M_depths.size());
            n_m.M_Ns_points[0] = static_cast<unsigned>(std::round(round(n_m.ppm * n_m.M_depths[0])));
            for (size_t i = 1; i < n_m.M_depths.size(); ++i)
                n_m.M_Ns_points[i] = static_cast<unsigned>(round(n_m.ppm * (n_m.M_depths[i] - n_m.M_depths[i - 1])));
            n_m.compute_khs();
            n_m.compute_mfunctions_zr();
            return n_m;
        }

        static auto _fill_data(const NormalModes& n_m, types::vector3d_t<V>& k_j, types::vector3d_t<T>& phi_j,
                const size_t& nx, const size_t& ny, const size_t& i, const size_t& j, size_t& m) {
            const auto n = n_m.khs.size();
            if (n > k_j.size()) {
                k_j.resize(n, types::vector2d_t<V>(nx, types::vector1d_t<V>(ny, V(0))));
                phi_j.resize(n, types::vector2d_t<T>(nx, types::vector1d_t<T>(ny, T(0))));
            }
            for (size_t k = 0; k < n; ++k) {
                k_j[k][i][j] = n_m.khs[k];
                phi_j[k][i][j] = n_m.mfunctions_zr[0][k];
            }
            for (size_t l = 0; l < j; ++l)
                for (size_t k = m; k < n; ++k)
                    k_j[k][i][l] = k_j[k][i][j];
            for (size_t k = n; k < m; ++k)
                k_j[k][i][j] = k_j[k][i][j - 1];
            m = n;
        }

        static auto _fill_data(const NormalModes& n_m, types::vector2d_t<V>& k_j, types::vector2d_t<T>& phi_j,
                               const size_t& ny, const size_t& i, size_t& m) {
            const auto n = n_m.khs.size();
            if (n > k_j.size()) {
                k_j.resize(n, types::vector1d_t<V>(ny, V(0)));
                phi_j.resize(n, types::vector1d_t<T>(ny, T(0)));
            }
            for (size_t k = 0; k < n; ++k) {
                k_j[k][i] = n_m.khs[k];
                phi_j[k][i] = n_m.mfunctions_zr[0][k];
            }
            for (size_t l = 0; l < i; ++l)
                for (size_t k = m; k < n; ++k)
                    k_j[k][l] = k_j[k][i];
            for (size_t k = n; k < m; ++k)
                k_j[k][i] = k_j[k][i - 1];
            m = n;
        }

    };

}// namespace acstc
