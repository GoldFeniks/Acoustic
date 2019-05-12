#pragma once
#include <tuple>
#include <vector>
#include <istream>
#include "../utils/types.hpp"

namespace acstc {

    template<typename T, typename V = T>
    struct read_data {

        types::vector1d_t<T> rows, cols;
        types::vector2d_t<V> data;

    };

    template<typename T, typename V = T>
    class table_reader {

    public:

        static auto read(std::istream& stream) {
            read_data<T, V> data;
            std::tie(std::ignore, data.cols) = _read_line(stream);
            while (_find_something(stream)) {
                const auto [val, row] = _read_line(stream);
                if (row.size()) {
                    data.rows.push_back(std::move(val));
                    data.data.push_back(std::move(row));
                }
            }
            return data;
        }

        static auto read(std::istream&& stream) {
            return read(stream);
        }

    private:

        static auto _read_line(std::istream& stream) {
            types::vector1d_t<V> data;
            T row;
            V buff;
            stream >> row;
            while (_find_something(stream)) {
                stream >> buff;
                data.push_back(buff);
            }
            stream.get();
            return std::make_tuple(row, data);
        }

        static auto _find_something(std::istream& stream) {
            auto c = stream.peek();
            while (c != '\n' && c != EOF) {
                if (c == ' ' || c == '\t')
                    while (c == ' ' || c == '\t') {
                        stream.get();
                        c = stream.peek();
                    }
                else
                    return true;
            }
            return false;
        }

    };

    template<typename T, typename V = T, typename S = uint32_t>
    class binary_table_reader {

    public:

        static auto read(std::istream& stream) {
            S n, m;
            stream.read(reinterpret_cast<char*>(&n), sizeof(S));
            stream.read(reinterpret_cast<char*>(&m), sizeof(S));
            read_data<T, V> data;
            data.data.resize(n, types::vector1d_t<V>(m));
            data.rows.resize(n);
            data.cols.resize(m);
            stream.read(reinterpret_cast<char*>(data.rows.data()), sizeof(T) * n);
            stream.read(reinterpret_cast<char*>(data.cols.data()), sizeof(T) * m);
            for (auto& it : data.data)
                stream.read(reinterpret_cast<char*>(it.data()), sizeof(V) * m);
            return data;
        }

        static auto read(std::istream&& stream) {
            return read(stream);
        }

    };

}// namespace acstc